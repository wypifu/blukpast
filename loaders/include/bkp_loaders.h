#ifndef BKP_LOADERS_H
#define BKP_LOADERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <system/include/bkp_export.h>
#include <include/blukpast.h>

/**
 * @defgroup loaders Model Loaders
 * @brief Load 3D model files (GLTF 2.0, OBJ, PLY, STL) into GPU-ready structures.
 *
 * All loaders fill a @ref BkpModel and upload geometry/textures to the GPU via the
 * provided BkpGpuAdapter.  Call @ref bkpUnloadModel to release all GPU and CPU
 * resources when the model is no longer needed.
 * @{
 */

/*___________________________________________________________________*/

/** @brief GPU texture belonging to a loaded model. */
typedef struct
{
    BkpImageResource image;
    char             name[64];
} BkpModelTexture;

typedef enum {
    BKP_ALPHA_OPAQUE = 0,
    BKP_ALPHA_MASK   = 1,
    BKP_ALPHA_BLEND  = 2,
} BkpAlphaMode;

/**
 * @brief PBR material read from a model file.
 *
 * All `*Idx` fields index into `BkpModel::textures`.  A value of -1 means the
 * texture is absent and the corresponding factor should be used instead.
 */
typedef struct
{
    int   albedoIdx;            /**< Base-colour texture index, or -1. */
    int   normalIdx;            /**< Normal-map texture index, or -1. */
    int   metallicRoughnessIdx; /**< Metallic-roughness (ORM) texture index, or -1. */
    int   emissiveIdx;          /**< Emissive texture index, or -1. */
    int   occlusionIdx;         /**< Occlusion texture index, or -1. */
    float   baseColor[4];       /**< RGBA base-colour factor (linear). */
    float   metallic;           /**< Metallic factor [0, 1]. */
    float   roughness;          /**< Roughness factor [0, 1]. */
    float   emissive[3];        /**< Emissive factor RGB. */
    float   alphaCutoff;        /**< Alpha cutoff for MASK mode [0, 1]. */
    uint8_t doubleSided;        /**< Non-zero → render both faces (disable back-face culling). */
    uint8_t alphaMode;          /**< BkpAlphaMode: OPAQUE / MASK / BLEND. */
    char    name[128];
} BkpMaterial;

/**
 * @brief Single mesh primitive.
 *
 * When `isSkinned` is @ref BKP_TRUE the vertex buffer contains @ref BkpVertexSkin
 * records; otherwise it contains @ref BkpVertex records.
 */
typedef struct
{
    BkpVertexGeometryInfo geo;
    int     materialIdx;  /**< Index into BkpModel::materials, or -1 for no material. */
    BkpBool isSkinned;    /**< BKP_TRUE when vertices carry joint/weight attributes. */
    char    name[128];
    float   meshAabbMin[3]; /**< Local-space AABB minimum corner. */
    float   meshAabbMax[3]; /**< Local-space AABB maximum corner. */
} BkpModelMesh;

/*___________________________________________________________________*/
/** @defgroup animation Animation
 *  @ingroup loaders
 *  @brief Keyframe animation data (samplers, channels, clips).
 * @{ */

/** @brief Interpolation method for an animation sampler. */
typedef enum { BKP_INTERP_LINEAR, BKP_INTERP_STEP, BKP_INTERP_CUBICSPLINE } BkpAnimInterp;

/** @brief Target transform component of an animation channel. */
typedef enum { BKP_ANIM_T, BKP_ANIM_R, BKP_ANIM_S }                        BkpAnimPath;

/** @brief Keyframe sampler — time inputs + transform values. */
typedef struct
{
    float *       times;   /**< [count] input keyframe times (seconds).             */
    float *       values;  /**< [count * compCount] T/S → 3 floats, R → 4 floats.  */
                           /**< Cubic spline: count * 3 * compCount (in, val, out). */
    uint32_t      count;
    BkpAnimInterp interp;
} BkpAnimSampler;

/** @brief Single animation channel — binds a sampler to a node transform path. */
typedef struct
{
    int          nodeIdx;
    BkpAnimPath  path;
    BkpAnimSampler sampler;
} BkpAnimChannel;

/** @brief Named animation clip containing one or more channels. */
typedef struct
{
    char            name[128];
    BkpAnimChannel *channels;
    uint32_t        channelCount;
    float           duration; /**< Clip duration in seconds. */
} BkpAnimation;

/** @} */ /* animation */

/*___________________________________________________________________*/
/** @brief Skeleton skin — inverse-bind matrices and joint node indices. */
typedef struct
{
    char     name[128];
    int *    joints;               /**< [jointCount] node indices.             */
    float *  inverseBindMatrices;  /**< [jointCount * 16] column-major.        */
    uint32_t jointCount;
    int      skeletonRootIdx;      /**< Node index of the skeleton root, or -1. */
} BkpSkin;

