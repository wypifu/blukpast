# Blukpast

Lightweight Vulkan graphics library in C — pipelines, descriptors, buffers, textures, GLTF/OBJ/PLY/STL loaders.

Blukpast provides a thin but practical abstraction layer over Vulkan, covering pipeline creation, descriptor management, GPU buffer and texture allocation, shader loading, and command recording — without hiding the underlying API or imposing an engine architecture.

## Design goals

- Pure C API, embeddable as a shared or static library
- No imposed scene graph or entity system — just building blocks
- Suitable as a foundation for tools, viewers, and lightweight engines

## Features

- Vulkan pipeline and descriptor set helpers (graphics, compute)
- GPU buffer and texture management with batch upload support
- GLTF 2.0 loader — meshes, PBR materials, skinning, animations
- OBJ, PLY and STL loaders
- Font rendering
- Math library (vectors, matrices, linear algebra)
- Logging, threading, file system utilities

## Requirements

| | Linux | Windows |
|---|---|---|
| Compiler | GCC / Clang (C23) | Clang / MinGW-w64 (C23) |
| CMake | ≥ 3.10 | ≥ 3.10 |
| Vulkan SDK | ✓ | ✓ (`VULKAN_SDK` env var) |
| GLFW3 | ✓ (system package) | ✓ (`GLFW` env var) |
| glslc | ✓ (from Vulkan SDK) | ✓ (from Vulkan SDK) |

## Build

Both a shared library (`libblukpast.so` / `blukpast.dll`) and a static library (`libblukpastStatic.a` / `blukpastStatic.lib`) are built by default.

### Linux

```bash
git clone https://github.com/wypifu/blukpast.git
cd blukpast
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

To install system-wide:

```bash
sudo make install
```

To build only one variant:

```bash
cmake .. -DLINK_MODE=staticLink   # static only
cmake .. -DLINK_MODE=dynamicLink  # shared only
cmake .. -DLINK_MODE=bothLink     # both (default)
```

### Windows

Set the following environment variables before building:

- `VULKAN_SDK` — path to the Vulkan SDK (e.g. `C:\VulkanSDK\1.3.xxx.x`)
- `GLFW` — path to the GLFW directory (e.g. `C:\libs\glfw-3.x.x`)

Then build with **Clang** (recommended on Windows):

```bash
git clone https://github.com/wypifu/blukpast.git
cd blukpast
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -G "Ninja" -DCMAKE_C_COMPILER=clang
ninja
```

Or with **MinGW-w64**:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
mingw32-make -j4
```

## Linking your project

### CMake (recommended)

```cmake
# shared
find_library(BKP_LIB blukpast HINTS /path/to/blukpast/build)
target_link_libraries(myapp ${BKP_LIB})
target_include_directories(myapp PRIVATE /path/to/blukpast)

# or static
find_library(BKP_LIB blukpastStatic HINTS /path/to/blukpast/build)
target_link_libraries(myapp ${BKP_LIB} glfw vulkan pthread m)
```

### Windows (static)

```cmake
find_library(BKP_LIB blukpastStatic HINTS /path/to/blukpast/build)
target_link_libraries(myapp ${BKP_LIB} glfw3 vulkan-1 winmm)
```

## Usage

```c
#include <gfx/vulkan/include/vulkan.h>
#include <gfx/vulkan/include/vk_pipeline.h>
#include <loaders/include/bkp_loaders.h>
```

All public symbols are prefixed with `bkp` / `Bkp` and exported via `BKP_EXPORTED`.

## Tutorials

Each tutorial builds on the previous one. Run from the `build/` directory.

| # | Name | What it demonstrates |
|---|------|----------------------|
| 00 | Hello World | Swap chain, minimal graphics pipeline, push constants — the classic triangle |
| 01 | Playground | All built-in geometry types, PBR materials, GLTF loading, free camera |
| 02 | Camera | Free camera (WASD + mouse drag), sky box, dynamic viewport/scissor |
| 03 | PBR / IBL | Physically-based rendering with image-based lighting — irradiance map, pre-filtered environment, GGX BRDF LUT |
| 04 | Font & Input | HUD overlay, TTF font rendering, keyboard and mouse input callbacks |
| 05 | Shadow Map | Offline 4096×4096 depth pre-pass, PCF shadow sampling in the scene shader |
| 06 | Floor Reflection | Off-screen reflection pass, reflected view matrix, floor blending |
| 07 | Animations | Click-to-spin with ray-sphere picking, per-object spin state, dynamic shadow updates |
| 08 | Deformation | CPU-side spring-damper vertex deformation — drag to dent, release to shake |
| 09 | Destruction | Compute shader particle system — click past the speed threshold to explode an object, watch it respawn after 5 s |

## Hello Triangle

Shaders — compile with `glslc`:

```glsl
/* triangle.vert */
#version 450
const vec2 pos[3] = vec2[](vec2(-0.5, 0.5), vec2(0.5, 0.5), vec2(0.0, -0.5));
const vec4 col[3] = vec4[](vec4(1,0,0,1), vec4(0,1,0,1), vec4(0,0,1,1));
layout(location = 0) out vec4 fragColor;
void main() { gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0); fragColor = col[gl_VertexIndex]; }
```

