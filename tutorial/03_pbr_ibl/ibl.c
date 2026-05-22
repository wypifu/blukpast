#include "ibl.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Constants
 * ========================================================================= */
#define IBL_PI        3.14159265359f
#define IBL_SKY_SIZE  256   /* cubemap face resolution for the sky box */
#define IBL_IRR_SIZE   32   /* irradiance map face resolution           */
#define IBL_BRDF_SIZE 128   /* BRDF LUT resolution (square)             */
#define IBL_SAMPLES   512   /* Hammersley samples per texel              */

/* =========================================================================
 * Low-discrepancy sampling (Hammersley sequence)
 * ========================================================================= */

static float radicalInverse(uint32_t bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return (float)bits * 2.3283064365386963e-10f;
}

static void hammersley(uint32_t i, uint32_t n, float * u, float * v)
{
    *u = (float)i / (float)n;
    *v = radicalInverse(i);
}

/* =========================================================================
 * Procedural sky colour model
 *
 * Matches the gradient baked into the sky cubemap:
 *   t = 0  (zenith / +Y) → deep blue  (15, 35, 90)
 *   t = 1  (nadir  / −Y) → white     (255,255,255)
 * with two gradient segments separated at t = 0.67.
 * ========================================================================= */
static void sampleSkyColor(float dx, float dy, float dz, float out[3])
{
    float len = sqrtf(dx*dx + dy*dy + dz*dz);
    if(len < 1e-6f) { out[0] = out[1] = out[2] = 1.0f; return; }
    float yn = dy / len;                    /* −1 = nadir, +1 = zenith */

    float t = 0.5f - 0.5f * yn;            /* 0 at top, 1 at bottom   */
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);

    float r, g, b;
    if(t <= 0.67f)
    {
        float s = t / 0.67f;
        r = ( 15.0f + s * (140.0f -  15.0f)) / 255.0f;
        g = ( 35.0f + s * (190.0f -  35.0f)) / 255.0f;
        b = ( 90.0f + s * (240.0f -  90.0f)) / 255.0f;
    }
    else
    {
        float s = (t - 0.67f) / 0.33f;
        r = (140.0f + s * (255.0f - 140.0f)) / 255.0f;
        g = (190.0f + s * (255.0f - 190.0f)) / 255.0f;
        b = (240.0f + s * (255.0f - 240.0f)) / 255.0f;
    }
    out[0] = r; out[1] = g; out[2] = b;
}

/* =========================================================================
 * Cubemap face UV → world direction
 *
 * Face order: 0=+X  1=−X  2=+Y  3=−Y  4=+Z  5=−Z
 * u, v in [0, 1] with (0,0) at top-left of the face image.
 * ========================================================================= */
static void faceUvToDir(int face, float u, float v, float dir[3])
{
    float s = u * 2.0f - 1.0f;
    float t = v * 2.0f - 1.0f;
    switch(face)
    {
        case 0: dir[0] = +1.0f; dir[1] =   -t; dir[2] =   -s; break; /* +X */
        case 1: dir[0] = -1.0f; dir[1] =   -t; dir[2] =   +s; break; /* −X */
        case 2: dir[0] =    s;  dir[1] = +1.0f; dir[2] =   +t; break; /* +Y */
        case 3: dir[0] =    s;  dir[1] = -1.0f; dir[2] =   -t; break; /* −Y */
        case 4: dir[0] =    s;  dir[1] =   -t; dir[2] = +1.0f; break; /* +Z */
        case 5: dir[0] =   -s;  dir[1] =   -t; dir[2] = -1.0f; break; /* −Z */
        default: dir[0] = dir[1] = dir[2] = 0.0f;
    }
}

/* =========================================================================
 * Orthonormal tangent frame around N
 * ========================================================================= */
