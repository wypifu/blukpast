#include "../common/scene.h"
#include "../common/hud.h"
#include "../common/free_camera.h"

#include <stdlib.h>
#include <string.h>

#define W 1280
#define H  720
#define SHADOW_SIZE 4096

#define SHADER_DIR        "tutorial/08_deformation/shaders"
#define SHADER_DIR_SHADOW "tutorial/05_shadow_map/shaders"
#define SHADER_DIR_SKY    "tutorial/02_camera/shaders"
#define SHADER_DIR_HUD    "tutorial/04_font_input/shaders"
#define FONT_PATH         "tutorial/04_font_input/AdwaitaMono-Regular.ttf"

typedef struct { BkpMat4 lightMVP; } ShadowPC;

/* =========================================================================
 * Spin state (from tuto 07)
 * ========================================================================= */
typedef struct {
    BkpVec3 axis;
    BkpVec3 scale;
    BkpVec3 translation;
    float   speed;
    float   angle;
    float   radius;
    int     active;
} SpinState;

#define ANIM_MAX_OBJS (OBJ_COUNT + 1)

typedef struct {
    SpinState spins[ANIM_MAX_OBJS];
    int       count;
    unsigned  seed;
} AnimState;

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

/* =========================================================================
 * Per-object deformation state
 * ========================================================================= */
#define DEFORM_SPRING_K  80.0f
#define DEFORM_DAMPING   14.0f
#define DEFORM_FALLOFF    0.65f
#define DEFORM_STRENGTH   0.025f
#define DEFORM_MAX_DISP   0.9f

typedef struct {
    BkpVec3    * restPos;
    BkpVec3    * displacement;
    BkpVec3    * velocity;
    BkpVertex  * verts;
    size_t       vertCount;
    BkpBufferGPU dynBuf;
    int          shaking;
} ObjDeformData;

static ObjDeformData g_deform[OBJ_COUNT];
static int           g_activeDeformIdx = -1;
static BkpVec3       g_deformHitLocal;
static int           g_deformDragging  = 0;

/* =========================================================================
 * Input state
 * ========================================================================= */
typedef struct {
    int   pendingPress;
    int   pendingRelease;
    float pressX, pressY;
    float dragDiffX, dragDiffY;
    float totalDragPx;
} DeformInput;

static DeformInput  gDeformInput;
static FreeCamera * g_fc = NULL;

static BkpBool combined_motion_cb(BkpInputState s, void * data)
{
    (void)data;
    if(g_fc && g_fc->midHeld) {
        g_fc->mouseDX += s.diff.x;
        g_fc->mouseDY += s.diff.y;
    }
    gDeformInput.dragDiffX += s.diff.x;
    gDeformInput.dragDiffY += s.diff.y;
    return BKP_TRUE;
}

static BkpBool mouseBtn_cb(BkpInputState s, void * data)
{
    (void)data;
    if(s.action == eINPUT_PRESSED) {
        gDeformInput.pendingPress  = 1;
        gDeformInput.pressX        = s.position.x;
        gDeformInput.pressY        = s.position.y;
        gDeformInput.dragDiffX     = 0.0f;
        gDeformInput.dragDiffY     = 0.0f;
        gDeformInput.totalDragPx   = 0.0f;
    } else if(s.action == eINPUT_RELEASED) {
        gDeformInput.pendingRelease = 1;
    }
    return BKP_TRUE;
}

/* =========================================================================
 * Ray–sphere intersection
 * ========================================================================= */
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

/* =========================================================================
 * Scene UBO (with reflection VP)
 * ========================================================================= */
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

/* Creates a CPU-GPU mapped vertex+index buffer and stores per-object deform data. */
static void initDeformableObject(BkpGpuAdapter adp, SceneObject * obj,
                                  ObjDeformData * od, BkpEGeometryType type,
                                  float x, float y, float z,
                                  float sx, float sy, float sz,
                                  float r, float g, float b,
                                  float roughness, float metallic)
{
    BkpVertexGeometry cpuGeo = {0};
    bkpCreateVertexGeometry(0, type, &cpuGeo); /* group 0 = default 100 MB pool */

    size_t vc = bkpArraySize(cpuGeo.positions);
    size_t ic;
    size_t idxBytes;
    void * idxPtr;
    VkIndexType idxType;
    if(cpuGeo.index32) {
        ic = bkpArraySize(cpuGeo.indices32); idxBytes = ic * sizeof(uint32_t);
        idxPtr = cpuGeo.indices32; idxType = VK_INDEX_TYPE_UINT32;
    } else {
        ic = bkpArraySize(cpuGeo.indices16); idxBytes = ic * sizeof(uint16_t);
        idxPtr = cpuGeo.indices16; idxType = VK_INDEX_TYPE_UINT16;
    }
    size_t vtxBytes = vc * sizeof(BkpVertex);

    od->restPos     = malloc(vc * sizeof(BkpVec3));
    od->displacement= calloc(vc, sizeof(BkpVec3));
    od->velocity    = calloc(vc, sizeof(BkpVec3));
    od->verts       = malloc(vc * sizeof(BkpVertex));
    od->vertCount   = vc;
    od->shaking     = 0;

    for(size_t i = 0; i < vc; ++i) {
        od->restPos[i]        = cpuGeo.positions[i];
        od->verts[i].position = cpuGeo.positions[i];
        od->verts[i].normal   = cpuGeo.normals[i];
        od->verts[i].uv       = cpuGeo.UVs[i];
        od->verts[i].tangent  = cpuGeo.tangents[i];
    }

    od->dynBuf.count = 1;
    od->dynBuf.size  = vtxBytes + idxBytes;
    bkpCreateBuffersGPU(adp, &od->dynBuf,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        eBUFFER_CPU_GPU, BKP_DO_MAP);
    bkpUploadBufferData(adp, od->dynBuf.buffer, od->verts, 0,        vtxBytes);
    bkpUploadBufferData(adp, od->dynBuf.buffer, idxPtr,    vtxBytes, idxBytes);

    obj->geo.buffer        = od->dynBuf.buffer;
    obj->geo.hasIndices    = BKP_TRUE;
    obj->geo.indicesOffset = vtxBytes;
    obj->geo.count         = ic;
    obj->geo.indexType     = idxType;
    obj->model             = makeModel(x, y, z, sx, sy, sz);
    obj->color[0] = r; obj->color[1] = g; obj->color[2] = b; obj->color[3] = 1.0f;
    obj->roughness    = roughness;
    obj->metallic     = metallic;
    obj->useAlbedoMap = 0.0f;

    bkpDestroyVertexGeometry(&cpuGeo);
}

