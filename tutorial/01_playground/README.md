# Tutorial 01 — Playground

Showcase of all built-in procedural geometries. Introduces the `common/scene.h` helper layer used by all subsequent tutorials.

## Concepts

- `eBKP_GEO_SPHERE`, `CUBE`, `CYLINDER`, `CONE`, `TORUS`, `TUBE`, `DISK_PLANE` — built-in geometry types
- `bkpCreateVertexIndexBuffer` — upload geometry to GPU in one call
- `BkpBufferGPU` — uniform buffer (UBO) with per-frame slots for double-buffering
- `BkpDescriptorPool` / `BkpDescriptorSet` — descriptor allocation and write
- Push constants for per-object model matrix, color, roughness, metallic
- Sky box: inverted sphere with a procedural gradient fragment shader
- Depth buffer: `bkpDefaultDepthBuffer` / `bkpCreateDepthResources`

## What you see

1280×720 window with seven primitives on a ground plane, lit by a single directional light using a basic diffuse + specular model. A procedural sky fills the background.
