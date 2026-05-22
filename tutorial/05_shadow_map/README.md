# Tutorial 05 — Shadow Map

Directional shadow mapping baked once at startup (light is fixed). The shadow map is never recomputed during the render loop, which keeps the per-frame cost minimal.

## Concepts

- **Shadow pass**: 4096×4096 `VK_FORMAT_D32_SFLOAT` depth image, rendered before the main loop
- **One-time bake**: single command buffer submitted with a fence — `bkpSubmit` + `bkpWaitForFence`
- **Comparison sampler**: `compareEnable = VK_TRUE`, `compareOp = VK_COMPARE_OP_LESS_OR_EQUAL`, `borderColor = OPAQUE_WHITE` — hardware PCF
- `lightViewProj`: orthographic projection from the light's point of view, passed as UBO field
- **Ground texture**: `images/bkpview_wallpaper2.png`, loaded with `bkpCreateTextureFromFile`
- Two pipelines for the main scene: shadow pipeline (depth-only, front-face culled) and scene pipeline (color + depth + shadow sampler)
- Image layout transitions: `UNDEFINED → DEPTH_ATTACHMENT → SHADER_READ_ONLY` for the shadow map, kept permanently in `SHADER_READ_ONLY` after bake

## What you see

The same geometry as previous tutorials, casting and receiving hard shadows on the ground. The ground displays a tiled texture.
