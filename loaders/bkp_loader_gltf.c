#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <system/include/bkp_log.h>
#include "include/bkp_loaders.h"

#define CGLTF_IMPLEMENTATION
#include <thirdparty/cgltf.h>

static const char * gTag = "GLTF";

/* slot helpers declared in bkp_loaders.c */
void              bkp_modelPushMesh  (BkpModel *, BkpModelMesh *);
BkpModelMesh *    bkp_modelMeshSlot  (BkpModel *);
BkpMaterial *     bkp_modelMatSlot   (BkpModel *);
BkpModelTexture * bkp_modelTexSlot   (BkpModel *);
BkpNode *         bkp_modelNodeSlot  (BkpModel *);
BkpAnimation *    bkp_modelAnimSlot  (BkpModel *);
BkpSkin *         bkp_modelSkinSlot  (BkpModel *);

/*___________________________________________________________________*/
static void normVec3(float v[3])
{
    float len = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if(len > 1e-6f) { v[0]/=len; v[1]/=len; v[2]/=len; }
}

/* resolve external texture URI relative to the GLTF file path */
static void resolvePath(const char * gltfPath, const char * uri,
                        char * out, size_t outSize)
{
    const char * s1 = strrchr(gltfPath, '/');
    const char * s2 = strrchr(gltfPath, '\\');
    const char * sep = (s2 > s1) ? s2 : s1;
    if(sep)
    {
        size_t dirLen = (size_t)(sep - gltfPath) + 1;
        if(dirLen < outSize)
        {
            memcpy(out, gltfPath, dirLen);
            strncpy(out + dirLen, uri, outSize - dirLen - 1);
            out[outSize-1] = '\0';
            return;
        }
    }
    strncpy(out, uri, outSize - 1);
    out[outSize-1] = '\0';
}

/*___________________________________________________________________*/
/* Phase 1 — textures
 * CPU decode (stb_image) runs in parallel — one thread per image.
 * GPU upload is sequential after all decodes finish (Vulkan staging
 * buffers require single-threaded command recording).                */
static void loadTextures(BkpGpuAdapter adapter, cgltf_data * data,
                         const char * path, BkpModel * model)
{
    size_t n = data->images_count;
    if(n == 0) return;

    /* resolve paths for external URIs (needs a stable buffer) */
    char (* resolved)[512] = (char (*)[512])malloc(n * 512);

    BkpTextureSource * sources =
        (BkpTextureSource *)calloc(n, sizeof(BkpTextureSource));

    /* allocate all texture slots up-front so the array won't realloc */
    size_t texBase = model->textureCount;
    for(size_t i = 0; i < n; ++i)
    {
        BkpModelTexture * tex = bkp_modelTexSlot(model);
        memset(tex, 0, sizeof(*tex));
        if(data->images[i].name) strncpy(tex->name, data->images[i].name, sizeof(tex->name)-1);
        bkpSetDefaultTextureInfo(&tex->image);
    }

    /* pre-pass: normal / ORM / occlusion textures are linear — override to UNORM */
    for(size_t m = 0; m < data->materials_count; ++m)
    {
        cgltf_material * mat = &data->materials[m];
        cgltf_texture  * linear[3] = {
            mat->normal_texture.texture,
            mat->has_pbr_metallic_roughness
                ? mat->pbr_metallic_roughness.metallic_roughness_texture.texture : NULL,
            mat->occlusion_texture.texture,
        };
        for(int t = 0; t < 3; ++t)
        {
            if(!linear[t] || !linear[t]->image) continue;
            size_t idx = (size_t)(linear[t]->image - data->images);
            if(idx >= n) continue;
            BkpImageResource * img = &model->textures[texBase + idx].image;
            img->imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            img->viewInfo.format  = VK_FORMAT_R8G8B8A8_UNORM;
        }
    }

    for(size_t i = 0; i < n; ++i)
    {
        cgltf_image * img = &data->images[i];
        if(img->buffer_view)
        {
            sources[i].encoded.buffer = img->buffer_view->buffer->data;
            sources[i].encoded.offset = img->buffer_view->offset;
            sources[i].encoded.size   = img->buffer_view->size;
        }
        else if(img->uri)
        {
            resolvePath(path, img->uri, resolved[i], 512);
            sources[i].filePath = resolved[i];
        }
        /* else: no source — bkpUploadTextureBatch will skip it */
    }

    /* bkpUploadTextureBatch expects a flat BkpImageResource array;
       BkpModelTexture has a larger stride, so use a temp array. */
    BkpImageResource * imgs = (BkpImageResource *)calloc(n, sizeof(BkpImageResource));
    for(size_t i = 0; i < n; ++i)
    {
      imgs[i] = model->textures[texBase + i].image;
    }

    LOG(eDEBUG, gTag, "uploadTextureBatch start: %zu texture(s)", n);
    bkpUploadTextureBatch(adapter, sources, n, imgs);
    LOG(eDEBUG, gTag, "uploadTextureBatch done");

    for(size_t i = 0; i < n; ++i)
    {
      model->textures[texBase + i].image = imgs[i];
    }

    free(imgs);
    free(sources);
    free(resolved);
}

