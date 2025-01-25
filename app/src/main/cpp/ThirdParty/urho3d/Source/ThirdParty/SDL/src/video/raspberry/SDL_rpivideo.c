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

#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_RPI

/* References
 * http://elinux.org/RPi_VideoCore_APIs
 * https://github.com/raspberrypi/firmware/blob/master/opt/vc/src/hello_pi/hello_triangle/triangle.c
 * http://cgit.freedesktop.org/wayland/weston/tree/src/rpi-renderer.c
 * http://cgit.freedesktop.org/wayland/weston/tree/src/compositor-rpi.c
 */

#include "SDL_assert.h"

/* SDL internals */
#include "../SDL_sysvideo.h"
#include "SDL_version.h"
#include "SDL_syswm.h"
#include "SDL_loadso.h"
#include "SDL_events.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_keyboard_c.h"
#include "SDL_hints.h"

#ifdef SDL_INPUT_LINUXEV
#include "../../core/linux/SDL_evdev.h"
#endif

/* X11 + RPI */
#if SDL_VIDEO_DRIVER_X11
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/Xatom.h>
    #include "../x11/SDL_x11video.h"
    #include "../x11/SDL_x11xinput2.h"
    #include "../x11/SDL_x11window.h"
#endif

/* RPI declarations */
#include "SDL_rpivideo.h"
#include "SDL_rpievents_c.h"
#include "SDL_rpiopengles.h"
#include "SDL_rpimouse.h"

#include <stdio.h>


int RPI_OVERSCAN_DISABLE = 0;
int RPI_OVERSCAN_LEFT = 48;
int RPI_OVERSCAN_TOP = 48;

void RPI_GetOverScan(void)
{
    FILE* file = fopen("/boot/config.txt", "r");

    if (file != NULL)
    {
		const char* overscanleftstr = "overscan_left";
		const char* overscantopstr = "overscan_top";
		const char* disableoverscanstr = "disable_overscan";
		const char* delim = "=";

		char bufline[256];
		char *token, c;

		unsigned char ccount = 0;

		while (!feof(file))
		{
			c = fgetc(file);

			// fill the line buffer
			if (c != '\n' && c != 0 && ccount < 256)
			{
				bufline[ccount++] = c;
	 			continue;
			}

			// end of line
			bufline[ccount] = 0;

			// skip empty line
			if (ccount == 0)
				continue;

			// reset character counter
			ccount = 0;

			// skip line starting with #
			if (bufline[0] == '#')
				continue;

			printf("%s\n", bufline);

			// split with delimiter =
			token = strtok(bufline, delim);
			if (token == NULL)
				continue;

			// check token and get value

			if (strcmp(token, disableoverscanstr) == 0)
			{
				RPI_OVERSCAN_DISABLE = (int)strtol(strtok(NULL, bufline), 0, 10);
				if (RPI_OVERSCAN_DISABLE == 1)
				{
					RPI_OVERSCAN_LEFT = 0;
					RPI_OVERSCAN_TOP = 0;
					break;
				}
			}
/// TODO : the overscan values from config.txt don't work ! where find the good ones ?
/*
			else if (RPI_OVERSCAN_DISABLE == 0)
			{
				if (strcmp(token, overscanleftstr) == 0)
					RPI_OVERSCAN_LEFT = (int)strtol(strtok(NULL, bufline), 0, 10);

				else if (strcmp(token, overscantopstr) == 0)
					RPI_OVERSCAN_TOP = (int)strtol(strtok(NULL, bufline), 0, 10);
			}
*/
		}

		fclose(file);
    }
	else
		printf("RPI_GetOverScan : Error no access to /boot/config.txt");

	printf("RPI_GetOverScan : disable_overscan=%d overscan_left=%d overscan_top=%d \n",
			RPI_OVERSCAN_DISABLE, RPI_OVERSCAN_LEFT, RPI_OVERSCAN_TOP);
}


static int
RPI_Available(void)
{
    return 1;
}

static void
RPI_Destroy(SDL_VideoDevice * device)
{
    SDL_free(device->driverdata);
    SDL_free(device);
}