static void buildTangentFrame(const float N[3], float T[3], float B[3])
{
    float up[3] = {0.0f, 1.0f, 0.0f};
    if(fabsf(N[1]) > 0.99f) { up[0] = 1.0f; up[1] = 0.0f; up[2] = 0.0f; }

    /* T = normalize(up × N) */
    T[0] = up[1]*N[2] - up[2]*N[1];
    T[1] = up[2]*N[0] - up[0]*N[2];
    T[2] = up[0]*N[1] - up[1]*N[0];
    float tlen = sqrtf(T[0]*T[0] + T[1]*T[1] + T[2]*T[2]);
    T[0] /= tlen; T[1] /= tlen; T[2] /= tlen;

    /* B = N × T */
    B[0] = N[1]*T[2] - N[2]*T[1];
    B[1] = N[2]*T[0] - N[0]*T[2];
    B[2] = N[0]*T[1] - N[1]*T[0];
}

/* =========================================================================
 * Irradiance generation (CPU, cosine-weighted Hammersley)
 *
 * For each output texel we integrate the sky colour over the upper hemisphere:
 *
 *   irr(N) = ∫ L(ωi) · cos(θi) dωi
 *
 * With cosine-weighted sampling (pdf = cos/π) the estimator simplifies to:
 *
 *   irr(N)/π ≈ (1/N) · Σ L(ωi)          (stored in the map)
 *
 * The fragment shader then uses:  diffuse = kD · albedo · irradiance
 * where the π factor in the Lambertian BRDF (1/π) and in the stored value
 * (irr/π) cancel, yielding the correct physical result.
 * ========================================================================= */
static void generateIrradianceFace(int face, int size, uint8_t * out)
{
    for(int py = 0; py < size; py++)
    {
        for(int px = 0; px < size; px++)
        {
            float u = (px + 0.5f) / (float)size;
            float v = (py + 0.5f) / (float)size;

            float N[3];
            faceUvToDir(face, u, v, N);
            float nlen = sqrtf(N[0]*N[0] + N[1]*N[1] + N[2]*N[2]);
            N[0] /= nlen; N[1] /= nlen; N[2] /= nlen;

            float T[3], B[3];
            buildTangentFrame(N, T, B);

            float irr[3] = {0.0f, 0.0f, 0.0f};
            for(int i = 0; i < IBL_SAMPLES; i++)
            {
                float u1, u2;
                hammersley((uint32_t)i, (uint32_t)IBL_SAMPLES, &u1, &u2);

                /* Cosine-weighted hemisphere sample in tangent space */
                float phi      = 2.0f * IBL_PI * u1;
                float sinTheta = sqrtf(u2);
                float cosTheta = sqrtf(1.0f - u2);   /* lz = cosTheta  */

                float lx = sinTheta * cosf(phi);
                float ly = sinTheta * sinf(phi);
                float lz = cosTheta;

                /* Transform to world space */
                float wx = lx*T[0] + ly*B[0] + lz*N[0];
                float wy = lx*T[1] + ly*B[1] + lz*N[1];
                float wz = lx*T[2] + ly*B[2] + lz*N[2];

                float col[3];
                sampleSkyColor(wx, wy, wz, col);
                irr[0] += col[0];
                irr[1] += col[1];
                irr[2] += col[2];
            }

            /* Divide by N: stored value = mean_L = irr(N)/π */
            irr[0] /= (float)IBL_SAMPLES;
            irr[1] /= (float)IBL_SAMPLES;
            irr[2] /= (float)IBL_SAMPLES;

            int idx = (py * size + px) * 4;
            out[idx+0] = (uint8_t)(fminf(irr[0], 1.0f) * 255.0f);
            out[idx+1] = (uint8_t)(fminf(irr[1], 1.0f) * 255.0f);
            out[idx+2] = (uint8_t)(fminf(irr[2], 1.0f) * 255.0f);
            out[idx+3] = 255;
        }
    }
}

/* =========================================================================
 * BRDF LUT generation (CPU, importance-sampled GGX split-sum)
 *
 * Each texel (x=NdotV, y=roughness) stores:
 *   R = scale (A)  — the F0-independent integral component
 *   G = bias  (B)  — the Fresnel-power-5 weighted component
 *
 * The fragment shader reconstructs the specular integral as:
 *   specular_ibl = prefilteredColor · (F0 · scale + bias)
 * ========================================================================= */

static float smithG1(float NdotV, float roughness)
{
    float k = (roughness * roughness) * 0.5f;   /* Schlick-GGX remapping */
    return NdotV / (NdotV * (1.0f - k) + k);
}

