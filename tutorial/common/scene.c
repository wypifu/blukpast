#include "scene.h"

/* =========================================================================
 * Math helpers
 * ========================================================================= */
BkpMat4 makeModel(float tx, float ty, float tz, float sx, float sy, float sz)
{
    BkpMat4 m;
    bkpSetIdentityMat4(&m);
    BkpVec3 t = bkpVec3(tx, ty, tz);
    BkpVec3 s = bkpVec3(sx, sy, sz);
    bkpMat4SetScale(&m, &s);
    bkpMat4SetTranslate(&m, &t);
    return m;
}

BkpMat4 makeGroundModel(float tx, float ty, float tz, float sx, float sz)
{
    BkpMat4 m;
    bkpSetIdentityMat4(&m);
    BkpVec3 t = bkpVec3(tx, ty, tz);
    /* SetRotate does m = m*R (rotation applied first to vertices).
     * SetScale left-multiplies diag(sx, 1, sz): scales post-rotation result.
     * After Rx(-90°): plane Y→world Z, so sz must multiply the post-rotation Z,
     * meaning the diag must have sz on the Z slot (not Y). */
    BkpVec3 s = bkpVec3(sx, 1.0f, sz);
    BkpVec3 axisX = bkpVec3(1.0f, 0.0f, 0.0f);
    bkpMat4SetRotate(&m, &axisX, -(float)BKP_PI_HALF);
    bkpMat4SetScale(&m, &s);
    bkpMat4SetTranslate(&m, &t);
    return m;
}

/* =========================================================================
 * Scene helpers
 * ========================================================================= */
void initGltfSphere(Renderer * r, float tx, float ty, float tz, float scale)
{
    if(!bkpLoadModel(r->adp, GLTF_SPHERE_PATH, &r->gltfModel, 0) || r->gltfModel.meshCount == 0)
        return;
    r->gltfSphere.geo        = r->gltfModel.meshes[0].geo;
    r->gltfSphere.model      = makeModel(tx, ty, tz, scale, scale, scale);
    r->gltfSphere.color[0]   = 0.9f;
    r->gltfSphere.color[1]   = 0.2f;
    r->gltfSphere.color[2]   = 0.2f;
    r->gltfSphere.color[3]   = 1.0f;
    r->gltfSphere.roughness  = 0.3f;
    r->gltfSphere.metallic   = 0.0f;
}

void initObject(BkpGpuAdapter adp, SceneObject * obj, BkpEGeometryType type,
                float tx, float ty, float tz,
                float sx, float sy, float sz,
                float r,  float g,  float b,
                float rough, float metal)
{
    bkpCreateVertexIndexBuffer(adp, type, &obj->geo);
    obj->model     = makeModel(tx, ty, tz, sx, sy, sz);
    obj->color[0]  = r; obj->color[1] = g; obj->color[2] = b; obj->color[3] = 1.0f;
    obj->roughness = rough;
    obj->metallic  = metal;
}

void uploadUBO(Renderer * r, uint32_t frame, Camera * cam)
{
    /* Keep r->width/height in sync with the actual swapchain after resize. */
    r->width  = r->adp->frameInfo.winWidth;
    r->height = r->adp->frameInfo.winHeight;
    SceneUBO ubo = {0};
    BkpVec3 eye    = bkpVec3(cam->pos[0],    cam->pos[1],    cam->pos[2]);
    BkpVec3 target = bkpVec3(cam->target[0], cam->target[1], cam->target[2]);
    BkpVec3 up     = bkpVec3(0.0f, 1.0f, 0.0f);
    ubo.view = bkpLookAt(eye, target, up);
    ubo.proj = bkpPerspective(45.0f, (float)r->width / (float)r->height, 0.1f, 500.0f);
    ubo.proj.mLine1.y *= -1.0f; /* Vulkan clip-space Y flip */
    float ld[3] = {1.2f, -0.7f, 0.5f};
    float len = sqrtf(ld[0]*ld[0] + ld[1]*ld[1] + ld[2]*ld[2]);
    ubo.lightDir[0] = ld[0]/len; ubo.lightDir[1] = ld[1]/len;
    ubo.lightDir[2] = ld[2]/len; ubo.lightDir[3] = 0.0f;
    ubo.camPos[0] = cam->pos[0]; ubo.camPos[1] = cam->pos[1];
    ubo.camPos[2] = cam->pos[2]; ubo.camPos[3] = 1.0f;
    bkpUploadBufferData(r->adp, r->ubo.buffer, &ubo, frame * r->ubo.size, sizeof(SceneUBO));
}