static SDL_VideoDevice *
RPI_Create()
{
    SDL_VideoDevice *device = NULL;
	int x11mode = 0;

#if SDL_VIDEO_DRIVER_X11
    printf("RPI_Create X11 mode \n");
	device = X11_InitX();
	if (device)
		x11mode = 1;
#endif

    if (device == NULL)
    {
		printf("RPI_Create EGL mode (no X11) \n");
		/* Initialize SDL_VideoDevice structure */
		device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
		if (device == NULL) {
			SDL_OutOfMemory();
			return NULL;
		}
		/* Initialize internal data */
		{
			SDL_VideoData *vdata = (SDL_VideoData *) SDL_calloc(1, sizeof(SDL_VideoData));
			if (vdata == NULL) {
				SDL_OutOfMemory();
				SDL_free(device);
				return NULL;
			}
			device->driverdata = vdata;
			vdata->display = 0;
		}
		/* Setup amount of available displays */
		device->num_displays = 0;
	}

    /* Set device free function */
    device->free = RPI_Destroy;

    /* Setup all functions which we can handle */
    device->VideoInit = RPI_VideoInit;
    device->VideoQuit = RPI_VideoQuit;
    device->GetDisplayModes = RPI_GetDisplayModes;
    device->SetDisplayMode = RPI_SetDisplayMode;
    device->CreateSDLWindow = RPI_CreateWindow;
    device->CreateSDLWindowFrom = RPI_CreateWindowFrom;
    device->SetWindowTitle = RPI_SetWindowTitle;
    device->SetWindowIcon = RPI_SetWindowIcon;
    device->SetWindowPosition = RPI_SetWindowPosition;
    device->SetWindowSize = RPI_SetWindowSize;
    device->ShowWindow = RPI_ShowWindow;
    device->HideWindow = RPI_HideWindow;
    device->RaiseWindow = RPI_RaiseWindow;
    device->MaximizeWindow = RPI_MaximizeWindow;
    device->MinimizeWindow = RPI_MinimizeWindow;
    device->RestoreWindow = RPI_RestoreWindow;
    device->SetWindowMouseGrab = RPI_SetWindowGrab;
    device->DestroyWindow = RPI_DestroyWindow;
    device->OnWindowEnter = RPI_OnWindowEnter;
    device->OnWindowLeave = RPI_OnWindowLeave;
    device->OnWindowBeginConfigure = RPI_OnWindowBeginConfigure;
	device->GetWindowWMInfo = RPI_GetWindowWMInfo;
    device->GL_LoadLibrary = RPI_GLES_LoadLibrary;
    device->GL_GetProcAddress = RPI_GLES_GetProcAddress;
    device->GL_UnloadLibrary = RPI_GLES_UnloadLibrary;
    device->GL_CreateContext = RPI_GLES_CreateContext;
    device->GL_MakeCurrent = RPI_GLES_MakeCurrent;
    device->GL_SetSwapInterval = RPI_GLES_SetSwapInterval;
    device->GL_GetSwapInterval = RPI_GLES_GetSwapInterval;
    device->GL_SwapWindow = RPI_GLES_SwapWindow;
    device->GL_DeleteContext = RPI_GLES_DeleteContext;
    device->GL_DefaultProfileConfig = RPI_GLES_DefaultProfileConfig;

#if SDL_VIDEO_DRIVER_X11
	if (x11mode) {
		device->PumpEvents = X11_PumpEvents;
		printf("RPI_Create X11 mode : X11_PumpEvents\n");
	} else {
		device->PumpEvents = RPI_PumpEvents;
		printf("RPI_Create X11 mode : RPI_PumpEvents\n");
	}
#else
    device->PumpEvents = RPI_PumpEvents;
#endif

    return device;
}

VideoBootStrap RPI_bootstrap = {
    "RPI",
    "RPI Video Driver",
    RPI_Create
};

/*****************************************************************************/
/* SDL Video and Display initialization/handling functions                   */
/*****************************************************************************/
int
RPI_VideoInit(_THIS)
{
	// Get Monitor OverScan Values
	RPI_GetOverScan();

#if SDL_VIDEO_DRIVER_X11
    X11_VideoInit(_this);
#else
    SDL_VideoDisplay display;
    SDL_DisplayMode current_mode;
    SDL_DisplayData *data;
    uint32_t w,h;

    SDL_zero(current_mode);

    if (graphics_get_display_size( 0, &w, &h) < 0) {
        return -1;
    }

    current_mode.w = w;
    current_mode.h = h;
    /* FIXME: Is there a way to tell the actual refresh rate? */
    current_mode.refresh_rate = 60;
    /* 32 bpp for default */
    current_mode.format = SDL_PIXELFORMAT_ABGR8888;

    current_mode.driverdata = NULL;

    SDL_zero(display);
    display.desktop_mode = current_mode;
    display.current_mode = current_mode;

    /* Allocate display internal data */
    data = (SDL_DisplayData *) SDL_calloc(1, sizeof(SDL_DisplayData));
    if (data == NULL) {
        return SDL_OutOfMemory();
    }

    display.driverdata = data;

    SDL_AddVideoDisplay(&display);

#ifdef SDL_INPUT_LINUXEV
    if (SDL_EVDEV_Init() < 0) {
        return -1;
    }
#endif
#endif

//    RPI_InitMouse(_this);

    return 1;
}

void
RPI_VideoQuit(_THIS)
{
#ifdef SDL_INPUT_LINUXEV
    SDL_EVDEV_Quit();
#endif
#if SDL_VIDEO_DRIVER_X11
    X11_VideoQuit(_this);
#endif
}

