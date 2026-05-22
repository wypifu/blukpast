#ifndef BKP_INPUT_H_
#define BKP_INPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <system/include/bkp_export.h>
#include <system/include/bkp_bool.h>
#include <math/include/bkp_vect.h>

/* GLFW — ensure Vulkan surface types are available even if included standalone */
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

/* -------------------------------------------------------------------------
 * Enums
 * ------------------------------------------------------------------------- */
typedef enum
{
    eINPUT_RELEASED = GLFW_RELEASE,   /* 0 */
    eINPUT_PRESSED  = GLFW_PRESS,     /* 1 */
    eINPUT_REPEATED = GLFW_REPEAT,    /* 2 */
    eINPUT_MOTION   = 128,
} BkpInputAction;

typedef enum
{
    eMOUSE_BUTTON_1      = GLFW_MOUSE_BUTTON_1,
    eMOUSE_BUTTON_2      = GLFW_MOUSE_BUTTON_2,
    eMOUSE_BUTTON_3      = GLFW_MOUSE_BUTTON_3,
    eMOUSE_BUTTON_4      = GLFW_MOUSE_BUTTON_4,
    eMOUSE_BUTTON_5      = GLFW_MOUSE_BUTTON_5,
    eMOUSE_BUTTON_6      = GLFW_MOUSE_BUTTON_6,
    eMOUSE_BUTTON_7      = GLFW_MOUSE_BUTTON_7,
    eMOUSE_BUTTON_8      = GLFW_MOUSE_BUTTON_8,
    eMOUSE_BUTTON_LEFT   = GLFW_MOUSE_BUTTON_LEFT,
    eMOUSE_BUTTON_RIGHT  = GLFW_MOUSE_BUTTON_RIGHT,
    eMOUSE_BUTTON_MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
    eMOUSE_MOTION        = 32,
    eMOUSE_SCROLL,
    eMOUSE_COUNT
} BkpInputMouse;

