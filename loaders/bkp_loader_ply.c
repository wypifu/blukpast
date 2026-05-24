#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <system/include/bkp_log.h>
#include "include/bkp_loaders.h"

static const char * gTag = "PLY";

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
typedef enum { PLY_ASCII, PLY_BINARY_LE, PLY_BINARY_BE } PlyFormat;

typedef enum
{
    PLY_PROP_FLOAT, PLY_PROP_DOUBLE,
    PLY_PROP_INT8,  PLY_PROP_UINT8,
    PLY_PROP_INT16, PLY_PROP_UINT16,
    PLY_PROP_INT32, PLY_PROP_UINT32,
    PLY_PROP_UNKNOWN
} PlyType;

typedef struct
{
    char    name[64];
    PlyType type;
    PlyType listCountType; /* PLY_UNKNOWN → not a list */
    PlyType listElemType;
    int     isList;
} PlyProp;

typedef struct
{
    char    name[64];
    size_t  count;
    PlyProp props[32];
    int     propCount;
} PlyElement;

/*___________________________________________________________________*/
static PlyType parsePlyType(const char * s)
{
    if(!strcmp(s,"float") || !strcmp(s,"float32"))  return PLY_PROP_FLOAT;
    if(!strcmp(s,"double")|| !strcmp(s,"float64"))  return PLY_PROP_DOUBLE;
    if(!strcmp(s,"char")  || !strcmp(s,"int8"))     return PLY_PROP_INT8;
    if(!strcmp(s,"uchar") || !strcmp(s,"uint8"))    return PLY_PROP_UINT8;
    if(!strcmp(s,"short") || !strcmp(s,"int16"))    return PLY_PROP_INT16;
    if(!strcmp(s,"ushort")|| !strcmp(s,"uint16"))   return PLY_PROP_UINT16;
    if(!strcmp(s,"int")   || !strcmp(s,"int32"))    return PLY_PROP_INT32;
    if(!strcmp(s,"uint")  || !strcmp(s,"uint32"))   return PLY_PROP_UINT32;
    return PLY_PROP_UNKNOWN;
}

static size_t plyTypeSize(PlyType t)
{
    switch(t)
    {
        case PLY_PROP_FLOAT:  return 4;
        case PLY_PROP_DOUBLE: return 8;
        case PLY_PROP_INT8:   return 1;
        case PLY_PROP_UINT8:  return 1;
        case PLY_PROP_INT16:  return 2;
        case PLY_PROP_UINT16: return 2;
        case PLY_PROP_INT32:  return 4;
        case PLY_PROP_UINT32: return 4;
        default:              return 0;
    }
}

/* read a scalar value from binary data as float */
static float plyReadScalar(PlyType t, const unsigned char * p, int bigEndian)
{
    union { float f; double d; int8_t i8; uint8_t u8;
            int16_t i16; uint16_t u16; int32_t i32; uint32_t u32; } v;
    switch(t)
    {
        case PLY_PROP_FLOAT:
            if(bigEndian)
            {
                unsigned char tmp[4] = {p[3],p[2],p[1],p[0]};
                memcpy(&v.f, tmp, 4);
            }
            else memcpy(&v.f, p, 4);
            return v.f;
        case PLY_PROP_DOUBLE:
            if(bigEndian)
            {
                unsigned char tmp[8] = {p[7],p[6],p[5],p[4],p[3],p[2],p[1],p[0]};
                memcpy(&v.d, tmp, 8);
            }
            else memcpy(&v.d, p, 8);
            return (float)v.d;
        case PLY_PROP_INT8:   memcpy(&v.i8,  p, 1); return (float)v.i8;
        case PLY_PROP_UINT8:  memcpy(&v.u8,  p, 1); return (float)v.u8;
        case PLY_PROP_INT16:
            if(bigEndian) { unsigned char tmp[2]={p[1],p[0]}; memcpy(&v.i16,tmp,2); }
            else memcpy(&v.i16, p, 2);
            return (float)v.i16;
        case PLY_PROP_UINT16:
            if(bigEndian) { unsigned char tmp[2]={p[1],p[0]}; memcpy(&v.u16,tmp,2); }
            else memcpy(&v.u16, p, 2);
            return (float)v.u16;
        case PLY_PROP_INT32:
            if(bigEndian) { unsigned char tmp[4]={p[3],p[2],p[1],p[0]}; memcpy(&v.i32,tmp,4); }
            else memcpy(&v.i32, p, 4);
            return (float)v.i32;
        case PLY_PROP_UINT32:
            if(bigEndian) { unsigned char tmp[4]={p[3],p[2],p[1],p[0]}; memcpy(&v.u32,tmp,4); }
            else memcpy(&v.u32, p, 4);
            return (float)v.u32;
        default: return 0.0f;
    }
}