void
RPI_GetDisplayModes(_THIS, SDL_VideoDisplay * display)
{
    /* Only one display mode available, the current one */
    SDL_AddDisplayMode(display, &display->current_mode);
}

int
RPI_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    return 0;
}

#ifndef SDL_VIDEO_DRIVER_X11
static void
RPI_vsync_callback(DISPMANX_UPDATE_HANDLE_T u, void *data)
{
	SDL_WindowData *wdata = ((SDL_WindowData *) data);

	SDL_LockMutex(wdata->vsync_cond_mutex);
	SDL_CondSignal(wdata->vsync_cond);
	SDL_UnlockMutex(wdata->vsync_cond_mutex);
}
#endif


#if SDL_VIDEO_DRIVER_X11

static void
RPI_HideDispman(_THIS, SDL_Window * window)
{
	SDL_WindowData *wdata = window->driverdata;
	VC_RECT_T dummy_rect;

	dummy_rect.x = 0;
	dummy_rect.y = 0;
	dummy_rect.width = 1;
	dummy_rect.height = 1;

	X11_XUngrabPointer(wdata->xdisplay, CurrentTime);

	wdata->d_update = vc_dispmanx_update_start( 0 /* Priority*/);
	vc_dispmanx_element_change_attributes(wdata->d_update, wdata->d_element, 0,
										  0, 255, &dummy_rect, &wdata->src_rect,
										  DISPMANX_PROTECTION_NONE, (DISPMANX_TRANSFORM_T) 0);
   	vc_dispmanx_update_submit_sync(wdata->d_update);

   	glViewport(0, 0, 0, 0);

   	printf("RPI_HideDispman \n");
}

static void
RPI_ShowDispman(_THIS, SDL_Window * window)
{
	SDL_WindowData *wdata = window->driverdata;
	wdata->dest_rect.x = window->x + RPI_OVERSCAN_LEFT;
	wdata->dest_rect.y = window->y + RPI_OVERSCAN_TOP;
	/*
	X11_XGrabPointer(wdata->xdisplay, DefaultRootWindow(wdata->xdisplay), 1,
			ButtonPressMask | ButtonReleaseMask |PointerMotionMask |
			FocusChangeMask | EnterWindowMask | FocusChangeMask, //| LeaveWindowMask,
			GrabModeAsync,GrabModeAsync, wdata->xwindow, None, CurrentTime);
	*/

	wdata->d_update = vc_dispmanx_update_start(0);
	vc_dispmanx_element_change_attributes(wdata->d_update, wdata->d_element, 0,
										0, 255, &wdata->dest_rect, &wdata->src_rect,
										DISPMANX_PROTECTION_NONE, (DISPMANX_TRANSFORM_T) 0);
	vc_dispmanx_update_submit_sync(wdata->d_update);

	glViewport(0, 0, wdata->dest_rect.width, wdata->dest_rect.height);

	printf("RPI_ShowDispman\n");
}

static void
RPI_ResizeDispman(_THIS, SDL_Window * window)
{
	SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;

	window->flags &= ~SDL_WINDOW_FULLSCREEN;

	wdata->dest_rect.width = window->w;
	wdata->dest_rect.height = window->h;

	RPI_ShowDispman(_this, window);

	printf("RPI_ResizeDispman at %d,%d %dx%d\n", wdata->dest_rect.x, wdata->dest_rect.y,
										wdata->dest_rect.width, wdata->dest_rect.height);
}

static void
RPI_FullScreen(_THIS, SDL_Window * window)
{
	SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;
	VC_RECT_T dummy_rect;

	window->flags |= SDL_WINDOW_FULLSCREEN;

	dummy_rect.x = 0;
	dummy_rect.y = 0;
	dummy_rect.width = 0;    // fullscreen
	dummy_rect.height = 0;   // fullscreen

	wdata->d_update = vc_dispmanx_update_start(0);
	vc_dispmanx_element_change_attributes(wdata->d_update, wdata->d_element, 0,
										0, 255, &dummy_rect, &wdata->src_rect,
										DISPMANX_PROTECTION_NONE,(DISPMANX_TRANSFORM_T) 0);
   	vc_dispmanx_update_submit_sync(wdata->d_update);

	glViewport(0 , 0 , wdata->src_rect.width >> 16 , wdata->src_rect.height >> 16);

	printf("RPI RPI_FullScreen (dst=%dx%d)\n", wdata->src_rect.width >> 16,
											   wdata->src_rect.height >> 16);
}

