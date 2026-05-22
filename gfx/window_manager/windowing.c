#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>

/*Copyright*/
/*
 * AUTHOR: lilington
 * addr	: lilington@yahoo.fr
 *
 */

#include "include/windowing.h"
#include <system/include/macro.h>
#include <system/include/bkp_log.h>
#include <system/include/bkp_input.h>

/**************************************************************************
 *	Defines & Maro
 **************************************************************************/

/**************************************************************************
 *	Structs, Enum, Unio and Typesdef
 **************************************************************************/

/**************************************************************************
 *	Globals
 **************************************************************************/

static const char * bkp_window_TAG = "glfw";
static int32_t stc_win_width= 1920;
static int32_t stc_win_height = 1080;
static unsigned char stc_frameBuffer_resized = 0;;
static GLFWwindow * stc_window = NULL;

static unsigned char stc_win_resizable = 0;
static unsigned char stc_win_decorated = 1;

void (* stc_customResizedCallback)(GLFWwindow * window, int width, int height) = NULL;

/***************************************************************************
 *	Prototypes
 **************************************************************************/
static void error_callback_glfw(int error,const char * description);
static void window_size_callback_glfw(GLFWwindow * window, int width, int height);
static void framebuffer_size_callback_glfw(GLFWwindow * window, int width, int height);

/**************************************************************************
 *	Main
 **************************************************************************/



/**************************************************************************
 *	Implementations
 **************************************************************************/
void bkpWindowSetResizedCallBack(void (* func)(GLFWwindow * window, int width, int height))
{
    stc_customResizedCallback = func;
	bkpWindowEnableResizable(1);
}

/*_________________________________________________________________________________*/
void bkpWindowEnableResizable(unsigned char bValue)
{
    stc_win_resizable = bValue;
}

/*_________________________________________________________________________________*/
void bkpWindowEnableDecoration(unsigned char bValue)
{
    stc_win_decorated = bValue;
}

/*_________________________________________________________________________________*/
int bkpWindowInitOpenglWindow(int V, int v)
{
    /*
       glfwSetErrorCallback(glfw_error_callback);
       if(GLFW_FALSE == glfwInit())
       {
       write(BKP_LOGGER_FATAL, TAG, "Could not Init GLFW3!\n");
       return -1;
       }

       glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,V);
       glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,v);
       glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
       glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
       glfwWindowHint(GLFW_SAMPLES, 4); // anti-aliasing  4 is ok
       */

    return 0;
}

/*_________________________________________________________________________________*/
unsigned char bkpWindowIsClosed(BkpWindow * win)
{
	if(win->type == eWIN_GLFW)
	{
		return glfwWindowShouldClose((GLFWwindow *)win->window);
	}

	return 0;
}

/*____________________________________________________________________________________*/
void bkpUpdateInputs(BkpWindow * win)
{
	if(win->type == eWIN_GLFW)
	{
		glfwPollEvents();
	}
}

/*_________________________________________________________________________________*/
int bkpWindowInitVulkanWindow()
{
    glfwSetErrorCallback(error_callback_glfw);
#if !defined(_WIN32) && !defined(NDEBUG)
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    if(GLFW_FALSE == glfwInit())
    {
        LOG(eERROR, bkp_window_TAG, "windowing system initialization failed");
        return 0;;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, stc_win_resizable == 1 ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED,  stc_win_decorated == 1 ? GLFW_TRUE : GLFW_FALSE);
#if !defined(_WIN32)
    glfwWindowHintString(GLFW_WAYLAND_APP_ID, "bkpview");
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "bkpview");
#endif
    return 0;
}

/*_________________________________________________________________________________*/
void bkpTerminateWindow(void)
{
    glfwTerminate();
}

/*_________________________________________________________________________________*/
BkpWindow * bkpCreateWindow(EWindowType type)
{
    BkpWindow * win;
    ALLOC_L(BkpWindow, win, 1);

	bkpWindowInitVulkanWindow();

	win->type = type;
//	win->api = api;
	win->window = NULL;

    return win;
}

