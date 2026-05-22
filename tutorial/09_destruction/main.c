/* =========================================================================
 * Tutorial 09 — Destruction with Compute Shaders
 *
 * Demonstrates using BKP's compute pipeline to simulate particle physics on
 * the GPU.  The workflow for each destroyed object, every frame:
 *
 *   1. [COMPUTE] explosion.comp dispatches 512/64 = 8 workgroups.
 *      Each invocation integrates one particle: velocity, gravity, lifetime.
 *      Storage buffer: VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VERTEX_BUFFER_BIT.
 *
 *   2. [BARRIER]  bkpCmdMemoryBarrier ensures compute writes are visible to
 *      the vertex input stage before the particle draw call reads them.
 *
 *   3. [DRAW]    particle.vert reads pos+vel as raw vertex attributes (the
 *      same buffer, now bound as a vertex buffer).  gl_PointSize varies with
 *      lifetime so particles shrink as they fade out.  particle.frag discards
 *      pixels outside the point circle and alpha-fades toward the edges.
 *
 * Interaction:
 *   • Left-click an object to start it spinning / accelerate it.
 *   • There is NO maximum speed cap — keep clicking to push past
 *     DESTROY_THRESHOLD (20 rad/s) and trigger the explosion.
 *   • The object respawns after RESPAWN_TIME (5 s).
 *
 * Builds on tutorial 07 (animations): same shadow, reflection, HUD and free
 * camera setup.  Only the spin logic, compute pipeline and particle render
 * pipeline are new.
 * ========================================================================= */
#include "../common/scene.h"
#include "../common/hud.h"
#include "../common/free_camera.h"

#define W 1280
#define H  720
#define SHADOW_SIZE 4096

#define SHADER_DIR        "tutorial/09_destruction/shaders"
#define SHADER_DIR_SHADOW "tutorial/05_shadow_map/shaders"
#define SHADER_DIR_SKY    "tutorial/02_camera/shaders"
#define SHADER_DIR_HUD    "tutorial/04_font_input/shaders"
#define FONT_PATH         "tutorial/04_font_input/AdwaitaMono-Regular.ttf"

/* ---- Destruction parameters ---------------------------------------------- */
#define PARTICLE_COUNT      512   /* per object; must be a multiple of local_size_x (64) */
#define DESTROY_THRESHOLD   20.0f /* rad/s — spin speed that triggers explosion          */
#define RESPAWN_TIME         5.0f /* seconds; also controls lifetime drain rate (1/5 /s) */

/* ---- Push-constant structs ------------------------------------------------ */
typedef struct { BkpMat4 lightMVP; } ShadowPC;

/* Sent to explosion.comp via vkCmdPushConstants every dispatch. */
typedef struct { float dt; float gravity; float lifetimeDrain; float pad; } ComputePC;

/* Sent to particle.vert: the object's albedo colour, used to tint particles. */
typedef struct { float color[4]; } ParticlePC;

/* ---- Per-particle data layout (maps to both storage buffer AND vertex input)
 *   pos.xyz — world-space position
 *   pos.w   — remaining lifetime in [1.0 .. 0.0]; 0 = dead (skipped by shader)
 *   vel.xyz — velocity in m/s
 *   vel.w   — unused / padding
 * Total: 32 bytes per particle, stride matches VkVertexInputBindingDescription. */
typedef struct { float pos[4]; float vel[4]; } Particle;

/* ---- Per-object spin + destruction state ---------------------------------- */
#define ANIM_MAX_OBJS (OBJ_COUNT + 1)

typedef struct {
    BkpVec3 axis;
    BkpVec3 scale;
    BkpVec3 translation;
    float   speed;
    float   angle;
    float   radius;
    int     active;
    /* destruction */
    int     destroyed;
    float   respawnTimer;
    float   color[3];        /* object albedo colour — copied to particles */
    int     needsParticleInit;
} SpinState;

typedef struct {
    SpinState spins[ANIM_MAX_OBJS];
    int       count;
    int       pendingClick;
    float     clickX, clickY;
    unsigned  seed;
} AnimState;

/* ---- Helpers -------------------------------------------------------------- */
static BkpVec3 randomAxis(unsigned * seed)
{
    float x, y, z, len;
    do {
        *seed = *seed * 1664525u + 1013904223u;
        x = (float)(int)((*seed >> 8) & 0xFF) / 127.5f - 1.0f;
        *seed = *seed * 1664525u + 1013904223u;
        y = (float)(int)((*seed >> 8) & 0xFF) / 127.5f - 1.0f;
        *seed = *seed * 1664525u + 1013904223u;
        z = (float)(int)((*seed >> 8) & 0xFF) / 127.5f - 1.0f;
        len = sqrtf(x*x + y*y + z*z);
    } while(len < 0.01f);
    return bkpVec3(x/len, y/len, z/len);
}

static float lcgf(unsigned * seed)
{
    *seed = *seed * 1664525u + 1013904223u;
    return (float)(*seed & 0xFFFF) / 65535.0f;
}

/* CPU-side particle spawn.  Called once per explosion (after bkpWaitDevice).
 * Particles are seeded at the object's centre with a uniform-sphere velocity
 * distribution plus an upward bias so the debris arcs realistically. */
static void initParticles(BkpGpuAdapter adp, BkpBufferGPU * buf,
                          float cx, float cy, float cz,
                          const float color[3], unsigned * seed)
{
    Particle particles[PARTICLE_COUNT];
    (void)color;
    for(int i = 0; i < PARTICLE_COUNT; ++i)
    {
        /* Uniform random direction on the unit sphere (spherical coordinates) */
        float theta = lcgf(seed) * 6.28318f;
        float phi   = acosf(1.0f - 2.0f * lcgf(seed));
        float spd   = 1.5f + lcgf(seed) * 4.5f;

        /* Small jitter around the object centre so particles don't all start at the same point */
        particles[i].pos[0] = cx + (lcgf(seed) - 0.5f) * 0.4f;
        particles[i].pos[1] = cy + (lcgf(seed) - 0.5f) * 0.4f;
        particles[i].pos[2] = cz + (lcgf(seed) - 0.5f) * 0.4f;
        particles[i].pos[3] = 1.0f;  /* lifetime = full (compute will drain this to 0) */

        particles[i].vel[0] = sinf(phi) * cosf(theta) * spd;
        particles[i].vel[1] = cosf(phi) * spd + 1.5f; /* +1.5 m/s upward bias */
        particles[i].vel[2] = sinf(phi) * sinf(theta) * spd;
        particles[i].vel[3] = 0.0f;
    }

    /* Direct memcpy into the persistently-mapped eBUFFER_CPU_GPU allocation.
     * Safe here because bkpWaitDevice() was called before this function. */
    bkpUploadBufferData(adp, buf->buffer, particles, 0, sizeof(particles));
}

static int raySphere(BkpVec3 orig, BkpVec3 dir,
                     BkpVec3 center, float r, float * tOut)
{
    BkpVec3 oc   = bkpVec3Substract(&orig, &center);
    float   b    = bkpDot3D(&oc, &dir);
    float   c    = bkpDot3D(&oc, &oc) - r * r;
    float   disc = b * b - c;
    if(disc < 0.0f) return 0;
    float sq = sqrtf(disc);
    float t  = -b - sq;
    if(t < 0.0f) t = -b + sq;
    if(t < 0.0f) return 0;
    *tOut = t;
    return 1;
}

static BkpBool leftClick_cb(BkpInputState s, void * data)
{
    if(s.action != eINPUT_PRESSED) return BKP_TRUE;
    AnimState * anim   = (AnimState *)data;
    anim->pendingClick = 1;
    anim->clickX       = s.position.x;
    anim->clickY       = s.position.y;
    return BKP_TRUE;
}