int
RPI_CreateGLX11FrameCopy(_THIS, SDL_Window * window) {

	SDL_WindowData * wdata = (SDL_WindowData *) window->driverdata;

	const int w = window->w;
	const int h = window->h;
	static unsigned int *image;

	if (wdata->gc == NULL) {
		/* Create the graphics context for drawing */
		#if 0
		XGCValues gcv;
		gcv.graphics_exposures = True;
		X11_XCreateGC(wdata->xdisplay, wdata->xwindow, GCGraphicsExposures, &gcv);
		#else
		wdata->gc = DefaultGC(wdata->xdisplay, 0);
		#endif
		if (!wdata->gc) {
			return SDL_SetError("Couldn't create graphics context");
		}
		/* Allocated image buffer for ximage */
		if (wdata->ximage == NULL) {
			image = SDL_malloc(w*h*4);
			if (image == NULL) {
				return SDL_OutOfMemory();
			}
		}
	}

	if (image)
	{
		static unsigned int tmp[2048];
		const int halfh = h/2;
		unsigned int *ptr1, *ptr2;
        int x, y;

		/* created Ximage structure if not exist */
		if (wdata->ximage == NULL) {
			wdata->ximage = X11_XCreateImage(wdata->xdisplay, wdata->visual,
							  24, ZPixmap, 0, image, w, h, 32, 0);
			if (!wdata->ximage) {
				SDL_free(image);
				return SDL_SetError("Couldn't create XImage");
			}
			image = &wdata->ximage->data[0];
		}

		/* Read GL framebuffer */
		//glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, image);
		glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image);

		/* convert GL to X11 image format */
		for(y=0; y < halfh; y++) {
			// flip y
			ptr1 = image + y*w;
			ptr2 = image + (h-y-1)*w;
			memcpy(tmp, ptr1, 4*w);
			memcpy(ptr1, ptr2, 4*w);
			memcpy(ptr2, tmp, 4*w);

			/* convert color from ARGB to ABGR (GL->X11) */
			#define BGRATOABGR(val)  (((val & 0xffffff00)>>8) | ((val & 0xff)<<24))
			/* (switch only R <=> B) */
			#define ARGBTOABGR(val)  ((val & 0xff00ff00) | (((val)&0xff)<<16) | ((val>>16)&0xff))
			for (x=0; x < w; x++) {
				*ptr1 = ARGBTOABGR(*ptr1);
				*ptr2 = ARGBTOABGR(*ptr2);
				ptr1++;
				ptr2++;
			}
		}

		/* put ximage in window */
		X11_XPutImage(wdata->xdisplay, wdata->xwindow, wdata->gc, wdata->ximage, 0, 0, 0, 0, w, h);
	}

    return 0;
}

static void
RPI_MoveDispman(_THIS, SDL_Window * window)
{
	SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;

	wdata->dest_rect.x = window->x + RPI_OVERSCAN_LEFT;
	wdata->dest_rect.y = window->y + RPI_OVERSCAN_TOP;
	wdata->dest_rect.width = window->w;
	wdata->dest_rect.height = window->h;

#if SDL_VIDEO_DRIVER_X11
	RPI_CreateGLX11FrameCopy(_this, window);
#endif

	if (SDL_GetMouse()->focus == window) {
		wdata->d_update = vc_dispmanx_update_start(0);
		vc_dispmanx_element_change_attributes(wdata->d_update, wdata->d_element, 0,
										0, 255, &wdata->dest_rect,
										&wdata->src_rect, DISPMANX_PROTECTION_NONE,
										(DISPMANX_TRANSFORM_T) 0);
		vc_dispmanx_update_submit_sync(wdata->d_update);

		printf("RPI_MoveDispman at %d,%d OVERSCAN(%d,%d) %dx%d\n", wdata->dest_rect.x, wdata->dest_rect.y,
												RPI_OVERSCAN_LEFT, RPI_OVERSCAN_TOP, wdata->dest_rect.width, wdata->dest_rect.height);
	} else {
		RPI_HideDispman(_this, window);
	}
}
#endif

