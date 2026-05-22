#include <include/blukpast.h>
#include <system/include/bkp_systemTime.h>

#define FRAMES 2
#define W 800
#define H 600

int main(void)
{
/*context for bkp check all possible configuration
* memory, log, vulkan
*/
    BkpContext ctx = {0};
    BkpConfig cfg = bkpDefaultConfig();
    cfg.vulkanContextInfo.winWidth         = W;
    cfg.vulkanContextInfo.winHeight        = H;
    cfg.vulkanContextInfo.maxFrameInFlight = FRAMES;
    bkpWindowEnableDecoration(1);
    bkpInit(&ctx, &cfg);

/*select the GPU with criteria. NULL = best score*/
    BkpGpuAdapter adp = NULL;
    bkpActivateGpuAdapter(&ctx.vulkanContext, &adp, NULL);

/* create a default swapchain modify sc.* to adjute at will */
    BkpSwapChain sc = {0};
    sc.info = bkpDefaultSwapChain(1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
              VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_TRUE, VK_PRESENT_MODE_FIFO_KHR);
    bkpCreateSwapChain(adp, &sc, (VkPresentModeKHR[]){VK_PRESENT_MODE_FIFO_KHR}, 1);

/*creating shadermodul activate automaticaly reflexion you can use it for pipelinelayout creation or manually create it*/
    BkpShaderModule vert = {0}, frag = {0};
    bkpCreateShaderModule(adp, "tutorial/00_hello_world/shaders/triangle.vert.spv", &vert);
    bkpCreateShaderModule(adp, "tutorial/00_hello_world/shaders/triangle.frag.spv", &frag);
    BkpShaderModule *mods[] = { &vert, &frag };
    BkpShaderProgram prog = {0};
    bkpCreateShader(adp, mods, 2, &prog);

    BkpPipelineGraphic gfx = {0};
    bkpCreatePipelineMinimalInfo(&gfx.info, (VkExtent2D){W, H});
    gfx.info.shaderProgram         = &prog;
    gfx.info.colorAttachmentFormat = sc.info.imageFormat;
    gfx.info.dynamicStates[0]      = VK_DYNAMIC_STATE_VIEWPORT;
    gfx.info.dynamicStates[1]      = VK_DYNAMIC_STATE_SCISSOR;
    gfx.info.dynamicState          = bkpDefaultDynamicStates(gfx.info.dynamicStates, 2);
    gfx.info.dynamicStateEnabled   = VK_TRUE;
    gfx.info.viewportState         = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
    bkpCreatePipelineLayoutFromShader(adp, &prog, &gfx.pipelineLayout);
    bkpCreatePipelineGraphic(adp, &gfx);

    BkpCommandBuffer cmd = { .commandPool = &adp->commandPoolGraphic,
                              .count = FRAMES,
                              .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY };
    bkpAllocateCmdBuffer(adp, &cmd);

    while (!bkpWindowIsClosed(ctx.vulkanContext.win))
    {
        bkpPollEvent(ctx.vulkanContext.win);
        bkpPrepareFrame(adp);
        uint32_t f = adp->frameInfo.currentFrame;
        uint32_t i = adp->frameInfo.imageAcquired;
        VkCommandBuffer vkcmd = cmd.cmds[f];

        bkpBeginCommandBuffer(&cmd, f, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        BkpImageBarrierInfo b0 = {
            .image = sc.images[i],
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcStage  = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
            .dstStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
        };
        bkpCmdBarrierImages(vkcmd, &b0, 1);

        float fw = (float)adp->frameInfo.winWidth;
        float fh = (float)adp->frameInfo.winHeight;
        bkpBeginRendering(vkcmd, sc.imageViews[i], (VkExtent2D){(uint32_t)fw, (uint32_t)fh},
                          (VkClearColorValue){{0.1f, 0.1f, 0.15f, 1.0f}});
        bkpUpdateWindowViewportScissor(vkcmd, fw, fh);
        bkpCmdBindPipeline(vkcmd, gfx.pipeline);
        float t = (float)bkp_time_getClockMilli() / 1000.0f;
        bkpCmdPushConstants(vkcmd, gfx.pipelineLayout.layout,
                            VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &t);
        bkpCmdDraw(vkcmd, 3, 0);
        bkpEndRendering(vkcmd);

        BkpImageBarrierInfo b1 = {
            .image = sc.images[i],
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStage  = VK_PIPELINE_STAGE_2_NONE, .dstAccess = VK_ACCESS_2_NONE,
            .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
        };
        bkpCmdBarrierImages(vkcmd, &b1, 1);

        bkpEndCommandBuffer(&cmd, f);
        bkpSubmitFrameBasic(adp, &cmd);
        bkpPresentFrame(adp);
    }

    bkpWaitDevice(adp);
    bkpDestroyCommandBuffers(adp, &cmd);
    bkpDestroyGraphicPipeline(adp, &gfx);
    bkpDestroyPipelineLayout(adp, &gfx.pipelineLayout);
    bkpDestroyShader(adp, &prog);
    bkpDestroyShaderModule(adp, &vert);
    bkpDestroyShaderModule(adp, &frag);
    bkpCleanupSwapChain(adp, &sc);
    bkpClearGpuAdapter(adp);
    bkpQuit(&ctx);
    return 0;
}