/* ---- Scene UBO (same as tuto07) ------------------------------------------ */
typedef struct {
    BkpMat4 view;
    BkpMat4 proj;
    float   lightDir[4];
    float   camPos[4];
    BkpMat4 lightViewProj;
    BkpMat4 reflectViewProj;
} SceneUBORefl;

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

/* =========================================================================
 * Main
 * ========================================================================= */
int main(void)
{
    /* 1. Init */
    BkpContext ctx = {0};
    BkpConfig  cfg = bkpDefaultConfig();
    cfg.vulkanContextInfo.winWidth         = W;
    cfg.vulkanContextInfo.winHeight        = H;
    cfg.vulkanContextInfo.maxFrameInFlight = FRAMES;
    bkpWindowEnableDecoration(1);
    bkpInit(&ctx, &cfg);

    /* largePoints: required to set gl_PointSize > 1.0 in particle.vert. */
    BkpFeatureHint hint = {0};
    hint.requested.shaderClipDistance = VK_TRUE;
    hint.requested.largePoints        = VK_TRUE;
    BkpAdapterCriteria criteria = {0, eGPUTYPE_DISCRETE, &hint};

    Renderer r = {0};
    r.width = W; r.height = H;
    bkpActivateGpuAdapter(&ctx.vulkanContext, &r.adp, &criteria);

    /* 2. Swap chain */
    r.sc.info = bkpDefaultSwapChain(1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_TRUE, VK_PRESENT_MODE_FIFO_KHR);
    bkpCreateSwapChain(r.adp, &r.sc, (VkPresentModeKHR[]){VK_PRESENT_MODE_FIFO_KHR}, 1);

    /* 3. Scene UBO: 2×FRAMES — slots [0..FRAMES-1]=main, [FRAMES..2*FRAMES-1]=reflect */
    r.ubo.count = FRAMES * 2;
    r.ubo.size  = sizeof(SceneUBORefl);
    bkpCreateBuffersGPU(r.adp, &r.ubo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        eBUFFER_CPU_GPU, BKP_DO_MAP);

    /* 4. Depth + sky */
    initDepth(&r);
    initSkyTexture(&r);

    /* 5. Shadow map */
    BkpImageResource shadowImg     = {0};
    BkpSampler       shadowSampler = {0};
    initShadowResources(r.adp, &shadowImg, &shadowSampler);

    /* 6. Ground texture */
    BkpImageResource groundTex     = {0};
    BkpSampler       groundSampler = {0};
    initGroundTexture(r.adp, &groundTex, &groundSampler);

    VkFormat colorFmt = r.sc.info.imageFormat;
    VkFormat depthFmt = r.depthImg.imageInfo.format;

    /* 7. Reflection color buffer */
    BkpImageResource reflectImg = bkpDefaultColorBuffer();
    reflectImg.imageInfo.extent = (VkExtent3D){W, H, 1};
    reflectImg.imageInfo.format = colorFmt;
    reflectImg.viewInfo.format  = colorFmt;
    bkpCreateImageResources(r.adp, &reflectImg);

    /* 8. Reflect depth */
    BkpImageResource reflDepth = bkpDefaultDepthBuffer();
    reflDepth.imageInfo.extent = (VkExtent3D){W, H, 1};
    bkpCreateDepthResources(r.adp, &reflDepth);

    /* 9. Reflect sampler */
    BkpSampler reflectSampler = {0};
    reflectSampler.info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    reflectSampler.info.magFilter    = VK_FILTER_LINEAR;
    reflectSampler.info.minFilter    = VK_FILTER_LINEAR;
    reflectSampler.info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    reflectSampler.info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    reflectSampler.info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    reflectSampler.info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    reflectSampler.info.minLod       = 0.0f;
    reflectSampler.info.maxLod       = 1.0f;
    bkpCreateSampler(r.adp, &reflectSampler);

    /* 10. Vertex layouts */
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

    /* Particle vertex layout: slot 0 = pos (w=lifetime), slot 1 = vel */
    static VkVertexInputBindingDescription partBinding =
    {
        0, (uint32_t)sizeof(Particle), VK_VERTEX_INPUT_RATE_VERTEX
    };
    static VkVertexInputAttributeDescription partAttrs[2] =
    {
        {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT,  0},  /* pos */
        {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 16},  /* vel */
    };
    BkpVertexLayout partLayout = {partAttrs, &partBinding, 2, 1};

    /* 11. Shadow pipeline */
    BkpShaderModule shadowVert = {0}, shadowFrag = {0};
    BkpShaderProgram shadowProg = {0};
    BkpPipelineGraphic shadowPipeline = {0};
    {
        char vs[256], fs[256];
        snprintf(vs, sizeof(vs), "%s/shadow.vert.spv", SHADER_DIR_SHADOW);
        snprintf(fs, sizeof(fs), "%s/shadow.frag.spv", SHADER_DIR_SHADOW);
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

    /* 12. Reflect pipeline */
    BkpShaderModule reflVert = {0}, reflFrag = {0};
    BkpShaderProgram reflProg = {0};
    BkpPipelineGraphic reflectPipeline = {0};
    {
        char vs[256], fs[256];
        snprintf(vs, sizeof(vs), "%s/reflect.vert.spv", SHADER_DIR);
        snprintf(fs, sizeof(fs), "%s/scene.frag.spv",   SHADER_DIR);
        bkpCreateShaderModule(r.adp, vs, &reflVert);
        bkpCreateShaderModule(r.adp, fs, &reflFrag);
        BkpShaderModule * mods[] = {&reflVert, &reflFrag};
        bkpCreateShader(r.adp, mods, 2, &reflProg);
        bkpCreatePipelineLayoutFromShader(r.adp, &reflProg, &reflectPipeline.pipelineLayout);
        bkpCreatePipelineMinimalInfo(&reflectPipeline.info, (VkExtent2D){W, H});
        reflectPipeline.info.shaderProgram         = &reflProg;
        reflectPipeline.info.colorAttachmentFormat = colorFmt;
        reflectPipeline.info.depthAttachmentFormat = depthFmt;
        reflectPipeline.info.vertexLayout          = vLayout;
        reflectPipeline.info.rasterizer.cullMode   = VK_CULL_MODE_FRONT_BIT;
        reflectPipeline.info.dynamicStates[0]      = VK_DYNAMIC_STATE_VIEWPORT;
        reflectPipeline.info.dynamicStates[1]      = VK_DYNAMIC_STATE_SCISSOR;
        reflectPipeline.info.dynamicState          = bkpDefaultDynamicStates(reflectPipeline.info.dynamicStates, 2);
        reflectPipeline.info.dynamicStateEnabled   = VK_TRUE;
        reflectPipeline.info.viewportState         = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
        bkpCreatePipelineGraphic(r.adp, &reflectPipeline);
    }

    /* 13. Scene pipeline */
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

    /* 14. Floor pipeline */
    BkpShaderModule floorVert = {0}, floorFrag = {0};
    BkpShaderProgram floorProg = {0};
    BkpPipelineGraphic floorPipeline = {0};
    {
        char vs[256], fs[256];
        snprintf(vs, sizeof(vs), "%s/floor.vert.spv", SHADER_DIR);
        snprintf(fs, sizeof(fs), "%s/floor.frag.spv", SHADER_DIR);
        bkpCreateShaderModule(r.adp, vs, &floorVert);
        bkpCreateShaderModule(r.adp, fs, &floorFrag);
        BkpShaderModule * mods[] = {&floorVert, &floorFrag};
        bkpCreateShader(r.adp, mods, 2, &floorProg);
        bkpCreatePipelineLayoutFromShader(r.adp, &floorProg, &floorPipeline.pipelineLayout);
        bkpCreatePipelineMinimalInfo(&floorPipeline.info, (VkExtent2D){W, H});
        floorPipeline.info.shaderProgram         = &floorProg;
        floorPipeline.info.colorAttachmentFormat = colorFmt;
        floorPipeline.info.depthAttachmentFormat = depthFmt;
        floorPipeline.info.vertexLayout          = vLayout;
        floorPipeline.info.dynamicStates[0]      = VK_DYNAMIC_STATE_VIEWPORT;
        floorPipeline.info.dynamicStates[1]      = VK_DYNAMIC_STATE_SCISSOR;
        floorPipeline.info.dynamicState          = bkpDefaultDynamicStates(floorPipeline.info.dynamicStates, 2);
        floorPipeline.info.dynamicStateEnabled   = VK_TRUE;
        floorPipeline.info.viewportState         = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
        bkpCreatePipelineGraphic(r.adp, &floorPipeline);
    }

    /* 15. Sky pipeline */
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
        r.skyPipeline.info.shaderProgram                  = &r.skyProg;
        r.skyPipeline.info.colorAttachmentFormat          = colorFmt;
        r.skyPipeline.info.depthAttachmentFormat          = depthFmt;
        r.skyPipeline.info.vertexLayout                   = vLayout;
        r.skyPipeline.info.depthStencil.depthWriteEnable  = VK_FALSE;
        r.skyPipeline.info.rasterizer.cullMode            = VK_CULL_MODE_NONE;
        r.skyPipeline.info.dynamicStates[0]               = VK_DYNAMIC_STATE_VIEWPORT;
        r.skyPipeline.info.dynamicStates[1]               = VK_DYNAMIC_STATE_SCISSOR;
        r.skyPipeline.info.dynamicState                   = bkpDefaultDynamicStates(r.skyPipeline.info.dynamicStates, 2);
        r.skyPipeline.info.dynamicStateEnabled            = VK_TRUE;
        r.skyPipeline.info.viewportState                  = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
        bkpCreatePipelineGraphic(r.adp, &r.skyPipeline);
    }

    /* 16. Compute pipeline (explosion.comp) — updates particle positions/lifetimes.
     *
     * BKP has no compute-specific bind wrappers: bkpCmdBindPipeline and
     * bkpCmdBindDescriptorSets both hardcode VK_PIPELINE_BIND_POINT_GRAPHICS.
     * In the main loop we therefore call vkCmdBindPipeline and
     * vkCmdBindDescriptorSets directly with VK_PIPELINE_BIND_POINT_COMPUTE. */
    BkpShaderModule  computeMod  = {0};
    BkpShaderProgram computeProg = {0};
    BkpPipelineCompute computePipeline = {0};
    {
        char cs[256];
        snprintf(cs, sizeof(cs), "%s/explosion.comp.spv", SHADER_DIR);
        bkpCreateShaderModule(r.adp, cs, &computeMod);
        BkpShaderModule * mods[] = {&computeMod};
        bkpCreateShader(r.adp, mods, 1, &computeProg);
        bkpCreatePipelineLayoutFromShader(r.adp, &computeProg, &computePipeline.pipelineLayout);
        computePipeline.info.shaderProgram  = &computeProg;
        bkpCreatePipelineCompute(r.adp, &computePipeline);
    }

    /* 17. Particle render pipeline (point sprites, alpha-blended).
     *
     * POINT_LIST: each vertex becomes one screen-space square whose size is set
     *   by gl_PointSize in the vertex shader.  The fragment shader reads
     *   gl_PointCoord (0..1 across the square) to draw circular soft discs.
     *   The largePoints GPU feature (requested in step 1) is required to set
     *   gl_PointSize > 1.
     *
     * depthWriteEnable = FALSE: particles are semi-transparent and drawn
     *   back-to-front is not guaranteed, so writing depth would silently occlude
     *   later particles drawn in the same frame.  Depth testing (reads) stays on
     *   so particles are correctly hidden behind opaque geometry.
     *
     * Alpha blend (SRC_ALPHA / ONE_MINUS_SRC_ALPHA): standard Porter-Duff
     *   "over" compositing.  The alpha in particle.frag fades toward the edges
     *   and decays with lifetime, giving a soft fading-out look. */
    BkpShaderModule  particleVert = {0}, particleFrag = {0};
    BkpShaderProgram particleProg = {0};
    BkpPipelineGraphic particlePipeline = {0};
    {
        char vs[256], fs[256];
        snprintf(vs, sizeof(vs), "%s/particle.vert.spv", SHADER_DIR);
        snprintf(fs, sizeof(fs), "%s/particle.frag.spv", SHADER_DIR);
        bkpCreateShaderModule(r.adp, vs, &particleVert);
        bkpCreateShaderModule(r.adp, fs, &particleFrag);
        BkpShaderModule * mods[] = {&particleVert, &particleFrag};
        bkpCreateShader(r.adp, mods, 2, &particleProg);
        bkpCreatePipelineLayoutFromShader(r.adp, &particleProg, &particlePipeline.pipelineLayout);
        bkpCreatePipelineMinimalInfo(&particlePipeline.info, (VkExtent2D){W, H});
        particlePipeline.info.shaderProgram          = &particleProg;
        particlePipeline.info.colorAttachmentFormat  = colorFmt;
        particlePipeline.info.depthAttachmentFormat  = depthFmt;
        particlePipeline.info.vertexLayout           = partLayout;
        particlePipeline.info.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        particlePipeline.info.rasterizer.cullMode    = VK_CULL_MODE_NONE;
        particlePipeline.info.depthStencil.depthWriteEnable = VK_FALSE;
        /* alpha blend: src_alpha / 1-src_alpha */
        particlePipeline.info.colorAttachments[0].blendEnable         = VK_TRUE;
        particlePipeline.info.colorAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        particlePipeline.info.colorAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        particlePipeline.info.colorAttachments[0].colorBlendOp        = VK_BLEND_OP_ADD;
        particlePipeline.info.colorAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        particlePipeline.info.colorAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        particlePipeline.info.colorAttachments[0].alphaBlendOp        = VK_BLEND_OP_ADD;
        particlePipeline.info.colorBlending =
            bkpDefaultPipelineColorBlend(particlePipeline.info.colorAttachments, 1);
        particlePipeline.info.dynamicStates[0]      = VK_DYNAMIC_STATE_VIEWPORT;
        particlePipeline.info.dynamicStates[1]      = VK_DYNAMIC_STATE_SCISSOR;
        particlePipeline.info.dynamicState          = bkpDefaultDynamicStates(particlePipeline.info.dynamicStates, 2);
        particlePipeline.info.dynamicStateEnabled   = VK_TRUE;
        particlePipeline.info.viewportState         = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
        bkpCreatePipelineGraphic(r.adp, &particlePipeline);
    }

    /* 18. Descriptor pool
     *   scene/reflect/floor/sky: 4 sets × FRAMES → 4×FRAMES UBOs, 8×FRAMES images
     *   particle render:          1 set  × FRAMES → 1×FRAMES UBOs
     *   compute:                  ANIM_MAX_OBJS sets (count=1) → ANIM_MAX_OBJS storage buffers */
    {
        VkDescriptorPoolSize poolSizes[3] =
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         (4 + 1) * FRAMES},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8 * FRAMES},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         ANIM_MAX_OBJS},
        };
        r.descPool.info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        r.descPool.info.maxSets       = (4 + 1) * FRAMES + ANIM_MAX_OBJS;
        r.descPool.info.poolSizeCount = 3;
        r.descPool.info.pPoolSizes    = poolSizes;
        bkpCreateDescriptorPool(r.adp, &r.descPool);
    }

    /* scene descriptor set */
    r.descSet.descPool   = &r.descPool;
    r.descSet.descLayout = &sceneProg.layouts[0];
    r.descSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &r.descSet);

    /* reflect descriptor set */
    BkpDescriptorSet reflectDescSet = {0};
    reflectDescSet.descPool   = &r.descPool;
    reflectDescSet.descLayout = &reflProg.layouts[0];
    reflectDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &reflectDescSet);

    /* floor descriptor set */
    BkpDescriptorSet floorDescSet = {0};
    floorDescSet.descPool   = &r.descPool;
    floorDescSet.descLayout = &floorProg.layouts[0];
    floorDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &floorDescSet);

    /* sky descriptor set */
    r.skyDescSet.descPool   = &r.descPool;
    r.skyDescSet.descLayout = &r.skyProg.layouts[0];
    r.skyDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &r.skyDescSet);

    /* particle render descriptor set: binding 0 = main UBO (view + proj) */
    BkpDescriptorSet particleRenderDescSet = {0};
    particleRenderDescSet.descPool   = &r.descPool;
    particleRenderDescSet.descLayout = &particleProg.layouts[0];
    particleRenderDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &particleRenderDescSet);

    /* compute descriptor sets: one per object, binding 0 = STORAGE_BUFFER */
    BkpDescriptorSet computeDescSet = {0};
    computeDescSet.descPool   = &r.descPool;
    computeDescSet.descLayout = &computeProg.layouts[0];
    computeDescSet.count      = ANIM_MAX_OBJS;
    bkpCreateDescriptorSet(r.adp, &computeDescSet);

    /* 19. Per-object particle storage buffers
     *   count=1 (single ring-buffer slot): safe because we call bkpWaitDevice before
     *   any CPU write, and within a frame, a pipeline barrier orders compute→vertex.  */
    BkpBufferGPU particleBufs[ANIM_MAX_OBJS];
    {
        size_t partBufSize = PARTICLE_COUNT * sizeof(Particle);
        for(int o = 0; o < ANIM_MAX_OBJS; ++o)
        {
            particleBufs[o].count = 1;
            particleBufs[o].size  = partBufSize;
            bkpCreateBuffersGPU(r.adp, &particleBufs[o],
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                eBUFFER_CPU_GPU, BKP_DO_MAP);
        }
    }

    /* 20. Write all descriptor sets */
    for(uint32_t f = 0; f < FRAMES; ++f)
    {
        VkDescriptorBufferInfo mainBuf = bkpMakeUboInfo(r.ubo.buffer, f,          r.ubo.size);
        VkDescriptorBufferInfo reflBuf = bkpMakeUboInfo(r.ubo.buffer, FRAMES + f, r.ubo.size);

        /* scene */
        {
            BkpWriteDescriptorSetInfo wr = {0};
            wr.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            wr.setBindings[0].ubos  = &mainBuf;
            wr.setBindings[0].count = 1;
            bkpWriteDescriptorSet(r.adp, &r.descSet, f, &wr);
            bkpWriteDescriptorImage(r.adp, r.descSet.descriptorSets[f], 1,
                groundSampler.sampler, groundTex.imageViews[0],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            bkpWriteDescriptorImage(r.adp, r.descSet.descriptorSets[f], 2,
                shadowSampler.sampler, shadowImg.imageViews[0],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        /* reflect */
        {
            BkpWriteDescriptorSetInfo wr = {0};
            wr.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            wr.setBindings[0].ubos  = &reflBuf;
            wr.setBindings[0].count = 1;
            bkpWriteDescriptorSet(r.adp, &reflectDescSet, f, &wr);
            bkpWriteDescriptorImage(r.adp, reflectDescSet.descriptorSets[f], 1,
                groundSampler.sampler, groundTex.imageViews[0],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            bkpWriteDescriptorImage(r.adp, reflectDescSet.descriptorSets[f], 2,
                shadowSampler.sampler, shadowImg.imageViews[0],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        /* floor */
        {
            BkpWriteDescriptorSetInfo wr = {0};
            wr.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            wr.setBindings[0].ubos  = &mainBuf;
            wr.setBindings[0].count = 1;
            bkpWriteDescriptorSet(r.adp, &floorDescSet, f, &wr);
            bkpWriteDescriptorImage(r.adp, floorDescSet.descriptorSets[f], 1,
                groundSampler.sampler, groundTex.imageViews[0],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            bkpWriteDescriptorImage(r.adp, floorDescSet.descriptorSets[f], 2,
                shadowSampler.sampler, shadowImg.imageViews[0],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            bkpWriteDescriptorImage(r.adp, floorDescSet.descriptorSets[f], 3,
                reflectSampler.sampler, reflectImg.imageViews[0],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        /* sky */
        {
            BkpWriteDescriptorSetInfo wr = {0};
            wr.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            wr.setBindings[0].ubos  = &mainBuf;
            wr.setBindings[0].count = 1;
            bkpWriteDescriptorSet(r.adp, &r.skyDescSet, f, &wr);
            bkpWriteDescriptorImage(r.adp, r.skyDescSet.descriptorSets[f], 1,
                r.skySampler.sampler, r.skyTex.imageViews[0],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        /* particle render */
        {
            BkpWriteDescriptorSetInfo wr = {0};
            wr.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            wr.setBindings[0].ubos  = &mainBuf;
            wr.setBindings[0].count = 1;
            bkpWriteDescriptorSet(r.adp, &particleRenderDescSet, f, &wr);
        }
    }

    /* compute: per-object storage buffer descriptors */
    for(int o = 0; o < ANIM_MAX_OBJS; ++o)
    {
        VkDescriptorBufferInfo sb = bkpMakeUboInfo(particleBufs[o].buffer, 0, particleBufs[o].size);
        BkpWriteDescriptorSetInfo wr = {0};
        wr.setBindings[0].type  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        wr.setBindings[0].ubos  = &sb;
        wr.setBindings[0].count = 1;
        bkpWriteDescriptorSet(r.adp, &computeDescSet, o, &wr);
    }

    /* 21. Geometry */
    bkpSetDefaultSegmentForCircularGeometries(32);
    bkpCreateVertexIndexBuffer(r.adp, eBKP_GEO_SPHERE, &r.skyGeo);
    bkpCreateVertexIndexBuffer(r.adp, eBKP_GEO_PLANE,  &r.groundGeo);

    initObject(r.adp, &r.objs[0], eBKP_GEO_SPHERE,    -5.0f, 1.5f,  0.0f, 1.0f,1.0f,1.0f, 0.9f,0.2f,0.2f, 0.3f,0.0f);
    initObject(r.adp, &r.objs[1], eBKP_GEO_CUBE,       0.0f, 1.5f,  0.0f, 1.0f,1.0f,1.0f, 0.2f,0.8f,0.3f, 0.5f,0.0f);
    initObject(r.adp, &r.objs[2], eBKP_GEO_CYLINDER,   5.0f, 1.5f,  0.0f, 1.0f,1.0f,1.0f, 0.3f,0.5f,0.9f, 0.2f,0.8f);
    initObject(r.adp, &r.objs[3], eBKP_GEO_CONE,      -2.5f, 1.5f, -5.0f, 1.0f,1.0f,1.0f, 0.9f,0.7f,0.1f, 0.6f,0.0f);
    initObject(r.adp, &r.objs[4], eBKP_GEO_TORUS,      2.5f, 1.3f, -5.0f, 1.5f,1.5f,1.5f, 0.8f,0.3f,0.8f, 0.4f,0.5f);
    initObject(r.adp, &r.objs[5], eBKP_GEO_TUBE,      -5.0f, 1.5f, -5.0f, 1.0f,1.0f,1.0f, 0.2f,0.9f,0.8f, 0.7f,0.1f);
    initObject(r.adp, &r.objs[6], eBKP_GEO_DISK_PLANE, 5.0f, 1.5f, -5.0f, 1.5f,1.5f,1.5f, 0.9f,0.9f,0.9f, 0.1f,0.9f);
    initGltfSphere(&r, -5.0f, 1.5f, 2.0f, 0.5f);

    /* 22. Spin + destruction state */
    AnimState anim = {0};
    anim.seed  = 0xDEADBEEFu;
    anim.count = OBJ_COUNT;
    for(int o = 0; o < OBJ_COUNT; ++o)
    {
        anim.spins[o].translation = bkpVec3(r.objs[o].model.mLine3.x,
                                            r.objs[o].model.mLine3.y,
                                            r.objs[o].model.mLine3.z);
        float sx = r.objs[o].model.mLine0.x;
        float sy = r.objs[o].model.mLine1.y;
        float sz = r.objs[o].model.mLine2.z;
        anim.spins[o].scale  = bkpVec3(sx, sy, sz);
        float maxS = sx > sy ? sx : sy; if(sz > maxS) maxS = sz;
        anim.spins[o].radius = maxS * 1.3f + 0.2f;
        anim.spins[o].color[0] = r.objs[o].color[0];
        anim.spins[o].color[1] = r.objs[o].color[1];
        anim.spins[o].color[2] = r.objs[o].color[2];
    }
    if(r.gltfModel.meshCount > 0)
    {
        int gi = OBJ_COUNT;
        anim.count = OBJ_COUNT + 1;
        anim.spins[gi].translation = bkpVec3(r.gltfSphere.model.mLine3.x,
                                             r.gltfSphere.model.mLine3.y,
                                             r.gltfSphere.model.mLine3.z);
        float s = r.gltfSphere.model.mLine0.x;
        anim.spins[gi].scale  = bkpVec3(s, s, s);
        anim.spins[gi].radius = s * 1.3f + 0.2f;
        anim.spins[gi].color[0] = 0.9f;
        anim.spins[gi].color[1] = 0.9f;
        anim.spins[gi].color[2] = 0.9f;
    }

    SceneObject ground = {0};
    ground.geo          = r.groundGeo;
    ground.model        = makeGroundModel(0.0f, 0.0f, 0.0f, 30.0f, 30.0f);
    ground.color[0]     = 1.0f; ground.color[1] = 1.0f; ground.color[2] = 1.0f; ground.color[3] = 1.0f;
    ground.roughness    = 0.03f;
    ground.metallic     = 0.95f;
    ground.useAlbedoMap = 1.0f;

    /* 23. Command buffers */
    initCommandBuffers(&r);

    /* 24. Bake shadow map */
    BkpMat4 lightViewProj = computeLightViewProj();
    {
        BkpFence bakeFence = {0};
        bkpCreateFence(r.adp, &bakeFence, BKP_FALSE);

        bkpBeginCommandBuffer(&r.cmd, 0, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VkCommandBuffer bcmd = r.cmd.cmds[0];

        BkpImageBarrierInfo toDepth =
        {
            .image = shadowImg.images[0], .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .srcStage = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
            .dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                       | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1,
            .baseLayer = 0, .layerCount = 1
        };
        bkpCmdBarrierImages(bcmd, &toDepth, 1);

        VkRenderingAttachmentInfo sdAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        sdAtt.imageView   = shadowImg.imageViews[0];
        sdAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        sdAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        sdAtt.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        sdAtt.clearValue  = (VkClearValue){.depthStencil = {1.0f, 0}};

        VkRenderingInfo sdInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
        sdInfo.renderArea.extent = (VkExtent2D){SHADOW_SIZE, SHADOW_SIZE};
        sdInfo.layerCount        = 1;
        sdInfo.pDepthAttachment  = &sdAtt;
        bkpCmdBeginRendering(bcmd, &sdInfo);
        VkViewport sdVP = {0.0f, 0.0f, (float)SHADOW_SIZE, (float)SHADOW_SIZE, 0.0f, 1.0f};
        VkRect2D   sdSc = {{0, 0}, {SHADOW_SIZE, SHADOW_SIZE}};
        vkCmdSetViewport(bcmd, 0, 1, &sdVP);
        vkCmdSetScissor(bcmd, 0, 1, &sdSc);
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
            .srcStage = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccess = VK_ACCESS_2_SHADER_READ_BIT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1,
            .baseLayer = 0, .layerCount = 1
        };
        bkpCmdBarrierImages(bcmd, &toReadOnly, 1);
        bkpEndCommandBuffer(&r.cmd, 0);

        BkpCommandBuffer * bakeCmd = &r.cmd;
        bkpSubmit(r.adp, eQFAMILY_GRAPHIC, &bakeCmd, 1, 0, NULL, &bakeFence);
        bkpWaitForFence(r.adp, &bakeFence, UINT64_MAX);
        bkpDestroyFence(r.adp, &bakeFence);
    }

    /* 25. HUD */
    Hud hud = {0};
    hudInit(r.adp, &hud, colorFmt, depthFmt, W, H, SHADER_DIR_HUD, FONT_PATH);
    bkpSetText(hud.helpText,
               "WASD EQ movement\nright-click drag to rotate\nr reset camera\n"
               "left-click to spin / accelerate\nkeep clicking to destroy!");

    /* 26. Camera + input */
    FreeCamera fc = makeFreeCamera(0.0f, 8.0f, 16.0f, 0.0f, -0.45f);
    Camera     cam = {0};
    registerCameraInput(&fc);
    bkpAddMouseAction(eMOUSE_BUTTON_LEFT, leftClick_cb, &anim);
    uint64_t prevTime = bkp_time_getClockMicro();

    /* =========================================================================
     * 27. Main loop
     * ========================================================================= */
    while(!bkpWindowIsClosed(ctx.vulkanContext.win))
    {
        bkpPollEvent(ctx.vulkanContext.win);

        uint64_t now = bkp_time_getClockMicro();
        float dt = (float)(now - prevTime) * 1e-6f;
        prevTime = now;
        if(dt > 0.1f) dt = 0.1f; /* clamp large dt (e.g. first frame, debugger pause) */

        updateFreeCamera(&fc, dt);
        cameraFromFree(&fc, &cam);
        hudUpdate(&hud, dt);

        /* ---- Animation update -------------------------------------------- */
        {
            BkpVec3 up     = bkpVec3(0.0f, 1.0f, 0.0f);
            BkpVec3 eye    = bkpVec3(cam.pos[0],    cam.pos[1],    cam.pos[2]);
            BkpVec3 target = bkpVec3(cam.target[0], cam.target[1], cam.target[2]);
            BkpMat4 view   = bkpLookAt(eye, target, up);
            BkpMat4 proj   = bkpPerspective(45.0f, (float)r.width / (float)r.height, 0.1f, 500.0f);

            /* Ray-cast left click */
            if(anim.pendingClick)
            {
                anim.pendingClick = 0;
                float ndcX = (anim.clickX / (float)r.width)  * 2.0f - 1.0f;
                float ndcY = (anim.clickY / (float)r.height) * 2.0f - 1.0f;
                BkpMat4 projFlipped = proj;
                projFlipped.mLine1.y *= -1.0f;
                BkpMat4 vp    = bkpMat4DotMat4(&projFlipped, &view);
                BkpMat4 invVP = bkpMat4Inverse(&vp);

                BkpVec4 clipNear = bkpVec4(ndcX, ndcY, 0.0f, 1.0f);
                BkpVec4 clipFar  = bkpVec4(ndcX, ndcY, 1.0f, 1.0f);
                BkpVec4 wn = bkpMat4DotVec4(&invVP, &clipNear);
                BkpVec4 wf = bkpMat4DotVec4(&invVP, &clipFar);
                BkpVec3 near3 = bkpVec3(wn.x/wn.w, wn.y/wn.w, wn.z/wn.w);
                BkpVec3 far3  = bkpVec3(wf.x/wf.w, wf.y/wf.w, wf.z/wf.w);
                BkpVec3 dir   = bkpVec3Substract(&far3, &near3);
                dir = bkpNormalized3D(dir);

                float bestT  = 1e30f;
                int   hitIdx = -1;
                for(int o = 0; o < anim.count; ++o)
                {
                    SpinState * sp = &anim.spins[o];
                    if(sp->destroyed) continue;  /* can't click destroyed objects */
                    float t;
                    if(raySphere(near3, dir, sp->translation, sp->radius, &t) && t < bestT)
                    {
                        bestT  = t;
                        hitIdx = o;
                    }
                }
                if(hitIdx >= 0)
                {
                    SpinState * sp = &anim.spins[hitIdx];
                    if(!sp->active)
                    {
                        sp->axis   = randomAxis(&anim.seed);
                        sp->angle  = 0.0f;
                        sp->active = 1;
                    }
                    sp->speed += 2.5f; /* no cap — keep accelerating until destruction */
                }
            }

            /* Advance spin angles + handle destruction / respawn */
            for(int o = 0; o < anim.count; ++o)
            {
                SpinState * sp = &anim.spins[o];

                if(sp->destroyed)
                {
                    sp->respawnTimer -= dt;
                    if(sp->respawnTimer <= 0.0f)
                    {
                        /* Respawn: reset spin state, rebuild identity-rotation model */
                        sp->destroyed = 0;
                        sp->active    = 0;
                        sp->speed     = 0.0f;
                        sp->angle     = 0.0f;
                        BkpMat4 S = bkpGetScaling(&sp->scale);
                        bkpMat4SetTranslate(&S, &sp->translation);
                        if(o < OBJ_COUNT)
                            r.objs[o].model = S;
                        else if(r.gltfModel.meshCount > 0)
                            r.gltfSphere.model = S;
                    }
                    continue;
                }

                if(!sp->active || sp->speed <= 0.0f) continue;

                sp->angle += sp->speed * dt;
                if(sp->angle > 6.28318530f) sp->angle -= 6.28318530f;
                sp->speed -= 0.3f * dt; /* gentle deceleration between clicks */
                if(sp->speed < 0.0f) sp->speed = 0.0f;

                /* Check destruction threshold */
                if(sp->speed >= DESTROY_THRESHOLD)
                {
                    sp->destroyed        = 1;
                    sp->active           = 0;
                    sp->speed            = 0.0f;
                    sp->respawnTimer     = RESPAWN_TIME;
                    sp->needsParticleInit = 1;
                    continue;
                }

                BkpMat4 R  = bkpMat4GetRotation(&sp->axis, sp->angle);
                BkpMat4 S  = bkpGetScaling(&sp->scale);
                BkpMat4 RS = bkpMat4DotMat4(&R, &S);
                bkpMat4SetTranslate(&RS, &sp->translation);
                if(o < OBJ_COUNT)
                    r.objs[o].model = RS;
                else if(r.gltfModel.meshCount > 0)
                    r.gltfSphere.model = RS;
            }
        }

        /* ---- Particle buffer initialization (needs device idle for safety) -- */
        {
            int needsWait = 0;
            for(int o = 0; o < anim.count; ++o)
                if(anim.spins[o].needsParticleInit) { needsWait = 1; break; }
            if(needsWait)
            {
                bkpWaitDevice(r.adp);
                for(int o = 0; o < anim.count; ++o)
                {
                    if(!anim.spins[o].needsParticleInit) continue;
                    anim.spins[o].needsParticleInit = 0;
                    SpinState * sp = &anim.spins[o];
                    initParticles(r.adp, &particleBufs[o],
                                  sp->translation.x, sp->translation.y, sp->translation.z,
                                  sp->color, &anim.seed);
                }
            }
        }

        bkpPrepareFrame(r.adp);
        r.width  = r.adp->frameInfo.winWidth;
        r.height = r.adp->frameInfo.winHeight;
        uint32_t f    = r.adp->frameInfo.currentFrame;
        uint32_t i    = r.adp->frameInfo.imageAcquired;
        VkCommandBuffer vkcmd = r.cmd.cmds[f];

        /* ---- Upload UBOs -------------------------------------------------- */
        {
            float ld[3]  = {1.2f, -0.7f, 0.5f};
            float llen   = sqrtf(ld[0]*ld[0] + ld[1]*ld[1] + ld[2]*ld[2]);
            BkpVec3 up     = bkpVec3(0.0f, 1.0f, 0.0f);
            BkpVec3 eye    = bkpVec3(cam.pos[0],    cam.pos[1],    cam.pos[2]);
            BkpVec3 target = bkpVec3(cam.target[0], cam.target[1], cam.target[2]);
            BkpMat4 proj   = bkpPerspective(45.0f, (float)r.width / (float)r.height, 0.1f, 500.0f);
            proj.mLine1.y *= -1.0f;

            BkpVec3 reflEye    = bkpVec3(cam.pos[0], -cam.pos[1], cam.pos[2]);
            BkpVec3 reflTarget = bkpVec3(cam.target[0], -cam.target[1], cam.target[2]);
            BkpMat4 reflView   = bkpLookAt(reflEye, reflTarget, up);
            BkpMat4 reflVP     = bkpMat4DotMat4(&proj, &reflView);

            SceneUBORefl mainUBO = {0};
            mainUBO.view            = bkpLookAt(eye, target, up);
            mainUBO.proj            = proj;
            mainUBO.lightDir[0]     = ld[0]/llen; mainUBO.lightDir[1] = ld[1]/llen;
            mainUBO.lightDir[2]     = ld[2]/llen; mainUBO.lightDir[3] = 0.0f;
            mainUBO.camPos[0]       = cam.pos[0]; mainUBO.camPos[1] = cam.pos[1];
            mainUBO.camPos[2]       = cam.pos[2]; mainUBO.camPos[3] = 1.0f;
            mainUBO.lightViewProj   = lightViewProj;
            mainUBO.reflectViewProj = reflVP;
            bkpUploadBufferData(r.adp, r.ubo.buffer, &mainUBO,
                                f * r.ubo.size, sizeof(SceneUBORefl));

            SceneUBORefl reflUBO = {0};
            reflUBO.view            = reflView;
            reflUBO.proj            = proj;
            reflUBO.lightDir[0]     = mainUBO.lightDir[0]; reflUBO.lightDir[1] = mainUBO.lightDir[1];
            reflUBO.lightDir[2]     = mainUBO.lightDir[2]; reflUBO.lightDir[3] = 0.0f;
            reflUBO.camPos[0]       = reflEye.x; reflUBO.camPos[1] = reflEye.y;
            reflUBO.camPos[2]       = reflEye.z; reflUBO.camPos[3] = 1.0f;
            reflUBO.lightViewProj   = lightViewProj;
            reflUBO.reflectViewProj = reflVP;
            bkpUploadBufferData(r.adp, r.ubo.buffer, &reflUBO,
                                (FRAMES + f) * r.ubo.size, sizeof(SceneUBORefl));
        }

        bkpBeginCommandBuffer(&r.cmd, f, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        /* ---- Compute: update particle systems for all destroyed objects --------
         * We bind and dispatch once per destroyed object.  bkpCmdBindPipeline and
         * bkpCmdBindDescriptorSets hardcode GRAPHICS, so the raw Vulkan calls with
         * VK_PIPELINE_BIND_POINT_COMPUTE are used here instead. */
        {
            int anyDestroyed = 0;
            for(int o = 0; o < anim.count; ++o)
            {
                if(!anim.spins[o].destroyed) continue;
                anyDestroyed = 1;
                vkCmdBindPipeline(vkcmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                                  computePipeline.pipeline);
                vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                    computePipeline.pipelineLayout.layout, 0, 1,
                    &computeDescSet.descriptorSets[o], 0, NULL);
                ComputePC cpc = {dt, 9.8f, 1.0f / RESPAWN_TIME, 0.0f};
                vkCmdPushConstants(vkcmd, computePipeline.pipelineLayout.layout,
                    VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePC), &cpc);
                /* PARTICLE_COUNT must be a multiple of local_size_x (64), so
                 * the +63 / 64 ceiling always gives exactly PARTICLE_COUNT/64 groups. */
                vkCmdDispatch(vkcmd, (PARTICLE_COUNT + 63) / 64, 1, 1);
            }

            /* One barrier covers all dispatches above.  It stalls the vertex input
             * stage until every compute shader write to the storage buffer is done,
             * so the particle draw below reads coherent data. */
            if(anyDestroyed)
            {
                bkpCmdMemoryBarrier(vkcmd,
                    VK_ACCESS_SHADER_WRITE_BIT,
                    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
            }
        }

        /* ---- Per-frame shadow pass (spinning objects cast dynamic shadows) -- */
        {
            BkpImageBarrierInfo toDepth =
            {
                .image = shadowImg.images[0],
                .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .srcStage  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .srcAccess = VK_ACCESS_2_SHADER_READ_BIT,
                .dstStage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                           | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1,
                .baseLayer = 0, .layerCount = 1
            };
            bkpCmdBarrierImages(vkcmd, &toDepth, 1);

            VkRenderingAttachmentInfo sdAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            sdAtt.imageView   = shadowImg.imageViews[0];
            sdAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            sdAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            sdAtt.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
            sdAtt.clearValue  = (VkClearValue){.depthStencil = {1.0f, 0}};

            VkRenderingInfo sdInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
            sdInfo.renderArea.extent = (VkExtent2D){SHADOW_SIZE, SHADOW_SIZE};
            sdInfo.layerCount        = 1;
            sdInfo.pDepthAttachment  = &sdAtt;
            bkpCmdBeginRendering(vkcmd, &sdInfo);
            VkViewport sdVP = {0.0f, 0.0f, (float)SHADOW_SIZE, (float)SHADOW_SIZE, 0.0f, 1.0f};
            VkRect2D   sdSc = {{0, 0}, {SHADOW_SIZE, SHADOW_SIZE}};
            vkCmdSetViewport(vkcmd, 0, 1, &sdVP);
            vkCmdSetScissor(vkcmd, 0, 1, &sdSc);
            bkpCmdBindPipeline(vkcmd, shadowPipeline.pipeline);
            drawShadow(vkcmd, &shadowPipeline, &ground, &lightViewProj);
            for(int o = 0; o < OBJ_COUNT; ++o)
                if(!anim.spins[o].destroyed)
                    drawShadow(vkcmd, &shadowPipeline, &r.objs[o], &lightViewProj);
            if(r.gltfModel.meshCount > 0 && !anim.spins[OBJ_COUNT].destroyed)
                drawShadow(vkcmd, &shadowPipeline, &r.gltfSphere, &lightViewProj);
            bkpCmdEndRendering(vkcmd);

            BkpImageBarrierInfo toReadOnly =
            {
                .image = shadowImg.images[0],
                .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcStage  = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                .srcAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstStage  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccess = VK_ACCESS_2_SHADER_READ_BIT,
                .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1,
                .baseLayer = 0, .layerCount = 1
            };
            bkpCmdBarrierImages(vkcmd, &toReadOnly, 1);
        }

        /* ---- Reflection pass ---------------------------------------------- */
        {
            BkpImageBarrierInfo barriers[2] =
            {
                {
                    .image = reflectImg.images[0], .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcStage = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                    .dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1,
                    .baseLayer = 0, .layerCount = 1
                },
                {
                    .image = reflDepth.images[0], .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    .srcStage = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                    .dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                    .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                               | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1,
                    .baseLayer = 0, .layerCount = 1
                }
            };
            bkpCmdBarrierImages(vkcmd, barriers, 2);

            VkRenderingAttachmentInfo cAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            cAtt.imageView   = reflectImg.imageViews[0];
            cAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            cAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            cAtt.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
            cAtt.clearValue  = (VkClearValue){.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

            VkRenderingAttachmentInfo dAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            dAtt.imageView   = reflDepth.imageViews[0];
            dAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            dAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            dAtt.storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            dAtt.clearValue  = (VkClearValue){.depthStencil = {1.0f, 0}};

            VkRenderingInfo rInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
            rInfo.renderArea.extent    = (VkExtent2D){W, H}; /* fixed texture size */
            rInfo.layerCount           = 1;
            rInfo.colorAttachmentCount = 1;
            rInfo.pColorAttachments    = &cAtt;
            rInfo.pDepthAttachment     = &dAtt;
            bkpCmdBeginRendering(vkcmd, &rInfo);
            bkpUpdateWindowViewportScissor(vkcmd, W, H); /* fixed texture size */

            bkpCmdBindPipeline(vkcmd, reflectPipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                reflectPipeline.pipelineLayout.layout, 0, 1,
                &reflectDescSet.descriptorSets[f], 0, NULL);
            for(int o = 0; o < OBJ_COUNT; ++o)
                if(!anim.spins[o].destroyed)
                    drawObject(vkcmd, &reflectPipeline, &r.objs[o]);
            if(r.gltfModel.meshCount > 0 && !anim.spins[OBJ_COUNT].destroyed)
                drawObject(vkcmd, &reflectPipeline, &r.gltfSphere);
            bkpCmdEndRendering(vkcmd);

            BkpImageBarrierInfo toReadOnly =
            {
                .image = reflectImg.images[0],
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStage  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccess = VK_ACCESS_2_SHADER_READ_BIT,
                .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1,
                .baseLayer = 0, .layerCount = 1
            };
            bkpCmdBarrierImages(vkcmd, &toReadOnly, 1);
        }

        /* ---- Main pass ------------------------------------------------------ */
        {
            BkpImageBarrierInfo beginBarriers[2] =
            {
                {
                    .image = r.sc.images[i], .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcStage = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                    .dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1,
                    .baseLayer = 0, .layerCount = 1
                },
                {
                    .image = r.depthImg.images[0], .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    .srcStage = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                    .dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                    .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                               | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1,
                    .baseLayer = 0, .layerCount = 1
                }
            };
            bkpCmdBarrierImages(vkcmd, beginBarriers, 2);

            VkRenderingAttachmentInfo cAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            cAtt.imageView   = r.sc.imageViews[i];
            cAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            cAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            cAtt.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
            cAtt.clearValue  = (VkClearValue){.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

            VkRenderingAttachmentInfo dAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            dAtt.imageView   = r.depthImg.imageViews[0];
            dAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            dAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            dAtt.storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            dAtt.clearValue  = (VkClearValue){.depthStencil = {1.0f, 0}};

            VkRenderingInfo renderInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
            renderInfo.renderArea.extent    = (VkExtent2D){(uint32_t)r.width, (uint32_t)r.height};
            renderInfo.layerCount           = 1;
            renderInfo.colorAttachmentCount = 1;
            renderInfo.pColorAttachments    = &cAtt;
            renderInfo.pDepthAttachment     = &dAtt;
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
                    vkCmdBindIndexBuffer(vkcmd, ibuf, voff + r.skyGeo.indicesOffset,
                                         r.skyGeo.indexType);
                    vkCmdDrawIndexed(vkcmd, (uint32_t)r.skyGeo.count, 1, 0, 0, 0);
                }
                else { vkCmdDraw(vkcmd, (uint32_t)r.skyGeo.count, 1, 0, 0); }
            }

            /* Scene objects (skip destroyed ones) */
            bkpCmdBindPipeline(vkcmd, scenePipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                scenePipeline.pipelineLayout.layout, 0, 1,
                &r.descSet.descriptorSets[f], 0, NULL);
            for(int o = 0; o < OBJ_COUNT; ++o)
                if(!anim.spins[o].destroyed)
                    drawObject(vkcmd, &scenePipeline, &r.objs[o]);
            if(r.gltfModel.meshCount > 0 && !anim.spins[OBJ_COUNT].destroyed)
                drawObject(vkcmd, &scenePipeline, &r.gltfSphere);

            /* Floor */
            bkpCmdBindPipeline(vkcmd, floorPipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                floorPipeline.pipelineLayout.layout, 0, 1,
                &floorDescSet.descriptorSets[f], 0, NULL);
            drawObject(vkcmd, &floorPipeline, &ground);

            /* Particles: the same buffer used by the compute pass as a storage
             * buffer is now bound as a vertex buffer — Vulkan allows both usage
             * flags on the same allocation.  The barrier above guarantees the
             * compute writes have landed before vertex input reads them here. */
            bkpCmdBindPipeline(vkcmd, particlePipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                particlePipeline.pipelineLayout.layout, 0, 1,
                &particleRenderDescSet.descriptorSets[f], 0, NULL);
            for(int o = 0; o < anim.count; ++o)
            {
                if(!anim.spins[o].destroyed) continue;
                SpinState * sp = &anim.spins[o];
                ParticlePC ppc = {{sp->color[0], sp->color[1], sp->color[2], 1.0f}};
                vkCmdPushConstants(vkcmd, particlePipeline.pipelineLayout.layout,
                    VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ParticlePC), &ppc);
                VkBuffer pbuf; VkDeviceSize poff;
                bkpGetBuffer(particleBufs[o].buffer, &pbuf);
                bkpGetBufferOffset(particleBufs[o].buffer, &poff);
                vkCmdBindVertexBuffers(vkcmd, 0, 1, &pbuf, &poff);
                vkCmdDraw(vkcmd, PARTICLE_COUNT, 1, 0, 0);
            }

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
                .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1,
                .baseLayer = 0, .layerCount = 1
            };
            bkpCmdBarrierImages(vkcmd, &presentBarrier, 1);
        }

        bkpEndCommandBuffer(&r.cmd, f);
        bkpSubmitFrameBasic(r.adp, &r.cmd);
        bkpPresentFrame(r.adp);
    }

    /* ---- Cleanup ------------------------------------------------------------- */
    bkpWaitDevice(r.adp);
    bkpRemoveMouseAction(eMOUSE_BUTTON_LEFT);
    if(r.gltfModel.meshCount > 0) bkpUnloadModel(r.adp, &r.gltfModel);
    unregisterCameraInput();
    hudCleanup(r.adp, &hud);
    bkpDestroyCommandBuffers(r.adp, &r.cmd);
    for(int o = 0; o < OBJ_COUNT; ++o) bkpFreeBuffer(r.adp, &r.objs[o].geo.buffer);
    for(int o = 0; o < ANIM_MAX_OBJS; ++o) bkpDestroyBuffersGPU(r.adp, &particleBufs[o]);
    bkpFreeBuffer(r.adp, &r.skyGeo.buffer);
    bkpFreeBuffer(r.adp, &r.groundGeo.buffer);
    bkpDestroyImageResource(r.adp, &r.depthImg);
    bkpDestroyImageResource(r.adp, &r.skyTex);
    bkpDestroyImageResource(r.adp, &shadowImg);
    bkpDestroyImageResource(r.adp, &groundTex);
    bkpDestroyImageResource(r.adp, &reflectImg);
    bkpDestroyImageResource(r.adp, &reflDepth);
    bkpDestroySampler(r.adp, &r.skySampler);
    bkpDestroySampler(r.adp, &shadowSampler);
    bkpDestroySampler(r.adp, &groundSampler);
    bkpDestroySampler(r.adp, &reflectSampler);
    bkpDestroyBuffersGPU(r.adp, &r.ubo);
    bkpDestroyDescriptorSet(r.adp, &r.descSet);
    bkpDestroyDescriptorSet(r.adp, &reflectDescSet);
    bkpDestroyDescriptorSet(r.adp, &floorDescSet);
    bkpDestroyDescriptorSet(r.adp, &r.skyDescSet);
    bkpDestroyDescriptorSet(r.adp, &particleRenderDescSet);
    bkpDestroyDescriptorSet(r.adp, &computeDescSet);
    bkpDestroyDescriptorPool(r.adp, &r.descPool);
    bkpDestroyComputePipeline(r.adp, &computePipeline);
    bkpDestroyPipelineLayout(r.adp, &computePipeline.pipelineLayout);
    bkpDestroyGraphicPipeline(r.adp, &particlePipeline);
    bkpDestroyPipelineLayout(r.adp, &particlePipeline.pipelineLayout);
    bkpDestroyGraphicPipeline(r.adp, &shadowPipeline);
    bkpDestroyPipelineLayout(r.adp, &shadowPipeline.pipelineLayout);
    bkpDestroyGraphicPipeline(r.adp, &reflectPipeline);
    bkpDestroyPipelineLayout(r.adp, &reflectPipeline.pipelineLayout);
    bkpDestroyGraphicPipeline(r.adp, &scenePipeline);
    bkpDestroyPipelineLayout(r.adp, &scenePipeline.pipelineLayout);
    bkpDestroyGraphicPipeline(r.adp, &floorPipeline);
    bkpDestroyPipelineLayout(r.adp, &floorPipeline.pipelineLayout);
    bkpDestroyGraphicPipeline(r.adp, &r.skyPipeline);
    bkpDestroyPipelineLayout(r.adp, &r.skyPipeline.pipelineLayout);
    bkpDestroyShader(r.adp, &computeProg);
    bkpDestroyShaderModule(r.adp, &computeMod);
    bkpDestroyShader(r.adp, &particleProg);
    bkpDestroyShaderModule(r.adp, &particleVert);
    bkpDestroyShaderModule(r.adp, &particleFrag);
    bkpDestroyShader(r.adp, &shadowProg);
    bkpDestroyShaderModule(r.adp, &shadowVert);
    bkpDestroyShaderModule(r.adp, &shadowFrag);
    bkpDestroyShader(r.adp, &reflProg);
    bkpDestroyShaderModule(r.adp, &reflVert);
    bkpDestroyShaderModule(r.adp, &reflFrag);
    bkpDestroyShader(r.adp, &sceneProg);
    bkpDestroyShaderModule(r.adp, &sceneVert);
    bkpDestroyShaderModule(r.adp, &sceneFrag);
    bkpDestroyShader(r.adp, &floorProg);
    bkpDestroyShaderModule(r.adp, &floorVert);
    bkpDestroyShaderModule(r.adp, &floorFrag);
    bkpDestroyShader(r.adp, &r.skyProg);
    bkpDestroyShaderModule(r.adp, &r.skyVert);
    bkpDestroyShaderModule(r.adp, &r.skyFrag);
    bkpCleanupSwapChain(r.adp, &r.sc);
    bkpClearGpuAdapter(r.adp);
    bkpQuit(&ctx);
    return 0;
}