int
RPI_CreateWindow(_THIS, SDL_Window * window)
{
	SDL_WindowData *wdata;
	SDL_VideoData *vdata;
	SDL_VideoDisplay *display;
	SDL_DisplayData *displaydata;
	int xflags = PointerMotionMask | KeyPressMask;

	printf("RPI_CreateWindow : %d, %d\n", window->w, window->h);

	vdata = (SDL_VideoData *) _this->driverdata;

	display = SDL_GetDisplayForWindow(window);
	displaydata = (SDL_DisplayData *) display->driverdata;

	/* Load EGL */
	if (!_this->egl_data) {
		if (SDL_GL_LoadLibrary(NULL) < 0) {
			return -1;
		}
	}

	/* Allocate window internal data */
	wdata = (SDL_WindowData *) SDL_calloc(1, sizeof(SDL_WindowData));
	if (wdata == NULL) {
		return SDL_OutOfMemory();
	}

	/* Setup driver data for this window */
	window->driverdata = wdata;

	// Uncomment to Test FS
	// window->flags |= SDL_WINDOW_FULLSCREEN;

#if SDL_VIDEO_DRIVER_X11
    wdata->paused = 1;
    wdata->ximage = NULL;
    wdata->gc = NULL;
	/* Set X Window */
	if (vdata->display)
	{
		wdata->xdisplay = vdata->display;
		if (wdata->xdisplay > 0)
		{
			XSetWindowAttributes swa;
			swa.event_mask = xflags |
								StructureNotifyMask |
								//ResizeRedirectMask |
								VisibilityChangeMask |
								//ExposureMask |
								KeyPressMask |
								KeyReleaseMask |
								LeaveWindowMask |
								EnterWindowMask |
								PointerMotionMask |
								ButtonMotionMask |
								ButtonPressMask |
								ButtonReleaseMask
								;
			swa.background_pixel = 0;

			wdata->xroot = DefaultRootWindow(wdata->xdisplay);
			wdata->xwindow = X11_XCreateWindow(wdata->xdisplay, wdata->xroot, 0, 0, window->w, window->h, 0, CopyFromParent,
                                InputOutput, CopyFromParent, CWBackPixel | CWEventMask, &swa);

			if (!wdata->xwindow)
				return -2;

			/* Intercept Atom X11 Messages */
			/*
			Atom wmDeleteMessage = XInternAtom(wdata->xdisplay, "WM_DELETE_WINDOW", False);
			XSetWMProtocols(wdata->xdisplay, wdata->xwindow, &wmDeleteMessage, 1);
			Atom wmMoveResizeMessage = XInternAtom(wdata->xdisplay, "_NET_WM_MOVERESIZE", False);
			XSetWMProtocols(wdata->xdisplay, wdata->xwindow, &wmMoveResizeMessage, 1);
			*/

			/* Setup WindowData */
			wdata->created = 1;
			wdata->videodata = vdata;
			wdata->window = window;
			#ifdef X_HAVE_UTF8_STRING
				if (SDL_X11_HAVE_UTF8 && vdata->im) {
					wdata->ic =
						X11_XCreateIC(vdata->im, XNClientWindow, wdata->xwindow, XNFocusWindow,
									wdata->xwindow, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, NULL);
				}
			#endif
			/* Associate the data with the window */
			{
				int numwindows = vdata->numwindows;
				int windowlistlength = vdata->windowlistlength;
				SDL_WindowData **windowlist = vdata->windowlist;

				if (numwindows < windowlistlength) {
					windowlist[numwindows] = wdata;
					vdata->numwindows++;
				} else {
					windowlist =
						(SDL_WindowData **) SDL_realloc(windowlist,
														(numwindows +
														 1) * sizeof(*windowlist));
					if (!windowlist) {
						SDL_free(wdata);
						return SDL_OutOfMemory();
					}
					windowlist[numwindows] = wdata;
					vdata->numwindows++;
					vdata->windowlistlength++;
					vdata->windowlist = windowlist;
				}
			}
			/* Set SizeHints, WMHints, ClassHints */
			{
				XSizeHints *sizehints;
				XWMHints *wmhints;
				XClassHint *classhints;

				/* Setup the normal size hints */
				sizehints = X11_XAllocSizeHints();
				sizehints->flags = 0;
				if (!(window->flags & SDL_WINDOW_RESIZABLE)) {
					sizehints->min_width = sizehints->max_width = window->w;
					sizehints->min_height = sizehints->max_height = window->h;
					sizehints->flags |= (PMaxSize | PMinSize);
				}
				sizehints->x = window->x;
				sizehints->y = window->y;
				sizehints->flags |= USPosition;

				/* Setup the input hints so we get keyboard input */
				wmhints = X11_XAllocWMHints();
				wmhints->input = True;
				wmhints->window_group = vdata->window_group;
				wmhints->flags = InputHint | WindowGroupHint;

				/* Setup the class hints so we can get an icon (AfterStep) */
				classhints = X11_XAllocClassHint();
				classhints->res_name = vdata->classname;
				classhints->res_class = vdata->classname;

				/* Set the size, input and class hints */
				X11_XSetWMProperties(wdata->xdisplay, wdata->xwindow, NULL, NULL, NULL, 0, sizehints, wmhints, classhints);

				X11_XFree(sizehints);
				X11_XFree(wmhints);
				X11_XFree(classhints);
			}
			/* Fill in the SDL window with the window data */
			{
				XWindowAttributes attrib;
				X11_XGetWindowAttributes(wdata->xdisplay, wdata->xwindow, &attrib);
				window->x = attrib.x;
				window->y = attrib.y;
				window->w = attrib.width;
				window->h = attrib.height;
				if (attrib.map_state != IsUnmapped) {
					window->flags |= SDL_WINDOW_SHOWN;
				} else {
					window->flags &= ~SDL_WINDOW_SHOWN;
				}
				wdata->visual = attrib.visual;
				wdata->colormap = attrib.colormap;
			}
			/* Set the window manager state */
			X11_SetNetWMState(_this, wdata->xwindow, window->flags);

			window->flags |= X11_GetNetWMState(_this, wdata->xwindow);
			{
				Window FocalWindow;
				int RevertTo=0;
				X11_XGetInputFocus(wdata->xdisplay, &FocalWindow, &RevertTo);
				if (FocalWindow == wdata->xwindow)
				{
					window->flags |= SDL_WINDOW_INPUT_FOCUS;
				}

				if (window->flags & SDL_WINDOW_INPUT_FOCUS) {
					SDL_SetKeyboardFocus(wdata->window);
				}

				if (window->flags & SDL_WINDOW_INPUT_GRABBED) {
					/* Tell x11 to clip mouse */
				}
			}
			// make the window visible on the screen
			X11_XMapRaised(wdata->xdisplay, wdata->xwindow);

			printf("RPI_CreateWindow : XWindow opened xdisplay=%u xwindow=%u size=%d,%d\n", (unsigned)wdata->xdisplay, (unsigned)wdata->xwindow, window->w, window->h);
		}
		else
		{
			printf("Could not open X window\n");
			window->flags |= SDL_WINDOW_FULLSCREEN;
		}
	}
	/* FullScreen size : No Window */
	else
#endif
	{
		window->flags |= SDL_WINDOW_FULLSCREEN;
	}
	/* Set DispManX : in XWindow or Not */
	{
		VC_DISPMANX_ALPHA_T dispman_alpha = {DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 255, 0};
		const char *env = SDL_GetHint(SDL_HINT_RPI_VIDEO_LAYER);
		unsigned int layer = env ? SDL_atoi(env) : SDL_RPI_VIDEOLAYER;

		bcm_host_init();

		wdata->src_rect.x 		= 0;
		wdata->src_rect.y 		= 0;
		wdata->src_rect.width	= window->w	<< 16;
		wdata->src_rect.height	= window->h	<< 16;

		wdata->dest_rect.x = RPI_OVERSCAN_LEFT;
		wdata->dest_rect.y = RPI_OVERSCAN_TOP;

		/* wait setting position/resize  */
		if (!(window->flags & SDL_WINDOW_FULLSCREEN)) {
			wdata->dest_rect.width = 1;
			wdata->dest_rect.height = 1;
		}
		/* go full screen with scaling */
		else
		{
			wdata->dest_rect.width = 0;
			wdata->dest_rect.height = 0;
		}

		printf("RPI_CreateWindow : RPI Window at %d,%d %dx%d\n", wdata->dest_rect.x, wdata->dest_rect.y,
						wdata->dest_rect.width, wdata->dest_rect.height);

		wdata->d_display 	= vc_dispmanx_display_open(0);
		wdata->d_update		= vc_dispmanx_update_start(0);
		wdata->d_element 	= vc_dispmanx_element_add (wdata->d_update, wdata->d_display, layer,
									&wdata->dest_rect, 0, &wdata->src_rect, DISPMANX_PROTECTION_NONE,
									&dispman_alpha, 0, (DISPMANX_TRANSFORM_T)0);

		vc_dispmanx_update_submit_sync(wdata->d_update);

		wdata->d_window.element = wdata->d_element;
		wdata->d_window.width = window->w;
		wdata->d_window.height = window->h;

		displaydata->dispman_display = wdata->d_display;
		displaydata->dispman_update = wdata->d_update;
		printf("RPI_CreateWindow : DispmanX opened ddisplay=%u dwindow=%u\n", wdata->d_display, wdata->d_window);
	}
	/* Open EGL GLES2 */
	{
		EGLConfig  egl_config;

		/* egl initilisation */
		{
			EGLint majorVersion, minorVersion;

			wdata->egl_display = _this->egl_data->eglGetDisplay(EGL_DEFAULT_DISPLAY);

			if (wdata->egl_display == EGL_NO_DISPLAY ) {
				printf("No EGL display.\n");
				return -1;
			}

			if (!_this->egl_data->eglInitialize(wdata->egl_display, &majorVersion, &minorVersion)) {
				printf("Unable to initialize EGL\n");
				return -1;
			}

			printf("EGL %d.%d Initialized\n", majorVersion, minorVersion);
		}

		/* egl config */
		{
			EGLint attr[] =
			{
				EGL_RED_SIZE,       8,
				EGL_GREEN_SIZE,     8,
				EGL_BLUE_SIZE,      8,
				EGL_ALPHA_SIZE,     8,
				EGL_DEPTH_SIZE,     24,
				EGL_STENCIL_SIZE,   EGL_DONT_CARE,
				EGL_SURFACE_TYPE, 	EGL_WINDOW_BIT,
			   //EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE,
				EGL_SAMPLE_BUFFERS, 1,
				EGL_NONE
			};
			EGLint num_config;

			if (!_this->egl_data->eglChooseConfig(wdata->egl_display, attr, &egl_config, 1, &num_config)) {
				printf("Failed to choose config (eglError: %d)\n", _this->egl_data->eglGetError());
				return -1;
			}

			if (num_config != 1) {
				printf("Didn't get exactly one config, but %d\n", num_config);
				return -1;
			}

			/* create the egl_surface in dispman window */
			wdata->egl_surface = _this->egl_data->eglCreateWindowSurface(wdata->egl_display, egl_config, &wdata->d_window, NULL);
			if (wdata->egl_surface == EGL_NO_SURFACE) {
				printf("Unable to create EGL surface eglError: %d\n", _this->egl_data->eglGetError());
				return -1;
			}
		}

		/* egl context */
		{
			// egl-contexts collect all state descriptions needed required for operation
			EGLint ctxattr[] = {
				EGL_CONTEXT_CLIENT_VERSION, 2,
				EGL_NONE
			};
			wdata->egl_context = _this->egl_data->eglCreateContext(wdata->egl_display, egl_config, EGL_NO_CONTEXT, ctxattr);
			if (wdata->egl_context == EGL_NO_CONTEXT) {
				printf("Unable to create EGL context eglError: %d\n", _this->egl_data->eglGetError());
				return -1;
			}
		}

		/* associate the egl-context with the egl-surface */
		_this->egl_data->eglMakeCurrent(wdata->egl_display, wdata->egl_surface, wdata->egl_surface, wdata->egl_context);

		printf("RPI_CreateWindow : EGL link to ddisplay=%u dwindow=%u src_w=%u src_h=%u\n", wdata->d_display, wdata->d_window, wdata->src_rect.width , wdata->src_rect.height);
	}

	/* Init RPI Mouse : always after bcm_host_init */
	RPI_InitMouse(_this);

	SDL_SetMouseFocus(window);
	SDL_SetKeyboardFocus(window);

	printf("RPI_CreateWindow() finished\n");

	return 0;
}

