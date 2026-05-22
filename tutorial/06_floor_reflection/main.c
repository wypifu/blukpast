#include "../common/scene.h"
#include "../common/hud.h"
#include "../common/free_camera.h"

#define W 1280
#define H  720
#define SHADOW_SIZE 4096

#define SHADER_DIR        "tutorial/06_floor_reflection/shaders"
#define SHADER_DIR_SHADOW "tutorial/05_shadow_map/shaders"
#define SHADER_DIR_SKY    "tutorial/02_camera/shaders"
#define SHADER_DIR_HUD    "tutorial/04_font_input/shaders"
#define FONT_PATH         "tutorial/04_font_input/AdwaitaMono-Regular.ttf"

typedef struct { BkpMat4 lightMVP; } ShadowPC;

/* Extends SceneUBOShadow with the reflected camera VP used by floor.frag */
typedef struct
{
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

    /* gl_ClipDistance requires shaderClipDistance feature */
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

    /* 3. Scene UBO: 2×FRAMES slots
     *   [0..FRAMES-1]        → main camera
     *   [FRAMES..2*FRAMES-1] → reflect camera  */
    r.ubo.count = FRAMES * 2;
    r.ubo.size  = sizeof(SceneUBORefl);   /* larger than SceneUBOShadow to fit reflectViewProj */
    bkpCreateBuffersGPU(r.adp, &r.ubo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        eBUFFER_CPU_GPU, BKP_DO_MAP);

    /* 4. Depth buffer + sky texture */
    initDepth(&r);
    initSkyTexture(&r);

    /* 5. Shadow map — baked once at startup, light is fixed */
    BkpImageResource shadowImg    = {0};
    BkpSampler       shadowSampler = {0};
    initShadowResources(r.adp, &shadowImg, &shadowSampler);

    /* 6. Ground texture */
    BkpImageResource groundTex    = {0};
    BkpSampler       groundSampler = {0};
    initGroundTexture(r.adp, &groundTex, &groundSampler);

    VkFormat colorFmt = r.sc.info.imageFormat;
    VkFormat depthFmt = r.depthImg.imageInfo.format;

    /* 7. Reflection color buffer (offscreen render target) */
    BkpImageResource reflectImg = bkpDefaultColorBuffer();
    reflectImg.imageInfo.extent = (VkExtent3D){W, H, 1};
    reflectImg.imageInfo.format = colorFmt;
    reflectImg.viewInfo.format  = colorFmt;
    bkpCreateImageResources(r.adp, &reflectImg);

    /* 8. Reflect depth buffer */
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

    /* 11. Shadow pipeline (depth-only, reuses tuto 05 shadow shaders) */
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

    /* 12. Reflect pipeline: reflect.vert + scene.frag
     *    VK_CULL_MODE_FRONT_BIT compensates for the Y-mirror winding reversal */
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

    /* 13. Scene pipeline: scene.vert + scene.frag */
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

    /* 14. Floor pipeline: floor.vert + floor.frag */
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

    /* 16. Descriptor pool
     *   4 sets × FRAMES: scene, reflect, floor, sky
     *   UBOs:     4 × FRAMES
     *   Samplers: scene(2) + reflect(2) + floor(3) + sky(1) = 8 × FRAMES  */
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

    /* scene descriptor set: binding 0=UBO, 1=albedoTex, 2=shadowMap */
    r.descSet.descPool   = &r.descPool;
    r.descSet.descLayout = &sceneProg.layouts[0];
    r.descSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &r.descSet);

    /* reflect descriptor set: same bindings as scene, reflect camera UBO */
    BkpDescriptorSet reflectDescSet = {0};
    reflectDescSet.descPool   = &r.descPool;
    reflectDescSet.descLayout = &reflProg.layouts[0];
    reflectDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &reflectDescSet);

    /* floor descriptor set: binding 0=UBO, 1=albedoTex, 2=shadowMap, 3=reflectionMap */
    BkpDescriptorSet floorDescSet = {0};
    floorDescSet.descPool   = &r.descPool;
    floorDescSet.descLayout = &floorProg.layouts[0];
    floorDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &floorDescSet);

    /* sky descriptor set: binding 0=UBO, 1=skyTex */
    r.skyDescSet.descPool   = &r.descPool;
    r.skyDescSet.descLayout = &r.skyProg.layouts[0];
    r.skyDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r.adp, &r.skyDescSet);

    for(uint32_t f = 0; f < FRAMES; ++f)
    {
        VkDescriptorBufferInfo mainBuf = bkpMakeUboInfo(r.ubo.buffer, f,          r.ubo.size);
        VkDescriptorBufferInfo reflBuf = bkpMakeUboInfo(r.ubo.buffer, FRAMES + f, r.ubo.size);

        /* scene */
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

        /* reflect */
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

        /* floor */
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

        /* sky */
        BkpWriteDescriptorSetInfo wrS = {0};
        wrS.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        wrS.setBindings[0].ubos  = &mainBuf;
        wrS.setBindings[0].count = 1;
        bkpWriteDescriptorSet(r.adp, &r.skyDescSet, f, &wrS);
        bkpWriteDescriptorImage(r.adp, r.skyDescSet.descriptorSets[f], 1,
                                r.skySampler.sampler, r.skyTex.imageViews[0],
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    /* 17. Geometry */
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
    /* polished metal with albedo texture — high metallic, very low roughness */
    ground.color[0]     = 1.0f; ground.color[1] = 1.0f; ground.color[2] = 1.0f; ground.color[3] = 1.0f;
    ground.roughness    = 0.03f;
    ground.metallic     = 0.95f;
    ground.useAlbedoMap = 1.0f;

    /* 18. Command buffers */
    initCommandBuffers(&r);

    /* 19. Bake shadow map once (light is fixed — never recomputed) */
    BkpMat4 lightViewProj = computeLightViewProj();
    {
        BkpFence bakeFence = {0};
        bkpCreateFence(r.adp, &bakeFence, BKP_FALSE);

        bkpBeginCommandBuffer(&r.cmd, 0, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VkCommandBuffer bcmd = r.cmd.cmds[0];

        BkpImageBarrierInfo toDepth =
        {
            .image     = shadowImg.images[0],
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
            .image     = shadowImg.images[0],
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

    /* 20. HUD */
    Hud hud = {0};
    hudInit(r.adp, &hud, colorFmt, depthFmt, W, H, SHADER_DIR_HUD, FONT_PATH);

    /* 21. Camera */
    FreeCamera fc = makeFreeCamera(0.0f, 8.0f, 16.0f, 0.0f, -0.45f);
    Camera     cam = {0};
    registerCameraInput(&fc);
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

            BkpVec3 up     = bkpVec3(0.0f, 1.0f, 0.0f);
            BkpVec3 eye    = bkpVec3(cam.pos[0],    cam.pos[1],    cam.pos[2]);
            BkpVec3 target = bkpVec3(cam.target[0], cam.target[1], cam.target[2]);

            BkpMat4 proj = bkpPerspective(45.0f, (float)r.width / (float)r.height, 0.1f, 500.0f);
            proj.mLine1.y *= -1.0f;

            BkpVec3 reflEye    = bkpVec3(cam.pos[0],    -cam.pos[1],    cam.pos[2]);
            BkpVec3 reflTarget = bkpVec3(cam.target[0], -cam.target[1], cam.target[2]);
            BkpMat4 reflView   = bkpLookAt(reflEye, reflTarget, up);
            BkpMat4 reflVP     = bkpMat4DotMat4(&proj, &reflView);

            /* Main UBO (SceneUBORefl — also carries reflectViewProj for floor.frag) */
            SceneUBORefl mainUBO = {0};
            mainUBO.view = bkpLookAt(eye, target, up);
            mainUBO.proj = proj;
            mainUBO.lightDir[0] = ld[0]/len; mainUBO.lightDir[1] = ld[1]/len;
            mainUBO.lightDir[2] = ld[2]/len; mainUBO.lightDir[3] = 0.0f;
            mainUBO.camPos[0] = cam.pos[0]; mainUBO.camPos[1] = cam.pos[1];
            mainUBO.camPos[2] = cam.pos[2]; mainUBO.camPos[3] = 1.0f;
            mainUBO.lightViewProj   = lightViewProj;
            mainUBO.reflectViewProj = reflVP;
            bkpUploadBufferData(r.adp, r.ubo.buffer, &mainUBO,
                                f * r.ubo.size, sizeof(SceneUBORefl));

            /* Reflect UBO (mirrored camera, same size so buffer strides match) */
            SceneUBORefl reflUBO = {0};
            reflUBO.view = reflView;
            reflUBO.proj = proj;
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

        /* --- Reflection pass ------------------------------------------------
         *   Render scene objects from the mirrored camera into reflectImg.
         *   reflect.vert sets gl_ClipDistance[0] = worldPos.y to discard
         *   everything below Y=0, so only above-floor geometry is reflected.  */
        {
            BkpImageBarrierInfo barriers[2] =
            {
                {
                    .image     = reflectImg.images[0],
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcStage  = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                    .dstStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
                },
                {
                    .image     = reflDepth.images[0],
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
            bkpCmdBarrierImages(vkcmd, barriers, 2);

            VkRenderingAttachmentInfo colorAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            colorAtt.imageView   = reflectImg.imageViews[0];
            colorAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAtt.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
            colorAtt.clearValue  = (VkClearValue){.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

            VkRenderingAttachmentInfo depthAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            depthAtt.imageView   = reflDepth.imageViews[0];
            depthAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAtt.storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAtt.clearValue  = (VkClearValue){.depthStencil = {1.0f, 0}};

            VkRenderingInfo renderInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
            renderInfo.renderArea.extent    = (VkExtent2D){W, H}; /* fixed texture size */
            renderInfo.layerCount           = 1;
            renderInfo.colorAttachmentCount = 1;
            renderInfo.pColorAttachments    = &colorAtt;
            renderInfo.pDepthAttachment     = &depthAtt;
            bkpCmdBeginRendering(vkcmd, &renderInfo);
            bkpUpdateWindowViewportScissor(vkcmd, W, H); /* fixed texture size */

            bkpCmdBindPipeline(vkcmd, reflectPipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                reflectPipeline.pipelineLayout.layout, 0, 1,
                &reflectDescSet.descriptorSets[f], 0, NULL);
            for(int o = 0; o < OBJ_COUNT; ++o)
                drawObject(vkcmd, &reflectPipeline, &r.objs[o]);
            if(r.gltfModel.meshCount > 0)
                drawObject(vkcmd, &reflectPipeline, &r.gltfSphere);

            bkpCmdEndRendering(vkcmd);

            /* Transition reflectImg to SHADER_READ_ONLY for the floor pass */
            BkpImageBarrierInfo toReadOnly =
            {
                .image     = reflectImg.images[0],
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStage  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccess = VK_ACCESS_2_SHADER_READ_BIT,
                .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
            };
            bkpCmdBarrierImages(vkcmd, &toReadOnly, 1);
        }

        /* --- Main pass --- */
        {
            BkpImageBarrierInfo beginBarriers[2] =
            {
                {
                    .image     = r.sc.images[i],
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcStage  = VK_PIPELINE_STAGE_2_NONE, .srcAccess = VK_ACCESS_2_NONE,
                    .dstStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
                },
                {
                    .image     = r.depthImg.images[0],
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

            /* Scene objects */
            bkpCmdBindPipeline(vkcmd, scenePipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                scenePipeline.pipelineLayout.layout, 0, 1,
                &r.descSet.descriptorSets[f], 0, NULL);
            for(int o = 0; o < OBJ_COUNT; ++o)
                drawObject(vkcmd, &scenePipeline, &r.objs[o]);
            if(r.gltfModel.meshCount > 0)
                drawObject(vkcmd, &scenePipeline, &r.gltfSphere);

            /* Floor: albedo texture + shadow PCF + Fresnel reflection blend */
            bkpCmdBindPipeline(vkcmd, floorPipeline.pipeline);
            vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                floorPipeline.pipelineLayout.layout, 0, 1,
                &floorDescSet.descriptorSets[f], 0, NULL);
            drawObject(vkcmd, &floorPipeline, &ground);

            hudDraw(r.adp, vkcmd, &hud, f);

            bkpCmdEndRendering(vkcmd);

            BkpImageBarrierInfo presentBarrier =
            {
                .image     = r.sc.images[i],
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

    /* 23. Cleanup */
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