static void generateBrdfLut(uint8_t * out, int size)
{
    for(int j = 0; j < size; j++)               /* j → roughness */
    {
        float roughness = (j + 0.5f) / (float)size;

        for(int i = 0; i < size; i++)           /* i → NdotV */
        {
            float NdotV = (i + 0.5f) / (float)size;

            /* V in tangent space: N = (0,0,1), so Vz = NdotV */
            float Vx = sqrtf(fmaxf(0.0f, 1.0f - NdotV * NdotV));
            float Vz = NdotV;

            float A = 0.0f, B = 0.0f;

            for(uint32_t k = 0; k < IBL_SAMPLES; k++)
            {
                float u1, u2;
                hammersley(k, IBL_SAMPLES, &u1, &u2);

                /* Importance-sample GGX to get H in tangent space.
                 * Note: Hy is not computed because V.y = 0, so VdotH
                 * and the reflected L are fully determined by Hx, Hz. */
                float a        = roughness * roughness;
                float phi      = 2.0f * IBL_PI * u1;
                float cosTheta = sqrtf((1.0f - u2)
                                 / (1.0f + (a*a - 1.0f) * u2 + 1e-7f));
                float sinTheta = sqrtf(fmaxf(0.0f, 1.0f - cosTheta * cosTheta));
                float Hx       = sinTheta * cosf(phi);
                float Hz       = cosTheta;

                float VdotH = fmaxf(Vx * Hx + Vz * Hz, 0.0f);

                /* L = 2·(V·H)·H − V  (reflection of V about H) */
                float Lz    = 2.0f * VdotH * Hz - Vz;
                float NdotL = fmaxf(Lz, 0.0f);
                float NdotH = fmaxf(Hz, 0.0f);

                if(NdotL > 0.0f)
                {
                    float G     = smithG1(NdotV, roughness) * smithG1(NdotL, roughness);
                    float G_vis = (G * VdotH) / (NdotH * NdotV + 1e-7f);
                    float Fc    = powf(1.0f - VdotH, 5.0f);
                    A += (1.0f - Fc) * G_vis;
                    B += Fc          * G_vis;
                }
            }

            A /= (float)IBL_SAMPLES;
            B /= (float)IBL_SAMPLES;

            int idx = (j * size + i) * 4;
            out[idx+0] = (uint8_t)(fminf(fmaxf(A, 0.0f), 1.0f) * 255.0f);
            out[idx+1] = (uint8_t)(fminf(fmaxf(B, 0.0f), 1.0f) * 255.0f);
            out[idx+2] = 0;
            out[idx+3] = 255;
        }
    }
}

/* =========================================================================
 * Public init functions
 * ========================================================================= */