/*___________________________________________________________________*/
/* Resolve cgltf_image* → index in model->textures (-1 if NULL) */
static int texIdx(cgltf_data * data, cgltf_texture * t)
{
    if(!t || !t->image) return -1;
    return (int)(t->image - data->images);
}

/* Phase 2 — PBR materials */
static void loadMaterials(cgltf_data * data, BkpModel * model)
{
    for(size_t i = 0; i < data->materials_count; ++i)
    {
        cgltf_material * cm  = &data->materials[i];
        BkpMaterial    * mat = bkp_modelMatSlot(model);
        memset(mat, 0, sizeof(*mat));
        if(cm->name) strncpy(mat->name, cm->name, sizeof(mat->name)-1);

        mat->albedoIdx             = -1;
        mat->normalIdx             = -1;
        mat->metallicRoughnessIdx  = -1;
        mat->emissiveIdx           = -1;
        mat->occlusionIdx          = -1;

        /* defaults */
        mat->baseColor[0] = mat->baseColor[1] = mat->baseColor[2] = mat->baseColor[3] = 1.0f;
        mat->metallic  = 1.0f;
        mat->roughness = 1.0f;

        if(cm->has_pbr_metallic_roughness)
        {
            cgltf_pbr_metallic_roughness * pbr = &cm->pbr_metallic_roughness;
            for(int k = 0; k < 4; ++k) mat->baseColor[k] = pbr->base_color_factor[k];
            mat->metallic  = pbr->metallic_factor;
            mat->roughness = pbr->roughness_factor;
            mat->albedoIdx            = texIdx(data, pbr->base_color_texture.texture);
            mat->metallicRoughnessIdx = texIdx(data, pbr->metallic_roughness_texture.texture);
        }
        mat->normalIdx    = texIdx(data, cm->normal_texture.texture);
        mat->emissiveIdx  = texIdx(data, cm->emissive_texture.texture);
        mat->occlusionIdx = texIdx(data, cm->occlusion_texture.texture);
        for(int k = 0; k < 3; ++k) mat->emissive[k] = cm->emissive_factor[k];
        mat->doubleSided  = cm->double_sided ? 1 : 0;
        mat->alphaCutoff  = cm->alpha_cutoff;
        switch(cm->alpha_mode) {
            case cgltf_alpha_mode_mask:  mat->alphaMode = BKP_ALPHA_MASK;  break;
            case cgltf_alpha_mode_blend: mat->alphaMode = BKP_ALPHA_BLEND; break;
            default:                     mat->alphaMode = BKP_ALPHA_OPAQUE; break;
        }
    }
}

