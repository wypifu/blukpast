#ifndef TUTORIAL_IBL_H_
#define TUTORIAL_IBL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../common/scene.h"

/* -------------------------------------------------------------------------
 * IblRenderer — extends the base Renderer with IBL-specific GPU resources.
 *
 * The `base` member MUST be first so that an IblRenderer* can be cast to
 * Renderer* when calling shared helpers (uploadUBO, drawObject, initDepth,
 * initCommandBuffers, initObject, makeModel …).
 *
 * Resources added on top of Renderer:
 *   skyCube        – procedural 256×256 cubemap (gradient sides, blue +Y,
 *                    white −Y); used both for sky rendering and as a simple
 *                    specular reflection probe.
 *   irradianceMap  – 32×32 cubemap, cosine-weighted average sky radiance
 *                    per direction; drives the diffuse IBL term.
 *   brdfLut        – 128×128 split-sum BRDF look-up table (R=scale, G=bias);
 *                    precomputed with importance-sampled GGX.
 *   cubeSampler    – linear sampler shared by skyCube and irradianceMap.
 *   brdfSampler    – linear clamp sampler for the 2-D BRDF LUT.
 * ------------------------------------------------------------------------- */
typedef struct
{
    Renderer         base;          /* must be first */

    BkpImageResource skyCube;
    BkpImageResource irradianceMap;
    BkpImageResource brdfLut;
    BkpSampler       cubeSampler;
    BkpSampler       brdfSampler;

    BkpImageResource damierTex;      /* checkerboard texture for the ground */
    BkpSampler       damierSampler;
} IblRenderer;

/**
 * @brief Generate the procedural sky cubemap and upload it to the GPU.
 *
 * Creates three images on the CPU (gradient, solid-blue, solid-white) and
 * assembles them into a 256 × 256 six-face cubemap:
 * - Face +Y  : solid deep-blue (sky zenith)
 * - Face −Y  : solid white    (ground / nadir)
 * - Faces ±X, ±Z : vertical gradient blue→white (horizon sides)
 *
 * Also creates `cubeSampler` and `brdfSampler`.
 * Call before @ref iblInitIrradianceMap and @ref iblInitPipelines.
 */
void iblInitSkyCubemap(IblRenderer * r);

/**
 * @brief Precompute the diffuse irradiance cubemap on the CPU.
 *
 * For each of the 32 × 32 × 6 output texels the function integrates the
 * sky colour over the cosine-weighted hemisphere using 512 Hammersley samples.
 * The stored value is E(N)/π — the irradiance divided by π — so the fragment
 * shader can compute diffuse as `kD * albedo * irradiance` (no further π term).
 *
 * The sky colour is sampled analytically from the same gradient model used
 * by @ref iblInitSkyCubemap (no GPU readback required).
 *
 * Must be called after @ref iblInitSkyCubemap.
 */
void iblInitIrradianceMap(IblRenderer * r);

/**
 * @brief Precompute the split-sum BRDF look-up table on the CPU.
 *
 * Generates a 128 × 128 RGBA8 texture where:
 * - x axis = NdotV  (0 → 1)
 * - y axis = roughness (0 → 1)
 * - R channel = scale (A term, F0-independent)
 * - G channel = bias  (B term)
 *
 * Each texel is computed with 512 importance-sampled GGX directions using
 * the Smith-GGX visibility term.
 *
 * Must be called after @ref iblInitSkyCubemap (uses `base.adp`).
 */
void iblInitBrdfLut(IblRenderer * r);

/**
 * @brief Generate and upload the checkerboard (damier) texture for the ground.
 *
 * Creates a 256 × 256 RGBA8 checkerboard (32 × 32 px square tiles) on the CPU
 * using @ref bkpGenerateImageDataDamier and uploads it as a 2-D texture.
 * Also creates @c damierSampler (linear, repeat).
 * Must be called before @ref iblInitPipelines.
 */
void iblInitDamierTexture(IblRenderer * r);

/**
 * @brief Compile shaders and create all Vulkan pipelines for the IBL tutorial.
 *
 * Replaces the common `initPipelines()`.  Sets up two pipelines:
 *   - **Scene** (PBR + IBL): set 0 has UBO (binding 0), irradianceMap (1),
 *     skyCube (2), brdfLut (3).  Back-face culling, depth test on.
 *   - **Sky** (cubemap): set 0 has UBO (binding 0), skyCube (1).
 *     Cull none, depth write off.
 *
 * Writes all static image descriptors (irradiance, sky, BRDF) once.
 *
 * @param r          IBL renderer to initialise.
 * @param shaderDir  Path prefix for compiled .spv files
 *                   (e.g. "tutorial/03_pbr_ibl/shaders").
 */
void iblInitPipelines(IblRenderer * r, const char * shaderDir);

/**
 * @brief Record and submit one frame for the IBL tutorial.
 *
 * Replaces the common `renderFrame()`.  Renders:
 *   1. Sky box (cube mesh, sky cubemap, depth write off)
 *   2. Ground plane (scene pipeline, IBL lighting)
 *   3. All scene objects (scene pipeline, IBL lighting)
 */
void iblRenderFrame(IblRenderer * r, uint32_t frame, uint32_t imageIndex,
                    Camera * cam, SceneObject * ground);

/**
 * @brief Destroy all GPU resources owned by the IblRenderer.
 *
 * Waits for the device to be idle, destroys IBL textures (skyCube,
 * irradianceMap, brdfLut), samplers, pipelines, descriptor sets and pool,
 * geometry buffers, depth image, command buffers, UBO, and the swap chain.
 */
void iblCleanup(IblRenderer * r);

#ifdef __cplusplus
}
#endif

#endif /* TUTORIAL_IBL_H_ */
