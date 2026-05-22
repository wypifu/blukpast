# Tutorial 04 — Font & Input

On-screen HUD with FPS counter and a help overlay, using TTF font rendering via stb_truetype. Introduces the bkp input API (`registerCameraInput` / `updateFreeCamera`) used by all subsequent tutorials.

## Controls

| Key | Action |
|---|---|
| W / S / A / D | Move |
| E / Q | Rise / fall |
| R | Reset camera |
| Right-click drag | Look around |
| H | Toggle help overlay |

## Concepts

- `bkp_font` — TTF atlas baking, glyph quad generation
- HUD pipeline: separate render pass over the main color attachment, no depth write
- `registerCameraInput` / `updateFreeCamera` / `unregisterCameraInput` — bkp input abstraction (wraps GLFW callbacks internally)
- `hudUpdate(dt)` / `hudDraw` — stateful HUD helper in `common/hud.c`
- Font: `AdwaitaMono-Regular.ttf` (included in this folder)
