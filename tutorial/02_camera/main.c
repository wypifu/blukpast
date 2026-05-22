#include "../common/scene.h"

#define W 1280
#define H  720

/* -------------------------------------------------------------------------
 * Free camera — pos + yaw/pitch angles
 *
 * Convention (right-handed, Y-up):
 *   yaw=0, pitch=0  → looks toward -Z
 *   yaw increases counterclockwise (looking from above)
 *   pitch > 0 → looks upward
 * ------------------------------------------------------------------------- */
typedef struct
{
    float pos[3];
    float yaw;
    float pitch;
    float speed;
    float sensitivity;

    float defaultPos[3];
    float defaultYaw;
    float defaultPitch;
} FreeCamera;

static FreeCamera makeFreeCamera(float x, float y, float z, float yaw, float pitch)
{
    FreeCamera fc;
    fc.pos[0] = x; fc.pos[1] = y; fc.pos[2] = z;
    fc.yaw    = yaw;
    fc.pitch  = pitch;
    fc.speed       = 5.0f;
    fc.sensitivity = 0.003f;
    fc.defaultPos[0] = x; fc.defaultPos[1] = y; fc.defaultPos[2] = z;
    fc.defaultYaw   = yaw;
    fc.defaultPitch = pitch;
    return fc;
}

/* Build view-space vectors from yaw/pitch */
static void freeCameraVectors(FreeCamera * fc,
                               float fwd[3], float right[3])
{
    float cy = cosf(fc->yaw), sy = sinf(fc->yaw);
    float cp = cosf(fc->pitch);
    /* forward = (sin(yaw)*cos(pitch), sin(pitch), -cos(yaw)*cos(pitch)) */
    fwd[0] =  sy * cp;
    fwd[1] =  sinf(fc->pitch);
    fwd[2] = -cy * cp;
    /* right = cross(fwd, worldUp) → (cos(yaw), 0, sin(yaw)) */
    right[0] = cy;
    right[1] = 0.0f;
    right[2] = sy;
}

