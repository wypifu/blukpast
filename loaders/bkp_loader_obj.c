#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <system/include/bkp_log.h>
#include "include/bkp_loaders.h"

static const char * gTag = "OBJ";

void      bkp_modelPushMesh (BkpModel * model, BkpModelMesh * mesh);
BkpNode * bkp_modelNodeSlot (BkpModel * model);

static void makeRootNodes(BkpModel * model, uint32_t firstMesh, uint32_t count)
{
    uint32_t base = model->nodeCount;
    model->rootNodes     = (int *)malloc(sizeof(int) * count);
    model->rootNodeCount = count;
    for(uint32_t i = 0; i < count; ++i)
    {
        BkpNode * node = bkp_modelNodeSlot(model);
        memset(node, 0, sizeof(*node));
        node->parentIdx   = -1;
        node->skinIdx     = -1;
        node->meshIdx     = (int)(firstMesh + i);
        node->meshCount   = 1;
        node->scale[0] = node->scale[1] = node->scale[2] = 1.0f;
        node->rotation[3] = 1.0f;
        node->localMatrix[0] = node->localMatrix[5] =
        node->localMatrix[10] = node->localMatrix[15] = 1.0f;
        model->rootNodes[i] = (int)(base + i);
    }
}

/*___________________________________________________________________*/
/* Dynamic scratch arrays (system malloc) */
typedef struct { float * data; size_t count, cap; } FArr;
typedef struct { uint32_t * data; size_t count, cap; } UArr;

static void fArrPush(FArr * a, float v)
{
    if(a->count >= a->cap)
    {
        a->cap = a->cap ? a->cap * 2 : 256;
        a->data = (float *)realloc(a->data, a->cap * sizeof(float));
    }
    a->data[a->count++] = v;
}

static void uArrPush(UArr * a, uint32_t v)
{
    if(a->count >= a->cap)
    {
        a->cap = a->cap ? a->cap * 2 : 256;
        a->data = (uint32_t *)realloc(a->data, a->cap * sizeof(uint32_t));
    }
    a->data[a->count++] = v;
}

/*___________________________________________________________________*/
/* OBJ indices are 1-based; negatives are relative. */
static int resolveIdx(int raw, int total)
{
    if(raw < 0) return total + raw;
    return raw - 1;
}

/* skip whitespace, return pointer past it */
static const char * skipWs(const char * p)
{
    while(*p == ' ' || *p == '\t') ++p;
    return p;
}

/* read one int or 0; advance pointer */
static int readInt(const char ** pp)
{
    const char * p = skipWs(*pp);
    if(*p == 0 || *p == '\n' || *p == '\r' || *p == '/') { return 0; }
    int sign = 1;
    if(*p == '-') { sign = -1; ++p; }
    int v = 0;
    while(*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); ++p; }
    *pp = p;
    return sign * v;
}

/*___________________________________________________________________*/
/* Per-group (or whole file) mesh builder */
typedef struct
{
    FArr pos;   /* x y z triples  */
    FArr nor;   /* x y z triples  */
    FArr uv;    /* u v pairs      */
} ObjGlobal;

typedef struct
{
    UArr vIdx;  /* index into pos  (0-based after resolve) */
    UArr nIdx;  /* index into nor  */
    UArr tIdx;  /* index into uv   */
    int  hasN;
    int  hasT;
} ObjGroup;

static void groupInit(ObjGroup * g)
{
    memset(g, 0, sizeof(*g));
}

static void groupFree(ObjGroup * g)
{
    free(g->vIdx.data);
    free(g->nIdx.data);
    free(g->tIdx.data);
}

