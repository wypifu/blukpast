#ifndef WINDOWING_H_
#define WINDOWING_H_

#define GLFW_INCLUDE_VULKAN


#ifdef __cplusplus
extern "C" {
#endif

#include <GLFW/glfw3.h>
#include <system/include/bkp_export.h>

/*********************************************************************
 * Defines
*********************************************************************/

/*********************************************************************
 * Type def & enum
*********************************************************************/

/*********************************************************************
 * Macro
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/
typedef enum {eWIN_NONE, eWIN_GLFW, eWIN_XCB, eWIN_WAYLAND, eWIN_WIN32, eWIN_QT, eWIN_TYPE_COUNT} EWindowType ;

/*********************************************************************
 * Global
*********************************************************************/


/*********************************************************************
 * Struct
*********************************************************************/
typedef struct
{
    void * window;
    EWindowType type;
	//EApi api;
    uint32_t width;
    uint32_t height;
}BkpWindow;

/*********************************************************************
 * Functions
*********************************************************************/
BKP_EXPORTED void bkpWindowSetResizedCallBack(void (* fucn)(GLFWwindow * window, int width, int height));
/** @brief Allow or forbid window resizing (must be called before bkpWindowInitVulkanWindow). */
BKP_EXPORTED void bkpWindowEnableResizable(unsigned char bValue);
/** @brief Show or hide the window title bar and borders (must be called before bkpWindowInitVulkanWindow). */
BKP_EXPORTED void bkpWindowEnableDecoration(unsigned char bValue);

BKP_EXPORTED int32_t bkpWindowInitOpenglWindow(int32_t V, int32_t v);

/**
 * @brief Initialize GLFW for Vulkan (`GLFW_NO_API`).
 *
 * Sets the window hints required for Vulkan.  Called automatically by
 * `bkpInitVulkanContext` when `BkpVulkanContextInfo::win` is NULL, so you
 * rarely need to call it directly.
 *
 * @return 0 on success, 0 on error (check the log).
 */
BKP_EXPORTED int32_t bkpWindowInitVulkanWindow(void);

/** @brief Calls `glfwTerminate`.  Call after `bkpClearContext`. */
BKP_EXPORTED void bkpTerminateWindow(void);

BKP_EXPORTED BkpWindow * bkpCreateWindow(EWindowType type);
BKP_EXPORTED void bkpUpdateInputs(BkpWindow * win);
/**
 * @brief Return non-zero if the window's close flag has been set.
 *
 * Wraps `glfwWindowShouldClose`.  Use as the main-loop condition:
 * @code
 *   while (!bkpWindowIsClosed(ctx.vulkanContext.win)) { ... }
 * @endcode
 *
 * @param win  Window to query.
 * @return     Non-zero when the user has requested closure (title-bar ×,
 *             Alt-F4, etc.), zero while the window should remain open.
 */
BKP_EXPORTED unsigned char bkpWindowIsClosed(BkpWindow * win);
BKP_EXPORTED void bkpDestroyWindow(BkpWindow ** win);

/**
 * @brief Create the GLFW window and register it as the global window.
 *
 * Must be preceded by `bkpWindowInitVulkanWindow`.  Not needed when
 * `BkpVulkanContextInfo::win` is NULL — `bkpInitVulkanContext` calls this
 * internally.
 *
 * @param w0         Width in pixels; 0 → native monitor resolution.
 * @param h0         Height in pixels; 0 → native monitor resolution.
 * @param sztitle    Window title.
 * @param fullscreen Non-zero to use the primary monitor in fullscreen.
 * @return           The created GLFWwindow, also accessible via `bkpGetWindow()`.
 *                   Calls `DIE_L` (aborts) if window creation fails.
 */
BKP_EXPORTED GLFWwindow * bkpOpenWindow(uint32_t w0, uint32_t h0, const char * sztitle, unsigned char fullscreen);

/**
 * @brief Return the global GLFW window created by `bkpOpenWindow` or `bkpInitVulkanContext`.
 * @return GLFWwindow pointer, or NULL if no window has been created yet.
 */
BKP_EXPORTED GLFWwindow * bkpGetWindow(void);

BKP_EXPORTED void bkpGetWindowSize(uint32_t * w, uint32_t * h);

/** @brief Poll pending window and input events.  Call once per frame before `bkpPrepareFrame`. */
BKP_EXPORTED void bkpPollEvent(BkpWindow * win);

BKP_EXPORTED unsigned char bkpWindowGetFrameBufferResized(void);
BKP_EXPORTED void bkpWindowResetFramebufferResized(void);
BKP_EXPORTED void bkpWindowWaitEventFrameBufferSize(int32_t * w, int32_t *h);

#ifdef __cplusplus
}
#endif

#endif