static uint32_t plyReadUint(PlyType t, const unsigned char * p, int bigEndian)
{
    return (uint32_t)plyReadScalar(t, p, bigEndian);
}

/*___________________________________________________________________*/
BkpBool bkpLoadPly(BkpGpuAdapter adapter, const char * path,
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

    /* ---- parse header ---- */
    if(strncmp((char *)buf, "ply", 3) != 0)
    {
        LOG(eERROR, gTag, "Not a PLY file: '%s'", path);
        free(buf); return BKP_FALSE;
    }

    PlyFormat   format   = PLY_ASCII;
    PlyElement  elems[8]; memset(elems, 0, sizeof(elems));
    int         elemCount = 0;

    const char * p = (const char *)buf;
    while(*p)
    {
        /* skip to next line */
        while(*p && *p != '\n') ++p;
        if(*p) ++p;

        if(strncmp(p, "end_header", 10) == 0)
        {
            while(*p && *p != '\n') ++p;
            if(*p) ++p;
            break;
        }
        else if(strncmp(p, "format ascii", 12) == 0)       format = PLY_ASCII;
        else if(strncmp(p, "format binary_little_endian", 27) == 0) format = PLY_BINARY_LE;
        else if(strncmp(p, "format binary_big_endian", 24) == 0)    format = PLY_BINARY_BE;
        else if(strncmp(p, "element ", 8) == 0 && elemCount < 8)
        {
            PlyElement * el = &elems[elemCount++];
            sscanf(p + 8, "%63s %zu", el->name, &el->count);
        }
        else if(strncmp(p, "property list ", 14) == 0 && elemCount > 0)
        {
            PlyElement * el = &elems[elemCount - 1];
            if(el->propCount < 32)
            {
                PlyProp * pr = &el->props[el->propCount++];
                char ct[32], et[32];
                sscanf(p + 14, "%31s %31s %63s", ct, et, pr->name);
                pr->isList       = 1;
                pr->listCountType = parsePlyType(ct);
                pr->listElemType  = parsePlyType(et);
                pr->type          = PLY_PROP_UNKNOWN;
            }
        }
        else if(strncmp(p, "property ", 9) == 0 && elemCount > 0)
        {
            PlyElement * el = &elems[elemCount - 1];
            if(el->propCount < 32)
            {
                PlyProp * pr = &el->props[el->propCount++];
                char tn[32];
                sscanf(p + 9, "%31s %63s", tn, pr->name);
                pr->type   = parsePlyType(tn);
                pr->isList = 0;
            }
        }
    }

    /* ---- find vertex and face elements ---- */
    PlyElement * vertEl = NULL, * faceEl = NULL;
    for(int i = 0; i < elemCount; ++i)
    {
        if(!strcmp(elems[i].name, "vertex") && !vertEl) vertEl = &elems[i];
        if(!strcmp(elems[i].name, "face")   && !faceEl) faceEl = &elems[i];
    }

    if(!vertEl || vertEl->count == 0)
    {
        LOG(eERROR, gTag, "No vertex element in '%s'", path);
        free(buf); return BKP_FALSE;
    }

    /* find property indices */
    int piX=-1,piY=-1,piZ=-1,piNX=-1,piNY=-1,piNZ=-1,piU=-1,piV=-1;
    for(int i = 0; i < vertEl->propCount; ++i)
    {
        const char * n = vertEl->props[i].name;
        if(!strcmp(n,"x"))  piX  = i;
        if(!strcmp(n,"y"))  piY  = i;
        if(!strcmp(n,"z"))  piZ  = i;
        if(!strcmp(n,"nx")) piNX = i;
        if(!strcmp(n,"ny")) piNY = i;
        if(!strcmp(n,"nz")) piNZ = i;
        if(!strcmp(n,"s") || !strcmp(n,"u") || !strcmp(n,"texture_u")) piU = i;
        if(!strcmp(n,"t") || !strcmp(n,"v") || !strcmp(n,"texture_v")) piV = i;
    }

    if(piX < 0 || piY < 0 || piZ < 0)
    {
        LOG(eERROR, gTag, "PLY missing x/y/z properties in '%s'", path);
        free(buf); return BKP_FALSE;
    }

    int hasNormals = (piNX>=0 && piNY>=0 && piNZ>=0);
    int hasUVs     = (piU>=0 && piV>=0);

    size_t vertCount = vertEl->count;
    BkpVec3 * positions = (BkpVec3 *)calloc(vertCount, sizeof(BkpVec3));
    BkpVec3 * normals   = (BkpVec3 *)calloc(vertCount, sizeof(BkpVec3));
    BkpVec3 * uvs       = (BkpVec3 *)calloc(vertCount, sizeof(BkpVec3));
    BkpVec3 * tangents  = (BkpVec3 *)calloc(vertCount, sizeof(BkpVec3));

    int bigEndian = (format == PLY_BINARY_BE);

    if(format == PLY_ASCII)
    {
        /* ASCII vertex reading */
        for(size_t vi = 0; vi < vertCount; ++vi)
        {
            float vals[32] = {0};
            int pc = vertEl->propCount < 32 ? vertEl->propCount : 32;
            for(int pi = 0; pi < pc; ++pi)
            {
                while(*p == ' ' || *p == '\t') ++p;
                if(*p == '\n' || *p == '\r') ++p;
                vals[pi] = (float)strtod(p, (char **)&p);
            }
            while(*p && *p != '\n') ++p;
            if(*p) ++p;

            positions[vi].x = vals[piX];
            positions[vi].y = vals[piY];
            positions[vi].z = vals[piZ];
            if(hasNormals) { normals[vi].x = vals[piNX]; normals[vi].y = vals[piNY]; normals[vi].z = vals[piNZ]; }
            if(hasUVs)     { uvs[vi].x = vals[piU]; uvs[vi].y = vals[piV]; }
        }
    }
    else
    {
        /* binary vertex reading — compute per-vertex stride */
        const unsigned char * bp = (const unsigned char *)p;
        for(size_t vi = 0; vi < vertCount; ++vi)
        {
            float vals[32] = {0};
            for(int pi = 0; pi < vertEl->propCount && pi < 32; ++pi)
            {
                PlyProp * pr = &vertEl->props[pi];
                if(pr->isList)
                {
                    /* skip list — unusual for vertex element but handle it */
                    size_t cSz = plyTypeSize(pr->listCountType);
                    uint32_t cnt = plyReadUint(pr->listCountType, bp, bigEndian);
                    bp += cSz + cnt * plyTypeSize(pr->listElemType);
                }
                else
                {
                    vals[pi] = plyReadScalar(pr->type, bp, bigEndian);
                    bp += plyTypeSize(pr->type);
                }
            }
            positions[vi].x = vals[piX];
            positions[vi].y = vals[piY];
            positions[vi].z = vals[piZ];
            if(hasNormals) { normals[vi].x = vals[piNX]; normals[vi].y = vals[piNY]; normals[vi].z = vals[piNZ]; }
            if(hasUVs)     { uvs[vi].x = vals[piU]; uvs[vi].y = vals[piV]; }
        }
        p = (const char *)bp;
    }

    /* ---- face / index data ---- */
    uint32_t * idx32   = NULL;
    uint16_t * idx16   = NULL;
    size_t     idxCount = 0;

    /* find face list property */
    PlyProp * faceProp = NULL;
    if(faceEl)
    {
        for(int i = 0; i < faceEl->propCount; ++i)
        {
            if(faceEl->props[i].isList)
            {
                faceProp = &faceEl->props[i];
                break;
            }
        }
    }

    if(faceEl && faceProp)
    {
        /* count total indices after triangulation */
        size_t triCapIdx = faceEl->count * 3;
        if(vertCount > 0xFFFF)
            idx32 = (uint32_t *)malloc(sizeof(uint32_t) * triCapIdx * 2);
        else
            idx16 = (uint16_t *)malloc(sizeof(uint16_t) * triCapIdx * 2);
        size_t idxCap = triCapIdx * 2;

        if(format == PLY_ASCII)
        {
            for(size_t fi = 0; fi < faceEl->count; ++fi)
            {
                while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
                int fc = (int)strtol(p, (char **)&p, 10);
                uint32_t fv[64];
                for(int k = 0; k < fc && k < 64; ++k)
                    fv[k] = (uint32_t)strtol(p, (char **)&p, 10);
                while(*p && *p != '\n') ++p;
                if(*p) ++p;

                /* fan triangulate */
                for(int t = 1; t + 1 < fc; ++t)
                {
                    if(idxCount + 3 > idxCap)
                    {
                        idxCap *= 2;
                        if(idx32) idx32 = (uint32_t *)realloc(idx32, sizeof(uint32_t)*idxCap);
                        if(idx16) idx16 = (uint16_t *)realloc(idx16, sizeof(uint16_t)*idxCap);
                    }
                    if(idx32) { idx32[idxCount]=fv[0]; idx32[idxCount+1]=fv[t]; idx32[idxCount+2]=fv[t+1]; }
                    if(idx16) { idx16[idxCount]=(uint16_t)fv[0]; idx16[idxCount+1]=(uint16_t)fv[t]; idx16[idxCount+2]=(uint16_t)fv[t+1]; }
                    idxCount += 3;
                }
            }
        }
        else
        {
            const unsigned char * bp = (const unsigned char *)p;
            size_t cSz = plyTypeSize(faceProp->listCountType);
            size_t eSz = plyTypeSize(faceProp->listElemType);

            for(size_t fi = 0; fi < faceEl->count; ++fi)
            {
                /* skip other non-list props before face prop */
                for(int pi = 0; pi < faceEl->propCount; ++pi)
                {
                    PlyProp * pr = &faceEl->props[pi];
                    if(pr == faceProp) break;
                    if(!pr->isList) bp += plyTypeSize(pr->type);
                }

                uint32_t fc = plyReadUint(faceProp->listCountType, bp, bigEndian);
                bp += cSz;
                uint32_t fv[64];
                for(uint32_t k = 0; k < fc && k < 64; ++k)
                {
                    fv[k] = plyReadUint(faceProp->listElemType, bp, bigEndian);
                    bp += eSz;
                }
                /* skip remaining face props */
                for(int pi = 0; pi < faceEl->propCount; ++pi)
                {
                    PlyProp * pr = &faceEl->props[pi];
                    if(pr == faceProp) continue;
                    if(pr->isList)
                    {
                        uint32_t cnt = plyReadUint(pr->listCountType, bp, bigEndian);
                        bp += plyTypeSize(pr->listCountType) + cnt * plyTypeSize(pr->listElemType);
                    }
                    else bp += plyTypeSize(pr->type);
                }

                for(uint32_t t = 1; t + 1 < fc; ++t)
                {
                    if(idxCount + 3 > idxCap)
                    {
                        idxCap *= 2;
                        if(idx32) idx32 = (uint32_t *)realloc(idx32, sizeof(uint32_t)*idxCap);
                        if(idx16) idx16 = (uint16_t *)realloc(idx16, sizeof(uint16_t)*idxCap);
                    }
                    if(idx32) { idx32[idxCount]=fv[0]; idx32[idxCount+1]=fv[t]; idx32[idxCount+2]=fv[t+1]; }
                    if(idx16) { idx16[idxCount]=(uint16_t)fv[0]; idx16[idxCount+1]=(uint16_t)fv[t]; idx16[idxCount+2]=(uint16_t)fv[t+1]; }
                    idxCount += 3;
                }
            }
        }
    }

    if(hasUVs)
    {
        if(idxCount > 0)
        {
            for(size_t tri = 0; tri + 2 < idxCount; tri += 3)
            {
                uint32_t a = idx32 ? idx32[tri] : idx16[tri];
                uint32_t b = idx32 ? idx32[tri+1] : idx16[tri+1];
                uint32_t c = idx32 ? idx32[tri+2] : idx16[tri+2];
                if(a < vertCount && b < vertCount && c < vertCount)
                {
                    BkpTangentBitangent tb = bkpCalculateTangent(
                        &positions[a], &positions[b], &positions[c],
                        &uvs[a],       &uvs[b],       &uvs[c]);
                    tangents[a].x += tb.t.x; tangents[a].y += tb.t.y; tangents[a].z += tb.t.z;
                    tangents[b].x += tb.t.x; tangents[b].y += tb.t.y; tangents[b].z += tb.t.z;
                    tangents[c].x += tb.t.x; tangents[c].y += tb.t.y; tangents[c].z += tb.t.z;
                }
            }
        }
        else
        {
            for(size_t tri = 0; tri + 2 < vertCount; tri += 3)
            {
                BkpTangentBitangent tb = bkpCalculateTangent(
                    &positions[tri], &positions[tri+1], &positions[tri+2],
                    &uvs[tri],       &uvs[tri+1],       &uvs[tri+2]);
                tangents[tri] = tangents[tri+1] = tangents[tri+2] = tb.t;
            }
        }
    }

    /* AABB */
    if(vertCount > 0)
    {
        float mn[3] = {positions[0].x, positions[0].y, positions[0].z};
        float mx[3] = {positions[0].x, positions[0].y, positions[0].z};
        for(size_t i = 1; i < vertCount; ++i)
        {
            if(positions[i].x < mn[0]) mn[0] = positions[i].x;
            if(positions[i].x > mx[0]) mx[0] = positions[i].x;
            if(positions[i].y < mn[1]) mn[1] = positions[i].y;
            if(positions[i].y > mx[1]) mx[1] = positions[i].y;
            if(positions[i].z < mn[2]) mn[2] = positions[i].z;
            if(positions[i].z > mx[2]) mx[2] = positions[i].z;
        }
        for(int k = 0; k < 3; ++k) { model->aabbMin[k] = mn[k]; model->aabbMax[k] = mx[k]; }
    }

    uint32_t meshCountBefore = model->meshCount;
    BkpModelMesh mesh = {};
    strncpy(mesh.name, path, sizeof(mesh.name) - 1);
    mesh.meshAabbMin[0] = model->aabbMin[0]; mesh.meshAabbMin[1] = model->aabbMin[1]; mesh.meshAabbMin[2] = model->aabbMin[2];
    mesh.meshAabbMax[0] = model->aabbMax[0]; mesh.meshAabbMax[1] = model->aabbMax[1]; mesh.meshAabbMax[2] = model->aabbMax[2];

    bkpMakeMeshBufferEx(adapter, vertCount,
                        positions, normals, uvs, tangents, NULL,
                        idxCount, idx32, idx16,
                        &mesh.geo, scratchGroupId, BKP_NORMALS_RECOMPUTE);
    bkp_modelPushMesh(model, &mesh);
    makeRootNodes(model, meshCountBefore, 1);

    free(positions); free(normals); free(uvs); free(tangents);
    free(idx32); free(idx16);
    free(buf);

    LOG(eDEBUG, gTag, "Loaded '%s' : %zu vertices, %zu indices", path, vertCount, idxCount);
    return BKP_TRUE;
}

#ifdef __cplusplus
}
#endif
