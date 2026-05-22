#include <system/include/bkp_input.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Internal entry — one per key/mouse event type
 * ------------------------------------------------------------------------- */
typedef struct
{
    BkpInputCallBack cb;
    BkpInputState    state;
    void *           data;
} InputEntry;

static InputEntry s_keys[eINPUTCODE_COUNT];
static InputEntry s_mouse[eMOUSE_COUNT];

/* -------------------------------------------------------------------------
 * GLFW callbacks (static — not part of public API)
 * ------------------------------------------------------------------------- */
static void cb_key(GLFWwindow * win, int key, int scancode, int action, int mods)
{
    (void)win; (void)scancode; (void)mods;

    /* dispatch to specific key */
    if(key >= 0 && key < (int)eINPUT_ANY)
    {
        /* only fire on state change (skip repeated GLFW_REPEAT if same) */
        if(s_keys[key].state.action != (BkpInputAction)action)
        {
            s_keys[key].state.action = (BkpInputAction)action;
            s_keys[key].state.key    = (BkpInputCode)key;
            if(s_keys[key].cb) { s_keys[key].cb(s_keys[key].state, s_keys[key].data); }
        }
    }

    /* catch-all */
    if(s_keys[eINPUT_ANY].cb)
    {
        s_keys[eINPUT_ANY].state.action = (BkpInputAction)action;
        s_keys[eINPUT_ANY].state.key    = (BkpInputCode)key;
        s_keys[eINPUT_ANY].cb(s_keys[eINPUT_ANY].state, s_keys[eINPUT_ANY].data);
    }
}

static void cb_cursor(GLFWwindow * win, double x, double y)
{
    (void)win;
    InputEntry * e = &s_mouse[eMOUSE_MOTION];
    if(!e->cb) { return; }
    BkpVec2 prev        = e->state.position;
    e->state.mouse      = eMOUSE_MOTION;
    e->state.action     = eINPUT_MOTION;
    e->state.diff       = bkpVec2((float)(x - prev.x), (float)(y - prev.y));
    e->state.position   = bkpVec2((float)x, (float)y);
    e->cb(e->state, e->data);
}

static void cb_mouse_button(GLFWwindow * win, int button, int action, int mods)
{
    (void)mods;
    if(button < 0 || button >= (int)eMOUSE_MOTION) { return; }
    InputEntry * e = &s_mouse[button];
    if(!e->cb) { return; }
    double x, y;
    glfwGetCursorPos(win, &x, &y);
    e->state.mouse    = (BkpInputMouse)button;
    e->state.action   = (BkpInputAction)action;
    e->state.position = bkpVec2((float)x, (float)y);
    e->state.diff     = bkpVec2(0.0f, 0.0f);
    e->cb(e->state, e->data);
}

static void cb_scroll(GLFWwindow * win, double dx, double dy)
{
    (void)win;
    InputEntry * e = &s_mouse[eMOUSE_SCROLL];
    if(!e->cb) { return; }
    e->state.mouse = eMOUSE_SCROLL;
    e->state.diff  = bkpVec2((float)dx, (float)dy);
    e->cb(e->state, e->data);
}

/* -------------------------------------------------------------------------
 * Init / clear
 * ------------------------------------------------------------------------- */
void bkpInitInputSystem(GLFWwindow * win)
{
    memset(s_keys,  0, sizeof(s_keys));
    memset(s_mouse, 0, sizeof(s_mouse));
    glfwSetKeyCallback(win,         cb_key);
    glfwSetCursorPosCallback(win,   cb_cursor);
    glfwSetMouseButtonCallback(win, cb_mouse_button);
    glfwSetScrollCallback(win,      cb_scroll);
}

void bkpClearInputSystem(void)
{
    memset(s_keys,  0, sizeof(s_keys));
    memset(s_mouse, 0, sizeof(s_mouse));
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */
void bkpAddKeyBoardAction(BkpInputCode code, BkpInputCallBack cb, void * data)
{
    if((int)code >= 0 && code < eINPUTCODE_COUNT)
    {
        s_keys[code].cb   = cb;
        s_keys[code].data = data;
    }
}

void bkpAddMouseAction(BkpInputMouse code, BkpInputCallBack cb, void * data)
{
    if(code >= 0 && code < eMOUSE_COUNT)
    {
        s_mouse[code].cb   = cb;
        s_mouse[code].data = data;
    }
}

void bkpRemoveKeyBoardAction(BkpInputCode code)
{
    if((int)code >= 0 && code < eINPUTCODE_COUNT)
    {
        s_keys[code].cb   = NULL;
        s_keys[code].data = NULL;
    }
}

void bkpRemoveMouseAction(BkpInputMouse code)
{
    if(code >= 0 && code < eMOUSE_COUNT)
    {
        s_mouse[code].cb   = NULL;
        s_mouse[code].data = NULL;
    }
}

const char * bkpGetInputActionName(BkpInputAction action)
{
    switch(action)
    {
        case eINPUT_RELEASED: return "RELEASED";
        case eINPUT_PRESSED:  return "PRESSED";
        case eINPUT_REPEATED: return "REPEATED";
        case eINPUT_MOTION:   return "MOTION";
        default:              return "UNKNOWN";
    }
}

const char * bkpGetInputKeyName(BkpInputCode code)
{
    return glfwGetKeyName((int)code, 0);
}

#ifdef __cplusplus
}
#endif
