#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <system/include/bkp_log.h>
#include "include/bkp_loaders.h"

static const char * gTag = "STL";

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

static void computeAabb(BkpModel * model, BkpVec3 * pos, size_t count)
{
    if(count == 0) return;
    float mn[3] = {pos[0].x, pos[0].y, pos[0].z};
    float mx[3] = {pos[0].x, pos[0].y, pos[0].z};
    for(size_t i = 1; i < count; ++i)
    {
        if(pos[i].x < mn[0]) mn[0] = pos[i].x;
        if(pos[i].x > mx[0]) mx[0] = pos[i].x;
        if(pos[i].y < mn[1]) mn[1] = pos[i].y;
        if(pos[i].y > mx[1]) mx[1] = pos[i].y;
        if(pos[i].z < mn[2]) mn[2] = pos[i].z;
        if(pos[i].z > mx[2]) mx[2] = pos[i].z;
    }
    for(int k = 0; k < 3; ++k) { model->aabbMin[k] = mn[k]; model->aabbMax[k] = mx[k]; }
}

/*___________________________________________________________________*/
static int isAsciiStl(const unsigned char * buf, size_t size)
{
    /* ASCII STL starts with "solid" (possibly preceded by whitespace).
       Binary STL can start with "solid" too, so also check that the
       declared triangle count is consistent with the file size.        */
    const char * p = (const char *)buf;
    while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
    if(strncmp(p, "solid", 5) != 0) return 0;

    /* Binary: 80-byte header + 4-byte count + count * 50 bytes */
    if(size < 84) return 1; /* too small to be valid binary */
    uint32_t triCount;
    memcpy(&triCount, buf + 80, 4);
    size_t expected = 84 + (size_t)triCount * 50;
    if(expected == size) return 0; /* exactly fits binary layout */
    return 1;
}

/*___________________________________________________________________*/
static BkpBool loadAscii(BkpGpuAdapter adapter, const char * path,
                          const char * buf, size_t size,
                          BkpModel * model, size_t scratchGroupId)
{
    (void)size;
    /* count "facet normal" occurrences first */
    size_t triCap = 1024;
    BkpVec3 * positions = (BkpVec3 *)malloc(sizeof(BkpVec3) * triCap * 3);
    BkpVec3 * normals   = (BkpVec3 *)malloc(sizeof(BkpVec3) * triCap * 3);
    size_t    triCount  = 0;

    const char * p = buf;
    while(*p)
    {
        /* skip to next "facet" */
        while(*p && strncmp(p, "facet", 5) != 0)
        {
            while(*p && *p != '\n') ++p;
            if(*p) ++p;
        }
        if(!*p) break;
        p += 5; /* skip "facet" */
        /* read normal */
        float nx, ny, nz;
        while(*p == ' ' || *p == '\t') ++p;
        if(strncmp(p, "normal", 6) == 0) p += 6;
        nx = (float)strtod(p, (char **)&p);
        ny = (float)strtod(p, (char **)&p);
        nz = (float)strtod(p, (char **)&p);

        /* skip to "outer loop" */
        while(*p && strncmp(p, "outer", 5) != 0)
        {
            while(*p && *p != '\n') ++p;
            if(*p) ++p;
        }
        if(*p) { while(*p && *p != '\n') ++p; if(*p) ++p; }

        /* read 3 vertices */
        float verts[3][3];
        int ok = 1;
        for(int v = 0; v < 3 && ok; ++v)
        {
            while(*p && strncmp(p, "vertex", 6) != 0 && strncmp(p, "endloop", 7) != 0)
            {
                while(*p && *p != '\n') ++p;
                if(*p) ++p;
            }
            if(strncmp(p, "vertex", 6) == 0)
            {
                p += 6;
                verts[v][0] = (float)strtod(p, (char **)&p);
                verts[v][1] = (float)strtod(p, (char **)&p);
                verts[v][2] = (float)strtod(p, (char **)&p);
            }
            else ok = 0;
        }
        if(!ok) break;

        /* grow if needed */
        if(triCount >= triCap)
        {
            triCap *= 2;
            positions = (BkpVec3 *)realloc(positions, sizeof(BkpVec3) * triCap * 3);
            normals   = (BkpVec3 *)realloc(normals,   sizeof(BkpVec3) * triCap * 3);
        }

        size_t base = triCount * 3;
        for(int v = 0; v < 3; ++v)
        {
            positions[base+v].x = verts[v][0];
            positions[base+v].y = verts[v][1];
            positions[base+v].z = verts[v][2];
            normals[base+v].x   = nx;
            normals[base+v].y   = ny;
            normals[base+v].z   = nz;
        }
        ++triCount;

        /* skip to next line */
        while(*p && *p != '\n') ++p;
        if(*p) ++p;
    }

    if(triCount == 0)
    {
        free(positions); free(normals);
        LOG(eWARNING, gTag, "No triangles found in ASCII STL '%s'", path);
        return BKP_FALSE;
    }

    size_t vertexCount = triCount * 3;
    computeAabb(model, positions, vertexCount);

    BkpVec3 * uvs      = (BkpVec3 *)calloc(vertexCount, sizeof(BkpVec3));
    BkpVec3 * tangents = (BkpVec3 *)calloc(vertexCount, sizeof(BkpVec3));

    uint32_t meshCountBefore = model->meshCount;
    BkpModelMesh mesh = {};
    strncpy(mesh.name, path, sizeof(mesh.name) - 1);
    mesh.meshAabbMin[0] = model->aabbMin[0]; mesh.meshAabbMin[1] = model->aabbMin[1]; mesh.meshAabbMin[2] = model->aabbMin[2];
    mesh.meshAabbMax[0] = model->aabbMax[0]; mesh.meshAabbMax[1] = model->aabbMax[1]; mesh.meshAabbMax[2] = model->aabbMax[2];

    bkpMakeMeshBufferEx(adapter, vertexCount,
                        positions, normals, uvs, tangents, NULL,
                        0, NULL, NULL,
                        &mesh.geo, scratchGroupId, BKP_NORMALS_RECOMPUTE);
    bkp_modelPushMesh(model, &mesh);
    makeRootNodes(model, meshCountBefore, 1);

    free(positions); free(normals); free(uvs); free(tangents);
    return BKP_TRUE;
}