int
RPI_CreateWindowFrom(_THIS, SDL_Window * window, const void *data)
{
    return -1;
}

void
RPI_SetWindowTitle(_THIS, SDL_Window * window)
{
#if SDL_VIDEO_DRIVER_X11
	if (!(window->flags & SDL_WINDOW_FULLSCREEN))
	{
		X11_SetWindowTitle(_this, window);
	}
#endif
}

void
RPI_SetWindowIcon(_THIS, SDL_Window * window, SDL_Surface * icon)
{
#if SDL_VIDEO_DRIVER_X11
	if (!(window->flags & SDL_WINDOW_FULLSCREEN))
	{
		X11_SetWindowIcon(_this, window, icon);
	}
#endif
}

void
RPI_SetWindowPosition(_THIS, SDL_Window * window)
{
	printf("RPI_SetWindowPosition ... window=%u ... \n", (unsigned)window);

	if (!(window->flags & SDL_WINDOW_FULLSCREEN)) {
	#if SDL_VIDEO_DRIVER_X11
		X11_SetWindowPosition(_this, window);
	#endif
	}
	RPI_MoveDispman(_this, window);
	printf("RPI_SetWindowPosition ... OK !\n");
}

void
RPI_SetWindowSize(_THIS, SDL_Window * window)
{
	printf("RPI_SetWindowSize ... \n");

	if (!(window->flags & SDL_WINDOW_FULLSCREEN)) {
	#if SDL_VIDEO_DRIVER_X11
		X11_SetWindowSize(_this, window);
	#endif
		if (window->flags & SDL_WINDOW_RESIZABLE) {
			RPI_ResizeDispman(_this, window);
			printf("RPI_SetWindowSize ... OK !\n");
		}
	}
	else
	{
		RPI_ResizeDispman(_this, window);
		printf("RPI_SetWindowSize ... FullScreen OK !\n");
	}
}

