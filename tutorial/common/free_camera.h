#ifndef TUTORIAL_FREE_CAMERA_H_
#define TUTORIAL_FREE_CAMERA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <include/blukpast.h>
#include "scene.h"
#include <math.h>

/* =========================================================================
 * Free camera — keyboard movement + right-click drag to rotate
 *
 * Controls: WASD (horizontal), E/Q (up/down), right-click drag (rotate), R (reset).
 * Uses bkp_input callbacks — no direct GLFW polling.
 * ========================================================================= */
typedef struct
{
    float pos[3];
    float yaw, pitch;
    float speed, sensitivity;
    float defaultPos[3];
    float defaultYaw, defaultPitch;

    /* bkp_input state (written by callbacks, consumed by updateFreeCamera) */
    int moveW, moveS, moveA, moveD, moveE, moveQ;
    int midHeld;
    float mouseDX, mouseDY;
    int doReset;
} FreeCamera;

static FreeCamera makeFreeCamera(float x, float y, float z, float yaw, float pitch)
{
    FreeCamera fc = {0};
    fc.pos[0] = x; fc.pos[1] = y; fc.pos[2] = z;
    fc.yaw = yaw; fc.pitch = pitch;
    fc.speed = 5.0f; fc.sensitivity = 0.003f;
    fc.defaultPos[0] = x; fc.defaultPos[1] = y; fc.defaultPos[2] = z;
    fc.defaultYaw = yaw; fc.defaultPitch = pitch;
    return fc;
}

static void freeCameraVectors(FreeCamera * fc, float fwd[3], float right[3])
{
    float cy = cosf(fc->yaw), sy = sinf(fc->yaw);
    float cp = cosf(fc->pitch);
    fwd[0] =  sy * cp;
    fwd[1] =  sinf(fc->pitch);
    fwd[2] = -cy * cp;
    right[0] = cy; right[1] = 0.0f; right[2] = sy;
}

static BkpBool freecam_cb_w(BkpInputState s, void * d) { ((FreeCamera*)d)->moveW = (s.action != eINPUT_RELEASED); return BKP_TRUE; }
static BkpBool freecam_cb_s(BkpInputState s, void * d) { ((FreeCamera*)d)->moveS = (s.action != eINPUT_RELEASED); return BKP_TRUE; }
static BkpBool freecam_cb_a(BkpInputState s, void * d) { ((FreeCamera*)d)->moveA = (s.action != eINPUT_RELEASED); return BKP_TRUE; }
static BkpBool freecam_cb_d(BkpInputState s, void * d) { ((FreeCamera*)d)->moveD = (s.action != eINPUT_RELEASED); return BKP_TRUE; }
static BkpBool freecam_cb_e(BkpInputState s, void * d) { ((FreeCamera*)d)->moveE = (s.action != eINPUT_RELEASED); return BKP_TRUE; }
static BkpBool freecam_cb_q(BkpInputState s, void * d) { ((FreeCamera*)d)->moveQ = (s.action != eINPUT_RELEASED); return BKP_TRUE; }

static BkpBool freecam_cb_r(BkpInputState s, void * d)
{
    if(s.action == eINPUT_PRESSED) { ((FreeCamera*)d)->doReset = 1; }
    return BKP_TRUE;
}

static BkpBool freecam_cb_mid(BkpInputState s, void * d)
{
    ((FreeCamera*)d)->midHeld = (s.action == eINPUT_PRESSED);
    return BKP_TRUE;
}

static BkpBool freecam_cb_motion(BkpInputState s, void * d)
{
    FreeCamera * fc = (FreeCamera *)d;
    if(fc->midHeld)
    {
        fc->mouseDX += s.diff.x;
        fc->mouseDY += s.diff.y;
    }
    return BKP_TRUE;
}

static void registerCameraInput(FreeCamera * fc)
{
    bkpAddKeyBoardAction(eKB_W, freecam_cb_w, fc);
    bkpAddKeyBoardAction(eKB_S, freecam_cb_s, fc);
    bkpAddKeyBoardAction(eKB_A, freecam_cb_a, fc);
    bkpAddKeyBoardAction(eKB_D, freecam_cb_d, fc);
    bkpAddKeyBoardAction(eKB_E, freecam_cb_e, fc);
    bkpAddKeyBoardAction(eKB_Q, freecam_cb_q, fc);
    bkpAddKeyBoardAction(eKB_R, freecam_cb_r, fc);
    bkpAddMouseAction(eMOUSE_BUTTON_RIGHT, freecam_cb_mid,    fc);
    bkpAddMouseAction(eMOUSE_MOTION,        freecam_cb_motion, fc);
}

static void unregisterCameraInput(void)
{
    bkpRemoveKeyBoardAction(eKB_W);
    bkpRemoveKeyBoardAction(eKB_S);
    bkpRemoveKeyBoardAction(eKB_A);
    bkpRemoveKeyBoardAction(eKB_D);
    bkpRemoveKeyBoardAction(eKB_E);
    bkpRemoveKeyBoardAction(eKB_Q);
    bkpRemoveKeyBoardAction(eKB_R);
    bkpRemoveMouseAction(eMOUSE_BUTTON_RIGHT);
    bkpRemoveMouseAction(eMOUSE_MOTION);
}

static void updateFreeCamera(FreeCamera * fc, float dt)
{
    float fwd[3], right[3];
    freeCameraVectors(fc, fwd, right);
    float spd = fc->speed * dt;

    if(fc->moveW) { fc->pos[0] += fwd[0]*spd; fc->pos[1] += fwd[1]*spd; fc->pos[2] += fwd[2]*spd; }
    if(fc->moveS) { fc->pos[0] -= fwd[0]*spd; fc->pos[1] -= fwd[1]*spd; fc->pos[2] -= fwd[2]*spd; }
    if(fc->moveA) { fc->pos[0] -= right[0]*spd; fc->pos[2] -= right[2]*spd; }
    if(fc->moveD) { fc->pos[0] += right[0]*spd; fc->pos[2] += right[2]*spd; }
    if(fc->moveE) { fc->pos[1] += spd; }
    if(fc->moveQ) { fc->pos[1] -= spd; }

    if(fc->doReset)
    {
        fc->pos[0] = fc->defaultPos[0]; fc->pos[1] = fc->defaultPos[1]; fc->pos[2] = fc->defaultPos[2];
        fc->yaw = fc->defaultYaw; fc->pitch = fc->defaultPitch;
        fc->doReset = 0;
    }

    if(fc->mouseDX != 0.0f || fc->mouseDY != 0.0f)
    {
        fc->yaw   += fc->mouseDX * fc->sensitivity;
        fc->pitch -= fc->mouseDY * fc->sensitivity;
        if(fc->pitch >  1.5f) { fc->pitch =  1.5f; }
        if(fc->pitch < -1.5f) { fc->pitch = -1.5f; }
        fc->mouseDX = 0.0f;
        fc->mouseDY = 0.0f;
    }
}

static void cameraFromFree(FreeCamera * fc, Camera * cam)
{
    float fwd[3], right[3];
    freeCameraVectors(fc, fwd, right);
    cam->pos[0] = fc->pos[0]; cam->pos[1] = fc->pos[1]; cam->pos[2] = fc->pos[2];
    cam->target[0] = fc->pos[0] + fwd[0];
    cam->target[1] = fc->pos[1] + fwd[1];
    cam->target[2] = fc->pos[2] + fwd[2];
}

#ifdef __cplusplus
}
#endif

#endif /* TUTORIAL_FREE_CAMERA_H_ */