/*___________________________________________________________________*/
/* Phase 3 — mesh primitives (skinned or static) */
static void loadPrimitive(BkpGpuAdapter adapter, cgltf_primitive * prim,
                          const char * name, int materialIdx,
                          BkpModel * model, size_t scratchGroupId)
{
    size_t n = 0;
    BkpVec3  * positions = NULL, * normals = NULL, * uvs = NULL, * tangents = NULL;
    BkpVec4i * joints    = NULL;
    BkpVec4  * weights   = NULL;

    for(uint32_t a = 0; a < prim->attributes_count; ++a)
    {
        cgltf_attribute * attr = &prim->attributes[a];
        cgltf_accessor  * acc  = attr->data;
        if(!n) n = acc->count;

        if(attr->type == cgltf_attribute_type_position && !positions)
        {
            positions = (BkpVec3 *)malloc(sizeof(BkpVec3) * n);
            for(size_t i = 0; i < n; ++i)
            {
                float p[3]; cgltf_accessor_read_float(acc, i, p, 3);
                positions[i].x = p[0]; positions[i].y = p[1]; positions[i].z = p[2];
            }
        }
        else if(attr->type == cgltf_attribute_type_normal && !normals)
        {
            normals = (BkpVec3 *)malloc(sizeof(BkpVec3) * n);
            for(size_t i = 0; i < n; ++i)
            {
                float v[3]; cgltf_accessor_read_float(acc, i, v, 3);
                normVec3(v);
                normals[i].x = v[0]; normals[i].y = v[1]; normals[i].z = v[2];
            }
        }
        else if(attr->type == cgltf_attribute_type_texcoord && attr->index == 0 && !uvs)
        {
            uvs = (BkpVec3 *)calloc(n, sizeof(BkpVec3));
            for(size_t i = 0; i < n; ++i)
            {
                float uv[2]; cgltf_accessor_read_float(acc, i, uv, 2);
                uvs[i].x = uv[0]; uvs[i].y = uv[1];
            }
        }
        else if(attr->type == cgltf_attribute_type_tangent && !tangents)
        {
            tangents = (BkpVec3 *)malloc(sizeof(BkpVec3) * n);
            for(size_t i = 0; i < n; ++i)
            {
                float t[4]; cgltf_accessor_read_float(acc, i, t, 4);
                tangents[i].x = t[0]; tangents[i].y = t[1]; tangents[i].z = t[2];
            }
        }
        else if(attr->type == cgltf_attribute_type_joints && attr->index == 0 && !joints)
        {
            joints = (BkpVec4i *)malloc(sizeof(BkpVec4i) * n);
            for(size_t i = 0; i < n; ++i)
            {
                unsigned int j[4]; cgltf_accessor_read_uint(acc, i, j, 4);
                joints[i].x = j[0]; joints[i].y = j[1];
                joints[i].z = j[2]; joints[i].w = j[3];
            }
        }
        else if(attr->type == cgltf_attribute_type_weights && attr->index == 0 && !weights)
        {
            weights = (BkpVec4 *)malloc(sizeof(BkpVec4) * n);
            for(size_t i = 0; i < n; ++i)
            {
                float w[4]; cgltf_accessor_read_float(acc, i, w, 4);
                weights[i].x = w[0]; weights[i].y = w[1];
                weights[i].z = w[2]; weights[i].w = w[3];
            }
        }
    }

    if(!positions || n == 0)
    {
        LOG(eWARNING, gTag, "Primitive '%s' has no positions, skipped", name);
        goto cleanup;
    }

    /* Ensure BkpVertex (eVERTEX, 48-byte stride) path is always taken for
       non-skinned meshes.  Any missing attribute would select a smaller vertex
       type (e.g. BkpVertexBasicUV = 36 bytes) that mismatches the pipeline. */
    if(!joints)
    {
        if(!uvs)
        {
            uvs = (BkpVec3 *)calloc(n, sizeof(BkpVec3));
        }
        if(!tangents)
        {
            tangents = (BkpVec3 *)malloc(sizeof(BkpVec3) * n);
            for(size_t i = 0; i < n; ++i)
                tangents[i].x = 1.0f, tangents[i].y = 0.0f, tangents[i].z = 0.0f;
        }
    }

    /* indices */
    uint32_t * idx32   = NULL;
    uint16_t * idx16   = NULL;
    size_t     idxCount = 0;

    if(prim->indices)
    {
        idxCount = prim->indices->count;
        if(n > 0xFFFF)
        {
            idx32 = (uint32_t *)malloc(sizeof(uint32_t) * idxCount);
            for(size_t i = 0; i < idxCount; ++i)
                idx32[i] = (uint32_t)cgltf_accessor_read_index(prim->indices, i);
        }
        else
        {
            idx16 = (uint16_t *)malloc(sizeof(uint16_t) * idxCount);
            for(size_t i = 0; i < idxCount; ++i)
                idx16[i] = (uint16_t)cgltf_accessor_read_index(prim->indices, i);
        }
    }

    float mMin[3]={1e30f,1e30f,1e30f}, mMax[3]={-1e30f,-1e30f,-1e30f};
    for(size_t i=0;i<n;++i){
        if(positions[i].x<mMin[0])mMin[0]=positions[i].x; if(positions[i].x>mMax[0])mMax[0]=positions[i].x;
        if(positions[i].y<mMin[1])mMin[1]=positions[i].y; if(positions[i].y>mMax[1])mMax[1]=positions[i].y;
        if(positions[i].z<mMin[2])mMin[2]=positions[i].z; if(positions[i].z>mMax[2])mMax[2]=positions[i].z;
    }

    {
        BkpModelMesh * mesh = bkp_modelMeshSlot(model);
        memset(mesh, 0, sizeof(*mesh));
        strncpy(mesh->name, name ? name : "mesh", sizeof(mesh->name)-1);
        mesh->materialIdx = materialIdx;
        mesh->isSkinned   = (joints != NULL) ? BKP_TRUE : BKP_FALSE;
        mesh->meshAabbMin[0]=mMin[0]; mesh->meshAabbMin[1]=mMin[1]; mesh->meshAabbMin[2]=mMin[2];
        mesh->meshAabbMax[0]=mMax[0]; mesh->meshAabbMax[1]=mMax[1]; mesh->meshAabbMax[2]=mMax[2];

        if(joints)
        {
            bkpMakeSkinMeshBufferEx(adapter, n,
                                    positions, normals, uvs, tangents,
                                    joints, weights,
                                    idxCount, idx32, idx16,
                                    &mesh->geo, scratchGroupId);
        }
        else
        {
            bkpMakeMeshBufferEx(adapter, n,
                                positions, normals, uvs, tangents, NULL,
                                idxCount, idx32, idx16,
                                &mesh->geo, scratchGroupId, BKP_NORMALS_KEEP);

}
    }

    free(idx32); free(idx16);
cleanup:
    free(positions); free(normals); free(uvs); free(tangents);
    free(joints); free(weights);
}