```glsl
/* triangle.frag — animated RGB cycling via push constant */
#version 450
layout(push_constant) uniform PC { float time; } pc;
layout(location = 0) in  vec4 fragColor;
layout(location = 0) out vec4 color;
void main() {
    float p = fract(pc.time * 0.2) * 3.0;
    vec3 c = fragColor.rgb;
    vec3 col;
    if      (p < 1.0) col = mix(c,     c.gbr, smoothstep(0.0, 1.0, p));
    else if (p < 2.0) col = mix(c.gbr, c.brg, smoothstep(0.0, 1.0, p - 1.0));
    else              col = mix(c.brg, c,     smoothstep(0.0, 1.0, p - 2.0));
    color = vec4(col, 1.0);
}
```

The fragment shader cycles RGB channels between the three vertices over time.
`bkpCreatePipelineLayoutFromShader` reflects the push constant range automatically from the SPIR-V.

```c
/* triangle.c */
#include <include/blukpast.h>
#include <system/include/bkp_systemTime.h>

#define FRAMES 2
#define W 800
#define H 600

int main(void) {
    BkpContext ctx = {0};
    BkpConfig cfg = bkpDefaultConfig();
    cfg.vulkanContextInfo.winWidth         = W;
    cfg.vulkanContextInfo.winHeight        = H;
    cfg.vulkanContextInfo.maxFrameInFlight = FRAMES;
    bkpInit(&ctx, &cfg);

    BkpGpuAdapter adp = NULL;
    bkpActivateGpuAdapter(&ctx.vulkanContext, &adp, NULL);

    BkpSwapChain sc = {0};
    sc.info = bkpDefaultSwapChain(1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
              VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_TRUE, VK_PRESENT_MODE_FIFO_KHR);
    bkpCreateSwapChain(adp, &sc); /* also creates sync objects internally */

    BkpShaderModule vert = {0}, frag = {0};
    bkpCreateShaderModule(adp, "test/shaders/triangle.vert.spv", &vert);
    bkpCreateShaderModule(adp, "test/shaders/triangle.frag.spv", &frag);
    BkpShaderModule *mods[] = { &vert, &frag };
    BkpShaderProgram prog = {0};
    bkpCreateShader(adp, mods, 2, &prog);

    BkpPipelineGraphic gfx = {0};
    bkpCreatePipelineMinimalInfo(&gfx.info, (VkExtent2D){W, H});
    gfx.info.shaderProgram         = &prog;
    gfx.info.colorAttachmentFormat = sc.info.imageFormat;
    gfx.info.dynamicStates[0]      = VK_DYNAMIC_STATE_VIEWPORT;
    gfx.info.dynamicStates[1]      = VK_DYNAMIC_STATE_SCISSOR;
    gfx.info.dynamicState          = bkpDefaultDynamicStates(gfx.info.dynamicStates, 2);
    gfx.info.dynamicStateEnabled   = VK_TRUE;
    gfx.info.viewportState         = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
    bkpCreatePipelineLayoutFromShader(adp, &prog, &gfx.pipelineLayout); /* reflects push constants */
    bkpCreatePipelineGraphic(adp, &gfx);

    BkpCommandBuffer cmd = { .commandPool = &adp->commandPoolGraphic,
                              .count = FRAMES,
                              .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY };
    bkpAllocateCmdBuffer(adp, &cmd);

    while (!bkpWindowIsClosed(ctx.vulkanContext.win)) {
        bkpPollEvent(ctx.vulkanContext.win);
        bkpPrepareFrame(adp);
        uint32_t f = adp->frameInfo.currentFrame;
        uint32_t i = adp->frameInfo.imageAcquired;
        VkCommandBuffer vkcmd = cmd.cmds[f];

        bkpBeginCommandBuffer(&cmd, f, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        bkpImageBarrier(vkcmd, sc.images[i],
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,             0,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        bkpBeginRendering(vkcmd, sc.imageViews[i], (VkExtent2D){W, H},
                          (VkClearColorValue){{0.1f, 0.1f, 0.15f, 1.0f}});
        bkpUpdateWindowViewportScissor(vkcmd, W, H);
        bkpCmdBindPipeline(vkcmd, gfx.pipeline);
        float t = (float)bkp_time_getClockMilli() / 1000.0f;
        bkpCmdPushConstants(vkcmd, gfx.pipelineLayout.layout,
                            VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &t);
        bkpCmdDraw(vkcmd, 3, 0);
        bkpEndRendering(vkcmd);

        bkpImageBarrier(vkcmd, sc.images[i],
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,          0,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        bkpEndCommandBuffer(&cmd, f);
        bkpSubmitFrameBasic(adp, &cmd);
        bkpPresentFrame(adp);
    }

    bkpWaitDevice(adp);
    bkpDestroyCommandBuffers(adp, &cmd);
    bkpDestroyGraphicPipeline(adp, &gfx);
    bkpDestroyPipelineLayout(adp, &gfx.pipelineLayout);
    bkpDestroyShader(adp, &prog);
    bkpDestroyShaderModule(adp, &vert);
    bkpDestroyShaderModule(adp, &frag);
    bkpCleanupSwapChain(adp, &sc);
    bkpClearGpuAdapter(adp);
    bkpQuit(&ctx);
    return 0;
}
```