void iblInitSkyCubemap(IblRenderer * r)
{
    /* --- Sky gradient (same stops as common initSkyTexture) --- */
    BkpColorGradianInfo grads[2];
    grads[0].color1[0] =  15; grads[0].color1[1] =  35; grads[0].color1[2] =  90; grads[0].color1[3] = 255;
    grads[0].color2[0] = 140; grads[0].color2[1] = 190; grads[0].color2[2] = 240; grads[0].color2[3] = 255;
    grads[0].begin = 0.0f; grads[0].end = 0.67f;
    grads[1].color1[0] = 140; grads[1].color1[1] = 190; grads[1].color1[2] = 240; grads[1].color1[3] = 255;
    grads[1].color2[0] = 255; grads[1].color2[1] = 255; grads[1].color2[2] = 255; grads[1].color2[3] = 255;
    grads[1].begin = 0.67f; grads[1].end = 1.0f;

    VkDeviceSize gradSz, blueSz, whiteSz;
    /* Sides: vertical gradient (blue top → white bottom) */
    uint8_t * grad  = bkpGenerateVerticalGradianImage(IBL_SKY_SIZE, IBL_SKY_SIZE, grads, 2, &gradSz);
    /* +Y face: deep-blue zenith */
    uint8_t * blue  = bkpGenerateImageDataRGBA(IBL_SKY_SIZE, IBL_SKY_SIZE, 15, 35, 90, 255, &blueSz);
    /* −Y face: white nadir */
    uint8_t * white = bkpGenerateImageDataRGBA(IBL_SKY_SIZE, IBL_SKY_SIZE, 255, 255, 255, 255, &whiteSz);

    /* Face order: +X −X +Y −Y +Z −Z */
    uint8_t * faces[6] = {grad, grad, blue, white, grad, grad};

    bkpSetDefaultCubemapInfo(&r->skyCube, VK_FORMAT_R8G8B8A8_UNORM);
    bkpCreateCubemapFromData(r->base.adp, faces, IBL_SKY_SIZE, &r->skyCube);

    bkpFree(grad);
    bkpFree(blue);
    bkpFree(white);

    /* Sampler shared by skyCube and irradianceMap */
    r->cubeSampler.info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    r->cubeSampler.info.magFilter    = VK_FILTER_LINEAR;
    r->cubeSampler.info.minFilter    = VK_FILTER_LINEAR;
    r->cubeSampler.info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    r->cubeSampler.info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    r->cubeSampler.info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    r->cubeSampler.info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    r->cubeSampler.info.minLod       = 0.0f;
    r->cubeSampler.info.maxLod       = 1.0f;
    bkpCreateSampler(r->base.adp, &r->cubeSampler);

    /* Sampler for the 2-D BRDF LUT */
    r->brdfSampler.info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    r->brdfSampler.info.magFilter    = VK_FILTER_LINEAR;
    r->brdfSampler.info.minFilter    = VK_FILTER_LINEAR;
    r->brdfSampler.info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    r->brdfSampler.info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    r->brdfSampler.info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    r->brdfSampler.info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    r->brdfSampler.info.minLod       = 0.0f;
    r->brdfSampler.info.maxLod       = 1.0f;
    bkpCreateSampler(r->base.adp, &r->brdfSampler);
}

void iblInitIrradianceMap(IblRenderer * r)
{
    const int S = IBL_IRR_SIZE;
    const int faceBytes = S * S * 4;

    uint8_t * faceData[6];
    for(int f = 0; f < 6; f++)
    {
        faceData[f] = (uint8_t *)malloc((size_t)faceBytes);
        generateIrradianceFace(f, S, faceData[f]);
    }

    bkpSetDefaultCubemapInfo(&r->irradianceMap, VK_FORMAT_R8G8B8A8_UNORM);
    bkpCreateCubemapFromData(r->base.adp, faceData, S, &r->irradianceMap);

    for(int f = 0; f < 6; f++) free(faceData[f]);
}

void iblInitBrdfLut(IblRenderer * r)
{
    const int S = IBL_BRDF_SIZE;
    uint8_t * pixels = (uint8_t *)malloc((size_t)(S * S * 4));
    generateBrdfLut(pixels, S);

    bkpSetDefaultTextureInfo(&r->brdfLut);
    r->brdfLut.imageInfo.format    = VK_FORMAT_R8G8B8A8_UNORM;
    r->brdfLut.viewInfo.format     = VK_FORMAT_R8G8B8A8_UNORM;
    r->brdfLut.imageInfo.mipLevels = 1;
    r->brdfLut.mipType             = eMIPMAP_NONE;
    r->brdfLut.hasAlpha            = BKP_FALSE;
    bkpCreateTextureLayersFromData(r->base.adp, &pixels, 1, S, S, 1, &r->brdfLut);

    free(pixels);
}

/* =========================================================================
 * Checkerboard ground texture
 * ========================================================================= */