void drawObject(VkCommandBuffer cmd, BkpPipelineGraphic * pip, SceneObject * obj)
{
    ObjectPC pc = {0};
    pc.model     = obj->model;
    memcpy(pc.color, obj->color, sizeof(float) * 4);
    pc.roughness = obj->roughness;
    pc.metallic  = obj->metallic;
    pc.pad[0]    = obj->useAlbedoMap;
    bkpCmdPushConstants(cmd, pip->pipelineLayout.layout,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0, sizeof(ObjectPC), &pc);
    VkBuffer vbuf; VkDeviceSize voff;
    bkpGetBuffer(obj->geo.buffer, &vbuf);
    bkpGetBufferOffset(obj->geo.buffer, &voff);
    vkCmdBindVertexBuffers(cmd, 0, 1, &vbuf, &voff);
    if(obj->geo.hasIndices)
    {
        VkBuffer ibuf; bkpGetBuffer(obj->geo.buffer, &ibuf);
        vkCmdBindIndexBuffer(cmd, ibuf, voff + obj->geo.indicesOffset, obj->geo.indexType);
        vkCmdDrawIndexed(cmd, (uint32_t)obj->geo.count, 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(cmd, (uint32_t)obj->geo.count, 1, 0, 0);
    }
}

/* =========================================================================
 * Shadow / texture helpers (tuto 06+)
 * ========================================================================= */
#define SCENE_SHADOW_SIZE  4096
#define SCENE_TEXTURE_PATH "images/bkpview_wallpaper2.png"

void initShadowResources(BkpGpuAdapter adp,
                         BkpImageResource * shadowImg,
                         BkpSampler       * shadowSampler)
{
    *shadowImg = bkpDefaultDepthBuffer();
    shadowImg->imageInfo.extent = (VkExtent3D){SCENE_SHADOW_SIZE, SCENE_SHADOW_SIZE, 1};
    shadowImg->imageInfo.format = VK_FORMAT_D32_SFLOAT;
    shadowImg->imageInfo.usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                | VK_IMAGE_USAGE_SAMPLED_BIT;
    shadowImg->viewInfo.format  = VK_FORMAT_D32_SFLOAT;
    bkpCreateDepthResources(adp, shadowImg);

    shadowSampler->info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    shadowSampler->info.magFilter    = VK_FILTER_LINEAR;
    shadowSampler->info.minFilter    = VK_FILTER_LINEAR;
    shadowSampler->info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    shadowSampler->info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    shadowSampler->info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    shadowSampler->info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    shadowSampler->info.borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    shadowSampler->info.compareEnable = VK_TRUE;
    shadowSampler->info.compareOp    = VK_COMPARE_OP_LESS_OR_EQUAL;
    shadowSampler->info.minLod       = 0.0f;
    shadowSampler->info.maxLod       = 1.0f;
    bkpCreateSampler(adp, shadowSampler);
}

void initGroundTexture(BkpGpuAdapter adp,
                       BkpImageResource * groundTex,
                       BkpSampler       * groundSampler)
{
    bkpSetDefaultTextureInfo(groundTex);
    bkpCreateTextureFromFile(adp, SCENE_TEXTURE_PATH, groundTex);

    groundSampler->info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    groundSampler->info.magFilter    = VK_FILTER_LINEAR;
    groundSampler->info.minFilter    = VK_FILTER_LINEAR;
    groundSampler->info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    groundSampler->info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    groundSampler->info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    groundSampler->info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    groundSampler->info.minLod       = 0.0f;
    groundSampler->info.maxLod       = VK_LOD_CLAMP_NONE;
    bkpCreateSampler(adp, groundSampler);
}

/* =========================================================================
 * Init
 * ========================================================================= */
void initSkyTexture(Renderer * r)
{
    /* V=0 = zenith (north pole, +Y, looking up)  → deep blue
     * V=1 = nadir  (south pole, -Y, looking down) → white
     * White zone starts at V=0.67 (bottom 1/3 of sky, near horizon). */
    BkpColorGradianInfo grads[2];
    grads[0].color1[0] =  15; grads[0].color1[1] =  35; grads[0].color1[2] =  90; grads[0].color1[3] = 255;
    grads[0].color2[0] = 140; grads[0].color2[1] = 190; grads[0].color2[2] = 240; grads[0].color2[3] = 255;
    grads[0].begin = 0.0f; grads[0].end = 0.67f;
    grads[1].color1[0] = 140; grads[1].color1[1] = 190; grads[1].color1[2] = 240; grads[1].color1[3] = 255;
    grads[1].color2[0] = 255; grads[1].color2[1] = 255; grads[1].color2[2] = 255; grads[1].color2[3] = 255;
    grads[1].begin = 0.67f; grads[1].end = 1.0f;

    VkDeviceSize imgSize = 0;
    uint8_t * pixels = bkpGenerateVerticalGradianImage(1, 256, grads, 2, &imgSize);
    bkpSetDefaultTextureInfo(&r->skyTex);
    r->skyTex.imageInfo.format    = VK_FORMAT_R8G8B8A8_UNORM;
    r->skyTex.viewInfo.format     = VK_FORMAT_R8G8B8A8_UNORM;
    r->skyTex.imageInfo.mipLevels = 1;
    r->skyTex.mipType             = eMIPMAP_NONE;
    r->skyTex.hasAlpha            = BKP_FALSE;
    bkpCreateTextureLayersFromData(r->adp, &pixels, 1, 1, 256, 1, &r->skyTex);
    bkpFree(pixels);

    r->skySampler.info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    r->skySampler.info.magFilter    = VK_FILTER_LINEAR;
    r->skySampler.info.minFilter    = VK_FILTER_LINEAR;
    r->skySampler.info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    r->skySampler.info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    r->skySampler.info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    r->skySampler.info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    r->skySampler.info.minLod       = 0.0f;
    r->skySampler.info.maxLod       = 1.0f;
    bkpCreateSampler(r->adp, &r->skySampler);
}

void initDepth(Renderer * r)
{
    r->depthImg = bkpDefaultDepthBuffer();
    r->depthImg.imageInfo.extent = (VkExtent3D){(uint32_t)r->width, (uint32_t)r->height, 1};
    bkpCreateDepthResources(r->adp, &r->depthImg);
}

void initPipelines(Renderer * r, const char * shaderDir)
{
    VkFormat colorFmt = r->sc.info.imageFormat;
    VkFormat depthFmt = r->depthImg.imageInfo.format;

    /* BkpVec3 and BkpVec4 both contain __m128 → sizeof = 16.
     * sizeof(BkpVertex) = 64. Always use sizeof/offsetof. */
    static VkVertexInputBindingDescription vBinding =
    {
        0, (uint32_t)sizeof(BkpVertex), VK_VERTEX_INPUT_RATE_VERTEX
    };
    static VkVertexInputAttributeDescription vAttrs[4] =
    {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(BkpVertex, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(BkpVertex, normal)},
        {2, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(BkpVertex, uv)},
        {3, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(BkpVertex, tangent)},
    };
    BkpVertexLayout vLayout = {vAttrs, &vBinding, 4, 1};

    char sceneVert[256], sceneFrag[256], skyVert[256], skyFrag[256];
    snprintf(sceneVert, sizeof(sceneVert), "%s/scene.vert.spv", shaderDir);
    snprintf(sceneFrag, sizeof(sceneFrag), "%s/scene.frag.spv", shaderDir);
    snprintf(skyVert,   sizeof(skyVert),   "%s/sky.vert.spv",   shaderDir);
    snprintf(skyFrag,   sizeof(skyFrag),   "%s/sky.frag.spv",   shaderDir);

    bkpCreateShaderModule(r->adp, sceneVert, &r->sceneVert);
    bkpCreateShaderModule(r->adp, sceneFrag, &r->sceneFrag);
    BkpShaderModule * sceneMods[] = {&r->sceneVert, &r->sceneFrag};
    bkpCreateShader(r->adp, sceneMods, 2, &r->sceneProg);
    bkpCreatePipelineLayoutFromShader(r->adp, &r->sceneProg, &r->scenePipeline.pipelineLayout);

    bkpCreateShaderModule(r->adp, skyVert, &r->skyVert);
    bkpCreateShaderModule(r->adp, skyFrag, &r->skyFrag);
    BkpShaderModule * skyMods[] = {&r->skyVert, &r->skyFrag};
    bkpCreateShader(r->adp, skyMods, 2, &r->skyProg);
    bkpCreatePipelineLayoutFromShader(r->adp, &r->skyProg, &r->skyPipeline.pipelineLayout);

    VkDescriptorPoolSize poolSizes[2] =
    {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         FRAMES * 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, FRAMES},
    };
    r->descPool.info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    r->descPool.info.maxSets       = FRAMES * 2;
    r->descPool.info.poolSizeCount = 2;
    r->descPool.info.pPoolSizes    = poolSizes;
    bkpCreateDescriptorPool(r->adp, &r->descPool);

    r->descSet.descPool   = &r->descPool;
    r->descSet.descLayout = &r->sceneProg.layouts[0];
    r->descSet.count      = FRAMES;
    bkpCreateDescriptorSet(r->adp, &r->descSet);

    r->skyDescSet.descPool   = &r->descPool;
    r->skyDescSet.descLayout = &r->skyProg.layouts[0];
    r->skyDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r->adp, &r->skyDescSet);

    for(uint32_t f = 0; f < FRAMES; ++f)
    {
        VkDescriptorBufferInfo bufInfo = bkpMakeUboInfo(r->ubo.buffer, f, r->ubo.size);
        BkpWriteDescriptorSetInfo wr   = {0};
        wr.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        wr.setBindings[0].ubos  = &bufInfo;
        wr.setBindings[0].count = 1;
        bkpWriteDescriptorSet(r->adp, &r->descSet,    f, &wr);
        bkpWriteDescriptorSet(r->adp, &r->skyDescSet, f, &wr);
    }

    bkpCreatePipelineMinimalInfo(&r->scenePipeline.info, (VkExtent2D){(uint32_t)r->width, (uint32_t)r->height});
    r->scenePipeline.info.shaderProgram         = &r->sceneProg;
    r->scenePipeline.info.colorAttachmentFormat = colorFmt;
    r->scenePipeline.info.depthAttachmentFormat = depthFmt;
    r->scenePipeline.info.vertexLayout          = vLayout;
    r->scenePipeline.info.dynamicStates[0]      = VK_DYNAMIC_STATE_VIEWPORT;
    r->scenePipeline.info.dynamicStates[1]      = VK_DYNAMIC_STATE_SCISSOR;
    r->scenePipeline.info.dynamicState          = bkpDefaultDynamicStates(r->scenePipeline.info.dynamicStates, 2);
    r->scenePipeline.info.dynamicStateEnabled   = VK_TRUE;
    r->scenePipeline.info.viewportState         = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
    bkpCreatePipelineGraphic(r->adp, &r->scenePipeline);

    bkpCreatePipelineMinimalInfo(&r->skyPipeline.info, (VkExtent2D){(uint32_t)r->width, (uint32_t)r->height});
    r->skyPipeline.info.shaderProgram               = &r->skyProg;
    r->skyPipeline.info.colorAttachmentFormat        = colorFmt;
    r->skyPipeline.info.depthAttachmentFormat        = depthFmt;
    r->skyPipeline.info.vertexLayout                 = vLayout;
    r->skyPipeline.info.depthStencil.depthWriteEnable = VK_FALSE;
    r->skyPipeline.info.rasterizer.cullMode          = VK_CULL_MODE_NONE;
    r->skyPipeline.info.dynamicStates[0]             = VK_DYNAMIC_STATE_VIEWPORT;
    r->skyPipeline.info.dynamicStates[1]             = VK_DYNAMIC_STATE_SCISSOR;
    r->skyPipeline.info.dynamicState                 = bkpDefaultDynamicStates(r->skyPipeline.info.dynamicStates, 2);
    r->skyPipeline.info.dynamicStateEnabled          = VK_TRUE;
    r->skyPipeline.info.viewportState                = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
    bkpCreatePipelineGraphic(r->adp, &r->skyPipeline);
}

