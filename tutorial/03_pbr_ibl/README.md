# Tutorial 03 — PBR / IBL

Image-Based Lighting with a procedural environment. All IBL resources (sky cubemap, irradiance map, BRDF LUT) are generated on the CPU at startup and uploaded once — no precomputed files required.

## Concepts

- **Sky cubemap** (256×256): procedural gradient sampled as `samplerCube`
- **Irradiance map** (32×32): cosine-weighted convolution of the sky, used for diffuse IBL
- **BRDF LUT** (128×128): split-sum GGX look-up table for specular IBL
- **Damier texture**: checkerboard pattern used as ground albedo
- `iblInitSkyCubemap`, `iblInitIrradianceMap`, `iblInitBrdfLut` — see `ibl.c` / `ibl.h`
- Per-object roughness and metallic values exposed as push constants

## Camera

Same `FreeCamera` as tutorial 02 — GLFW used directly (right-click drag to look, WASD to move, R to reset). This is intentional: it shows that GLFW can be used alongside bkp without going through the bkp input API.

## What you see

Seven PBR primitives with varied roughness/metallic combinations, reflecting the procedural sky environment.
