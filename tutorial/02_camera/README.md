# Tutorial 02 — Camera

First-person free-fly camera wired directly to GLFW — intentionally without the bkp input API, to show the raw integration path.

## Controls

| Key / Button | Action |
|---|---|
| W / S | Move forward / backward |
| A / D | Strafe left / right |
| E / Q | Rise / fall |
| R | Reset to initial position |
| Right-click drag | Look around |

## Concepts

- `FreeCamera` struct — pos + yaw/pitch angles, frame-rate independent via `dt`
- `glfwGetKey`, `glfwGetMouseButton`, `glfwGetCursorPos` used directly
- `bkp_time_getClockMicro` — high-resolution delta time
- `bkpLookAt` / `bkpPerspective` — view and projection matrices
- UBO upload each frame with `bkpUploadBufferData`