void iblInitDamierTexture(IblRenderer * r)
{
    VkDeviceSize sz;
    /* light tile: warm light grey / dark tile: dark grey */
    uint8_t * pixels = bkpGenerateImageDataDamier(256, 256, 4,
                           200, 200, 195,   /* color1: light warm grey */
                           40,  40,  45,    /* color2: dark grey        */
                           &sz);

    bkpSetDefaultTextureInfo(&r->damierTex);
    r->damierTex.imageInfo.format    = VK_FORMAT_R8G8B8A8_UNORM;
    r->damierTex.viewInfo.format     = VK_FORMAT_R8G8B8A8_UNORM;
    r->damierTex.imageInfo.mipLevels = 1;
    r->damierTex.mipType             = eMIPMAP_NONE;
    r->damierTex.hasAlpha            = BKP_FALSE;
    bkpCreateTextureLayersFromData(r->base.adp, &pixels, 1, 256, 256, 1, &r->damierTex);
    bkpFree(pixels);

    r->damierSampler.info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    r->damierSampler.info.magFilter    = VK_FILTER_LINEAR;
    r->damierSampler.info.minFilter    = VK_FILTER_LINEAR;
    r->damierSampler.info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    r->damierSampler.info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    r->damierSampler.info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    r->damierSampler.info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    r->damierSampler.info.minLod       = 0.0f;
    r->damierSampler.info.maxLod       = 1.0f;
    bkpCreateSampler(r->base.adp, &r->damierSampler);
}

/* =========================================================================
 * Draw helper — same as drawObject but sets pc.pad[0] = useAlbedoMap flag
 * ========================================================================= */
static void drawObjectWithFlag(VkCommandBuffer vkcmd, BkpPipelineGraphic * pipe,
                               SceneObject * obj, float useAlbedoMap)
{
    ObjectPC pc = {0};
    pc.model     = obj->model;
    memcpy(pc.color, obj->color, sizeof(float) * 4);
    pc.roughness = obj->roughness;
    pc.metallic  = obj->metallic;
    pc.pad[0]    = useAlbedoMap;
    bkpCmdPushConstants(vkcmd, pipe->pipelineLayout.layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(ObjectPC), &pc);

    VkBuffer vbuf; VkDeviceSize voff;
    bkpGetBuffer(obj->geo.buffer, &vbuf);
    bkpGetBufferOffset(obj->geo.buffer, &voff);
    vkCmdBindVertexBuffers(vkcmd, 0, 1, &vbuf, &voff);
    if(obj->geo.hasIndices)
    {
        VkBuffer ibuf; bkpGetBuffer(obj->geo.buffer, &ibuf);
        vkCmdBindIndexBuffer(vkcmd, ibuf, voff + obj->geo.indicesOffset, obj->geo.indexType);
        vkCmdDrawIndexed(vkcmd, (uint32_t)obj->geo.count, 1, 0, 0, 0);
    }
    else { vkCmdDraw(vkcmd, (uint32_t)obj->geo.count, 1, 0, 0); }
}

/* =========================================================================
 * Pipeline setup (replaces common initPipelines)
 * ========================================================================= */
