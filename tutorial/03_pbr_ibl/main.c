#include "ibl.h"

#define W 1280
#define H  720

/* -------------------------------------------------------------------------
 * Free camera — identical to tutorial 02.
 * Controls: WASD fly, E/Q rise/fall, right-click drag to rotate, R to reset.
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

static void freeCameraVectors(FreeCamera * fc, float fwd[3], float right[3])
{
    float cy = cosf(fc->yaw), sy = sinf(fc->yaw);
    float cp = cosf(fc->pitch);
    fwd[0] =  sy * cp;
    fwd[1] =  sinf(fc->pitch);
    fwd[2] = -cy * cp;
    right[0] = cy;
    right[1] = 0.0f;
    right[2] = sy;
}

static void updateFreeCamera(FreeCamera * fc, GLFWwindow * win,
                               float dt,
                               double * prevMouseX, double * prevMouseY,
                               int * prevMiddle)
{
    float fwd[3], right[3];
    freeCameraVectors(fc, fwd, right);
    float spd = fc->speed * dt;

    if(glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
    { fc->pos[0] += fwd[0]*spd; fc->pos[1] += fwd[1]*spd; fc->pos[2] += fwd[2]*spd; }
    if(glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
    { fc->pos[0] -= fwd[0]*spd; fc->pos[1] -= fwd[1]*spd; fc->pos[2] -= fwd[2]*spd; }
    if(glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
    { fc->pos[0] -= right[0]*spd; fc->pos[2] -= right[2]*spd; }
    if(glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
    { fc->pos[0] += right[0]*spd; fc->pos[2] += right[2]*spd; }
    if(glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) { fc->pos[1] += spd; }
    if(glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) { fc->pos[1] -= spd; }

    if(glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS)
    {
        fc->pos[0] = fc->defaultPos[0]; fc->pos[1] = fc->defaultPos[1]; fc->pos[2] = fc->defaultPos[2];
        fc->yaw    = fc->defaultYaw;    fc->pitch   = fc->defaultPitch;
    }

    int middleNow = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT);
    double mx, my;
    glfwGetCursorPos(win, &mx, &my);
    if(middleNow == GLFW_PRESS && *prevMiddle == GLFW_PRESS)
    {
        fc->yaw   += (float)((mx - *prevMouseX) * fc->sensitivity);
        fc->pitch -= (float)((my - *prevMouseY) * fc->sensitivity);
        if(fc->pitch >  1.5f) fc->pitch =  1.5f;
        if(fc->pitch < -1.5f) fc->pitch = -1.5f;
    }
    *prevMouseX = mx; *prevMouseY = my; *prevMiddle = middleNow;
}

static void cameraFromFree(FreeCamera * fc, Camera * cam)
{
    float fwd[3], right[3];
    freeCameraVectors(fc, fwd, right);
    cam->pos[0] = fc->pos[0]; cam->pos[1] = fc->pos[1]; cam->pos[2] = fc->pos[2];
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
     * Same as tutorials 01/02.  bkpDefaultConfig() fills safe defaults; we
     * override dimensions and max in-flight frames (double-buffering). */
    IblRenderer r = {0};
    BkpContext ctx = {0};
    BkpConfig cfg  = bkpDefaultConfig();
    cfg.vulkanContextInfo.winWidth         = W;
    cfg.vulkanContextInfo.winHeight        = H;
    cfg.vulkanContextInfo.maxFrameInFlight = FRAMES;
    bkpWindowEnableDecoration(1);
    bkpInit(&ctx, &cfg);

    r.base.width  = W;
    r.base.height = H;

    /* ---- 2. GPU adapter -------------------------------------------------------- */
    bkpActivateGpuAdapter(&ctx.vulkanContext, &r.base.adp, NULL);

    /* ---- 3. Swap chain --------------------------------------------------------- */
    BkpSwapChain sc = {0};
    sc.info = bkpDefaultSwapChain(1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
              VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_TRUE, VK_PRESENT_MODE_FIFO_KHR);
    bkpCreateSwapChain(r.base.adp, &sc, (VkPresentModeKHR[]){VK_PRESENT_MODE_FIFO_KHR}, 1);
    r.base.sc = sc;

    /* ---- 4. Uniform buffer (scene UBO) ----------------------------------------- */
    r.base.ubo.count = FRAMES;
    r.base.ubo.size  = sizeof(SceneUBO);
    bkpCreateBuffersGPU(r.base.adp, &r.base.ubo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        eBUFFER_CPU_GPU, BKP_DO_MAP);

    /* ---- 5. Depth buffer ------------------------------------------------------- */
    initDepth(&r.base);

    /* ---- 6. IBL resource generation (CPU, done once at startup) ---------------
     * iblInitSkyCubemap  – 256×256 procedural cubemap + samplers.
     * iblInitIrradianceMap – 32×32 cosine-weighted convolution of the sky.
     * iblInitBrdfLut     – 128×128 split-sum GGX BRDF look-up table. */
    iblInitSkyCubemap(&r);
    iblInitIrradianceMap(&r);
    iblInitBrdfLut(&r);
    iblInitDamierTexture(&r);

    /* ---- 7. Pipelines ----------------------------------------------------------
     * iblInitPipelines replaces the common initPipelines.  The scene shader now
     * has 4 bindings (UBO + irradiance + sky + BRDF LUT); the sky shader uses
     * samplerCube instead of sampler2D. */
    iblInitPipelines(&r, "tutorial/03_pbr_ibl/shaders");

    /* ---- 8. Geometry -----------------------------------------------------------
     * Circular primitives at 32 segments (default is 64).
     * Sky box: cube mesh (vertices are used directly as cubemap directions).
     * Ground: plane rotated flat in XZ via makeGroundModel(). */
    bkpSetDefaultSegmentForCircularGeometries(32);

    bkpCreateVertexIndexBuffer(r.base.adp, eBKP_GEO_CUBE,  &r.base.skyGeo);    /* sky box */
    bkpCreateVertexIndexBuffer(r.base.adp, eBKP_GEO_PLANE, &r.base.groundGeo);

    /* Seven primitives with varied roughness and metallic values.
     * Signature: initObject(adp, obj, type, tx,ty,tz, sx,sy,sz, r,g,b, roughness,metallic) */
    initObject(r.base.adp, &r.base.objs[0], eBKP_GEO_SPHERE,    -3.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.9f,0.2f,0.2f, 0.3f,0.0f);
    initObject(r.base.adp, &r.base.objs[1], eBKP_GEO_CUBE,       0.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.2f,0.8f,0.3f, 0.5f,0.0f);
    initObject(r.base.adp, &r.base.objs[2], eBKP_GEO_CYLINDER,   3.0f, 0.5f,  0.0f, 1.0f,1.0f,1.0f, 0.3f,0.5f,0.9f, 0.2f,0.8f);
    initObject(r.base.adp, &r.base.objs[3], eBKP_GEO_CONE,      -1.5f, 0.5f, -3.0f, 1.0f,1.0f,1.0f, 0.9f,0.7f,0.1f, 0.6f,0.0f);
    initObject(r.base.adp, &r.base.objs[4], eBKP_GEO_TORUS,      1.5f, 0.3f, -3.0f, 1.5f,1.5f,1.5f, 0.8f,0.3f,0.8f, 0.4f,0.5f);
    initObject(r.base.adp, &r.base.objs[5], eBKP_GEO_TUBE,      -3.0f, 0.5f, -3.0f, 1.0f,1.0f,1.0f, 0.2f,0.9f,0.8f, 0.7f,0.1f);
    /* Disk: ty=1.0 so the disk (radius 0.75 after scale) clears the ground plane. */
    initObject(r.base.adp, &r.base.objs[6], eBKP_GEO_DISK_PLANE, 3.0f, 1.0f, -3.0f, 1.5f,1.5f,1.5f, 0.9f,0.9f,0.9f, 0.1f,0.9f);
    initGltfSphere(&r.base, -3.0f, 0.5f, 2.0f, 0.5f);

    /* ---- 9. Command buffers ---------------------------------------------------- */
    initCommandBuffers(&r.base);

    /* ---- 10. Camera and ground -------------------------------------------------
     * Free camera starts behind and above, looking slightly downward.
     * Ground: 20×20 plane.  */
    FreeCamera fc = makeFreeCamera(0.0f, 6.0f, 12.0f, 0.0f, -0.4f);
    Camera cam = {0};

    SceneObject ground = {0};
    ground.geo          = r.base.groundGeo;
    ground.model        = makeGroundModel(0.0f, 0.0f, 0.0f, 20.0f, 20.0f);
    ground.color[0]     = 0.6f; ground.color[1] = 0.6f; ground.color[2] = 0.6f; ground.color[3] = 1.0f;
    ground.roughness    = 0.8f;
    ground.metallic     = 0.0f;
    ground.useAlbedoMap = 1.0f;

    GLFWwindow * win  = bkpGetWindow();
    double prevMouseX = 0.0, prevMouseY = 0.0;
    int    prevMiddle = GLFW_RELEASE;
    glfwGetCursorPos(win, &prevMouseX, &prevMouseY);

    uint64_t prevTime = bkp_time_getClockMicro();

    /* ---- 11. Main loop --------------------------------------------------------
     * Delta time is measured with bkp_time_getClockMicro() for frame-rate
     * independent camera movement.  The IBL resources (irradiance map, sky
     * cubemap, BRDF LUT) are static — they are uploaded once in step 6 and
     * bound at pipeline creation in step 7; no per-frame update needed. */
    while(!bkpWindowIsClosed(ctx.vulkanContext.win))
    {
        bkpPollEvent(ctx.vulkanContext.win);

        uint64_t now = bkp_time_getClockMicro();
        float dt = (float)(now - prevTime) * 1e-6f;
        prevTime = now;

        updateFreeCamera(&fc, win, dt, &prevMouseX, &prevMouseY, &prevMiddle);
        cameraFromFree(&fc, &cam);

        bkpPrepareFrame(r.base.adp);

        uint32_t f = r.base.adp->frameInfo.currentFrame;   /* ring-buffer slot [0, FRAMES) */
        uint32_t i = r.base.adp->frameInfo.imageAcquired;  /* swap-chain image index */

        iblRenderFrame(&r, f, i, &cam, &ground);
        bkpSubmitFrameBasic(r.base.adp, &r.base.cmd);
        bkpPresentFrame(r.base.adp);
    }

    /* ---- 12. Cleanup ---------------------------------------------------------- */
    iblCleanup(&r);
    bkpQuit(&ctx);
    return 0;
}