/*_________________________________________________________________________________*/
void bkpDestroyWindow(BkpWindow ** window)
{
    BkpWindow * w = *window;
	if(w->type == eWIN_GLFW)
	{
		GLFWwindow * win  = (GLFWwindow *) w->window;
		glfwDestroyWindow(win);
	}

	free(w);
}

/*_________________________________________________________________________________*/
GLFWwindow * bkpOpenWindow(uint32_t w0, uint32_t h0, const char * sztitle, unsigned char fullscreen)
{
    //GLFWmonitor * mon = glfwGetPrimaryMonitor();
    const GLFWvidmode * vmode = NULL;
    int i;
    int mn;
    int w = w0, h = h0;
    GLFWmonitor ** mon = glfwGetMonitors(&mn);
    GLFWmonitor * primary = NULL;

    LOG(eINFO, bkp_window_TAG, "Monitor info:");
    for(i = mn - 1; i >= 0; --i)
    {
        vmode = glfwGetVideoMode(mon[i]);
        LOG(eINFO, bkp_window_TAG, "\t#%d %ix%i",i,vmode->width, vmode->height);
    }
    LOG(eINFO, bkp_window_TAG, "--------------------------\n");

    if(fullscreen == 1)
    {
        LOG(eINFO, bkp_window_TAG, "Fullscreen mode activated!");
        primary = mon[0];
    }

    if(w != 0 && h != 0)
    {
        stc_window = glfwCreateWindow(w, h, sztitle, primary, NULL);

        stc_win_width = w;
        stc_win_height = h;
    }
    else
    {
        stc_window = glfwCreateWindow(vmode->width, vmode->height, sztitle, primary, NULL);
        stc_win_width = vmode->width ;
        stc_win_height = vmode->height;
    }

    if(NULL != stc_window)
    {
        glfwSetWindowSizeCallback(stc_window, window_size_callback_glfw);
        glfwSetFramebufferSizeCallback(stc_window, framebuffer_size_callback_glfw);
        bkpInitInputSystem(stc_window);
    }
    else
    {
        DIE_L("Cannot create Window", bkp_window_TAG, -1);
    }

    return stc_window;
}

/*_________________________________________________________________________________*/
GLFWwindow * bkpGetWindow(void)
{
    return stc_window;
}

/*_________________________________________________________________________________*/
static void error_callback_glfw(int error,const char * description)
{
    LOG(eERROR, bkp_window_TAG, "[code : %i ] -> %s\n", error, description);
    return;
}

/*_________________________________________________________________________________*/
static void window_size_callback_glfw(GLFWwindow * window, int width, int height)
{
    stc_win_width = width;
    stc_win_height = height;

    if(stc_customResizedCallback != NULL)
    {
        stc_customResizedCallback(window, width, height);
    }

    return;
}

/*_________________________________________________________________________________*/
static void framebuffer_size_callback_glfw(GLFWwindow * window, int width, int height)
{
    stc_win_width = width;
    stc_win_height = height;
    stc_frameBuffer_resized = 1;

    if(stc_customResizedCallback != NULL)
    {
        stc_customResizedCallback(window, width, height);
    }

    return;
}

/*_________________________________________________________________________________*/
void bkpGetWindowSize(uint32_t * w, uint32_t * h)
{
    //getting the new size from glfwl before sending to caller
    glfwGetFramebufferSize(stc_window, &stc_win_width, &stc_win_height);

    *w = stc_win_width;
    *h = stc_win_height;
    return;
}

/*_________________________________________________________________________________*/
void bkpPollEvent(BkpWindow * win)
{
    (void)win;
    glfwPollEvents();
}

/*_________________________________________________________________________________*/
unsigned char bkpWindowGetFrameBufferResized(void)
{
    return stc_frameBuffer_resized;
}

/*_________________________________________________________________________________*/
void bkpWindowResetFramebufferResized(void)
{
    stc_frameBuffer_resized = 0;;
    return;
}

/*_________________________________________________________________________________*/
void bkpWindowWaitEventFrameBufferSize(int32_t * w, int32_t *h)
{
    glfwGetFramebufferSize(stc_window, &stc_win_width, &stc_win_height);
    glfwWaitEvents();
    *w = stc_win_width;
    *h = stc_win_height;
    return;
}

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/


/******************** static functions ********************************/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

#ifdef __cplusplus
}
#endif