/* Phase 3 entry: load all cgltf_mesh primitives, record first/count per mesh */
static void loadMeshes(BkpGpuAdapter adapter, cgltf_data * data,
                       BkpModel * model, size_t scratchGroupId,
                       size_t * firstPrim, uint32_t * primCount)
{
    for(size_t mi = 0; mi < data->meshes_count; ++mi)
    {
        cgltf_mesh * m = &data->meshes[mi];
        firstPrim[mi]  = model->meshCount;
        primCount[mi]  = 0;
        for(cgltf_size pi = 0; pi < m->primitives_count; ++pi)
        {
            cgltf_primitive * prim = &m->primitives[pi];
            int matIdx = prim->material
                         ? (int)(prim->material - data->materials) : -1;
            loadPrimitive(adapter, prim,
                          m->name ? m->name : "mesh",
                          matIdx, model, scratchGroupId);
            ++primCount[mi];
        }
    }
}

/*___________________________________________________________________*/
/* Phase 4 — nodes */
static void loadNodes(cgltf_data * data, BkpModel * model,
                      const size_t * firstPrim, const uint32_t * primCount)
{
    /* First pass: allocate all nodes with TRS / matrix and mesh/skin refs */
    for(size_t i = 0; i < data->nodes_count; ++i)
    {
        cgltf_node * cn   = &data->nodes[i];
        BkpNode    * node = bkp_modelNodeSlot(model);
        memset(node, 0, sizeof(*node));
        node->parentIdx = -1;
        node->meshIdx   = -1;
        node->skinIdx   = -1;
        node->scale[0] = node->scale[1] = node->scale[2] = 1.0f;
        node->rotation[3] = 1.0f; /* identity quaternion w=1 */

        if(cn->name) strncpy(node->name, cn->name, sizeof(node->name)-1);

        if(cn->has_matrix)
        {
            /* explicit 4×4 — TRS left as identity defaults */
            memcpy(node->localMatrix, cn->matrix, sizeof(node->localMatrix));
        }
        else
        {
            if(cn->has_translation)
            {
                node->translation[0] = cn->translation[0];
                node->translation[1] = cn->translation[1];
                node->translation[2] = cn->translation[2];
            }
            if(cn->has_rotation)
            {
                node->rotation[0] = cn->rotation[0];
                node->rotation[1] = cn->rotation[1];
                node->rotation[2] = cn->rotation[2];
                node->rotation[3] = cn->rotation[3];
            }
            if(cn->has_scale)
            {
                node->scale[0] = cn->scale[0];
                node->scale[1] = cn->scale[1];
                node->scale[2] = cn->scale[2];
            }
            /* cgltf_node_transform_local handles has_matrix / TRS */
            cgltf_node_transform_local(cn, node->localMatrix);
        }

        if(cn->mesh)
        {
            size_t mi       = (size_t)(cn->mesh - data->meshes);
            node->meshIdx   = (int)firstPrim[mi];
            node->meshCount = primCount[mi];
        }
        if(cn->skin)
            node->skinIdx = (int)(cn->skin - data->skins);
    }

    /* Second pass: children arrays + parents
     * Parent may come after child in the array — scan all nodes' children. */
    for(size_t i = 0; i < data->nodes_count; ++i)
    {
        cgltf_node * cn   = &data->nodes[i];
        BkpNode    * node = &model->nodes[i];

        node->childCount = (uint32_t)cn->children_count;
        if(cn->children_count > 0)
        {
            node->children = (int *)malloc(sizeof(int) * cn->children_count);
            for(size_t c = 0; c < cn->children_count; ++c)
            {
                int childIdx = (int)(cn->children[c] - data->nodes);
                node->children[c] = childIdx;
                model->nodes[childIdx].parentIdx = (int)i;
            }
        }
    }
}