/* Move camera based on GLFW key state and delta time */
static void updateFreeCamera(FreeCamera * fc, GLFWwindow * win,
                               float dt,
                               double * prevMouseX, double * prevMouseY,
                               int * prevMiddle)
{
    float fwd[3], right[3];
    freeCameraVectors(fc, fwd, right);

    float spd = fc->speed * dt;

    if(glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
    {
        fc->pos[0] += fwd[0] * spd;
        fc->pos[1] += fwd[1] * spd;
        fc->pos[2] += fwd[2] * spd;
    }
    if(glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
    {
        fc->pos[0] -= fwd[0] * spd;
        fc->pos[1] -= fwd[1] * spd;
        fc->pos[2] -= fwd[2] * spd;
    }
    if(glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
    {
        fc->pos[0] -= right[0] * spd;
        fc->pos[2] -= right[2] * spd;
    }
    if(glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
    {
        fc->pos[0] += right[0] * spd;
        fc->pos[2] += right[2] * spd;
    }
    if(glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) { fc->pos[1] += spd; }
    if(glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) { fc->pos[1] -= spd; }

    if(glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS)
    {
        fc->pos[0] = fc->defaultPos[0];
        fc->pos[1] = fc->defaultPos[1];
        fc->pos[2] = fc->defaultPos[2];
        fc->yaw    = fc->defaultYaw;
        fc->pitch  = fc->defaultPitch;
    }

    /* Middle-click drag to rotate */
    int middleNow = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE);
    double mx, my;
    glfwGetCursorPos(win, &mx, &my);

    if(middleNow == GLFW_PRESS && *prevMiddle == GLFW_PRESS)
    {
        double dx = mx - *prevMouseX;
        double dy = my - *prevMouseY;
        /* dx > 0 (mouse right) → look right → yaw increases
         * dy > 0 (mouse down in screen) → look down → pitch decreases */
        fc->yaw   += (float)(dx * fc->sensitivity);
        fc->pitch -= (float)(dy * fc->sensitivity);
        if(fc->pitch >  1.5f) { fc->pitch =  1.5f; }
        if(fc->pitch < -1.5f) { fc->pitch = -1.5f; }
    }

    *prevMouseX = mx;
    *prevMouseY = my;
    *prevMiddle = middleNow;
}

/* Convert FreeCamera to Camera (pos + target) for the shared UBO upload */
static void cameraFromFree(FreeCamera * fc, Camera * cam)
{
    float fwd[3], right[3];
    freeCameraVectors(fc, fwd, right);
    cam->pos[0] = fc->pos[0];
    cam->pos[1] = fc->pos[1];
    cam->pos[2] = fc->pos[2];
    cam->target[0] = fc->pos[0] + fwd[0];
    cam->target[1] = fc->pos[1] + fwd[1];
    cam->target[2] = fc->pos[2] + fwd[2];
}

/* =========================================================================
 * Main
 * ========================================================================= */
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
    initPipelines(&r, "tutorial/02_camera/shaders");

    /* Bind the sky gradient texture to descriptor set slot 1 for every frame. */
    for(uint32_t f = 0; f < FRAMES; ++f)
        bkpWriteDescriptorImage(r.adp, r.skyDescSet.descriptorSets[f], 1,
                                r.skySampler.sampler, r.skyTex.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    /* ---- 6. Geometry -----------------------------------------------------------
     * Circular primitives (sphere, cylinder, cone, torus, tube, disk) share a
     * segment count that controls tessellation.  32 is a good trade-off between
     * visual smoothness and vertex budget; the library default is 64.
     * Objects are spread at 5-unit intervals to give the free camera room to
     * navigate around them. */
    bkpSetDefaultSegmentForCircularGeometries(32);

    bkpCreateVertexIndexBuffer(r.adp, eBKP_GEO_SPHERE, &r.skyGeo);   /* sky dome */
    bkpCreateVertexIndexBuffer(r.adp, eBKP_GEO_PLANE,  &r.groundGeo);

    /* Seven primitives showcasing every built-in geometry type.
     * Signature: initObject(adp, obj, type, tx,ty,tz, sx,sy,sz, r,g,b, roughness,metallic) */
    initObject(r.adp, &r.objs[0], eBKP_GEO_SPHERE,    -5.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.9f,0.2f,0.2f, 0.3f,0.0f);
    initObject(r.adp, &r.objs[1], eBKP_GEO_CUBE,       0.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.2f,0.8f,0.3f, 0.5f,0.0f);
    initObject(r.adp, &r.objs[2], eBKP_GEO_CYLINDER,   5.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.3f,0.5f,0.9f, 0.2f,0.8f);
    initObject(r.adp, &r.objs[3], eBKP_GEO_CONE,      -2.5f, 0.5f, -5.0f, 1.0f,1.0f,1.0f, 0.9f,0.7f,0.1f, 0.6f,0.0f);
    initObject(r.adp, &r.objs[4], eBKP_GEO_TORUS,      2.5f, 0.3f, -5.0f, 1.5f,1.5f,1.5f, 0.8f,0.3f,0.8f, 0.4f,0.5f);
    initObject(r.adp, &r.objs[5], eBKP_GEO_TUBE,      -5.0f, 0.5f, -5.0f, 1.0f,1.0f,1.0f, 0.2f,0.9f,0.8f, 0.7f,0.1f);
    /* Disk: ty=1.0 so the disk (radius 0.75 after scale) clears the ground plane. */
    initObject(r.adp, &r.objs[6], eBKP_GEO_DISK_PLANE, 5.0f, 1.0f, -5.0f, 1.5f,1.5f,1.5f, 0.9f,0.9f,0.9f, 0.1f,0.9f);

    /* ---- 7. Command buffers ----------------------------------------------------
     * One primary command buffer per in-flight frame, allocated from the
     * graphics pool.  They are re-recorded every frame in renderFrame(). */
    initCommandBuffers(&r);

    /* ---- 8. Camera and ground --------------------------------------------------
     * Free camera: starts behind and above the scene, looking slightly downward.
     * Controls: WASD to fly, E/Q to rise/fall, middle-click drag to rotate, R to reset.
     * Ground: a PLANE geometry rotated -90° on X so it lies flat in world XZ,
     * then scaled to 30×30 units via makeGroundModel(). */
    FreeCamera fc = makeFreeCamera(0.0f, 8.0f, 16.0f, 0.0f, -0.45f);
    Camera cam = {0};

    SceneObject ground = {0};
    ground.geo          = r.groundGeo;
    ground.model        = makeGroundModel(0.0f, 0.0f, 0.0f, 30.0f, 30.0f);
    ground.color[0]     = 0.6f; ground.color[1] = 0.6f; ground.color[2] = 0.6f; ground.color[3] = 1.0f;
    ground.roughness    = 0.8f;
    ground.metallic     = 0.0f;
    ground.useAlbedoMap = 1.0f;

    GLFWwindow * win  = bkpGetWindow();
    double prevMouseX = 0.0, prevMouseY = 0.0;
    int    prevMiddle = GLFW_RELEASE;
    glfwGetCursorPos(win, &prevMouseX, &prevMouseY);

    uint64_t prevTime = bkp_time_getClockMicro();

    /* ---- 9. Main loop ----------------------------------------------------------
     * Delta time (dt) is measured in seconds using bkp_time_getClockMicro() so
     * camera movement is frame-rate independent.
     * bkpPrepareFrame acquires the next swap-chain image and waits for the
     * matching in-flight fence, so it is safe to update the UBO for that slot.
     * bkpSubmitFrameBasic submits the recorded command buffer to the GPU.
     * bkpPresentFrame queues the finished image for display. */
    while(!bkpWindowIsClosed(ctx.vulkanContext.win))
    {
        bkpPollEvent(ctx.vulkanContext.win);

        uint64_t now = bkp_time_getClockMicro();
        float dt = (float)(now - prevTime) * 1e-6f;
        prevTime = now;

        updateFreeCamera(&fc, win, dt, &prevMouseX, &prevMouseY, &prevMiddle);
        cameraFromFree(&fc, &cam);

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
