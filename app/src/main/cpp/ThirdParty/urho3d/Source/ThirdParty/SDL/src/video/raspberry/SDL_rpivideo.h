/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __SDL_RPIVIDEO_H__
#define __SDL_RPIVIDEO_H__

#include "../../SDL_internal.h"
#include "../SDL_sysvideo.h"

#include <bcm_host.h>
#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#ifndef ELEMENT_CHANGE_LAYER
/* Copied from vc_vchi_dispmanx.h which is bugged and tries to include a non existing file */
/* Attributes changes flag mask */
#define ELEMENT_CHANGE_LAYER          (1<<0)
#define ELEMENT_CHANGE_OPACITY        (1<<1)
#define ELEMENT_CHANGE_DEST_RECT      (1<<2)
#define ELEMENT_CHANGE_SRC_RECT       (1<<3)
#define ELEMENT_CHANGE_MASK_RESOURCE  (1<<4)
#define ELEMENT_CHANGE_TRANSFORM      (1<<5)
/* End copied from vc_vchi_dispmanx.h */
#endif

// C.VILLE : Duplicate struct EGL_DISPMANX_WINDOW_T
typedef struct DispManX_Window
{
   DISPMANX_ELEMENT_HANDLE_T element;
   int width;
   int height;
} DispManX_Window;

#ifndef SDL_VIDEO_DRIVER_X11
typedef struct SDL_VideoData
{
// OpenGL ES EGL reference count
    uint32_t egl_refcount;
} SDL_VideoData;

typedef struct SDL_WindowData
{
// Dispmanx variables
	DISPMANX_ELEMENT_HANDLE_T 	d_element;
	DISPMANX_DISPLAY_HANDLE_T 	d_display;
	DISPMANX_UPDATE_HANDLE_T 	d_update;
	DispManX_Window 			d_window;

#if SDL_VIDEO_OPENGL_EGL
// EGL variables
	EGLDisplay  egl_display;
	EGLContext  egl_context;
	EGLSurface  egl_surface;
#endif

    /* Vsync callback cond and mutex */
    SDL_cond  *vsync_cond;
    SDL_mutex *vsync_cond_mutex;
    SDL_bool double_buffer;
} SDL_WindowData;

typedef struct SDL_DisplayData
{
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
} SDL_DisplayData;
#endif

#define SDL_RPI_VIDEOLAYER 10000 // High enough so to occlude everything
#define SDL_RPI_MOUSELAYER SDL_RPI_VIDEOLAYER + 1


/****************************************************************************/
/* SDL_VideoDevice functions declaration                                    */
/****************************************************************************/

/* Display and window functions */
int RPI_VideoInit(_THIS);
void RPI_VideoQuit(_THIS);
void RPI_GetDisplayModes(_THIS, SDL_VideoDisplay * display);
int RPI_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
int RPI_CreateWindow(_THIS, SDL_Window * window);
int RPI_CreateWindowFrom(_THIS, SDL_Window * window, const void *data);
void RPI_SetWindowTitle(_THIS, SDL_Window * window);
void RPI_SetWindowIcon(_THIS, SDL_Window * window, SDL_Surface * icon);
void RPI_SetWindowPosition(_THIS, SDL_Window * window);
void RPI_SetWindowSize(_THIS, SDL_Window * window);
void RPI_ShowWindow(_THIS, SDL_Window * window);
void RPI_HideWindow(_THIS, SDL_Window * window);
void RPI_RaiseWindow(_THIS, SDL_Window * window);
void RPI_MaximizeWindow(_THIS, SDL_Window * window);
void RPI_MinimizeWindow(_THIS, SDL_Window * window);
void RPI_RestoreWindow(_THIS, SDL_Window * window);
void RPI_SetWindowGrab(_THIS, SDL_Window * window, SDL_bool grabbed);
void RPI_DestroyWindow(_THIS, SDL_Window * window);
void RPI_OnWindowEnter(_THIS, SDL_Window * window);
void RPI_OnWindowLeave(_THIS, SDL_Window * window);
void RPI_OnWindowBeginConfigure(_THIS, SDL_Window * window);

/* Window manager function */
SDL_bool RPI_GetWindowWMInfo(_THIS, SDL_Window * window,
                             struct SDL_SysWMinfo *info);

/* OpenGL/OpenGL ES functions */
int RPI_GLES_LoadLibrary(_THIS, const char *path);
void *RPI_GLES_GetProcAddress(_THIS, const char *proc);
void RPI_GLES_UnloadLibrary(_THIS);
SDL_GLContext RPI_GLES_CreateContext(_THIS, SDL_Window * window);
int RPI_GLES_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context);
int RPI_GLES_SetSwapInterval(_THIS, int interval);
int RPI_GLES_GetSwapInterval(_THIS);
int RPI_GLES_SwapWindow(_THIS, SDL_Window * window);
void RPI_GLES_DeleteContext(_THIS, SDL_GLContext context);
#endif /* __SDL_RPIVIDEO_H__ */

/* vi: set ts=4 sw=4 expandtab: */