void
RPI_ShowWindow(_THIS, SDL_Window * window)
{
	printf("RPI_ShowWindow ... \n");

	if (!(window->flags & SDL_WINDOW_FULLSCREEN)) {
	#if SDL_VIDEO_DRIVER_X11
		SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;
		X11_ShowWindow(_this, window);
		if (wdata->ximage) {
			printf("RPI_ShowWindow ... X11_XPutImage OK! \n");
			X11_XPutImage(wdata->xdisplay, wdata->xwindow, wdata->gc, wdata->ximage, 0, 0, 0, 0, window->w, window->h);
		}
	#endif
		if (SDL_GetMouse()->focus == window)
			RPI_ShowDispman(_this, window);
		printf("RPI_ShowWindow ... OK !\n");
	}
}

void
RPI_HideWindow(_THIS, SDL_Window * window)
{
	printf("RPI_HideWindow ... \n");
    RPI_HideDispman(_this, window);
#if SDL_VIDEO_DRIVER_X11
    X11_HideWindow(_this, window);
#endif
}

void
RPI_RaiseWindow(_THIS, SDL_Window * window)
{
	printf("RPI_RaiseWindow ... \n");

	if (!(window->flags & SDL_WINDOW_FULLSCREEN)) {
	#if SDL_VIDEO_DRIVER_X11
		X11_RaiseWindow(_this, window);
	#endif
		RPI_ShowDispman(_this, window);
		RPI_ResizeDispman(_this, window);
		RPI_MoveDispman(_this, window);
		printf("RPI_RaiseWindow ... OK !\n");
	}
}