/*___________________________________________________________________*/
static BkpBool loadBinary(BkpGpuAdapter adapter, const char * path,
                           const unsigned char * buf, size_t size,
                           BkpModel * model, size_t scratchGroupId)
{
    if(size < 84) { LOG(eERROR, gTag, "Binary STL too small: '%s'", path); return BKP_FALSE; }

    uint32_t triCount;
    memcpy(&triCount, buf + 80, 4);

    size_t expected = 84 + (size_t)triCount * 50;
    if(expected > size)
    {
        LOG(eERROR, gTag, "Binary STL truncated: '%s'", path);
        return BKP_FALSE;
    }

    size_t vertexCount = (size_t)triCount * 3;
    BkpVec3 * positions = (BkpVec3 *)malloc(sizeof(BkpVec3) * vertexCount);
    BkpVec3 * normals   = (BkpVec3 *)malloc(sizeof(BkpVec3) * vertexCount);

    const unsigned char * p = buf + 84;
    for(uint32_t t = 0; t < triCount; ++t)
    {
        float nv[3], v0[3], v1[3], v2[3];
        memcpy(nv, p,      12); p += 12;
        memcpy(v0, p,      12); p += 12;
        memcpy(v1, p,      12); p += 12;
        memcpy(v2, p,      12); p += 12;
        p += 2; /* attribute byte count */

        size_t base = t * 3;
        positions[base+0].x = v0[0]; positions[base+0].y = v0[1]; positions[base+0].z = v0[2];
        positions[base+1].x = v1[0]; positions[base+1].y = v1[1]; positions[base+1].z = v1[2];
        positions[base+2].x = v2[0]; positions[base+2].y = v2[1]; positions[base+2].z = v2[2];
        for(int k = 0; k < 3; ++k)
        {
            normals[base+k].x = nv[0];
            normals[base+k].y = nv[1];
            normals[base+k].z = nv[2];
        }
    }

    computeAabb(model, positions, vertexCount);

    BkpVec3 * uvs      = (BkpVec3 *)calloc(vertexCount, sizeof(BkpVec3));
    BkpVec3 * tangents = (BkpVec3 *)calloc(vertexCount, sizeof(BkpVec3));

    uint32_t meshCountBefore = model->meshCount;
    BkpModelMesh mesh = {};
    strncpy(mesh.name, path, sizeof(mesh.name) - 1);
    mesh.meshAabbMin[0] = model->aabbMin[0]; mesh.meshAabbMin[1] = model->aabbMin[1]; mesh.meshAabbMin[2] = model->aabbMin[2];
    mesh.meshAabbMax[0] = model->aabbMax[0]; mesh.meshAabbMax[1] = model->aabbMax[1]; mesh.meshAabbMax[2] = model->aabbMax[2];

    bkpMakeMeshBufferEx(adapter, vertexCount,
                        positions, normals, uvs, tangents, NULL,
                        0, NULL, NULL,
                        &mesh.geo, scratchGroupId, BKP_NORMALS_RECOMPUTE);
    bkp_modelPushMesh(model, &mesh);
    makeRootNodes(model, meshCountBefore, 1);

    free(positions); free(normals); free(uvs); free(tangents);
    return BKP_TRUE;
}

/*___________________________________________________________________*/
BkpBool bkpLoadStl(BkpGpuAdapter adapter, const char * path,
                   BkpModel * model, size_t scratchGroupId)
{
    FILE * f = fopen(path, "rb");
    if(!f) { LOG(eERROR, gTag, "Cannot open '%s'", path); return BKP_FALSE; }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);
    unsigned char * buf = (unsigned char *)malloc((size_t)fsize + 1);
    if(!buf) { fclose(f); return BKP_FALSE; }
    (void)fread(buf, 1, (size_t)fsize, f);
    buf[fsize] = '\0';
    fclose(f);

    BkpBool ok;
    if(isAsciiStl(buf, (size_t)fsize))
        ok = loadAscii(adapter, path, (const char *)buf, (size_t)fsize, model, scratchGroupId);
    else
        ok = loadBinary(adapter, path, buf, (size_t)fsize, model, scratchGroupId);

    free(buf);
    if(ok) LOG(eDEBUG, gTag, "Loaded '%s'", path);
    return ok;
}

#ifdef __cplusplus
}
#endif
