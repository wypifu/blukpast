#ifndef TUTORIAL_SCENE_H_
#define TUTORIAL_SCENE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <include/blukpast.h>
#include <math/include/bkp_mat.h>
#include <math/include/basics.h>
#include <system/include/bkp_systemTime.h>
#include <loaders/include/bkp_loaders.h>
#include <string.h>
#include <math.h>

#define GLTF_SPHERE_PATH "tutorial/gltf/sphere.glb"

#define FRAMES    2
#define OBJ_COUNT 7

/* -------------------------------------------------------------------------
 * Scene UBO (set=0, binding=0) — matches scene.vert / scene.frag
 * ------------------------------------------------------------------------- */
typedef struct
{
    BkpMat4 view;
    BkpMat4 proj;
    float   lightDir[4];
    float   camPos[4];
} SceneUBO;

/* SceneUBOShadow extends SceneUBO with a light view-projection for shadow mapping.
 * Used from tutorial 05 onwards. */
typedef struct
{
    BkpMat4 view;
    BkpMat4 proj;
    float   lightDir[4];
    float   camPos[4];
    BkpMat4 lightViewProj;
} SceneUBOShadow;

/* -------------------------------------------------------------------------
 * Push constant — mat4 model (64) + vec4 color (16) + float*4 = 96 bytes
 * ------------------------------------------------------------------------- */
typedef struct
{
    BkpMat4 model;
    float   color[4];
    float   roughness;
    float   metallic;
    float   pad[2];
} ObjectPC;

/* -------------------------------------------------------------------------
 * Scene object
 * ------------------------------------------------------------------------- */
typedef struct
{
    BkpVertexGeometryInfo geo;
    BkpMat4               model;
    float color[4];
    float roughness;
    float metallic;
    float useAlbedoMap;  /* pad0 in PC: 1 = sample albedo texture, 0 = flat color */
} SceneObject;

/* -------------------------------------------------------------------------
 * Camera (pos + target)
 * ------------------------------------------------------------------------- */
typedef struct
{
    float pos[3];
    float target[3];
} Camera;

/* -------------------------------------------------------------------------
 * Base renderer state (shared across all tutorials)
 * ------------------------------------------------------------------------- */
typedef struct
{
    BkpGpuAdapter      adp;
    BkpSwapChain       sc;

    BkpShaderModule    sceneVert, sceneFrag;
    BkpShaderProgram   sceneProg;
    BkpPipelineGraphic scenePipeline;

    BkpShaderModule    skyVert, skyFrag;
    BkpShaderProgram   skyProg;
    BkpPipelineGraphic skyPipeline;

    BkpDescriptorPool  descPool;
    BkpDescriptorSet   descSet;
    BkpDescriptorSet   skyDescSet;

    BkpBufferGPU       ubo;

    BkpImageResource   skyTex;
    BkpSampler         skySampler;
    BkpImageResource   depthImg;

    BkpCommandBuffer   cmd;

    SceneObject        objs[OBJ_COUNT];
    BkpVertexGeometryInfo skyGeo;
    BkpVertexGeometryInfo groundGeo;

    BkpModel    gltfModel;
    SceneObject gltfSphere;

    int width, height;
} Renderer;

/* -------------------------------------------------------------------------
 * Math helpers
 * ------------------------------------------------------------------------- */
BkpMat4 makeModel(float tx, float ty, float tz, float sx, float sy, float sz);

/* XY plane → XZ ground plane via Rx(-90°), normals +Y. */
BkpMat4 makeGroundModel(float tx, float ty, float tz, float sx, float sz);

/* -------------------------------------------------------------------------
 * Shadow / texture helpers (tutorial 06+)
 * Tuto 05 keeps its own inline versions to stay self-contained.
 * From tuto 06 onwards these common helpers replace the inline code.
 * ------------------------------------------------------------------------- */

/* Creates a 4096×4096 D32 depth image and a hardware PCF comparison sampler. */
void initShadowResources(BkpGpuAdapter adp,
                         BkpImageResource * shadowImg,
                         BkpSampler       * shadowSampler);

/* Loads the shared ground texture (bkpview_wallpaper2.png) and a repeat sampler. */
void initGroundTexture(BkpGpuAdapter adp,
                       BkpImageResource * groundTex,
                       BkpSampler       * groundSampler);

/* -------------------------------------------------------------------------
 * Scene helpers
 * ------------------------------------------------------------------------- */
/* Loads GLTF_SPHERE_PATH and populates r->gltfSphere at (tx, ty, tz).
 * Safe to call even if the file doesn't exist — gltfModel.meshCount stays 0. */
void initGltfSphere(Renderer * r, float tx, float ty, float tz, float scale);

void initObject(BkpGpuAdapter adp, SceneObject * obj, BkpEGeometryType type,
                float tx, float ty, float tz,
                float sx, float sy, float sz,
                float r,  float g,  float b,
                float rough, float metal);

void uploadUBO(Renderer * r, uint32_t frame, Camera * cam);

void drawObject(VkCommandBuffer cmd, BkpPipelineGraphic * pip, SceneObject * obj);

/* -------------------------------------------------------------------------
 * Init / cleanup
 * ------------------------------------------------------------------------- */
void initSkyTexture(Renderer * r);
void initDepth(Renderer * r);

/* shaderDir: path to compiled .spv files, e.g. "tutorial/01_playground/shaders" */
void initPipelines(Renderer * r, const char * shaderDir);

void initCommandBuffers(Renderer * r);

/* renderFrame: records full frame commands (UBO upload, sky, scene).
 * Caller must call bkpSubmitFrameBasic / bkpPresentFrame after. */
void renderFrame(Renderer * r, uint32_t frame, uint32_t imageIndex,
                 Camera * cam, SceneObject * ground);

/* Destroys all Renderer resources and clears the GPU adapter. */
void cleanupRenderer(Renderer * r);

#ifdef __cplusplus
}
#endif

#endif /* TUTORIAL_SCENE_H_ */
