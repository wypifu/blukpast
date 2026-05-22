#include "../common/scene.h"
#include "../common/hud.h"
#include "../common/free_camera.h"

#define W 1280
#define H  720
#define SHADOW_SIZE 4096

#define SHADER_DIR     "tutorial/05_shadow_map/shaders"
#define SHADER_DIR_SKY "tutorial/02_camera/shaders"
#define SHADER_DIR_HUD "tutorial/04_font_input/shaders"
#define FONT_PATH      "tutorial/04_font_input/AdwaitaMono-Regular.ttf"
#define TEXTURE_PATH   "images/bkpview_wallpaper2.png"

typedef struct
{
    BkpMat4 view;
    BkpMat4 proj;
    float   lightDir[4];
    float   camPos[4];
    BkpMat4 lightViewProj;
} SceneUBO05;

typedef struct { BkpMat4 lightMVP; } ShadowPC;

static BkpMat4 computeLightViewProj(void)
{
    float ld[3] = {1.2f, -0.7f, 0.5f};
    float len   = sqrtf(ld[0]*ld[0] + ld[1]*ld[1] + ld[2]*ld[2]);
    BkpVec3 eye    = bkpVec3(-ld[0]/len * 40.0f, -ld[1]/len * 40.0f, -ld[2]/len * 40.0f);
    BkpVec3 target = bkpVec3(0.0f, 0.0f, 0.0f);
    BkpVec3 up     = bkpVec3(0.0f, 1.0f, 0.0f);
    BkpMat4 lightVP = bkpOrtho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 80.0f);
    BkpMat4 view    = bkpLookAt(eye, target, up);
    return bkpMat4DotMat4(&lightVP, &view);
}

static void drawShadow(VkCommandBuffer cmd, BkpPipelineGraphic * shadowPip,
                       SceneObject * obj, const BkpMat4 * lightVP)
{
    ShadowPC spc;
    spc.lightMVP = bkpMat4DotMat4(lightVP, &obj->model);
    bkpCmdPushConstants(cmd, shadowPip->pipelineLayout.layout,
                        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowPC), &spc);
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

