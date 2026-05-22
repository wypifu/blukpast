# Tutorial 06 — Floor Reflection

Planar reflection rendered into an offscreen color buffer, blended onto the floor with a Schlick Fresnel approximation.

## Controls

| Key | Action |
|---|---|
| W / S / A / D | Move |
| E / Q | Rise / fall |
| R | Reset camera |
| Right-click drag | Look around |
| H | Toggle help overlay |

## Concepts

- **Planar reflection**: the scene is rendered a second time each frame from a camera mirrored across the Y=0 plane (`eye.y → -eye.y`, `target.y → -target.y`)
- **`gl_ClipDistance[0]`**: the reflect vertex shader sets `gl_ClipDistance[0] = worldPos.y`; the GPU discards fragments where the value is negative, so only geometry above the floor appears in the reflection
- **`shaderClipDistance` feature**: requested via `BkpFeatureHint` before calling `bkpActivateGpuAdapter`
- **`bkpDefaultColorBuffer()`**: helper that pre-fills a `BkpImageResource` for an offscreen color render target (`COLOR_ATTACHMENT | SAMPLED`, device-local)
- **Winding reversal**: Y-mirroring reverses triangle winding; the reflect pipeline uses `VK_CULL_MODE_FRONT_BIT` to compensate
- **Screen-space UV**: the floor vertex shader passes `vClipPos = gl_Position`; the fragment shader computes `uv = (vClipPos.xy / vClipPos.w) * 0.5 + 0.5` to look up the reflection texture
- **Schlick Fresnel**: `F = 0.02 + 0.98 * pow(1 - NdotV, 5)` — more reflection at grazing angles; attenuated by surface roughness: `reflAmount = F * (1 - roughness)`
- **Two UBO slots per frame**: main camera at `uboBuffer[f]`, reflected camera at `uboBuffer[FRAMES + f]`

## What you see

The same geometry as previous tutorials, reflected in a shiny tiled floor. The reflection fades at steep viewing angles (low Fresnel) and disappears entirely for rough floor materials. Lowering the floor `roughness` to 0 gives a near-perfect mirror.
