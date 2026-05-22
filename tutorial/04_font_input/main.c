#include "../common/scene.h"
#include "../common/hud.h"
#include "../common/free_camera.h"

#define W 1280
#define H  720

#define SHADER_DIR "tutorial/04_font_input/shaders"
#define FONT_PATH  "tutorial/04_font_input/AdwaitaMono-Regular.ttf"

/* =========================================================================
 * Main
 * ========================================================================= */
int main(void)
{
    /* ---- 1. Library and window init ------------------------------------ */
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

    /* ---- 2. Swap chain -------------------------------------------------- */
    r.sc.info = bkpDefaultSwapChain(1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_TRUE, VK_PRESENT_MODE_FIFO_KHR);
    bkpCreateSwapChain(r.adp, &r.sc, (VkPresentModeKHR[]){VK_PRESENT_MODE_FIFO_KHR}, 1);

    /* ---- 3. Scene UBO --------------------------------------------------- */
    r.ubo.count = FRAMES;
    r.ubo.size  = sizeof(SceneUBO);
    bkpCreateBuffersGPU(r.adp, &r.ubo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        eBUFFER_CPU_GPU, BKP_DO_MAP);

    /* ---- 4. Sky texture, depth buffer, scene + sky pipelines ----------- */
    initSkyTexture(&r);
    initDepth(&r);
    initPipelines(&r, "tutorial/02_camera/shaders");

    for(uint32_t f = 0; f < FRAMES; ++f)
        bkpWriteDescriptorImage(r.adp, r.skyDescSet.descriptorSets[f], 1,
                                r.skySampler.sampler, r.skyTex.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    /* ---- 5. Geometry ---------------------------------------------------- */
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
    ground.color[0]     = 0.6f; ground.color[1] = 0.6f; ground.color[2] = 0.6f; ground.color[3] = 1.0f;
    ground.roughness    = 0.8f;
    ground.metallic     = 0.0f;
    ground.useAlbedoMap = 1.0f;

    /* ---- 6. Command buffers --------------------------------------------- */
    initCommandBuffers(&r);

    /* ---- 7. HUD --------------------------------------------------------- */
    Hud hud = {0};
    hudInit(r.adp, &hud, r.sc.info.imageFormat, r.depthImg.imageInfo.format, W, H,
            SHADER_DIR, FONT_PATH);

    /* ---- 8. Camera + input registration --------------------------------- */
    FreeCamera fc = makeFreeCamera(0.0f, 8.0f, 16.0f, 0.0f, -0.45f);
    Camera cam = {0};
    registerCameraInput(&fc);
    uint64_t prevTime = bkp_time_getClockMicro();

    /* ---- 9. Main loop --------------------------------------------------- */
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
        uint32_t f = r.adp->frameInfo.currentFrame;
        uint32_t i = r.adp->frameInfo.imageAcquired;

        VkCommandBuffer vkcmd = r.cmd.cmds[f];
        uploadUBO(&r, f, &cam);
        bkpBeginCommandBuffer(&r.cmd, f, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        BkpImageBarrierInfo beginBarriers[2] = {
            { .image = r.sc.images[i],
              .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
              .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              .srcStage  = VK_PIPELINE_STAGE_2_NONE,
              .srcAccess = VK_ACCESS_2_NONE,
              .dstStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
              .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
              .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1 },
            { .image = r.depthImg.images[0],
              .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
              .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
              .srcStage  = VK_PIPELINE_STAGE_2_NONE,
              .srcAccess = VK_ACCESS_2_NONE,
              .dstStage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
              .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
              .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1 }
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
        renderInfo.renderArea.offset     = (VkOffset2D){0, 0};
        renderInfo.renderArea.extent     = (VkExtent2D){(uint32_t)r.width, (uint32_t)r.height};
        renderInfo.layerCount            = 1;
        renderInfo.colorAttachmentCount  = 1;
        renderInfo.pColorAttachments     = &colorAtt;
        renderInfo.pDepthAttachment      = &depthAtt;
        bkpCmdBeginRendering(vkcmd, &renderInfo);
        bkpUpdateWindowViewportScissor(vkcmd, (float)r.width, (float)r.height);

        bkpCmdBindPipeline(vkcmd, r.skyPipeline.pipeline);
        vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            r.skyPipeline.pipelineLayout.layout, 0, 1,
            &r.skyDescSet.descriptorSets[f], 0, NULL);
        {
            VkBuffer vbuf; VkDeviceSize voff;
            bkpGetBuffer(r.skyGeo.buffer, &vbuf);
            bkpGetBufferOffset(r.skyGeo.buffer, &voff);
            vkCmdBindVertexBuffers(vkcmd, 0, 1, &vbuf, &voff);
            VkBuffer ibuf; bkpGetBuffer(r.skyGeo.buffer, &ibuf);
            vkCmdBindIndexBuffer(vkcmd, ibuf, voff + r.skyGeo.indicesOffset, r.skyGeo.indexType);
            vkCmdDrawIndexed(vkcmd, (uint32_t)r.skyGeo.count, 1, 0, 0, 0);
        }

        bkpCmdBindPipeline(vkcmd, r.scenePipeline.pipeline);
        vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            r.scenePipeline.pipelineLayout.layout, 0, 1,
            &r.descSet.descriptorSets[f], 0, NULL);
        drawObject(vkcmd, &r.scenePipeline, &ground);
        for(int o = 0; o < OBJ_COUNT; ++o)
            drawObject(vkcmd, &r.scenePipeline, &r.objs[o]);
        if(r.gltfModel.meshCount > 0)
            drawObject(vkcmd, &r.scenePipeline, &r.gltfSphere);

        hudDraw(r.adp, vkcmd, &hud, f);

        bkpCmdEndRendering(vkcmd);

        BkpImageBarrierInfo presentBarrier = {
            .image = r.sc.images[i],
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStage  = VK_PIPELINE_STAGE_2_NONE,
            .dstAccess = VK_ACCESS_2_NONE,
            .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
        };
        bkpCmdBarrierImages(vkcmd, &presentBarrier, 1);
        bkpEndCommandBuffer(&r.cmd, f);

        bkpSubmitFrameBasic(r.adp, &r.cmd);
        bkpPresentFrame(r.adp);
    }

    /* ---- 10. Cleanup ---------------------------------------------------- */
    bkpWaitDevice(r.adp);
    unregisterCameraInput();
    hudCleanup(r.adp, &hud);
    cleanupRenderer(&r);
    bkpQuit(&ctx);
    return 0;
}
