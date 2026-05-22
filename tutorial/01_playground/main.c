#include "../common/scene.h"

#define W 1280
#define H  720

int main(void)
{
    /* ---- 1. Library init -------------------------------------------------------
     * bkpDefaultConfig() fills a BkpConfig with safe defaults (Vulkan validation
     * layers ON in debug, one window, etc.).  We override width/height and the
     * number of frames that can be in flight simultaneously (double-buffering). */
    BkpContext ctx = {0};
    BkpConfig cfg  = bkpDefaultConfig();
    cfg.vulkanContextInfo.winWidth         = W;
    cfg.vulkanContextInfo.winHeight        = H;
    cfg.vulkanContextInfo.maxFrameInFlight = FRAMES;
    bkpWindowEnableDecoration(1);
    bkpInit(&ctx, &cfg);   /* opens window + creates VkInstance / VkDevice */

    Renderer r = {0};
    r.width  = W;
    r.height = H;

    /* ---- 2. GPU adapter --------------------------------------------------------
     * Selects the first suitable physical device and creates a logical device.
     * NULL = use the default selection strategy. */
    bkpActivateGpuAdapter(&ctx.vulkanContext, &r.adp, NULL);

    /* ---- 3. Swap chain ---------------------------------------------------------
     * The swap chain owns the presentable images.
     * FIFO = vsync on.  We pass the preferred-mode list so bkp can fall back
     * gracefully if the driver doesn't support the first choice. */
    BkpSwapChain sc = {0};
    sc.info = bkpDefaultSwapChain(1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
              VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_TRUE, VK_PRESENT_MODE_FIFO_KHR);
    bkpCreateSwapChain(r.adp, &sc, (VkPresentModeKHR[]){VK_PRESENT_MODE_FIFO_KHR}, 1);
    r.sc = sc;

    /* ---- 4. Uniform buffer (scene UBO) -----------------------------------------
     * One buffer slot per in-flight frame so the CPU can update frame N+1 while
     * the GPU is still rendering frame N.
     * eBUFFER_CPU_GPU  = host-visible + device-local (upload heap).
     * BKP_DO_MAP       = keep the buffer mapped persistently for fast writes. */
    r.ubo.count = FRAMES;
    r.ubo.size  = sizeof(SceneUBO);
    bkpCreateBuffersGPU(r.adp, &r.ubo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        eBUFFER_CPU_GPU, BKP_DO_MAP);

    /* ---- 5. Sky, depth and pipelines -------------------------------------------
     * initSkyTexture  – procedural gradient baked into a 256×1 image.
     * initDepth       – depth attachment (VK_FORMAT_D32_SFLOAT).
     * initPipelines   – compiles scene + sky SPIR-V shaders, builds descriptor
     *                   sets and two VkPipeline objects (scene / sky). */
    initSkyTexture(&r);
    initDepth(&r);
    initPipelines(&r, "tutorial/01_playground/shaders");

    /* Bind the sky gradient texture to descriptor set slot 1 for every frame. */
    for(uint32_t f = 0; f < FRAMES; ++f)
        bkpWriteDescriptorImage(r.adp, r.skyDescSet.descriptorSets[f], 1,
                                r.skySampler.sampler, r.skyTex.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    /* ---- 6. Geometry -----------------------------------------------------------
     * Circular primitives (sphere, cylinder, cone, torus, tube, disk) share a
     * segment count that controls tessellation.  32 is a good trade-off between
     * visual smoothness and vertex budget; the library default is 64. */
    bkpSetDefaultSegmentForCircularGeometries(32);

    bkpCreateVertexIndexBuffer(r.adp, eBKP_GEO_SPHERE, &r.skyGeo);   /* sky dome */
    bkpCreateVertexIndexBuffer(r.adp, eBKP_GEO_PLANE,  &r.groundGeo);

    /* Seven primitives showcasing every built-in geometry type.
     * Signature: initObject(adp, obj, type, tx,ty,tz, sx,sy,sz, r,g,b, roughness,metallic) */
    initObject(r.adp, &r.objs[0], eBKP_GEO_SPHERE,     -3.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.9f,0.2f,0.2f, 0.3f,0.0f);
    initObject(r.adp, &r.objs[1], eBKP_GEO_CUBE,         0.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.2f,0.8f,0.3f, 0.5f,0.0f);
    initObject(r.adp, &r.objs[2], eBKP_GEO_CYLINDER,     3.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.3f,0.5f,0.9f, 0.2f,0.8f);
    initObject(r.adp, &r.objs[3], eBKP_GEO_CONE,        -1.5f, 0.5f, -3.0f, 1.0f,1.0f,1.0f, 0.9f,0.7f,0.1f, 0.6f,0.0f);
    initObject(r.adp, &r.objs[4], eBKP_GEO_TORUS,        1.5f, 0.3f, -3.0f, 1.5f,1.5f,1.5f, 0.8f,0.3f,0.8f, 0.4f,0.5f);
    initObject(r.adp, &r.objs[5], eBKP_GEO_TUBE,        -3.0f, 0.5f, -3.0f, 1.0f,1.0f,1.0f, 0.2f,0.9f,0.8f, 0.7f,0.1f);
    /* Disk: ty=1.0 so the disk (radius 0.75 after scale) clears the ground plane. */
    initObject(r.adp, &r.objs[6], eBKP_GEO_DISK_PLANE,   3.0f, 1.0f, -3.0f, 1.5f,1.5f,1.5f, 0.9f,0.9f,0.9f, 0.1f,0.9f);

    /* ---- 7. Command buffers ----------------------------------------------------
     * One primary command buffer per in-flight frame, allocated from the
     * graphics pool.  They are re-recorded every frame in renderFrame(). */
    initCommandBuffers(&r);

    /* ---- 8. Camera and ground --------------------------------------------------
     * Static camera: positioned behind and above, looking slightly downward.
     * Ground: a PLANE geometry rotated -90° on X so it lies flat in world XZ,
     * then scaled to 20×20 units via makeGroundModel(). */
    Camera cam = {0};
    cam.pos[0] = 0.0f; cam.pos[1] = 6.0f; cam.pos[2] = 12.0f;
    cam.target[0] = 0.0f; cam.target[1] = 0.5f; cam.target[2] = 0.0f;

    SceneObject ground = {0};
    ground.geo       = r.groundGeo;
    ground.model     = makeGroundModel(0.0f, 0.0f, 0.0f, 20.0f, 20.0f);
    ground.color[0]  = 0.6f; ground.color[1] = 0.6f; ground.color[2] = 0.6f; ground.color[3] = 1.0f;
    ground.roughness = 0.8f;
    ground.metallic  = 0.0f;

    /* ---- 9. Main loop ----------------------------------------------------------
     * bkpPrepareFrame acquires the next swap-chain image and waits for the
     * matching in-flight fence, so it is safe to update the UBO for that slot.
     * bkpSubmitFrameBasic submits the recorded command buffer to the GPU.
     * bkpPresentFrame queues the finished image for display. */
    while(!bkpWindowIsClosed(ctx.vulkanContext.win))
    {
        bkpPollEvent(ctx.vulkanContext.win);
        bkpPrepareFrame(r.adp);

        uint32_t f = r.adp->frameInfo.currentFrame;   /* ring-buffer slot [0, FRAMES) */
        uint32_t i = r.adp->frameInfo.imageAcquired;  /* swap-chain image index */

        renderFrame(&r, f, i, &cam, &ground);
        bkpSubmitFrameBasic(r.adp, &r.cmd);
        bkpPresentFrame(r.adp);
    }

    /* ---- 10. Cleanup ----------------------------------------------------------- */
    cleanupRenderer(&r);
    bkpQuit(&ctx);
    return 0;
}