void iblInitPipelines(IblRenderer * r, const char * shaderDir)
{
    VkFormat colorFmt = r->base.sc.info.imageFormat;
    VkFormat depthFmt = r->base.depthImg.imageInfo.format;

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

    bkpCreateShaderModule(r->base.adp, sceneVert, &r->base.sceneVert);
    bkpCreateShaderModule(r->base.adp, sceneFrag, &r->base.sceneFrag);
    BkpShaderModule * sceneMods[] = {&r->base.sceneVert, &r->base.sceneFrag};
    bkpCreateShader(r->base.adp, sceneMods, 2, &r->base.sceneProg);
    bkpCreatePipelineLayoutFromShader(r->base.adp, &r->base.sceneProg, &r->base.scenePipeline.pipelineLayout);

    bkpCreateShaderModule(r->base.adp, skyVert, &r->base.skyVert);
    bkpCreateShaderModule(r->base.adp, skyFrag, &r->base.skyFrag);
    BkpShaderModule * skyMods[] = {&r->base.skyVert, &r->base.skyFrag};
    bkpCreateShader(r->base.adp, skyMods, 2, &r->base.skyProg);
    bkpCreatePipelineLayoutFromShader(r->base.adp, &r->base.skyProg, &r->base.skyPipeline.pipelineLayout);

    /* Descriptor pool:
     *   scene set (FRAMES): binding0=UBO + binding1=irradiance + binding2=skyCube
     *                       + binding3=brdfLut + binding4=damierTex
     *   sky   set (FRAMES): binding0=UBO + binding1=skyCube
     * → 2*FRAMES UBOs, 5*FRAMES combined image samplers, 2*FRAMES sets total */
    VkDescriptorPoolSize poolSizes[2] =
    {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         2 * FRAMES},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 * FRAMES},
    };
    r->base.descPool.info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    r->base.descPool.info.maxSets       = 2 * FRAMES;
    r->base.descPool.info.poolSizeCount = 2;
    r->base.descPool.info.pPoolSizes    = poolSizes;
    bkpCreateDescriptorPool(r->base.adp, &r->base.descPool);

    r->base.descSet.descPool   = &r->base.descPool;
    r->base.descSet.descLayout = &r->base.sceneProg.layouts[0];
    r->base.descSet.count      = FRAMES;
    bkpCreateDescriptorSet(r->base.adp, &r->base.descSet);

    r->base.skyDescSet.descPool   = &r->base.descPool;
    r->base.skyDescSet.descLayout = &r->base.skyProg.layouts[0];
    r->base.skyDescSet.count      = FRAMES;
    bkpCreateDescriptorSet(r->base.adp, &r->base.skyDescSet);

    /* Write per-frame UBO and per-set static image descriptors */
    for(uint32_t f = 0; f < FRAMES; ++f)
    {
        /* UBO → scene and sky sets, binding 0 */
        VkDescriptorBufferInfo bufInfo = bkpMakeUboInfo(r->base.ubo.buffer, f, r->base.ubo.size);
        BkpWriteDescriptorSetInfo wr   = {0};
        wr.setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        wr.setBindings[0].ubos  = &bufInfo;
        wr.setBindings[0].count = 1;
        bkpWriteDescriptorSet(r->base.adp, &r->base.descSet,    f, &wr);
        bkpWriteDescriptorSet(r->base.adp, &r->base.skyDescSet, f, &wr);

        /* Scene set — static IBL images (same data every frame) */
        VkDescriptorSet ds = r->base.descSet.descriptorSets[f];
        bkpWriteDescriptorImage(r->base.adp, ds, 1,
            r->cubeSampler.sampler, r->irradianceMap.imageViews[0],
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        bkpWriteDescriptorImage(r->base.adp, ds, 2,
            r->cubeSampler.sampler, r->skyCube.imageViews[0],
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        bkpWriteDescriptorImage(r->base.adp, ds, 3,
            r->brdfSampler.sampler, r->brdfLut.imageViews[0],
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        bkpWriteDescriptorImage(r->base.adp, ds, 4,
            r->damierSampler.sampler, r->damierTex.imageViews[0],
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        /* Sky set — sky cubemap */
        bkpWriteDescriptorImage(r->base.adp, r->base.skyDescSet.descriptorSets[f], 1,
            r->cubeSampler.sampler, r->skyCube.imageViews[0],
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    /* Scene pipeline */
    bkpCreatePipelineMinimalInfo(&r->base.scenePipeline.info,
        (VkExtent2D){(uint32_t)r->base.width, (uint32_t)r->base.height});
    r->base.scenePipeline.info.shaderProgram         = &r->base.sceneProg;
    r->base.scenePipeline.info.colorAttachmentFormat = colorFmt;
    r->base.scenePipeline.info.depthAttachmentFormat = depthFmt;
    r->base.scenePipeline.info.vertexLayout          = vLayout;
    r->base.scenePipeline.info.dynamicStates[0]      = VK_DYNAMIC_STATE_VIEWPORT;
    r->base.scenePipeline.info.dynamicStates[1]      = VK_DYNAMIC_STATE_SCISSOR;
    r->base.scenePipeline.info.dynamicState          = bkpDefaultDynamicStates(r->base.scenePipeline.info.dynamicStates, 2);
    r->base.scenePipeline.info.dynamicStateEnabled   = VK_TRUE;
    r->base.scenePipeline.info.viewportState         = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
    bkpCreatePipelineGraphic(r->base.adp, &r->base.scenePipeline);

    /* Sky pipeline — depth write off, cull none (cube viewed from inside) */
    bkpCreatePipelineMinimalInfo(&r->base.skyPipeline.info,
        (VkExtent2D){(uint32_t)r->base.width, (uint32_t)r->base.height});
    r->base.skyPipeline.info.shaderProgram                  = &r->base.skyProg;
    r->base.skyPipeline.info.colorAttachmentFormat          = colorFmt;
    r->base.skyPipeline.info.depthAttachmentFormat          = depthFmt;
    r->base.skyPipeline.info.vertexLayout                   = vLayout;
    r->base.skyPipeline.info.depthStencil.depthWriteEnable  = VK_FALSE;
    r->base.skyPipeline.info.rasterizer.cullMode            = VK_CULL_MODE_NONE;
    r->base.skyPipeline.info.dynamicStates[0]               = VK_DYNAMIC_STATE_VIEWPORT;
    r->base.skyPipeline.info.dynamicStates[1]               = VK_DYNAMIC_STATE_SCISSOR;
    r->base.skyPipeline.info.dynamicState                   = bkpDefaultDynamicStates(r->base.skyPipeline.info.dynamicStates, 2);
    r->base.skyPipeline.info.dynamicStateEnabled            = VK_TRUE;
    r->base.skyPipeline.info.viewportState                  = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
    bkpCreatePipelineGraphic(r->base.adp, &r->base.skyPipeline);
}

/* =========================================================================
 * Render frame
 * ========================================================================= */
void iblRenderFrame(IblRenderer * r, uint32_t frame, uint32_t imageIndex,
                    Camera * cam, SceneObject * ground)
{
    VkCommandBuffer vkcmd = r->base.cmd.cmds[frame];

    uploadUBO(&r->base, frame, cam);
    bkpBeginCommandBuffer(&r->base.cmd, frame, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    BkpImageBarrierInfo beginBarriers[2] =
    {
        { .image = r->base.sc.images[imageIndex],
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .srcStage  = VK_PIPELINE_STAGE_2_NONE,   .srcAccess = VK_ACCESS_2_NONE,
          .dstStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
          .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
          .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1 },
        { .image = r->base.depthImg.images[0],
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
          .srcStage  = VK_PIPELINE_STAGE_2_NONE,   .srcAccess = VK_ACCESS_2_NONE,
          .dstStage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
          .dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
          .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1 }
    };
    bkpCmdBarrierImages(vkcmd, beginBarriers, 2);

    VkRenderingAttachmentInfo colorAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colorAtt.imageView   = r->base.sc.imageViews[imageIndex];
    colorAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.clearValue  = (VkClearValue){.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderingAttachmentInfo depthAtt = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depthAtt.imageView   = r->base.depthImg.imageViews[0];
    depthAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.clearValue  = (VkClearValue){.depthStencil = {1.0f, 0}};

    VkRenderingInfo renderInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    renderInfo.renderArea.offset     = (VkOffset2D){0, 0};
    renderInfo.renderArea.extent     = (VkExtent2D){(uint32_t)r->base.width, (uint32_t)r->base.height};
    renderInfo.layerCount            = 1;
    renderInfo.colorAttachmentCount  = 1;
    renderInfo.pColorAttachments     = &colorAtt;
    renderInfo.pDepthAttachment      = &depthAtt;
    bkpCmdBeginRendering(vkcmd, &renderInfo);
    bkpUpdateWindowViewportScissor(vkcmd, r->base.width, r->base.height);

    /* Sky cube — depth write off, rendered first so scene objects overdraw it */
    bkpCmdBindPipeline(vkcmd, r->base.skyPipeline.pipeline);
    vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        r->base.skyPipeline.pipelineLayout.layout, 0, 1,
        &r->base.skyDescSet.descriptorSets[frame], 0, NULL);
    {
        VkBuffer vbuf; VkDeviceSize voff;
        bkpGetBuffer(r->base.skyGeo.buffer, &vbuf);
        bkpGetBufferOffset(r->base.skyGeo.buffer, &voff);
        vkCmdBindVertexBuffers(vkcmd, 0, 1, &vbuf, &voff);
        if(r->base.skyGeo.hasIndices)
        {
            VkBuffer ibuf; bkpGetBuffer(r->base.skyGeo.buffer, &ibuf);
            vkCmdBindIndexBuffer(vkcmd, ibuf, voff + r->base.skyGeo.indicesOffset, r->base.skyGeo.indexType);
            vkCmdDrawIndexed(vkcmd, (uint32_t)r->base.skyGeo.count, 1, 0, 0, 0);
        }
        else { vkCmdDraw(vkcmd, (uint32_t)r->base.skyGeo.count, 1, 0, 0); }
    }

    /* Scene objects */
    bkpCmdBindPipeline(vkcmd, r->base.scenePipeline.pipeline);
    vkCmdBindDescriptorSets(vkcmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        r->base.scenePipeline.pipelineLayout.layout, 0, 1,
        &r->base.descSet.descriptorSets[frame], 0, NULL);
    drawObjectWithFlag(vkcmd, &r->base.scenePipeline, ground, 1.0f);
    for(int o = 0; o < OBJ_COUNT; ++o)
        drawObjectWithFlag(vkcmd, &r->base.scenePipeline, &r->base.objs[o], 0.0f);
    if(r->base.gltfModel.meshCount > 0)
        drawObjectWithFlag(vkcmd, &r->base.scenePipeline, &r->base.gltfSphere, 0.0f);

    bkpCmdEndRendering(vkcmd);

    BkpImageBarrierInfo presentBarrier =
    {
        .image = r->base.sc.images[imageIndex],
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStage  = VK_PIPELINE_STAGE_2_NONE, .dstAccess = VK_ACCESS_2_NONE,
        .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1
    };
    bkpCmdBarrierImages(vkcmd, &presentBarrier, 1);
    bkpEndCommandBuffer(&r->base.cmd, frame);
}

/* =========================================================================
 * Cleanup
 * ========================================================================= */
void iblCleanup(IblRenderer * r)
{
    bkpWaitDevice(r->base.adp);

    bkpDestroyCommandBuffers(r->base.adp, &r->base.cmd);

    if(r->base.gltfModel.meshCount > 0) bkpUnloadModel(r->base.adp, &r->base.gltfModel);
    for(int o = 0; o < OBJ_COUNT; ++o)
        bkpFreeBuffer(r->base.adp, &r->base.objs[o].geo.buffer);
    bkpFreeBuffer(r->base.adp, &r->base.skyGeo.buffer);
    bkpFreeBuffer(r->base.adp, &r->base.groundGeo.buffer);

    bkpDestroyImageResource(r->base.adp, &r->base.depthImg);

    /* IBL-specific resources */
    bkpDestroyImageResource(r->base.adp, &r->skyCube);
    bkpDestroyImageResource(r->base.adp, &r->irradianceMap);
    bkpDestroyImageResource(r->base.adp, &r->brdfLut);
    bkpDestroyImageResource(r->base.adp, &r->damierTex);
    bkpDestroySampler(r->base.adp, &r->cubeSampler);
    bkpDestroySampler(r->base.adp, &r->brdfSampler);
    bkpDestroySampler(r->base.adp, &r->damierSampler);

    bkpDestroyBuffersGPU(r->base.adp, &r->base.ubo);

    bkpDestroyGraphicPipeline(r->base.adp, &r->base.scenePipeline);
    bkpDestroyPipelineLayout(r->base.adp, &r->base.scenePipeline.pipelineLayout);
    bkpDestroyGraphicPipeline(r->base.adp, &r->base.skyPipeline);
    bkpDestroyPipelineLayout(r->base.adp, &r->base.skyPipeline.pipelineLayout);

    bkpDestroyShader(r->base.adp, &r->base.sceneProg);
    bkpDestroyShaderModule(r->base.adp, &r->base.sceneVert);
    bkpDestroyShaderModule(r->base.adp, &r->base.sceneFrag);
    bkpDestroyShader(r->base.adp, &r->base.skyProg);
    bkpDestroyShaderModule(r->base.adp, &r->base.skyVert);
    bkpDestroyShaderModule(r->base.adp, &r->base.skyFrag);

    bkpDestroyDescriptorSet(r->base.adp, &r->base.descSet);
    bkpDestroyDescriptorSet(r->base.adp, &r->base.skyDescSet);
    bkpDestroyDescriptorPool(r->base.adp, &r->base.descPool);

    bkpCleanupSwapChain(r->base.adp, &r->base.sc);
    bkpClearGpuAdapter(r->base.adp);
}