## Memory allocator

Blukpast provides three allocator modes configured through `BkpConfig`:

| Mode | Description |
|---|---|
| `eVMA_BKP` | Built-in chunk allocator (default) |
| `eVMA_CUSTOM` | User-supplied callbacks — plug in any allocator |
| `eVMA_NONE` | No allocator — buffer operations disabled (hello triangle, compute without buffers) |

```c
cfg.vulkanContextInfo.vmaMode = eVMA_NONE; /* disable allocator */
```

### Custom allocator (AMD VulkanMemoryAllocator example)

AMD VMA requires a `VkDevice` and `VkPhysicalDevice`, which are created inside
`bkpActivateGpuAdapter`. The setup is therefore a two-step process: init bkp first,
then create the VMA allocator and wire it in with `bkpSetupVmaVtable`.

```c
/* callbacks.c — compiled as C++ to use vk_mem_alloc.h */
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include <gfx/vulkan/include/vkAllocator.h>

static BkpBool my_allocBuffer(void * ud, VkDevice dev, VkPhysicalDevice pDev,
                               const VkBufferCreateInfo * info, EBufferType type,
                               VkBuffer * outBuf, VkDeviceMemory * outMem, void ** outAlloc)
{
    (void)dev; (void)pDev; (void)outMem;
    VmaAllocationCreateInfo ai = {0};
    ai.usage = (type == eBUFFER_GPU) ? VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
                                     : VMA_MEMORY_USAGE_AUTO;
    VmaAllocation alloc;
    if (vmaCreateBuffer((VmaAllocator)ud, info, &ai, outBuf, &alloc, NULL) != VK_SUCCESS)
        return BKP_FALSE;
    *outAlloc = alloc;
    return BKP_TRUE;
}

static void my_freeBuffer(void * ud, VkBuffer buf, VkDeviceMemory mem, void * alloc)
{
    (void)mem;
    vmaDestroyBuffer((VmaAllocator)ud, buf, (VmaAllocation)alloc);
}

static void * my_mapMemory(void * ud, void * alloc)
{
    void * data = NULL;
    vmaMapMemory((VmaAllocator)ud, (VmaAllocation)alloc, &data);
    return data;
}

static void my_unmapMemory(void * ud, void * alloc)
{
    vmaUnmapMemory((VmaAllocator)ud, (VmaAllocation)alloc);
}

static BkpBool my_allocImage(void * ud, VkDevice dev, VkPhysicalDevice pDev,
                              const VkImageCreateInfo * info, EBufferType type,
                              VkImage * outImg, VkDeviceMemory * outMem, void ** outAlloc)
{
    (void)dev; (void)pDev; (void)type; (void)outMem;
    VmaAllocationCreateInfo ai = {.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE};
    VmaAllocation alloc;
    if (vmaCreateImage((VmaAllocator)ud, info, &ai, outImg, &alloc, NULL) != VK_SUCCESS)
        return BKP_FALSE;
    *outAlloc = alloc;
    return BKP_TRUE;
}

static void my_freeImage(void * ud, VkImage img, VkDeviceMemory mem, void * alloc)
{
    (void)mem;
    vmaDestroyImage((VmaAllocator)ud, img, (VmaAllocation)alloc);
}
```

```c
/* main.c */
BkpContext ctx = {0};
BkpConfig cfg = bkpDefaultConfig();
cfg.vulkanContextInfo.vmaMode = eVMA_CUSTOM; /* skip built-in allocator */
bkpInit(&ctx, &cfg);

BkpGpuAdapter adp = NULL;
bkpActivateGpuAdapter(&ctx.vulkanContext, &adp, NULL);

/* VkDevice and VkPhysicalDevice are now available */
VmaAllocatorCreateInfo vmaInfo = {0};
vmaInfo.physicalDevice = adp->gpu;
vmaInfo.device         = adp->device;
vmaInfo.instance       = ctx.vulkanContext.instance;
VmaAllocator vma;
vmaCreateAllocator(&vmaInfo, &vma);

BkpVmaCallbacks cb = {
    .userData    = vma,
    .allocBuffer = my_allocBuffer,
    .freeBuffer  = my_freeBuffer,
    .mapMemory   = my_mapMemory,
    .unmapMemory = my_unmapMemory,
    .allocImage  = my_allocImage,
    .freeImage   = my_freeImage,
};
bkpSetupVmaVtable(adp, eVMA_CUSTOM, &cb);

/* buffer allocations from here use AMD VMA */
```

## License

LGPL v2.1 — you may use blukpast in proprietary projects without opening your source code.
If you modify blukpast itself, you must publish those modifications under the same license.