/* flush a group into a BkpModelMesh and push it */
static void flushGroup(BkpGpuAdapter adapter, ObjGlobal * gl,
                       ObjGroup * grp, const char * name,
                       BkpModel * model, size_t scratchGroupId)
{
    if(grp->vIdx.count == 0) return;

    size_t triCount = grp->vIdx.count; /* already triangulated */
    BkpVec3 * positions = (BkpVec3 *)calloc(triCount, sizeof(BkpVec3));
    BkpVec3 * normals   = (BkpVec3 *)calloc(triCount, sizeof(BkpVec3));
    BkpVec3 * uvs       = (BkpVec3 *)calloc(triCount, sizeof(BkpVec3));
    BkpVec3 * tangents  = (BkpVec3 *)calloc(triCount, sizeof(BkpVec3));

    size_t posTotal = gl->pos.count / 3;
    size_t norTotal = gl->nor.count / 3;
    size_t uvTotal  = gl->uv.count  / 2;

    for(size_t i = 0; i < triCount; ++i)
    {
        uint32_t vi = grp->vIdx.data[i];
        if(vi < posTotal)
        {
            positions[i].x = gl->pos.data[vi*3+0];
            positions[i].y = gl->pos.data[vi*3+1];
            positions[i].z = gl->pos.data[vi*3+2];
        }
        if(grp->hasN)
        {
            uint32_t ni = grp->nIdx.data[i];
            if(ni < norTotal)
            {
                normals[i].x = gl->nor.data[ni*3+0];
                normals[i].y = gl->nor.data[ni*3+1];
                normals[i].z = gl->nor.data[ni*3+2];
            }
        }
        if(grp->hasT)
        {
            uint32_t ti = grp->tIdx.data[i];
            if(ti < uvTotal)
            {
                uvs[i].x = gl->uv.data[ti*2+0];
                uvs[i].y = gl->uv.data[ti*2+1];
            }
        }
    }

    if(grp->hasT)
    {
        for(size_t tri = 0; tri + 2 < triCount; tri += 3)
        {
            BkpTangentBitangent tb = bkpCalculateTangent(
                &positions[tri], &positions[tri+1], &positions[tri+2],
                &uvs[tri],       &uvs[tri+1],       &uvs[tri+2]);
            tangents[tri] = tangents[tri+1] = tangents[tri+2] = tb.t;
        }
    }

    float mMin[3]={1e30f,1e30f,1e30f}, mMax[3]={-1e30f,-1e30f,-1e30f};
    for(size_t i=0; i<triCount; ++i) {
        float v[3]={positions[i].x, positions[i].y, positions[i].z};
        for(int k=0;k<3;++k){if(v[k]<mMin[k])mMin[k]=v[k]; if(v[k]>mMax[k])mMax[k]=v[k];}
    }

    BkpModelMesh mesh = {};
    strncpy(mesh.name, name ? name : "mesh", sizeof(mesh.name) - 1);
    mesh.meshAabbMin[0]=mMin[0]; mesh.meshAabbMin[1]=mMin[1]; mesh.meshAabbMin[2]=mMin[2];
    mesh.meshAabbMax[0]=mMax[0]; mesh.meshAabbMax[1]=mMax[1]; mesh.meshAabbMax[2]=mMax[2];

    bkpMakeMeshBufferEx(adapter, triCount,
                        positions, normals, uvs, tangents, NULL,
                        0, NULL, NULL,
                        &mesh.geo, scratchGroupId, BKP_NORMALS_RECOMPUTE);
    bkp_modelPushMesh(model, &mesh);

    free(positions); free(normals); free(uvs); free(tangents);
}

/*___________________________________________________________________*/
/* Parse a face token: v, v/t, v//n, v/t/n — push resolved indices  */
static void parseFaceVert(const char ** pp, ObjGlobal * gl, ObjGroup * grp,
                          uint32_t * outV, uint32_t * outT, uint32_t * outN)
{
    int vi = readInt(pp);
    int ti = 0, ni = 0;
    if(**pp == '/')
    {
        ++(*pp);
        if(**pp != '/') ti = readInt(pp);
        if(**pp == '/') { ++(*pp); ni = readInt(pp); }
    }
    *outV = (uint32_t)resolveIdx(vi, (int)(gl->pos.count / 3));
    *outT = ti ? (uint32_t)resolveIdx(ti, (int)(gl->uv.count  / 2)) : 0;
    *outN = ni ? (uint32_t)resolveIdx(ni, (int)(gl->nor.count / 3)) : 0;
    if(ti) grp->hasT = 1;
    if(ni) grp->hasN = 1;
}