void initCommandBuffers(Renderer * r)
{
    r->cmd.commandPool = &r->adp->commandPoolGraphic;
    r->cmd.count       = FRAMES;
    r->cmd.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bkpAllocateCmdBuffer(r->adp, &r->cmd);
}

/* =========================================================================
 * Render frame
 * ========================================================================= */
void renderFrame(Renderer * r, uint32_t frame, uint32_t imageIndex,
                 Camera * cam, SceneObject * ground)
{
    r->width  = r->adp->frameInfo.winWidth;
    r->height = r->adp->frameInfo.winHeight;
    VkCommandBuffer vkcmd = r->cmd.cmds[frame];

    uploadUBO(r, frame, cam);
    bkpBeginCommandBuffer(&r->cmd, frame, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    BkpImageBarrierInfo beginBarriers[2] =
    {
        { .image = r->sc.images[imageIndex],
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .srcStage  = VK_PIPELINE_STAGE_2_NONE,   .srcAccess = VK_ACCESS_2_NONE,
          .dstStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
          .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
          .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1 },
        { .image = r->depthImg.images[0],
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
          .srcStage  = VK_PIPELINE_STAGE_2_NONE,   .srcAccess = VK_ACCESS_2_NONE,
          .dstStage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
          .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
          .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1 }
    };
    bkpCmdBarrierImages(vkcmd, beginBarriers, 2);

    VkRenderingAttachmentInfo colorAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colorAtt.imageView   = r->sc.imageViews[imageIndex];
    colorAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.clearValue  = (VkClearValue){.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderingAttachmentInfo depthAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depthAtt.imageView   = r->depthImg.imageViews[0];
    depthAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.clearValue  = (VkClearValue){.depthStencil = {1.0f, 0}};

    VkRenderingInfo renderInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    renderInfo.renderArea.offset     = (VkOffset2D){0, 0};
    renderInfo.renderArea.extent     = (VkExtent2D){(uint32_t)r->width, (uint32_t)r->height};
    renderInfo.layerCount            = 1;
    renderInfo.colorAttachmentCount  = 1;
    renderInfo.pColorAttachments     = &colorAtt;
    renderInfo.pDepthAttachment      = &depthAtt;
    bkpCmdBeginRendering(vkcmd, &renderInfo);
    bkpUpdateWindowViewportScissor(vkcmd, r->width, r->height);

    /* sky sphere — depth write off, cull front (sphere viewed from inside) */
    bkpCmdBindPipeline(vkcmd, r->skyPipeline.pipeline);
    vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        r->skyPipeline.pipelineLayout.layout, 0, 1,
        &r->skyDescSet.descriptorSets[frame], 0, NULL);
    {
        VkBuffer vbuf; VkDeviceSize voff;
        bkpGetBuffer(r->skyGeo.buffer, &vbuf);
        bkpGetBufferOffset(r->skyGeo.buffer, &voff);
        vkCmdBindVertexBuffers(vkcmd, 0, 1, &vbuf, &voff);
        if(r->skyGeo.hasIndices)
        {
            VkBuffer ibuf; bkpGetBuffer(r->skyGeo.buffer, &ibuf);
            vkCmdBindIndexBuffer(vkcmd, ibuf, voff + r->skyGeo.indicesOffset, r->skyGeo.indexType);
            vkCmdDrawIndexed(vkcmd, (uint32_t)r->skyGeo.count, 1, 0, 0, 0);
        }
        else { vkCmdDraw(vkcmd, (uint32_t)r->skyGeo.count, 1, 0, 0); }
    }

    /* scene objects */
    bkpCmdBindPipeline(vkcmd, r->scenePipeline.pipeline);
    vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        r->scenePipeline.pipelineLayout.layout, 0, 1,
        &r->descSet.descriptorSets[frame], 0, NULL);
    drawObject(vkcmd, &r->scenePipeline, ground);
    for(int o = 0; o < OBJ_COUNT; ++o)
        drawObject(vkcmd, &r->scenePipeline, &r->objs[o]);

    bkpCmdEndRendering(vkcmd);

    BkpImageBarrierInfo presentBarrier =
    {
        .image = r->sc.images[imageIndex],
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStage  = VK_PIPELINE_STAGE_2_NONE, .dstAccess = VK_ACCESS_2_NONE,
        .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
    };
    bkpCmdBarrierImages(vkcmd, &presentBarrier, 1);
    bkpEndCommandBuffer(&r->cmd, frame);
}