typedef enum
{
    eKB_UNKNOWN        = GLFW_KEY_UNKNOWN,     /* -1 */

    eKB_SPACE          = GLFW_KEY_SPACE,
    eKB_APOSTROPHE     = GLFW_KEY_APOSTROPHE,
    eKB_COMMA          = GLFW_KEY_COMMA,
    eKB_MINUS          = GLFW_KEY_MINUS,
    eKB_PERIOD         = GLFW_KEY_PERIOD,
    eKB_SLASH          = GLFW_KEY_SLASH,
    eKB_0              = GLFW_KEY_0,
    eKB_1              = GLFW_KEY_1,
    eKB_2              = GLFW_KEY_2,
    eKB_3              = GLFW_KEY_3,
    eKB_4              = GLFW_KEY_4,
    eKB_5              = GLFW_KEY_5,
    eKB_6              = GLFW_KEY_6,
    eKB_7              = GLFW_KEY_7,
    eKB_8              = GLFW_KEY_8,
    eKB_9              = GLFW_KEY_9,
    eKB_SEMICOLON      = GLFW_KEY_SEMICOLON,
    eKB_EQUAL          = GLFW_KEY_EQUAL,
    eKB_A              = GLFW_KEY_A,
    eKB_B              = GLFW_KEY_B,
    eKB_C              = GLFW_KEY_C,
    eKB_D              = GLFW_KEY_D,
    eKB_E              = GLFW_KEY_E,
    eKB_F              = GLFW_KEY_F,
    eKB_G              = GLFW_KEY_G,
    eKB_H              = GLFW_KEY_H,
    eKB_I              = GLFW_KEY_I,
    eKB_J              = GLFW_KEY_J,
    eKB_K              = GLFW_KEY_K,
    eKB_L              = GLFW_KEY_L,
    eKB_M              = GLFW_KEY_M,
    eKB_N              = GLFW_KEY_N,
    eKB_O              = GLFW_KEY_O,
    eKB_P              = GLFW_KEY_P,
    eKB_Q              = GLFW_KEY_Q,
    eKB_R              = GLFW_KEY_R,
    eKB_S              = GLFW_KEY_S,
    eKB_T              = GLFW_KEY_T,
    eKB_U              = GLFW_KEY_U,
    eKB_V              = GLFW_KEY_V,
    eKB_W              = GLFW_KEY_W,
    eKB_X              = GLFW_KEY_X,
    eKB_Y              = GLFW_KEY_Y,
    eKB_Z              = GLFW_KEY_Z,
    eKB_LEFT_BRACKET   = GLFW_KEY_LEFT_BRACKET,
    eKB_BACKSLASH      = GLFW_KEY_BACKSLASH,
    eKB_RIGHT_BRACKET  = GLFW_KEY_RIGHT_BRACKET,
    eKB_GRAVE_ACCENT   = GLFW_KEY_GRAVE_ACCENT,
    eKB_ESCAPE         = GLFW_KEY_ESCAPE,
    eKB_ENTER          = GLFW_KEY_ENTER,
    eKB_TAB            = GLFW_KEY_TAB,
    eKB_BACKSPACE      = GLFW_KEY_BACKSPACE,
    eKB_INSERT         = GLFW_KEY_INSERT,
    eKB_DELETE         = GLFW_KEY_DELETE,
    eKB_RIGHT          = GLFW_KEY_RIGHT,
    eKB_LEFT           = GLFW_KEY_LEFT,
    eKB_DOWN           = GLFW_KEY_DOWN,
    eKB_UP             = GLFW_KEY_UP,
    eKB_PAGE_UP        = GLFW_KEY_PAGE_UP,
    eKB_PAGE_DOWN      = GLFW_KEY_PAGE_DOWN,
    eKB_HOME           = GLFW_KEY_HOME,
    eKB_END            = GLFW_KEY_END,
    eKB_CAPS_LOCK      = GLFW_KEY_CAPS_LOCK,
    eKB_SCROLL_LOCK    = GLFW_KEY_SCROLL_LOCK,
    eKB_NUM_LOCK       = GLFW_KEY_NUM_LOCK,
    eKB_PRINT_SCREEN   = GLFW_KEY_PRINT_SCREEN,
    eKB_PAUSE          = GLFW_KEY_PAUSE,
    eKB_F1             = GLFW_KEY_F1,
    eKB_F2             = GLFW_KEY_F2,
    eKB_F3             = GLFW_KEY_F3,
    eKB_F4             = GLFW_KEY_F4,
    eKB_F5             = GLFW_KEY_F5,
    eKB_F6             = GLFW_KEY_F6,
    eKB_F7             = GLFW_KEY_F7,
    eKB_F8             = GLFW_KEY_F8,
    eKB_F9             = GLFW_KEY_F9,
    eKB_F10            = GLFW_KEY_F10,
    eKB_F11            = GLFW_KEY_F11,
    eKB_F12            = GLFW_KEY_F12,
    eKB_KP_0           = GLFW_KEY_KP_0,
    eKB_KP_1           = GLFW_KEY_KP_1,
    eKB_KP_2           = GLFW_KEY_KP_2,
    eKB_KP_3           = GLFW_KEY_KP_3,
    eKB_KP_4           = GLFW_KEY_KP_4,
    eKB_KP_5           = GLFW_KEY_KP_5,
    eKB_KP_6           = GLFW_KEY_KP_6,
    eKB_KP_7           = GLFW_KEY_KP_7,
    eKB_KP_8           = GLFW_KEY_KP_8,
    eKB_KP_9           = GLFW_KEY_KP_9,
    eKB_KP_DECIMAL     = GLFW_KEY_KP_DECIMAL,
    eKB_KP_DIVIDE      = GLFW_KEY_KP_DIVIDE,
    eKB_KP_MULTIPLY    = GLFW_KEY_KP_MULTIPLY,
    eKB_KP_SUBTRACT    = GLFW_KEY_KP_SUBTRACT,
    eKB_KP_ADD         = GLFW_KEY_KP_ADD,
    eKB_KP_ENTER       = GLFW_KEY_KP_ENTER,
    eKB_KP_EQUAL       = GLFW_KEY_KP_EQUAL,
    eKB_LEFT_SHIFT     = GLFW_KEY_LEFT_SHIFT,
    eKB_LEFT_CONTROL   = GLFW_KEY_LEFT_CONTROL,
    eKB_LEFT_ALT       = GLFW_KEY_LEFT_ALT,
    eKB_RIGHT_SHIFT    = GLFW_KEY_RIGHT_SHIFT,
    eKB_RIGHT_CONTROL  = GLFW_KEY_RIGHT_CONTROL,
    eKB_RIGHT_ALT      = GLFW_KEY_RIGHT_ALT,
    eKB_MENU           = GLFW_KEY_MENU,         /* 348 */

    eINPUT_ANY,          /* catch-all: fires for every key event */
    eINPUTCODE_COUNT
} BkpInputCode;

/* -------------------------------------------------------------------------
 * State delivered to callbacks
 * ------------------------------------------------------------------------- */
typedef struct
{
    union
    {
        BkpInputCode  key;
        BkpInputMouse mouse;
    };
    BkpVec2        position; /* cursor position (pixels) */
    BkpVec2        diff;     /* delta since last event   */
    BkpInputAction action;
} BkpInputState;

typedef BkpBool (* BkpInputCallBack)(BkpInputState state, void * data);

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/* Register a callback for a specific keyboard key.
 * Only one callback per key — calling again overwrites the previous one. */
BKP_EXPORTED void bkpAddKeyBoardAction(BkpInputCode code, BkpInputCallBack cb, void * data);

/* Register a callback for a mouse button / motion / scroll event.
 * Only one callback per event type. */
BKP_EXPORTED void bkpAddMouseAction(BkpInputMouse code, BkpInputCallBack cb, void * data);

BKP_EXPORTED void bkpRemoveKeyBoardAction(BkpInputCode code);
BKP_EXPORTED void bkpRemoveMouseAction(BkpInputMouse code);

BKP_EXPORTED const char * bkpGetInputActionName(BkpInputAction action);
BKP_EXPORTED const char * bkpGetInputKeyName(BkpInputCode code);

/* -------------------------------------------------------------------------
 * Internal — called by bkpOpenWindow, not for user code
 * ------------------------------------------------------------------------- */
void bkpInitInputSystem(GLFWwindow * win);
void bkpClearInputSystem(void);

#ifdef __cplusplus
}
#endif

#endif /* BKP_INPUT_H_ */
