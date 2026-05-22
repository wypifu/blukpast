# Tutorial 00 — Hello World

Minimum viable bkp application: a triangle and a sphere, no vertex buffers.

## Concepts

- `bkpInit` / `bkpQuit` — library lifecycle
- `bkpActivateGpuAdapter` — GPU selection
- `BkpSwapChain` — presentable image chain
- `BkpShaderModule` / `BkpShaderProgram` — SPIR-V loading + reflection
- `bkpCreatePipelineLayoutFromShader` — push constant range reflected from SPIR-V
- `BkpCommandBuffer` — allocation and per-frame recording
- `bkpPrepareFrame` / `bkpSubmitFrameBasic` / `bkpPresentFrame` — frame loop
- Image barriers (sync2): `UNDEFINED → COLOR_ATTACHMENT → PRESENT_SRC`

## What you see

800×600 window. The triangle's RGB channels cycle smoothly over time using a `float time` push constant, reflected automatically from the fragment shader SPIR-V. The sphere uses hardcoded vertex positions in the vertex shader — no vertex buffer needed.