int main(void)
{
    /* 1. Init
     * The default Vulkan context page (SIZE_1_MIB) is too small when the GFX
     * group has to track 7 CPU-GPU mapped vertex buffers simultaneously.
     * Bump to SIZE_4_MIB so the internal allocator doesn't run out of space. */
    BkpContext ctx = {0};
    BkpConfig  cfg = bkpDefaultConfig();
    cfg.contextMemoryInfo.vulkanContextPageSize = SIZE_10_MIB;
    cfg.vulkanContextInfo.winWidth              = W;
    cfg.vulkanContextInfo.winHeight             = H;
    cfg.vulkanContextInfo.maxFrameInFlight      = FRAMES;
    bkpWindowEnableDecoration(1);
    bkpInit(&ctx, &cfg);

    BkpFeatureHint hint = {0};
    hint.requested.shaderClipDistance = VK_TRUE;
    BkpAdapterCriteria criteria = {0, eGPUTYPE_DISCRETE, &hint};

    Renderer r = {0};
    r.width = W; r.height = H;
    bkpActivateGpuAdapter(&ctx.vulkanContext, &r.adp, &criteria);

    /* 2. Swap chain */
    r.sc.info = bkpDefaultSwapChain(1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_TRUE, VK_PRESENT_MODE_FIFO_KHR);
    bkpCreateSwapChain(r.adp, &r.sc, (VkPresentModeKHR[]){VK_PRESENT_MODE_FIFO_KHR}, 1);

    /* 3. Scene UBO */
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

    /* 16. Descriptor pool + sets */
    {
        VkDescriptorPoolSize poolSizes[2] =
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         4 * FRAMES},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8 * FRAMES},
        };
        r.descPool.info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        r.descPool.info.maxSets       = 4 * FRAMES;
        r.descPool.info.poolSizeCount = 2;
        r.descPool.info.pPoolSizes    = poolSizes;
        bkpCreateDescriptorPool(r.adp, &r.descPool);
    }

    r.descSet.descPool   = &r.descPool;
    r.descSet.descLayout = &sceneProg.layouts[0];
    r.descSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &r.descSet);

    BkpDescriptorSet reflectDescSet = {0};
    reflectDescSet.descPool   = &r.descPool;
    reflectDescSet.descLayout = &reflProg.layouts[0];
    reflectDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &reflectDescSet);

    BkpDescriptorSet floorDescSet = {0};
    floorDescSet.descPool   = &r.descPool;
    floorDescSet.descLayout = &floorProg.layouts[0];
    floorDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &floorDescSet);

    r.skyDescSet.descPool   = &r.descPool;
    r.skyDescSet.descLayout = &r.skyProg.layouts[0];
    r.skyDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &r.skyDescSet);

    for(uint32_t f = 0; f < FRAMES; ++f)
    {
        VkDescriptorBufferInfo mainBuf = bkpMakeUboInfo(r.ubo.buffer, f,          r.ubo.size);
        VkDescriptorBufferInfo reflBuf = bkpMakeUboInfo(r.ubo.buffer, FRAMES + f, r.ubo.size);

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

        BkpWriteDescriptorSetInfo wrR = {0};
        wrR.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        wrR.setBindings[0].ubos  = &reflBuf;
        wrR.setBindings[0].count = 1;
        bkpWriteDescriptorSet(r.adp, &reflectDescSet, f, &wrR);
        bkpWriteDescriptorImage(r.adp, reflectDescSet.descriptorSets[f], 1,
                                groundSampler.sampler, groundTex.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        bkpWriteDescriptorImage(r.adp, reflectDescSet.descriptorSets[f], 2,
                                shadowSampler.sampler, shadowImg.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        BkpWriteDescriptorSetInfo wrF = {0};
        wrF.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        wrF.setBindings[0].ubos  = &mainBuf;
        wrF.setBindings[0].count = 1;
        bkpWriteDescriptorSet(r.adp, &floorDescSet, f, &wrF);
        bkpWriteDescriptorImage(r.adp, floorDescSet.descriptorSets[f], 1,
                                groundSampler.sampler, groundTex.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        bkpWriteDescriptorImage(r.adp, floorDescSet.descriptorSets[f], 2,
                                shadowSampler.sampler, shadowImg.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        bkpWriteDescriptorImage(r.adp, floorDescSet.descriptorSets[f], 3,
                                reflectSampler.sampler, reflectImg.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        BkpWriteDescriptorSetInfo wrS = {0};
        wrS.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        wrS.setBindings[0].ubos  = &mainBuf;
        wrS.setBindings[0].count = 1;
        bkpWriteDescriptorSet(r.adp, &r.skyDescSet, f, &wrS);
        bkpWriteDescriptorImage(r.adp, r.skyDescSet.descriptorSets[f], 1,
                                r.skySampler.sampler, r.skyTex.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    /* 17. Static geometry */
    bkpSetDefaultSegmentForCircularGeometries(32);
    bkpCreateVertexIndexBuffer(r.adp, eBKP_GEO_SPHERE, &r.skyGeo);
    bkpCreateVertexIndexBuffer(r.adp, eBKP_GEO_PLANE,  &r.groundGeo);

    /* 17a. Scene objects — all use CPU-GPU buffers so vertices can be deformed.
     * bkpCreateVertexGeometry uses the default memory group (id=0, 100 MB pool).
     * Using non-zero ids would reference uninitialized allocator groups and crash. */
    initDeformableObject(r.adp, &r.objs[0], &g_deform[0], eBKP_GEO_SPHERE,
                         -5.0f, 1.5f,  0.0f, 1.0f,1.0f,1.0f, 0.9f,0.2f,0.2f, 0.3f,0.0f);
    initDeformableObject(r.adp, &r.objs[1], &g_deform[1], eBKP_GEO_CUBE,
                          0.0f, 1.5f,  0.0f, 1.0f,1.0f,1.0f, 0.2f,0.8f,0.3f, 0.5f,0.0f);
    initDeformableObject(r.adp, &r.objs[2], &g_deform[2], eBKP_GEO_CYLINDER,
                          5.0f, 1.5f,  0.0f, 1.0f,1.0f,1.0f, 0.3f,0.5f,0.9f, 0.2f,0.8f);
    initDeformableObject(r.adp, &r.objs[3], &g_deform[3], eBKP_GEO_CONE,
                         -2.5f, 1.5f, -5.0f, 1.0f,1.0f,1.0f, 0.9f,0.7f,0.1f, 0.6f,0.0f);
    initDeformableObject(r.adp, &r.objs[4], &g_deform[4], eBKP_GEO_TORUS,
                          2.5f, 1.3f, -5.0f, 1.5f,1.5f,1.5f, 0.8f,0.3f,0.8f, 0.4f,0.5f);
    initDeformableObject(r.adp, &r.objs[5], &g_deform[5], eBKP_GEO_TUBE,
                         -5.0f, 1.5f, -5.0f, 1.0f,1.0f,1.0f, 0.2f,0.9f,0.8f, 0.7f,0.1f);
    initDeformableObject(r.adp, &r.objs[6], &g_deform[6], eBKP_GEO_DISK_PLANE,
                          5.0f, 1.5f, -5.0f, 1.5f,1.5f,1.5f, 0.9f,0.9f,0.9f, 0.1f,0.9f);

    /* GLTF sphere — spin only, no deform (GPU-only buffer) */
    initGltfSphere(&r, -5.0f, 1.5f, 2.0f, 0.5f);

    SceneObject ground = {0};
    ground.geo          = r.groundGeo;
    ground.model        = makeGroundModel(0.0f, 0.0f, 0.0f, 30.0f, 30.0f);
    ground.color[0]     = 1.0f; ground.color[1] = 1.0f;
    ground.color[2]     = 1.0f; ground.color[3] = 1.0f;
    ground.roughness    = 0.03f;
    ground.metallic     = 0.95f;
    ground.useAlbedoMap = 1.0f;

    /* 17b. Spin states */
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
    }

    /* 18. Command buffers */
    initCommandBuffers(&r);

    /* 19. Initial shadow bake */
    BkpMat4 lightViewProj = computeLightViewProj();
    {
        BkpFence bakeFence = {0};
        bkpCreateFence(r.adp, &bakeFence, BKP_FALSE);
        bkpBeginCommandBuffer(&r.cmd, 0, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VkCommandBuffer bcmd = r.cmd.cmds[0];

        BkpImageBarrierInfo toDepth = {
            .image = shadowImg.images[0],
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .srcStage = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
            .dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
        };
        bkpCmdBarrierImages(bcmd, &toDepth, 1);

        VkRenderingAttachmentInfo sdAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        sdAtt.imageView = shadowImg.imageViews[0]; sdAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        sdAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; sdAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        sdAtt.clearValue = (VkClearValue){.depthStencil = {1.0f, 0}};

        VkRenderingInfo sdInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
        sdInfo.renderArea.extent = (VkExtent2D){SHADOW_SIZE, SHADOW_SIZE};
        sdInfo.layerCount = 1; sdInfo.pDepthAttachment = &sdAtt;
        bkpCmdBeginRendering(bcmd, &sdInfo);

        VkViewport sdVP = {0.0f, 0.0f, (float)SHADOW_SIZE, (float)SHADOW_SIZE, 0.0f, 1.0f};
        VkRect2D   sdSc = {{0, 0}, {SHADOW_SIZE, SHADOW_SIZE}};
        vkCmdSetViewport(bcmd, 0, 1, &sdVP);
        vkCmdSetScissor(bcmd, 0, 1, &sdSc);
        bkpCmdBindPipeline(bcmd, shadowPipeline.pipeline);
        drawShadow(bcmd, &shadowPipeline, &ground, &lightViewProj);
        for(int o = 0; o < OBJ_COUNT; ++o) drawShadow(bcmd, &shadowPipeline, &r.objs[o], &lightViewProj);
        if(r.gltfModel.meshCount > 0) drawShadow(bcmd, &shadowPipeline, &r.gltfSphere, &lightViewProj);
        bkpCmdEndRendering(bcmd);

        BkpImageBarrierInfo toRO = {
            .image = shadowImg.images[0],
            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcStage = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, .srcAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, .dstAccess = VK_ACCESS_2_SHADER_READ_BIT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
        };
        bkpCmdBarrierImages(bcmd, &toRO, 1);
        bkpEndCommandBuffer(&r.cmd, 0);
        BkpCommandBuffer * bakeCmd = &r.cmd;
        bkpSubmit(r.adp, eQFAMILY_GRAPHIC, &bakeCmd, 1, 0, NULL, &bakeFence);
        bkpWaitForFence(r.adp, &bakeFence, UINT64_MAX);
        bkpDestroyFence(r.adp, &bakeFence);
    }

    /* 20. HUD */
    Hud hud = {0};
    hudInit(r.adp, &hud, colorFmt, depthFmt, W, H, SHADER_DIR_HUD, FONT_PATH);
    bkpSetText(hud.helpText,
               "WASD EQ movement\nright-click drag to rotate\nr reset camera\n"
               "left-click object to spin / fast-click to accelerate\n"
               "left-click drag any object to deform / release to shake");

    /* 21. Camera + input */
    FreeCamera fc = makeFreeCamera(0.0f, 8.0f, 16.0f, 0.0f, -0.45f);
    Camera     cam = {0};
    registerCameraInput(&fc);
    g_fc = &fc;
    bkpAddMouseAction(eMOUSE_MOTION,      combined_motion_cb, NULL);
    bkpAddMouseAction(eMOUSE_BUTTON_LEFT, mouseBtn_cb,        NULL);
    uint64_t prevTime = bkp_time_getClockMicro();

    /* 22. Main loop */
    while(!bkpWindowIsClosed(ctx.vulkanContext.win))
    {
        bkpPollEvent(ctx.vulkanContext.win);

        uint64_t now = bkp_time_getClockMicro();
        float dt = (float)(now - prevTime) * 1e-6f;
        prevTime = now;

        updateFreeCamera(&fc, dt);
        cameraFromFree(&fc, &cam);
        hudUpdate(&hud, dt);

        /* Consume drag delta */
        float motionDX = gDeformInput.dragDiffX; gDeformInput.dragDiffX = 0.0f;
        float motionDY = gDeformInput.dragDiffY; gDeformInput.dragDiffY = 0.0f;
        gDeformInput.totalDragPx += fabsf(motionDX) + fabsf(motionDY);

        /* Ray helpers */
        BkpVec3 eye    = bkpVec3(cam.pos[0],    cam.pos[1],    cam.pos[2]);
        BkpVec3 target = bkpVec3(cam.target[0], cam.target[1], cam.target[2]);
        BkpVec3 worldUp= bkpVec3(0.0f, 1.0f, 0.0f);
        BkpMat4 view   = bkpLookAt(eye, target, worldUp);
        BkpMat4 proj   = bkpPerspective(45.0f, (float)r.width / (float)r.height, 0.1f, 500.0f);

        /* Left-click press: ray pick the closest object */
        if(gDeformInput.pendingPress)
        {
            gDeformInput.pendingPress = 0;

            float ndcX = (gDeformInput.pressX / (float)r.width)  * 2.0f - 1.0f;
            float ndcY = (gDeformInput.pressY / (float)r.height) * 2.0f - 1.0f;

            BkpMat4 projFlipped = proj; projFlipped.mLine1.y *= -1.0f;
            BkpMat4 vp    = bkpMat4DotMat4(&projFlipped, &view);
            BkpMat4 invVP = bkpMat4Inverse(&vp);

            BkpVec4 clipNear = bkpVec4(ndcX, ndcY, 0.0f, 1.0f);
            BkpVec4 clipFar  = bkpVec4(ndcX, ndcY, 1.0f, 1.0f);
            BkpVec4 wn = bkpMat4DotVec4(&invVP, &clipNear);
            BkpVec4 wf = bkpMat4DotVec4(&invVP, &clipFar);
            BkpVec3 near3 = bkpVec3(wn.x/wn.w, wn.y/wn.w, wn.z/wn.w);
            BkpVec3 far3  = bkpVec3(wf.x/wf.w, wf.y/wf.w, wf.z/wf.w);
            BkpVec3 dir   = bkpNormalized3D(bkpVec3Substract(&far3, &near3));

            float bestT  = 1e30f;
            int   hitIdx = -1;
            for(int o = 0; o < anim.count; ++o)
            {
                float t;
                if(raySphere(near3, dir, anim.spins[o].translation,
                             anim.spins[o].radius, &t) && t < bestT)
                {
                    bestT  = t;
                    hitIdx = o;
                }
            }

            if(hitIdx >= 0)
            {
                g_activeDeformIdx = hitIdx;
                g_deformDragging  = 1;

                /* Compute hit point in object local space */
                BkpMat4 * mdl = (hitIdx < OBJ_COUNT) ? &r.objs[hitIdx].model
                                                      : &r.gltfSphere.model;
                BkpVec3 worldHit = bkpVec3(near3.x + dir.x*bestT,
                                            near3.y + dir.y*bestT,
                                            near3.z + dir.z*bestT);
                BkpMat4 invModel = bkpMat4Inverse(mdl);
                BkpVec4 h  = bkpVec4(worldHit.x, worldHit.y, worldHit.z, 1.0f);
                BkpVec4 hl = bkpMat4DotVec4(&invModel, &h);
                g_deformHitLocal = bkpVec3(hl.x/hl.w, hl.y/hl.w, hl.z/hl.w);

                /* Reset deform state for this object (deformable objects only) */
                if(hitIdx < OBJ_COUNT) {
                    g_deform[hitIdx].shaking = 0;
                    memset(g_deform[hitIdx].displacement, 0,
                           g_deform[hitIdx].vertCount * sizeof(BkpVec3));
                    memset(g_deform[hitIdx].velocity, 0,
                           g_deform[hitIdx].vertCount * sizeof(BkpVec3));
                }
            }
        }

        /* Left-click release: spin (tap) or shake (drag) */
        if(gDeformInput.pendingRelease)
        {
            gDeformInput.pendingRelease = 0;
            int idx = g_activeDeformIdx;

            if(idx >= 0)
            {
                if(gDeformInput.totalDragPx > 8.0f && idx < OBJ_COUNT)
                {
                    /* Drag → spring-damper shake */
                    g_deform[idx].shaking = 1;
                    memset(g_deform[idx].velocity, 0,
                           g_deform[idx].vertCount * sizeof(BkpVec3));
                }
                else
                {
                    /* Tap → spin (or GLTF sphere tap) */
                    SpinState * sp = &anim.spins[idx];
                    if(!sp->active) {
                        sp->axis   = randomAxis(&anim.seed);
                        sp->angle  = 0.0f;
                        sp->active = 1;
                    }
                    sp->speed += 1.5f;
                    if(sp->speed > 12.0f) sp->speed = 12.0f;

                    /* Reset any deformation from the tap */
                    if(idx < OBJ_COUNT) {
                        memset(g_deform[idx].displacement, 0,
                               g_deform[idx].vertCount * sizeof(BkpVec3));
                        memset(g_deform[idx].velocity, 0,
                               g_deform[idx].vertCount * sizeof(BkpVec3));
                    }
                }
            }
            g_deformDragging  = 0;
            g_activeDeformIdx = -1;
        }

        /* Spin update */
        for(int o = 0; o < OBJ_COUNT; ++o)
        {
            SpinState * sp = &anim.spins[o];
            if(!sp->active || sp->speed <= 0.0f) continue;
            sp->angle += sp->speed * dt;
            if(sp->angle > 6.28318530f) sp->angle -= 6.28318530f;
            sp->speed -= 0.5f * dt;
            if(sp->speed < 0.0f) sp->speed = 0.0f;
            BkpMat4 R  = bkpMat4GetRotation(&sp->axis, sp->angle);
            BkpMat4 S  = bkpGetScaling(&sp->scale);
            BkpMat4 RS = bkpMat4DotMat4(&R, &S);
            bkpMat4SetTranslate(&RS, &sp->translation);
            r.objs[o].model = RS;
        }
        if(r.gltfModel.meshCount > 0)
        {
            SpinState * sp = &anim.spins[OBJ_COUNT];
            if(sp->active && sp->speed > 0.0f) {
                sp->angle += sp->speed * dt;
                if(sp->angle > 6.28318530f) sp->angle -= 6.28318530f;
                sp->speed -= 0.5f * dt;
                if(sp->speed < 0.0f) sp->speed = 0.0f;
                BkpMat4 R  = bkpMat4GetRotation(&sp->axis, sp->angle);
                BkpMat4 S  = bkpGetScaling(&sp->scale);
                BkpMat4 RS = bkpMat4DotMat4(&R, &S);
                bkpMat4SetTranslate(&RS, &sp->translation);
                r.gltfSphere.model = RS;
            }
        }

        /* Deformation drag: Gaussian displacement in local space */
        if(g_deformDragging && g_activeDeformIdx >= 0 && g_activeDeformIdx < OBJ_COUNT
           && (motionDX != 0.0f || motionDY != 0.0f))
        {
            int idx = g_activeDeformIdx;
            BkpVec3 fwd   = bkpNormalized3D(bkpVec3Substract(&target, &eye));
            BkpVec3 right = bkpNormalized3D(bkpCross3D(&fwd, &worldUp));
            BkpVec3 up    = bkpNormalized3D(bkpCross3D(&right, &fwd));

            float s = DEFORM_STRENGTH;
            BkpVec3 worldDrag = bkpVec3(right.x*motionDX*s + up.x*(-motionDY)*s,
                                         right.y*motionDX*s + up.y*(-motionDY)*s,
                                         right.z*motionDX*s + up.z*(-motionDY)*s);

            BkpMat4 invModel = bkpMat4Inverse(&r.objs[idx].model);
            BkpVec4 wd = bkpVec4(worldDrag.x, worldDrag.y, worldDrag.z, 0.0f);
            BkpVec4 ld = bkpMat4DotVec4(&invModel, &wd);
            BkpVec3 localDrag = bkpVec3(ld.x, ld.y, ld.z);

            float sig2 = DEFORM_FALLOFF * DEFORM_FALLOFF;
            float maxD2= DEFORM_MAX_DISP * DEFORM_MAX_DISP;
            ObjDeformData * od = &g_deform[idx];
            for(size_t i = 0; i < od->vertCount; ++i)
            {
                BkpVec3 d = bkpVec3Substract(&od->restPos[i], &g_deformHitLocal);
                float dist2 = bkpDot3D(&d, &d);
                float w = expf(-dist2 / sig2);
                od->displacement[i].x += localDrag.x * w;
                od->displacement[i].y += localDrag.y * w;
                od->displacement[i].z += localDrag.z * w;
                float mag2 = od->displacement[i].x * od->displacement[i].x
                           + od->displacement[i].y * od->displacement[i].y
                           + od->displacement[i].z * od->displacement[i].z;
                if(mag2 > maxD2) {
                    float inv = DEFORM_MAX_DISP / sqrtf(mag2);
                    od->displacement[i].x *= inv;
                    od->displacement[i].y *= inv;
                    od->displacement[i].z *= inv;
                }
            }
        }

        /* Spring-damper shake for all objects */
        for(int o = 0; o < OBJ_COUNT; ++o)
        {
            ObjDeformData * od = &g_deform[o];
            if(!od->shaking) continue;
            int anySignificant = 0;
            for(size_t i = 0; i < od->vertCount; ++i)
            {
                BkpVec3 * disp = &od->displacement[i];
                BkpVec3 * vel  = &od->velocity[i];
                vel->x += (-DEFORM_SPRING_K * disp->x - DEFORM_DAMPING * vel->x) * dt;
                vel->y += (-DEFORM_SPRING_K * disp->y - DEFORM_DAMPING * vel->y) * dt;
                vel->z += (-DEFORM_SPRING_K * disp->z - DEFORM_DAMPING * vel->z) * dt;
                disp->x += vel->x * dt;
                disp->y += vel->y * dt;
                disp->z += vel->z * dt;
                float mag2 = disp->x*disp->x + disp->y*disp->y + disp->z*disp->z;
                if(mag2 > 0.00001f) anySignificant = 1;
            }
            if(!anySignificant) {
                od->shaking = 0;
                memset(od->displacement, 0, od->vertCount * sizeof(BkpVec3));
                memset(od->velocity,     0, od->vertCount * sizeof(BkpVec3));
            }
        }

        /* Rebuild and upload vertices for objects that are deforming or shaking */
        for(int o = 0; o < OBJ_COUNT; ++o)
        {
            int isDragging = (g_deformDragging && g_activeDeformIdx == o);
            int isShaking  = g_deform[o].shaking;
            if(!isDragging && !isShaking) continue;

            ObjDeformData * od = &g_deform[o];
            for(size_t i = 0; i < od->vertCount; ++i)
            {
                BkpVec3 p = bkpVec3(od->restPos[i].x + od->displacement[i].x,
                                     od->restPos[i].y + od->displacement[i].y,
                                     od->restPos[i].z + od->displacement[i].z);
                od->verts[i].position = p;
                od->verts[i].normal   = bkpNormalized3D(p);
            }
            bkpUploadBufferData(r.adp, od->dynBuf.buffer,
                                od->verts, 0, od->vertCount * sizeof(BkpVertex));
        }

        bkpPrepareFrame(r.adp);
        r.width  = r.adp->frameInfo.winWidth;
        r.height = r.adp->frameInfo.winHeight;
        uint32_t f    = r.adp->frameInfo.currentFrame;
        uint32_t i    = r.adp->frameInfo.imageAcquired;
        VkCommandBuffer vkcmd = r.cmd.cmds[f];

        /* Upload UBOs */
        {
            float ld[3] = {1.2f, -0.7f, 0.5f};
            float len   = sqrtf(ld[0]*ld[0] + ld[1]*ld[1] + ld[2]*ld[2]);
            BkpVec3 up      = bkpVec3(0.0f, 1.0f, 0.0f);
            BkpVec3 eye2    = bkpVec3(cam.pos[0],    cam.pos[1],    cam.pos[2]);
            BkpVec3 target2 = bkpVec3(cam.target[0], cam.target[1], cam.target[2]);
            BkpMat4 proj2   = bkpPerspective(45.0f, (float)r.width / (float)r.height, 0.1f, 500.0f);
            proj2.mLine1.y *= -1.0f;
            BkpVec3 reflEye    = bkpVec3(cam.pos[0],    -cam.pos[1],    cam.pos[2]);
            BkpVec3 reflTarget = bkpVec3(cam.target[0], -cam.target[1], cam.target[2]);
            BkpMat4 reflView   = bkpLookAt(reflEye, reflTarget, up);
            BkpMat4 reflVP     = bkpMat4DotMat4(&proj2, &reflView);

            SceneUBORefl mainUBO = {0};
            mainUBO.view = bkpLookAt(eye2, target2, up);
            mainUBO.proj = proj2;
            mainUBO.lightDir[0] = ld[0]/len; mainUBO.lightDir[1] = ld[1]/len;
            mainUBO.lightDir[2] = ld[2]/len; mainUBO.lightDir[3] = 0.0f;
            mainUBO.camPos[0] = cam.pos[0]; mainUBO.camPos[1] = cam.pos[1];
            mainUBO.camPos[2] = cam.pos[2]; mainUBO.camPos[3] = 1.0f;
            mainUBO.lightViewProj   = lightViewProj;
            mainUBO.reflectViewProj = reflVP;
            bkpUploadBufferData(r.adp, r.ubo.buffer, &mainUBO,
                                f * r.ubo.size, sizeof(SceneUBORefl));

            SceneUBORefl reflUBO = {0};
            reflUBO.view = reflView; reflUBO.proj = proj2;
            reflUBO.lightDir[0] = mainUBO.lightDir[0]; reflUBO.lightDir[1] = mainUBO.lightDir[1];
            reflUBO.lightDir[2] = mainUBO.lightDir[2]; reflUBO.lightDir[3] = 0.0f;
            reflUBO.camPos[0] = reflEye.x; reflUBO.camPos[1] = reflEye.y;
            reflUBO.camPos[2] = reflEye.z; reflUBO.camPos[3] = 1.0f;
            reflUBO.lightViewProj   = lightViewProj;
            reflUBO.reflectViewProj = reflVP;
            bkpUploadBufferData(r.adp, r.ubo.buffer, &reflUBO,
                                (FRAMES + f) * r.ubo.size, sizeof(SceneUBORefl));
        }

        bkpBeginCommandBuffer(&r.cmd, f, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        /* Shadow pass */
        {
            BkpImageBarrierInfo toDepth = {
                .image = shadowImg.images[0],
                .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .srcStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, .srcAccess = VK_ACCESS_2_SHADER_READ_BIT,
                .dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
            };
            bkpCmdBarrierImages(vkcmd, &toDepth, 1);

            VkRenderingAttachmentInfo sdAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            sdAtt.imageView = shadowImg.imageViews[0]; sdAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            sdAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; sdAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            sdAtt.clearValue = (VkClearValue){.depthStencil = {1.0f, 0}};

            VkRenderingInfo sdInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
            sdInfo.renderArea.extent = (VkExtent2D){SHADOW_SIZE, SHADOW_SIZE};
            sdInfo.layerCount = 1; sdInfo.pDepthAttachment = &sdAtt;
            bkpCmdBeginRendering(vkcmd, &sdInfo);

            VkViewport sdVP = {0.0f, 0.0f, (float)SHADOW_SIZE, (float)SHADOW_SIZE, 0.0f, 1.0f};
            VkRect2D   sdSc = {{0, 0}, {SHADOW_SIZE, SHADOW_SIZE}};
            vkCmdSetViewport(vkcmd, 0, 1, &sdVP); vkCmdSetScissor(vkcmd, 0, 1, &sdSc);
            bkpCmdBindPipeline(vkcmd, shadowPipeline.pipeline);
            drawShadow(vkcmd, &shadowPipeline, &ground, &lightViewProj);
            for(int o = 0; o < OBJ_COUNT; ++o) drawShadow(vkcmd, &shadowPipeline, &r.objs[o], &lightViewProj);
            if(r.gltfModel.meshCount > 0) drawShadow(vkcmd, &shadowPipeline, &r.gltfSphere, &lightViewProj);
            bkpCmdEndRendering(vkcmd);

            BkpImageBarrierInfo toRO = {
                .image = shadowImg.images[0],
                .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcStage = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, .srcAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, .dstAccess = VK_ACCESS_2_SHADER_READ_BIT,
                .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
            };
            bkpCmdBarrierImages(vkcmd, &toRO, 1);
        }

        /* Reflection pass */
        {
            BkpImageBarrierInfo barriers[2] = {
                {.image = reflectImg.images[0],
                 .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                 .srcStage = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                 .dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                 .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1},
                {.image = reflDepth.images[0],
                 .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                 .srcStage = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                 .dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                 .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                 .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1}
            };
            bkpCmdBarrierImages(vkcmd, barriers, 2);

            VkRenderingAttachmentInfo cAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            cAtt.imageView = reflectImg.imageViews[0]; cAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            cAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; cAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            cAtt.clearValue = (VkClearValue){.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

            VkRenderingAttachmentInfo dAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            dAtt.imageView = reflDepth.imageViews[0]; dAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            dAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; dAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            dAtt.clearValue = (VkClearValue){.depthStencil = {1.0f, 0}};

            VkRenderingInfo rInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
            rInfo.renderArea.extent = (VkExtent2D){W, H}; /* fixed texture size */ rInfo.layerCount = 1;
            rInfo.colorAttachmentCount = 1; rInfo.pColorAttachments = &cAtt; rInfo.pDepthAttachment = &dAtt;
            bkpCmdBeginRendering(vkcmd, &rInfo);
            bkpUpdateWindowViewportScissor(vkcmd, W, H); /* fixed texture size */
            bkpCmdBindPipeline(vkcmd, reflectPipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                reflectPipeline.pipelineLayout.layout, 0, 1, &reflectDescSet.descriptorSets[f], 0, NULL);
            for(int o = 0; o < OBJ_COUNT; ++o) drawObject(vkcmd, &reflectPipeline, &r.objs[o]);
            if(r.gltfModel.meshCount > 0) drawObject(vkcmd, &reflectPipeline, &r.gltfSphere);
            bkpCmdEndRendering(vkcmd);

            BkpImageBarrierInfo toRO = {
                .image = reflectImg.images[0],
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, .dstAccess = VK_ACCESS_2_SHADER_READ_BIT,
                .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
            };
            bkpCmdBarrierImages(vkcmd, &toRO, 1);
        }

        /* Main pass */
        {
            BkpImageBarrierInfo beginBarriers[2] = {
                {.image = r.sc.images[i],
                 .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                 .srcStage = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                 .dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                 .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1},
                {.image = r.depthImg.images[0],
                 .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                 .srcStage = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                 .dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                 .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                 .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1}
            };
            bkpCmdBarrierImages(vkcmd, beginBarriers, 2);

            VkRenderingAttachmentInfo cAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            cAtt.imageView = r.sc.imageViews[i]; cAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            cAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; cAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            cAtt.clearValue = (VkClearValue){.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

            VkRenderingAttachmentInfo dAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            dAtt.imageView = r.depthImg.imageViews[0]; dAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            dAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; dAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            dAtt.clearValue = (VkClearValue){.depthStencil = {1.0f, 0}};

            VkRenderingInfo rInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
            rInfo.renderArea.offset = (VkOffset2D){0, 0}; rInfo.renderArea.extent = (VkExtent2D){(uint32_t)r.width, (uint32_t)r.height};
            rInfo.layerCount = 1; rInfo.colorAttachmentCount = 1;
            rInfo.pColorAttachments = &cAtt; rInfo.pDepthAttachment = &dAtt;
            bkpCmdBeginRendering(vkcmd, &rInfo);
            bkpUpdateWindowViewportScissor(vkcmd, (float)r.width, (float)r.height);

            bkpCmdBindPipeline(vkcmd, r.skyPipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                r.skyPipeline.pipelineLayout.layout, 0, 1, &r.skyDescSet.descriptorSets[f], 0, NULL);
            {
                VkBuffer vbuf; VkDeviceSize voff;
                bkpGetBuffer(r.skyGeo.buffer, &vbuf); bkpGetBufferOffset(r.skyGeo.buffer, &voff);
                vkCmdBindVertexBuffers(vkcmd, 0, 1, &vbuf, &voff);
                if(r.skyGeo.hasIndices) {
                    VkBuffer ibuf; bkpGetBuffer(r.skyGeo.buffer, &ibuf);
                    vkCmdBindIndexBuffer(vkcmd, ibuf, voff + r.skyGeo.indicesOffset, r.skyGeo.indexType);
                    vkCmdDrawIndexed(vkcmd, (uint32_t)r.skyGeo.count, 1, 0, 0, 0);
                } else { vkCmdDraw(vkcmd, (uint32_t)r.skyGeo.count, 1, 0, 0); }
            }

            bkpCmdBindPipeline(vkcmd, scenePipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                scenePipeline.pipelineLayout.layout, 0, 1, &r.descSet.descriptorSets[f], 0, NULL);
            for(int o = 0; o < OBJ_COUNT; ++o) drawObject(vkcmd, &scenePipeline, &r.objs[o]);
            if(r.gltfModel.meshCount > 0) drawObject(vkcmd, &scenePipeline, &r.gltfSphere);

            bkpCmdBindPipeline(vkcmd, floorPipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                floorPipeline.pipelineLayout.layout, 0, 1, &floorDescSet.descriptorSets[f], 0, NULL);
            drawObject(vkcmd, &floorPipeline, &ground);

            hudDraw(r.adp, vkcmd, &hud, f);
            bkpCmdEndRendering(vkcmd);

            BkpImageBarrierInfo presentBarrier = {
                .image = r.sc.images[i],
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStage = VK_PIPELINE_STAGE_2_NONE, .dstAccess = VK_ACCESS_2_NONE,
                .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
            };
            bkpCmdBarrierImages(vkcmd, &presentBarrier, 1);
        }

        bkpEndCommandBuffer(&r.cmd, f);
        bkpSubmitFrameBasic(r.adp, &r.cmd);
        bkpPresentFrame(r.adp);
    }

    /* 23. Cleanup */
    bkpWaitDevice(r.adp);
    g_fc = NULL;
    bkpRemoveMouseAction(eMOUSE_BUTTON_LEFT);
    bkpRemoveMouseAction(eMOUSE_MOTION);
    for(int o = 0; o < OBJ_COUNT; ++o) {
        free(g_deform[o].restPos);
        free(g_deform[o].displacement);
        free(g_deform[o].velocity);
        free(g_deform[o].verts);
        bkpDestroyBuffersGPU(r.adp, &g_deform[o].dynBuf);
    }
    if(r.gltfModel.meshCount > 0) bkpUnloadModel(r.adp, &r.gltfModel);
    unregisterCameraInput();
    hudCleanup(r.adp, &hud);
    bkpDestroyCommandBuffers(r.adp, &r.cmd);
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
    bkpDestroyDescriptorPool(r.adp, &r.descPool);
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
    bkpDestroyShader(r.adp, &shadowProg);
    bkpDestroyShaderModule(r.adp, &shadowVert); bkpDestroyShaderModule(r.adp, &shadowFrag);
    bkpDestroyShader(r.adp, &reflProg);
    bkpDestroyShaderModule(r.adp, &reflVert); bkpDestroyShaderModule(r.adp, &reflFrag);
    bkpDestroyShader(r.adp, &sceneProg);
    bkpDestroyShaderModule(r.adp, &sceneVert); bkpDestroyShaderModule(r.adp, &sceneFrag);
    bkpDestroyShader(r.adp, &floorProg);
    bkpDestroyShaderModule(r.adp, &floorVert); bkpDestroyShaderModule(r.adp, &floorFrag);
    bkpDestroyShader(r.adp, &r.skyProg);
    bkpDestroyShaderModule(r.adp, &r.skyVert); bkpDestroyShaderModule(r.adp, &r.skyFrag);
    bkpCleanupSwapChain(r.adp, &r.sc);
    bkpClearGpuAdapter(r.adp);
    bkpQuit(&ctx);
    return 0;
}
