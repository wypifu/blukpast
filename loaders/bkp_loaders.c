#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <system/include/bkp_log.h>
#include "include/bkp_loaders.h"

static const char * gTag = "Loader";

#ifdef _WIN32
#  define bkp_strcasecmp _stricmp
#else
#  include <strings.h>
#  define bkp_strcasecmp strcasecmp
#endif

/*___________________________________________________________________*/
static const char * fileExtension(const char * path)
{
    const char * dot = strrchr(path, '.');
    return dot ? dot + 1 : "";
}

/*___________________________________________________________________*/
/* Generic grow-by-2x helper */
#define PUSH_SLOT(arr, cnt, cap, T)                                        \
    if(model->cnt >= model->cap)                                           \
    {                                                                      \
        model->cap = model->cap ? model->cap * 2 : 8;                     \
        model->arr = (T *)realloc(model->arr,                              \
                     sizeof(T) * model->cap);                              \
    }                                                                      \
    return &model->arr[model->cnt++]

static BkpModelMesh *    meshSlot   (BkpModel * model){ PUSH_SLOT(meshes,      meshCount,      _meshCap, BkpModelMesh);    }
static BkpMaterial *     matSlot    (BkpModel * model){ PUSH_SLOT(materials,   materialCount,  _matCap,  BkpMaterial);     }
static BkpModelTexture * texSlot    (BkpModel * model){ PUSH_SLOT(textures,    textureCount,   _texCap,  BkpModelTexture); }
static BkpNode *         nodeSlot   (BkpModel * model){ PUSH_SLOT(nodes,       nodeCount,      _nodeCap, BkpNode);         }
static BkpAnimation *    animSlot   (BkpModel * model){ PUSH_SLOT(animations,  animationCount, _animCap, BkpAnimation);    }
static BkpSkin *         skinSlot   (BkpModel * model){ PUSH_SLOT(skins,       skinCount,      _skinCap, BkpSkin);         }

/*___________________________________________________________________*/
BkpBool bkpLoadModel(BkpGpuAdapter adapter, const char * path,
                     BkpModel * model, size_t scratchGroupId)
{
    memset(model, 0, sizeof(*model));

    const char * ext = fileExtension(path);

    if(bkp_strcasecmp(ext, "glb") == 0 || bkp_strcasecmp(ext, "gltf") == 0)
        return bkpLoadGltf(adapter, path, model, scratchGroupId);
    if(bkp_strcasecmp(ext, "obj") == 0)
        return bkpLoadObj(adapter, path, model, scratchGroupId);
    if(bkp_strcasecmp(ext, "stl") == 0)
        return bkpLoadStl(adapter, path, model, scratchGroupId);
    if(bkp_strcasecmp(ext, "ply") == 0)
        return bkpLoadPly(adapter, path, model, scratchGroupId);

    LOG(eERROR, gTag, "Unknown format: '%s'", ext);
    return BKP_FALSE;
}

/*___________________________________________________________________*/
void bkpUnloadModel(BkpGpuAdapter adapter, BkpModel * model)
{
    for(uint32_t i = 0; i < model->meshCount; ++i)
        bkpFreeBuffer(adapter, &model->meshes[i].geo.buffer);
    free(model->meshes);

    for(uint32_t i = 0; i < model->textureCount; ++i)
        bkpDestroyImageResource(adapter, &model->textures[i].image);
    free(model->textures);

    free(model->materials);

    for(uint32_t i = 0; i < model->nodeCount; ++i)
        free(model->nodes[i].children);
    free(model->nodes);

    free(model->rootNodes);

    for(uint32_t i = 0; i < model->animationCount; ++i)
    {
        BkpAnimation * a = &model->animations[i];
        for(uint32_t c = 0; c < a->channelCount; ++c)
        {
            free(a->channels[c].sampler.times);
            free(a->channels[c].sampler.values);
        }
        free(a->channels);
    }
    free(model->animations);

    for(uint32_t i = 0; i < model->skinCount; ++i)
    {
        free(model->skins[i].joints);
        free(model->skins[i].inverseBindMatrices);
    }
    free(model->skins);

    memset(model, 0, sizeof(*model));
}

/*___________________________________________________________________*/
/* Exposed to format-specific TUs */

void bkp_modelPushMesh(BkpModel * model, BkpModelMesh * mesh)
{
    *meshSlot(model) = *mesh;
}

BkpModelMesh *    bkp_modelMeshSlot   (BkpModel * m) { return meshSlot(m);  }
BkpMaterial *     bkp_modelMatSlot    (BkpModel * m) { return matSlot(m);   }
BkpModelTexture * bkp_modelTexSlot    (BkpModel * m) { return texSlot(m);   }
BkpNode *         bkp_modelNodeSlot   (BkpModel * m) { return nodeSlot(m);  }
BkpAnimation *    bkp_modelAnimSlot   (BkpModel * m) { return animSlot(m);  }
BkpSkin *         bkp_modelSkinSlot   (BkpModel * m) { return skinSlot(m);  }

#ifdef __cplusplus
}
#endif