/*___________________________________________________________________*/
/** @brief Scene-graph node holding a transform, optional mesh, and children. */
typedef struct
{
    char     name[128];
    float    translation[3];
    float    rotation[4];    /**< Quaternion xyzw.                              */
    float    scale[3];
    float    localMatrix[16];/**< TRS-combined or explicit matrix (column-major). */
    int      meshIdx;        /**< First primitive in BkpModel::meshes, or -1.   */
    uint32_t meshCount;      /**< Number of consecutive primitives for this node. */
    int      skinIdx;        /**< Index into BkpModel::skins, or -1.            */
    int *    children;       /**< [childCount] node indices.                    */
    uint32_t childCount;
    int      parentIdx;      /**< -1 for scene-root nodes.                      */
} BkpNode;

/*___________________________________________________________________*/
/**
 * @brief Complete loaded model — meshes, materials, textures, scene graph.
 *
 * Filled by any of the `bkpLoad*` functions.  Release with @ref bkpUnloadModel.
 */
typedef struct
{
    BkpModelMesh *    meshes;      uint32_t meshCount;
    BkpMaterial *     materials;   uint32_t materialCount;
    BkpModelTexture * textures;    uint32_t textureCount;
    BkpNode *         nodes;       uint32_t nodeCount;
    int *             rootNodes;   uint32_t rootNodeCount;
    BkpAnimation *    animations;  uint32_t animationCount;
    BkpSkin *         skins;       uint32_t skinCount;
    float aabbMin[3]; /**< World-space AABB minimum corner (computed on load). */
    float aabbMax[3]; /**< World-space AABB maximum corner (computed on load). */
    /* internal */
    uint32_t _meshCap, _matCap, _texCap, _nodeCap, _animCap, _skinCap;
} BkpModel;

/*___________________________________________________________________*/

/**
 * @brief Load a 3D model, auto-detecting the format from the file extension.
 *
 * Supports `.gltf`, `.glb`, `.obj`, `.ply`, and `.stl`.  Geometry and textures
 * are uploaded to the GPU before the function returns.
 *
 * @param adapter       GPU adapter used for buffer and texture upload.
 * @param path          Path to the model file.
 * @param model         Output model; must point to a zero-initialised BkpModel.
 * @param scratchGroupId Allocator group for temporary vertex arrays.
 *                      Pass 0 for the default group, or create a large group and
 *                      clear it after loading very large files.
 * @return BKP_TRUE on success, BKP_FALSE on error (see log for details).
 */
BKP_EXPORTED BkpBool bkpLoadModel  (BkpGpuAdapter adapter, const char * path,
                                    BkpModel * model, size_t scratchGroupId);

/**
 * @brief Release all GPU and CPU resources owned by a model.
 * @param adapter GPU adapter that was used to load the model.
 * @param model   Model to unload; fields are zeroed after the call.
 */
BKP_EXPORTED void    bkpUnloadModel(BkpGpuAdapter adapter, BkpModel * model);

/**
 * @brief Load a GLTF 2.0 file (`.gltf` or `.glb`).
 *
 * Supports meshes, PBR materials, skins, and animations.
 *
 * @param adapter       GPU adapter.
 * @param path          Path to the `.gltf` or `.glb` file.
 * @param model         Output model.
 * @param scratchGroupId Scratch allocator group (see @ref bkpLoadModel).
 * @return BKP_TRUE on success.
 */
BKP_EXPORTED BkpBool bkpLoadGltf(BkpGpuAdapter adapter, const char * path,
                                  BkpModel * model, size_t scratchGroupId);

/**
 * @brief Load a Wavefront OBJ file.
 * @param adapter       GPU adapter.
 * @param path          Path to the `.obj` file.
 * @param model         Output model.
 * @param scratchGroupId Scratch allocator group.
 * @return BKP_TRUE on success.
 */
BKP_EXPORTED BkpBool bkpLoadObj (BkpGpuAdapter adapter, const char * path,
                                  BkpModel * model, size_t scratchGroupId);

/**
 * @brief Load a stereolithography STL file (ASCII or binary).
 * @param adapter       GPU adapter.
 * @param path          Path to the `.stl` file.
 * @param model         Output model.
 * @param scratchGroupId Scratch allocator group.
 * @return BKP_TRUE on success.
 */
BKP_EXPORTED BkpBool bkpLoadStl (BkpGpuAdapter adapter, const char * path,
                                  BkpModel * model, size_t scratchGroupId);

/**
 * @brief Load a Stanford PLY file.
 * @param adapter       GPU adapter.
 * @param path          Path to the `.ply` file.
 * @param model         Output model.
 * @param scratchGroupId Scratch allocator group.
 * @return BKP_TRUE on success.
 */
BKP_EXPORTED BkpBool bkpLoadPly (BkpGpuAdapter adapter, const char * path,
                                  BkpModel * model, size_t scratchGroupId);

/** @} */ /* loaders */

#ifdef __cplusplus
}
#endif
#endif