void
RPI_MaximizeWindow(_THIS, SDL_Window * window)
{
	printf("RPI_MaximizeWindow ... \n");
	RPI_ShowWindow(_this, window);
}

void
RPI_MinimizeWindow(_THIS, SDL_Window * window)
{
	printf("RPI_MinimizeWindow ... \n");
	RPI_HideWindow(_this, window);
}

void
RPI_RestoreWindow(_THIS, SDL_Window * window)
{
	printf("RPI_RestoreWindow ... \n");
}

void
RPI_SetWindowGrab(_THIS, SDL_Window * window, SDL_bool grabbed)
{
	printf("RPI_SetWindowGrab ... grabbed=%u\n", (unsigned)grabbed);

	if (!(window->flags & SDL_WINDOW_FULLSCREEN)) {
	#if SDL_VIDEO_DRIVER_X11
		X11_SetWindowMouseGrab(_this, window, grabbed);
		printf("RPI_SetWindowGrab ... OK !\n");
	#endif
	}
}

void
RPI_DestroyWindow(_THIS, SDL_Window * window)
{
	SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;

	printf("RPI_DestroyWindow ...\n");

	if (wdata) {
		if (wdata->double_buffer) {
			/* Wait for vsync, and then stop vsync callbacks and destroy related stuff, if needed */
			SDL_LockMutex(wdata->vsync_cond_mutex);
			SDL_CondWait(wdata->vsync_cond, wdata->vsync_cond_mutex);
			SDL_UnlockMutex(wdata->vsync_cond_mutex);
			vc_dispmanx_vsync_callback(wdata->d_display, NULL, NULL);

			SDL_DestroyCond(wdata->vsync_cond);
			SDL_DestroyMutex(wdata->vsync_cond_mutex);
		}
		if (_this->egl_data) {
			_this->egl_data->eglDestroyContext(wdata->egl_display, wdata->egl_context);
			_this->egl_data->eglDestroySurface(wdata->egl_display, wdata->egl_surface);
			_this->egl_data->eglTerminate(wdata->egl_display);

		}
	#if defined(SDL_VIDEO_DRIVER_X11)
		if (wdata->xdisplay) {
			X11_DestroyWindow(_this, window);
		}
	#else
		SDL_free(wdata);
	#endif
	}

	window->driverdata = NULL;

	printf("RPI_DestroyWindow ... OK !\n");
}

void
RPI_OnWindowEnter(_THIS, SDL_Window * window)
{
	printf("RPI_OnWindowEnter ... \n");
	RPI_ShowDispman(_this, window);
}

void
RPI_OnWindowLeave(_THIS, SDL_Window * window)
{
	printf("RPI_OnWindowLeave ... \n");
#if SDL_VIDEO_DRIVER_X11
	RPI_CreateGLX11FrameCopy(_this, window);
#endif
	RPI_HideDispman(_this, window);
}

void
RPI_OnWindowBeginConfigure(_THIS, SDL_Window * window)
{
	// TODO : intercept message Atom _NET_WM_MOVERESIZE ? see SDLx11events InitiateWindowResize for internal window
	printf("RPI_OnWindowBeginConfigure ... \n");
	// HideFrame
}

/*****************************************************************************/
/* SDL Window Manager function                                               */
/*****************************************************************************/

SDL_bool
RPI_GetWindowWMInfo(_THIS, SDL_Window * window, struct SDL_SysWMinfo *info)
{
    if (info->version.major <= SDL_MAJOR_VERSION) {
        return SDL_TRUE;
    } else {
        SDL_SetError("application not compiled with SDL %d.%d",
                     SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
        return SDL_FALSE;
    }

    /* Failed to get window manager information */
    return SDL_FALSE;
}


#endif /* SDL_VIDEO_DRIVER_RPI */

/* vi: set ts=4 sw=4 expandtab: */