/*___________________________________________________________________*/
/* Phase 5 — skins */
static void loadSkins(cgltf_data * data, BkpModel * model)
{
    for(size_t i = 0; i < data->skins_count; ++i)
    {
        cgltf_skin * cs   = &data->skins[i];
        BkpSkin    * skin = bkp_modelSkinSlot(model);
        memset(skin, 0, sizeof(*skin));
        if(cs->name) strncpy(skin->name, cs->name, sizeof(skin->name)-1);

        skin->jointCount      = (uint32_t)cs->joints_count;
        skin->skeletonRootIdx = cs->skeleton
                                ? (int)(cs->skeleton - data->nodes) : -1;

        skin->joints = (int *)malloc(sizeof(int) * cs->joints_count);
        for(size_t j = 0; j < cs->joints_count; ++j)
            skin->joints[j] = (int)(cs->joints[j] - data->nodes);

        if(cs->inverse_bind_matrices)
        {
            skin->inverseBindMatrices =
                (float *)malloc(sizeof(float) * 16 * cs->joints_count);
            for(size_t j = 0; j < cs->joints_count; ++j)
                cgltf_accessor_read_float(cs->inverse_bind_matrices, j,
                                          skin->inverseBindMatrices + j * 16, 16);
        }
    }
}

/*___________________________________________________________________*/
/* Phase 6 — animations */
static void loadAnimations(cgltf_data * data, BkpModel * model)
{
    for(size_t a = 0; a < data->animations_count; ++a)
    {
        cgltf_animation * ca   = &data->animations[a];
        BkpAnimation    * anim = bkp_modelAnimSlot(model);
        memset(anim, 0, sizeof(*anim));
        if(ca->name) strncpy(anim->name, ca->name, sizeof(anim->name)-1);

        anim->channelCount = (uint32_t)ca->channels_count;
        anim->channels     = (BkpAnimChannel *)
                             calloc(ca->channels_count, sizeof(BkpAnimChannel));

        for(size_t c = 0; c < ca->channels_count; ++c)
        {
            cgltf_animation_channel * cc  = &ca->channels[c];
            cgltf_animation_sampler * cs  = cc->sampler;
            BkpAnimChannel          * ch  = &anim->channels[c];

            ch->nodeIdx = cc->target_node
                          ? (int)(cc->target_node - data->nodes) : -1;

            switch(cc->target_path)
            {
                case cgltf_animation_path_type_translation: ch->path = BKP_ANIM_T; break;
                case cgltf_animation_path_type_rotation:    ch->path = BKP_ANIM_R; break;
                default:                                    ch->path = BKP_ANIM_S; break;
            }

            switch(cs->interpolation)
            {
                case cgltf_interpolation_type_step:         ch->sampler.interp = BKP_INTERP_STEP;        break;
                case cgltf_interpolation_type_cubic_spline: ch->sampler.interp = BKP_INTERP_CUBICSPLINE; break;
                default:                                    ch->sampler.interp = BKP_INTERP_LINEAR;       break;
            }

            /* input — keyframe times */
            size_t kCount = cs->input->count;
            ch->sampler.count = (uint32_t)kCount;
            ch->sampler.times = (float *)malloc(sizeof(float) * kCount);
            for(size_t k = 0; k < kCount; ++k)
            {
                float t;
                cgltf_accessor_read_float(cs->input, k, &t, 1);
                ch->sampler.times[k] = t;
                if(t > anim->duration) anim->duration = t;
            }

            /* output — per-keyframe values */
            uint32_t comp = (ch->path == BKP_ANIM_R) ? 4u : 3u;
            size_t valCount = kCount;
            if(ch->sampler.interp == BKP_INTERP_CUBICSPLINE) valCount *= 3;

            ch->sampler.values = (float *)malloc(sizeof(float) * valCount * comp);
            for(size_t k = 0; k < valCount; ++k)
                cgltf_accessor_read_float(cs->output, k,
                                          ch->sampler.values + k * comp, comp);
        }
    }
}