/*___________________________________________________________________*/
BkpBool bkpLoadObj(BkpGpuAdapter adapter, const char * path,
                   BkpModel * model, size_t scratchGroupId)
{
    FILE * f = fopen(path, "rb");
    if(!f)
    {
        LOG(eERROR, gTag, "Cannot open '%s'", path);
        return BKP_FALSE;
    }

    /* read whole file */
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);
    char * buf = (char *)malloc((size_t)fsize + 1);
    if(!buf) { fclose(f); return BKP_FALSE; }
    (void)fread(buf, 1, (size_t)fsize, f);
    buf[fsize] = '\0';
    fclose(f);

    ObjGlobal gl;  memset(&gl,  0, sizeof(gl));
    ObjGroup  grp; groupInit(&grp);
    char      groupName[128] = "default";
    uint32_t  meshCountBefore = model->meshCount;

    const char * line = buf;
    while(*line)
    {
        /* find end of line */
        const char * eol = line;
        while(*eol && *eol != '\n') ++eol;

        /* handle continuation lines */
        const char * cur = skipWs(line);

        if(cur[0] == 'v' && (cur[1] == ' ' || cur[1] == '\t'))
        {
            cur += 2;
            fArrPush(&gl.pos, (float)atof(skipWs(cur)));
            cur = skipWs(cur); while(*cur && *cur != ' ' && *cur != '\t') ++cur;
            fArrPush(&gl.pos, (float)atof(skipWs(cur)));
            cur = skipWs(cur); while(*cur && *cur != ' ' && *cur != '\t') ++cur;
            fArrPush(&gl.pos, (float)atof(skipWs(cur)));
        }
        else if(cur[0] == 'v' && cur[1] == 'n')
        {
            cur += 2;
            fArrPush(&gl.nor, (float)atof(skipWs(cur)));
            cur = skipWs(cur); while(*cur && *cur != ' ' && *cur != '\t') ++cur;
            fArrPush(&gl.nor, (float)atof(skipWs(cur)));
            cur = skipWs(cur); while(*cur && *cur != ' ' && *cur != '\t') ++cur;
            fArrPush(&gl.nor, (float)atof(skipWs(cur)));
        }
        else if(cur[0] == 'v' && cur[1] == 't')
        {
            cur += 2;
            fArrPush(&gl.uv, (float)atof(skipWs(cur)));
            cur = skipWs(cur); while(*cur && *cur != ' ' && *cur != '\t') ++cur;
            fArrPush(&gl.uv, (float)atof(skipWs(cur)));
        }
        else if(cur[0] == 'g' || cur[0] == 'o')
        {
            /* flush previous group */
            flushGroup(adapter, &gl, &grp, groupName, model, scratchGroupId);
            groupFree(&grp); groupInit(&grp);
            cur += 1; cur = skipWs(cur);
            size_t len = 0;
            while(cur[len] && cur[len] != '\n' && cur[len] != '\r') ++len;
            if(len >= sizeof(groupName)) len = sizeof(groupName) - 1;
            memcpy(groupName, cur, len); groupName[len] = '\0';
        }
        else if(cur[0] == 'f' && (cur[1] == ' ' || cur[1] == '\t'))
        {
            cur += 1;
            /* polygon: fan triangulation v0 v1 v2, v0 v2 v3, ... */
            uint32_t fv[64], ft[64], fn[64];
            int fc = 0;
            const char * p = cur;
            while(*p && *p != '\n' && *p != '\r' && *p != '#')
            {
                p = skipWs(p);
                if(!*p || *p == '\n' || *p == '\r') break;
                if(fc < 64)
                    parseFaceVert(&p, &gl, &grp, &fv[fc], &ft[fc], &fn[fc]);
                ++fc;
            }
            /* fan triangulate */
            for(int t = 1; t + 1 < fc; ++t)
            {
                int idx[3] = {0, t, t+1};
                for(int k = 0; k < 3; ++k)
                {
                    uArrPush(&grp.vIdx, fv[idx[k]]);
                    uArrPush(&grp.tIdx, ft[idx[k]]);
                    uArrPush(&grp.nIdx, fn[idx[k]]);
                }
            }
        }

        line = (*eol == '\n') ? eol + 1 : eol;
    }

    /* flush last group */
    flushGroup(adapter, &gl, &grp, groupName, model, scratchGroupId);
    groupFree(&grp);

    uint32_t loaded = model->meshCount - meshCountBefore;
    if(loaded > 0)
    {
        if(gl.pos.count >= 3)
        {
            float mn[3] = {gl.pos.data[0], gl.pos.data[1], gl.pos.data[2]};
            float mx[3] = {gl.pos.data[0], gl.pos.data[1], gl.pos.data[2]};
            for(size_t i = 3; i + 2 < gl.pos.count; i += 3)
            {
                for(int k = 0; k < 3; ++k)
                {
                    if(gl.pos.data[i+k] < mn[k]) mn[k] = gl.pos.data[i+k];
                    if(gl.pos.data[i+k] > mx[k]) mx[k] = gl.pos.data[i+k];
                }
            }
            for(int k = 0; k < 3; ++k) { model->aabbMin[k] = mn[k]; model->aabbMax[k] = mx[k]; }
        }
        makeRootNodes(model, meshCountBefore, loaded);
    }

    free(gl.pos.data); free(gl.nor.data); free(gl.uv.data);
    free(buf);
    LOG(eDEBUG, gTag, "Loaded '%s' : %u mesh(es)", path, loaded);
    return loaded > 0 ? BKP_TRUE : BKP_FALSE;
}

#ifdef __cplusplus
}
#endif