/* =========================================================================
 * Cleanup
 * ========================================================================= */
void cleanupRenderer(Renderer * r)
{
    bkpWaitDevice(r->adp);
    if(r->gltfModel.meshCount > 0) bkpUnloadModel(r->adp, &r->gltfModel);
    bkpDestroyCommandBuffers(r->adp, &r->cmd);
    for(int o = 0; o < OBJ_COUNT; ++o) bkpFreeBuffer(r->adp, &r->objs[o].geo.buffer);
    bkpFreeBuffer(r->adp, &r->skyGeo.buffer);
    bkpFreeBuffer(r->adp, &r->groundGeo.buffer);
    bkpDestroyImageResource(r->adp, &r->depthImg);
    bkpDestroyImageResource(r->adp, &r->skyTex);
    bkpDestroySampler(r->adp, &r->skySampler);
    bkpDestroyBuffersGPU(r->adp, &r->ubo);
    bkpDestroyGraphicPipeline(r->adp, &r->scenePipeline);
    bkpDestroyPipelineLayout(r->adp, &r->scenePipeline.pipelineLayout);
    bkpDestroyGraphicPipeline(r->adp, &r->skyPipeline);
    bkpDestroyPipelineLayout(r->adp, &r->skyPipeline.pipelineLayout);
    bkpDestroyShader(r->adp, &r->sceneProg);
    bkpDestroyShaderModule(r->adp, &r->sceneVert);
    bkpDestroyShaderModule(r->adp, &r->sceneFrag);
    bkpDestroyShader(r->adp, &r->skyProg);
    bkpDestroyShaderModule(r->adp, &r->skyVert);
    bkpDestroyShaderModule(r->adp, &r->skyFrag);
    bkpDestroyDescriptorSet(r->adp, &r->descSet);
    bkpDestroyDescriptorSet(r->adp, &r->skyDescSet);
    bkpDestroyDescriptorPool(r->adp, &r->descPool);
    bkpCleanupSwapChain(r->adp, &r->sc);
    bkpClearGpuAdapter(r->adp);
}