/*___________________________________________________________________*/
BkpBool bkpLoadGltf(BkpGpuAdapter adapter, const char * path,
                    BkpModel * model, size_t scratchGroupId)
{
    cgltf_options options = {};
    cgltf_data  * data    = NULL;

    if(cgltf_parse_file(&options, path, &data) != cgltf_result_success)
    { LOG(eERROR, gTag, "Failed to parse '%s'", path); return BKP_FALSE; }

    if(cgltf_load_buffers(&options, data, path) != cgltf_result_success)
    { cgltf_free(data); LOG(eERROR, gTag, "Failed to load buffers for '%s'", path); return BKP_FALSE; }

    if(cgltf_validate(data) != cgltf_result_success)
    { cgltf_free(data); LOG(eERROR, gTag, "Validation failed for '%s'", path); return BKP_FALSE; }

    /* scratch arrays to map cgltf_mesh index → first primitive + count */
    size_t   * firstPrim = (size_t *)  calloc(data->meshes_count + 1, sizeof(size_t));
    uint32_t * primCount = (uint32_t *)calloc(data->meshes_count + 1, sizeof(uint32_t));

//    LOG(eDEBUG, gTag, "loadTextures start");
    loadTextures  (adapter, data, path, model);
 //   LOG(eDEBUG, gTag, "loadMaterials start");
    loadMaterials (data, model);
  //  LOG(eDEBUG, gTag, "loadMeshes start");
    loadMeshes    (adapter, data, model, scratchGroupId, firstPrim, primCount);
   // LOG(eDEBUG, gTag, "loadNodes start");
    loadNodes     (data, model, firstPrim, primCount);
    loadSkins     (data, model);
    loadAnimations(data, model);

    /* root nodes from active scene */
    if(data->scene)
    {
        model->rootNodeCount = (uint32_t)data->scene->nodes_count;
        model->rootNodes = (int *)malloc(sizeof(int) * data->scene->nodes_count);
        for(size_t i = 0; i < data->scene->nodes_count; ++i)
            model->rootNodes[i] = (int)(data->scene->nodes[i] - data->nodes);
    }

    /* world-space AABB from position accessor min/max through node transforms */
    {
        float mn[3] = { 1e30f,  1e30f,  1e30f};
        float mx[3] = {-1e30f, -1e30f, -1e30f};
        for(size_t ni = 0; ni < data->nodes_count; ++ni)
        {
            cgltf_node * cn = &data->nodes[ni];
            if(!cn->mesh) continue;
            float world[16];
            cgltf_node_transform_world(cn, world);
            for(cgltf_size pi = 0; pi < cn->mesh->primitives_count; ++pi)
            {
                cgltf_primitive * prim = &cn->mesh->primitives[pi];
                for(cgltf_size ai = 0; ai < prim->attributes_count; ++ai)
                {
                    if(prim->attributes[ai].type != cgltf_attribute_type_position) continue;
                    cgltf_accessor * acc = prim->attributes[ai].data;
                    if(!acc->has_min || !acc->has_max) continue;
                    for(int c = 0; c < 8; ++c)
                    {
                        float p[4] = {
                            (c & 1) ? (float)acc->max[0] : (float)acc->min[0],
                            (c & 2) ? (float)acc->max[1] : (float)acc->min[1],
                            (c & 4) ? (float)acc->max[2] : (float)acc->min[2],
                            1.0f
                        };
                        /* column-major: world[col*4 + row] */
                        float wp[3];
                        for(int r = 0; r < 3; ++r)
                            wp[r] = world[0*4+r]*p[0] + world[1*4+r]*p[1]
                                  + world[2*4+r]*p[2] + world[3*4+r]*p[3];
                        if(wp[0] < mn[0]) mn[0] = wp[0];
                        if(wp[1] < mn[1]) mn[1] = wp[1];
                        if(wp[2] < mn[2]) mn[2] = wp[2];
                        if(wp[0] > mx[0]) mx[0] = wp[0];
                        if(wp[1] > mx[1]) mx[1] = wp[1];
                        if(wp[2] > mx[2]) mx[2] = wp[2];
                    }
                }
            }
        }
        if(mn[0] > mx[0]) { mn[0]=mn[1]=mn[2]=-1.f; mx[0]=mx[1]=mx[2]=1.f; }
        model->aabbMin[0] = mn[0]; model->aabbMin[1] = mn[1]; model->aabbMin[2] = mn[2];
        model->aabbMax[0] = mx[0]; model->aabbMax[1] = mx[1]; model->aabbMax[2] = mx[2];
    }

    free(firstPrim);
    free(primCount);
    cgltf_free(data);

    LOG(eDEBUG, gTag, "Loaded '%s': %u mesh(es) %u node(s) %u anim(s) %u skin(s)",
        path, model->meshCount, model->nodeCount,
        model->animationCount, model->skinCount);
    return BKP_TRUE;
}

#ifdef __cplusplus
}
#endif