int main(void)
{
    /* 1. Init */
    BkpContext ctx = {0};
    BkpConfig cfg  = bkpDefaultConfig();
    cfg.vulkanContextInfo.winWidth         = W;
    cfg.vulkanContextInfo.winHeight        = H;
    cfg.vulkanContextInfo.maxFrameInFlight = FRAMES;
    bkpWindowEnableDecoration(1);
    bkpInit(&ctx, &cfg);

    Renderer r = {0};
    r.width = W; r.height = H;
    bkpActivateGpuAdapter(&ctx.vulkanContext, &r.adp, NULL);

    /* 2. Swap chain */
    r.sc.info = bkpDefaultSwapChain(1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_TRUE, VK_PRESENT_MODE_FIFO_KHR);
    bkpCreateSwapChain(r.adp, &r.sc, (VkPresentModeKHR[]){VK_PRESENT_MODE_FIFO_KHR}, 1);

    /* 3. Scene UBO */
    r.ubo.count = FRAMES;
    r.ubo.size  = sizeof(SceneUBO05);
    bkpCreateBuffersGPU(r.adp, &r.ubo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        eBUFFER_CPU_GPU, BKP_DO_MAP);

    /* 4. Depth buffer + sky texture */
    initDepth(&r);
    initSkyTexture(&r);

    /* 5. Shadow map — baked once (light is fixed), stays SHADER_READ_ONLY permanently */
    BkpImageResource shadowImg = bkpDefaultDepthBuffer();
    shadowImg.imageInfo.extent = (VkExtent3D){SHADOW_SIZE, SHADOW_SIZE, 1};
    shadowImg.imageInfo.format = VK_FORMAT_D32_SFLOAT;
    shadowImg.imageInfo.usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                | VK_IMAGE_USAGE_SAMPLED_BIT;
    shadowImg.viewInfo.format  = VK_FORMAT_D32_SFLOAT;
    bkpCreateDepthResources(r.adp, &shadowImg);

    BkpSampler shadowSampler = {0};
    shadowSampler.info.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    shadowSampler.info.magFilter     = VK_FILTER_LINEAR;
    shadowSampler.info.minFilter     = VK_FILTER_LINEAR;
    shadowSampler.info.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    shadowSampler.info.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    shadowSampler.info.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    shadowSampler.info.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    shadowSampler.info.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    shadowSampler.info.compareEnable = VK_TRUE;
    shadowSampler.info.compareOp     = VK_COMPARE_OP_LESS_OR_EQUAL;
    shadowSampler.info.minLod        = 0.0f;
    shadowSampler.info.maxLod        = 1.0f;
    bkpCreateSampler(r.adp, &shadowSampler);

    /* 6. Ground texture */
    BkpImageResource groundTex = {0};
    bkpSetDefaultTextureInfo(&groundTex);
    bkpCreateTextureFromFile(r.adp, TEXTURE_PATH, &groundTex);

    BkpSampler groundSampler = {0};
    groundSampler.info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    groundSampler.info.magFilter    = VK_FILTER_LINEAR;
    groundSampler.info.minFilter    = VK_FILTER_LINEAR;
    groundSampler.info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    groundSampler.info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    groundSampler.info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    groundSampler.info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    groundSampler.info.minLod       = 0.0f;
    groundSampler.info.maxLod       = VK_LOD_CLAMP_NONE;
    bkpCreateSampler(r.adp, &groundSampler);

    /* 7. Vertex layouts */
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

    static VkVertexInputAttributeDescription shadowAttr[1] =
    {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(BkpVertex, position)},
    };
    BkpVertexLayout shadowVLayout = {shadowAttr, &vBinding, 1, 1};

    VkFormat colorFmt = r.sc.info.imageFormat;
    VkFormat depthFmt = r.depthImg.imageInfo.format;

    /* 8. Scene pipeline */
    BkpShaderModule sceneVert = {0}, sceneFrag = {0};
    BkpShaderProgram sceneProg = {0};
    BkpPipelineGraphic scenePipeline = {0};
    {
        char vs[256], fs[256];
        snprintf(vs, sizeof(vs), "%s/scene.vert.spv", SHADER_DIR);
        snprintf(fs, sizeof(fs), "%s/scene.frag.spv", SHADER_DIR);
        bkpCreateShaderModule(r.adp, vs, &sceneVert);
        bkpCreateShaderModule(r.adp, fs, &sceneFrag);
        BkpShaderModule * mods[] = {&sceneVert, &sceneFrag};
        bkpCreateShader(r.adp, mods, 2, &sceneProg);
        bkpCreatePipelineLayoutFromShader(r.adp, &sceneProg, &scenePipeline.pipelineLayout);
        bkpCreatePipelineMinimalInfo(&scenePipeline.info, (VkExtent2D){W, H});
        scenePipeline.info.shaderProgram         = &sceneProg;
        scenePipeline.info.colorAttachmentFormat = colorFmt;
        scenePipeline.info.depthAttachmentFormat = depthFmt;
        scenePipeline.info.vertexLayout          = vLayout;
        scenePipeline.info.dynamicStates[0]      = VK_DYNAMIC_STATE_VIEWPORT;
        scenePipeline.info.dynamicStates[1]      = VK_DYNAMIC_STATE_SCISSOR;
        scenePipeline.info.dynamicState          = bkpDefaultDynamicStates(scenePipeline.info.dynamicStates, 2);
        scenePipeline.info.dynamicStateEnabled   = VK_TRUE;
        scenePipeline.info.viewportState         = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
        bkpCreatePipelineGraphic(r.adp, &scenePipeline);
    }

    /* 9. Shadow pipeline */
    BkpShaderModule shadowVert = {0}, shadowFrag = {0};
    BkpShaderProgram shadowProg = {0};
    BkpPipelineGraphic shadowPipeline = {0};
    {
        char vs[256], fs[256];
        snprintf(vs, sizeof(vs), "%s/shadow.vert.spv", SHADER_DIR);
        snprintf(fs, sizeof(fs), "%s/shadow.frag.spv", SHADER_DIR);
        bkpCreateShaderModule(r.adp, vs, &shadowVert);
        bkpCreateShaderModule(r.adp, fs, &shadowFrag);
        BkpShaderModule * mods[] = {&shadowVert, &shadowFrag};
        bkpCreateShader(r.adp, mods, 2, &shadowProg);
        bkpCreatePipelineLayoutFromShader(r.adp, &shadowProg, &shadowPipeline.pipelineLayout);
        bkpCreatePipelineMinimalInfo(&shadowPipeline.info, (VkExtent2D){SHADOW_SIZE, SHADOW_SIZE});
        shadowPipeline.info.shaderProgram         = &shadowProg;
        shadowPipeline.info.colorAttachmentCount  = 0;
        shadowPipeline.info.colorBlending         = bkpDefaultPipelineColorBlend(NULL, 0);
        shadowPipeline.info.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
        shadowPipeline.info.vertexLayout          = shadowVLayout;
        shadowPipeline.info.rasterizer.cullMode   = VK_CULL_MODE_FRONT_BIT;
        shadowPipeline.info.dynamicStates[0]      = VK_DYNAMIC_STATE_VIEWPORT;
        shadowPipeline.info.dynamicStates[1]      = VK_DYNAMIC_STATE_SCISSOR;
        shadowPipeline.info.dynamicState          = bkpDefaultDynamicStates(shadowPipeline.info.dynamicStates, 2);
        shadowPipeline.info.dynamicStateEnabled   = VK_TRUE;
        shadowPipeline.info.viewportState         = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
        bkpCreatePipelineGraphic(r.adp, &shadowPipeline);
    }

    /* 10. Sky pipeline */
    {
        char vs[256], fs[256];
        snprintf(vs, sizeof(vs), "%s/sky.vert.spv", SHADER_DIR_SKY);
        snprintf(fs, sizeof(fs), "%s/sky.frag.spv", SHADER_DIR_SKY);
        bkpCreateShaderModule(r.adp, vs, &r.skyVert);
        bkpCreateShaderModule(r.adp, fs, &r.skyFrag);
        BkpShaderModule * mods[] = {&r.skyVert, &r.skyFrag};
        bkpCreateShader(r.adp, mods, 2, &r.skyProg);
        bkpCreatePipelineLayoutFromShader(r.adp, &r.skyProg, &r.skyPipeline.pipelineLayout);
        bkpCreatePipelineMinimalInfo(&r.skyPipeline.info, (VkExtent2D){W, H});
        r.skyPipeline.info.shaderProgram                 = &r.skyProg;
        r.skyPipeline.info.colorAttachmentFormat         = colorFmt;
        r.skyPipeline.info.depthAttachmentFormat         = depthFmt;
        r.skyPipeline.info.vertexLayout                  = vLayout;
        r.skyPipeline.info.depthStencil.depthWriteEnable = VK_FALSE;
        r.skyPipeline.info.rasterizer.cullMode           = VK_CULL_MODE_NONE;
        r.skyPipeline.info.dynamicStates[0]              = VK_DYNAMIC_STATE_VIEWPORT;
        r.skyPipeline.info.dynamicStates[1]              = VK_DYNAMIC_STATE_SCISSOR;
        r.skyPipeline.info.dynamicState                  = bkpDefaultDynamicStates(r.skyPipeline.info.dynamicStates, 2);
        r.skyPipeline.info.dynamicStateEnabled           = VK_TRUE;
        r.skyPipeline.info.viewportState                 = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
        bkpCreatePipelineGraphic(r.adp, &r.skyPipeline);
    }

    /* 11. Descriptor pool */
    {
        VkDescriptorPoolSize poolSizes[2] =
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         FRAMES * 2},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, FRAMES * 3},
        };
        r.descPool.info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        r.descPool.info.maxSets       = FRAMES * 2;
        r.descPool.info.poolSizeCount = 2;
        r.descPool.info.pPoolSizes    = poolSizes;
        bkpCreateDescriptorPool(r.adp, &r.descPool);
    }

    /* scene descriptor set (binding 0=UBO, 1=albedoTex, 2=shadowMap) */
    r.descSet.descPool   = &r.descPool;
    r.descSet.descLayout = &sceneProg.layouts[0];
    r.descSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &r.descSet);

    /* sky descriptor set (binding 0=UBO, 1=skyTex) */
    r.skyDescSet.descPool   = &r.descPool;
    r.skyDescSet.descLayout = &r.skyProg.layouts[0];
    r.skyDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &r.skyDescSet);

    for(uint32_t f = 0; f < FRAMES; ++f)
    {
        VkDescriptorBufferInfo bufInfo = bkpMakeUboInfo(r.ubo.buffer, f, r.ubo.size);
        BkpWriteDescriptorSetInfo wr   = {0};
        wr.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        wr.setBindings[0].ubos  = &bufInfo;
        wr.setBindings[0].count = 1;
        bkpWriteDescriptorSet(r.adp, &r.descSet,    f, &wr);
        bkpWriteDescriptorSet(r.adp, &r.skyDescSet, f, &wr);

        bkpWriteDescriptorImage(r.adp, r.descSet.descriptorSets[f], 1,
                                groundSampler.sampler, groundTex.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        bkpWriteDescriptorImage(r.adp, r.descSet.descriptorSets[f], 2,
                                shadowSampler.sampler, shadowImg.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        bkpWriteDescriptorImage(r.adp, r.skyDescSet.descriptorSets[f], 1,
                                r.skySampler.sampler, r.skyTex.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    /* 12. Geometry */
    bkpSetDefaultSegmentForCircularGeometries(32);
    bkpCreateVertexIndexBuffer(r.adp, eBKP_GEO_SPHERE, &r.skyGeo);
    bkpCreateVertexIndexBuffer(r.adp, eBKP_GEO_PLANE,  &r.groundGeo);

    initObject(r.adp, &r.objs[0], eBKP_GEO_SPHERE,    -5.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.9f,0.2f,0.2f, 0.3f,0.0f);
    initObject(r.adp, &r.objs[1], eBKP_GEO_CUBE,       0.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.2f,0.8f,0.3f, 0.5f,0.0f);
    initObject(r.adp, &r.objs[2], eBKP_GEO_CYLINDER,   5.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.3f,0.5f,0.9f, 0.2f,0.8f);
    initObject(r.adp, &r.objs[3], eBKP_GEO_CONE,      -2.5f, 0.5f, -5.0f, 1.0f,1.0f,1.0f, 0.9f,0.7f,0.1f, 0.6f,0.0f);
    initObject(r.adp, &r.objs[4], eBKP_GEO_TORUS,      2.5f, 0.3f, -5.0f, 1.5f,1.5f,1.5f, 0.8f,0.3f,0.8f, 0.4f,0.5f);
    initObject(r.adp, &r.objs[5], eBKP_GEO_TUBE,      -5.0f, 0.5f, -5.0f, 1.0f,1.0f,1.0f, 0.2f,0.9f,0.8f, 0.7f,0.1f);
    initObject(r.adp, &r.objs[6], eBKP_GEO_DISK_PLANE, 5.0f, 1.0f, -5.0f, 1.5f,1.5f,1.5f, 0.9f,0.9f,0.9f, 0.1f,0.9f);
    initGltfSphere(&r, -5.0f, 0.5f, 2.0f, 0.5f);

    SceneObject ground = {0};
    ground.geo          = r.groundGeo;
    ground.model        = makeGroundModel(0.0f, 0.0f, 0.0f, 30.0f, 30.0f);
    ground.color[0]     = 1.0f; ground.color[1] = 1.0f; ground.color[2] = 1.0f; ground.color[3] = 1.0f;
    ground.roughness    = 0.8f;
    ground.metallic     = 0.0f;
    ground.useAlbedoMap = 1.0f;

    /* 13. Command buffers */
    initCommandBuffers(&r);

    /* 14. Bake shadow map once (light is fixed — never recomputed) */
    BkpMat4 lightViewProj = computeLightViewProj();
    {
        BkpFence bakeFence = {0};
        bkpCreateFence(r.adp, &bakeFence, BKP_FALSE);

        bkpBeginCommandBuffer(&r.cmd, 0, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VkCommandBuffer bcmd = r.cmd.cmds[0];

        BkpImageBarrierInfo toDepth =
        {
            .image = shadowImg.images[0],
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .srcStage  = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
            .dstStage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                       | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
        };
        bkpCmdBarrierImages(bcmd, &toDepth, 1);

        VkRenderingAttachmentInfo shadowDepthAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        shadowDepthAtt.imageView   = shadowImg.imageViews[0];
        shadowDepthAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        shadowDepthAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        shadowDepthAtt.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        shadowDepthAtt.clearValue  = (VkClearValue){.depthStencil = {1.0f, 0}};

        VkRenderingInfo shadowRenderInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
        shadowRenderInfo.renderArea.extent = (VkExtent2D){SHADOW_SIZE, SHADOW_SIZE};
        shadowRenderInfo.layerCount        = 1;
        shadowRenderInfo.pDepthAttachment  = &shadowDepthAtt;
        bkpCmdBeginRendering(bcmd, &shadowRenderInfo);

        VkViewport shadowVP = {0.0f, 0.0f, (float)SHADOW_SIZE, (float)SHADOW_SIZE, 0.0f, 1.0f};
        VkRect2D   shadowSc = {{0, 0}, {SHADOW_SIZE, SHADOW_SIZE}};
        vkCmdSetViewport(bcmd, 0, 1, &shadowVP);
        vkCmdSetScissor(bcmd, 0, 1, &shadowSc);

        bkpCmdBindPipeline(bcmd, shadowPipeline.pipeline);
        drawShadow(bcmd, &shadowPipeline, &ground, &lightViewProj);
        for(int o = 0; o < OBJ_COUNT; ++o)
            drawShadow(bcmd, &shadowPipeline, &r.objs[o], &lightViewProj);
        if(r.gltfModel.meshCount > 0)
            drawShadow(bcmd, &shadowPipeline, &r.gltfSphere, &lightViewProj);

        bkpCmdEndRendering(bcmd);

        BkpImageBarrierInfo toReadOnly =
        {
            .image = shadowImg.images[0],
            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcStage  = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStage  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccess = VK_ACCESS_2_SHADER_READ_BIT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
        };
        bkpCmdBarrierImages(bcmd, &toReadOnly, 1);

        bkpEndCommandBuffer(&r.cmd, 0);
        BkpCommandBuffer * bakeCmd = &r.cmd;
        bkpSubmit(r.adp, eQFAMILY_GRAPHIC, &bakeCmd, 1, 0, NULL, &bakeFence);
        bkpWaitForFence(r.adp, &bakeFence, UINT64_MAX);
        bkpDestroyFence(r.adp, &bakeFence);
    }

    /* 15. HUD */
    Hud hud = {0};
    hudInit(r.adp, &hud, colorFmt, depthFmt, W, H, SHADER_DIR_HUD, FONT_PATH);

    /* 16. Camera */
    FreeCamera fc = makeFreeCamera(0.0f, 8.0f, 16.0f, 0.0f, -0.45f);
    Camera cam    = {0};
    registerCameraInput(&fc);
    uint64_t prevTime = bkp_time_getClockMicro();

    /* 17. Main loop */
    while(!bkpWindowIsClosed(ctx.vulkanContext.win))
    {
        bkpPollEvent(ctx.vulkanContext.win);

        uint64_t now = bkp_time_getClockMicro();
        float dt = (float)(now - prevTime) * 1e-6f;
        prevTime = now;

        updateFreeCamera(&fc, dt);
        cameraFromFree(&fc, &cam);
        hudUpdate(&hud, dt);

        bkpPrepareFrame(r.adp);
        r.width  = r.adp->frameInfo.winWidth;
        r.height = r.adp->frameInfo.winHeight;
        uint32_t f = r.adp->frameInfo.currentFrame;
        uint32_t i = r.adp->frameInfo.imageAcquired;
        VkCommandBuffer vkcmd = r.cmd.cmds[f];

        /* Upload UBO */
        {
            SceneUBO05 ubo = {0};
            BkpVec3 eye    = bkpVec3(cam.pos[0],    cam.pos[1],    cam.pos[2]);
            BkpVec3 target = bkpVec3(cam.target[0], cam.target[1], cam.target[2]);
            BkpVec3 up     = bkpVec3(0.0f, 1.0f, 0.0f);
            ubo.view = bkpLookAt(eye, target, up);
            ubo.proj = bkpPerspective(45.0f, (float)r.width / (float)r.height, 0.1f, 500.0f);
            ubo.proj.mLine1.y *= -1.0f;
            float ld[3] = {1.2f, -0.7f, 0.5f};
            float len = sqrtf(ld[0]*ld[0] + ld[1]*ld[1] + ld[2]*ld[2]);
            ubo.lightDir[0] = ld[0]/len; ubo.lightDir[1] = ld[1]/len;
            ubo.lightDir[2] = ld[2]/len; ubo.lightDir[3] = 0.0f;
            ubo.camPos[0] = cam.pos[0]; ubo.camPos[1] = cam.pos[1];
            ubo.camPos[2] = cam.pos[2]; ubo.camPos[3] = 1.0f;
            ubo.lightViewProj = lightViewProj;
            bkpUploadBufferData(r.adp, r.ubo.buffer, &ubo,
                                f * r.ubo.size, sizeof(SceneUBO05));
        }

        bkpBeginCommandBuffer(&r.cmd, f, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        /* --- Main pass --- */
        {
            BkpImageBarrierInfo beginBarriers[2] =
            {
                {
                    .image = r.sc.images[i],
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcStage  = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                    .dstStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
                },
                {
                    .image = r.depthImg.images[0],
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    .srcStage  = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                    .dstStage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                    .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                               | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
                }
            };
            bkpCmdBarrierImages(vkcmd, beginBarriers, 2);

            VkRenderingAttachmentInfo colorAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            colorAtt.imageView   = r.sc.imageViews[i];
            colorAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAtt.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
            colorAtt.clearValue  = (VkClearValue){.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

            VkRenderingAttachmentInfo depthAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            depthAtt.imageView   = r.depthImg.imageViews[0];
            depthAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAtt.storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAtt.clearValue  = (VkClearValue){.depthStencil = {1.0f, 0}};

            VkRenderingInfo renderInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
            renderInfo.renderArea.offset    = (VkOffset2D){0, 0};
            renderInfo.renderArea.extent    = (VkExtent2D){(uint32_t)r.width, (uint32_t)r.height};
            renderInfo.layerCount           = 1;
            renderInfo.colorAttachmentCount = 1;
            renderInfo.pColorAttachments    = &colorAtt;
            renderInfo.pDepthAttachment     = &depthAtt;
            bkpCmdBeginRendering(vkcmd, &renderInfo);
            bkpUpdateWindowViewportScissor(vkcmd, (float)r.width, (float)r.height);

            /* Sky */
            bkpCmdBindPipeline(vkcmd, r.skyPipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                r.skyPipeline.pipelineLayout.layout, 0, 1,
                &r.skyDescSet.descriptorSets[f], 0, NULL);
            {
                VkBuffer vbuf; VkDeviceSize voff;
                bkpGetBuffer(r.skyGeo.buffer, &vbuf);
                bkpGetBufferOffset(r.skyGeo.buffer, &voff);
                vkCmdBindVertexBuffers(vkcmd, 0, 1, &vbuf, &voff);
                if(r.skyGeo.hasIndices)
                {
                    VkBuffer ibuf; bkpGetBuffer(r.skyGeo.buffer, &ibuf);
                    vkCmdBindIndexBuffer(vkcmd, ibuf, voff + r.skyGeo.indicesOffset, r.skyGeo.indexType);
                    vkCmdDrawIndexed(vkcmd, (uint32_t)r.skyGeo.count, 1, 0, 0, 0);
                }
                else { vkCmdDraw(vkcmd, (uint32_t)r.skyGeo.count, 1, 0, 0); }
            }

            /* Scene */
            bkpCmdBindPipeline(vkcmd, scenePipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                scenePipeline.pipelineLayout.layout, 0, 1,
                &r.descSet.descriptorSets[f], 0, NULL);
            drawObject(vkcmd, &scenePipeline, &ground);
            for(int o = 0; o < OBJ_COUNT; ++o)
                drawObject(vkcmd, &scenePipeline, &r.objs[o]);
            if(r.gltfModel.meshCount > 0)
                drawObject(vkcmd, &scenePipeline, &r.gltfSphere);

            hudDraw(r.adp, vkcmd, &hud, f);

            bkpCmdEndRendering(vkcmd);

            BkpImageBarrierInfo presentBarrier =
            {
                .image = r.sc.images[i],
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStage  = VK_PIPELINE_STAGE_2_NONE, .dstAccess = VK_ACCESS_2_NONE,
                .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
            };
            bkpCmdBarrierImages(vkcmd, &presentBarrier, 1);
        }

        bkpEndCommandBuffer(&r.cmd, f);
        bkpSubmitFrameBasic(r.adp, &r.cmd);
        bkpPresentFrame(r.adp);
    }

    /* 18. Cleanup */
    bkpWaitDevice(r.adp);
    if(r.gltfModel.meshCount > 0) bkpUnloadModel(r.adp, &r.gltfModel);
    unregisterCameraInput();
    hudCleanup(r.adp, &hud);
    bkpDestroyCommandBuffers(r.adp, &r.cmd);
    for(int o = 0; o < OBJ_COUNT; ++o) bkpFreeBuffer(r.adp, &r.objs[o].geo.buffer);
    bkpFreeBuffer(r.adp, &r.skyGeo.buffer);
    bkpFreeBuffer(r.adp, &r.groundGeo.buffer);
    bkpDestroyImageResource(r.adp, &r.depthImg);
    bkpDestroyImageResource(r.adp, &r.skyTex);
    bkpDestroyImageResource(r.adp, &shadowImg);
    bkpDestroyImageResource(r.adp, &groundTex);
    bkpDestroySampler(r.adp, &r.skySampler);
    bkpDestroySampler(r.adp, &shadowSampler);
    bkpDestroySampler(r.adp, &groundSampler);
    bkpDestroyBuffersGPU(r.adp, &r.ubo);
    bkpDestroyGraphicPipeline(r.adp, &scenePipeline);
    bkpDestroyPipelineLayout(r.adp, &scenePipeline.pipelineLayout);
    bkpDestroyGraphicPipeline(r.adp, &shadowPipeline);
    bkpDestroyPipelineLayout(r.adp, &shadowPipeline.pipelineLayout);
    bkpDestroyGraphicPipeline(r.adp, &r.skyPipeline);
    bkpDestroyPipelineLayout(r.adp, &r.skyPipeline.pipelineLayout);
    bkpDestroyShader(r.adp, &sceneProg);
    bkpDestroyShaderModule(r.adp, &sceneVert);
    bkpDestroyShaderModule(r.adp, &sceneFrag);
    bkpDestroyShader(r.adp, &shadowProg);
    bkpDestroyShaderModule(r.adp, &shadowVert);
    bkpDestroyShaderModule(r.adp, &shadowFrag);
    bkpDestroyShader(r.adp, &r.skyProg);
    bkpDestroyShaderModule(r.adp, &r.skyVert);
    bkpDestroyShaderModule(r.adp, &r.skyFrag);
    bkpDestroyDescriptorSet(r.adp, &r.descSet);
    bkpDestroyDescriptorSet(r.adp, &r.skyDescSet);
    bkpDestroyDescriptorPool(r.adp, &r.descPool);
    bkpCleanupSwapChain(r.adp, &r.sc);
    bkpClearGpuAdapter(r.adp);
    bkpQuit(&ctx);
    return 0;
}
