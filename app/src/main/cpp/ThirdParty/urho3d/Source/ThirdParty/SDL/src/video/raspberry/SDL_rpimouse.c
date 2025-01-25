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

#include "SDL_assert.h"
#include "SDL_surface.h"
#include "SDL_hints.h"

#ifdef SDL_VIDEO_DRIVER_X11
#include "../x11/SDL_x11video.h"
#include "../x11/SDL_x11modes.h"
#endif

#include "SDL_rpivideo.h"
#include "SDL_rpimouse.h"

#include "../SDL_sysvideo.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/default_cursor.h"

static SDL_Cursor *RPI_CreateDefaultCursor(void);
static SDL_Cursor *RPI_CreateCursor(SDL_Surface * surface, int hot_x, int hot_y);
static int RPI_ShowCursor(SDL_Cursor * cursor);
static void RPI_MoveCursor(SDL_Cursor * cursor);
static void RPI_FreeCursor(SDL_Cursor * cursor);
static void RPI_WarpMouse(SDL_Window * window, int x, int y);
static int RPI_WarpMouseGlobal(int x, int y);

/* Create a cursor from a surface */
static SDL_Cursor *
RPI_CreateCursor(SDL_Surface * surface, int hot_x, int hot_y)
{
    RPI_CursorData *curdata;
    SDL_Cursor *cursor;
    int ret;
    VC_RECT_T dst_rect;
    Uint32 dummy;
    DISPMANX_UPDATE_HANDLE_T update; 
	VC_IMAGE_TYPE_T type = VC_IMAGE_ARGB8888;
    
    if (surface == NULL)
		return NULL;
		
    cursor = (SDL_Cursor *) SDL_calloc(1, sizeof(*cursor));
    if (cursor == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }
    curdata = (RPI_CursorData *) SDL_calloc(1, sizeof(*curdata));
    if (curdata == NULL) {
        SDL_OutOfMemory();
        SDL_free(cursor);
        return NULL;
	}
		
    curdata->hot_x = hot_x;
    curdata->hot_y = hot_y;
    curdata->w = surface->w;
    curdata->h = surface->h;                             
	curdata->element = DISPMANX_NO_HANDLE;

	curdata->resource = vc_dispmanx_resource_create(type, curdata->w, curdata->h, &dummy);
	SDL_assert(curdata->resource);
	
	update = vc_dispmanx_update_start(10);
	SDL_assert(update);	
	vc_dispmanx_rect_set(&dst_rect, 0, 0, curdata->w, curdata->h);
	ret = vc_dispmanx_resource_write_data(curdata->resource, type, curdata->w*4, surface->pixels, &dst_rect);
	SDL_assert( ret == 0 );
	ret = vc_dispmanx_update_submit_sync(update);
	SDL_assert( ret == 0 );

	cursor->driverdata = curdata;
	    
	printf("RPI_CreateCursor cursor = %u ! \n", (unsigned)cursor);
    return cursor;	
}

#ifdef RPI_CURSOR
static SDL_Cursor *
RPI_CreateDefaultCursor(void)
{
	SDL_Cursor * cursor;
	printf("RPI_CreateDefaultCursor : ...\n");
	cursor = SDL_CreateCursor(default_cdata, default_cmask, DEFAULT_CWIDTH, DEFAULT_CHEIGHT, DEFAULT_CHOTX, DEFAULT_CHOTY);
	printf("RPI_CreateDefaultCursor : cursor = %u OK! \n", (unsigned)cursor);
	
    return cursor;
}
#else
static SDL_Cursor *
RPI_CreateDefaultCursor(void)
{
    SDL_Cursor *cursor;

    cursor = SDL_calloc(1, sizeof(*cursor));
    if (cursor) {
        // None is used to indicate the default cursor
        cursor->driverdata = (void*)None;
    } else {
        SDL_OutOfMemory();
    }

    return cursor;
}
#endif

static int
RPI_HideCursor(SDL_Cursor * cursor)
{
	if (cursor != NULL && cursor->driverdata != NULL) {
		DISPMANX_UPDATE_HANDLE_T update;
		int ret;
		RPI_CursorData *curdata = (RPI_CursorData *) cursor->driverdata;
		
		printf("RPI_HideCursor cursor=%u ... \n", (unsigned)cursor);
		
		if (curdata->element != DISPMANX_NO_HANDLE) {                
			update = vc_dispmanx_update_start(10);
			SDL_assert(update);
			ret = vc_dispmanx_element_remove(update, curdata->element);
			SDL_assert(ret == DISPMANX_SUCCESS);               
			ret = vc_dispmanx_update_submit_sync(update);
			SDL_assert(ret == DISPMANX_SUCCESS);       
			curdata->element = DISPMANX_NO_HANDLE;
			printf("RPI_HideCursor ... OK !\n");
		}
		return 0;
	}
	
	return -1;	
}

/* Show the specified cursor, or hide if cursor is NULL or no focus */
static int
RPI_ShowCursor(SDL_Cursor *cursor)
{
    int ret;
    RPI_CursorData *curdata;
    DISPMANX_UPDATE_HANDLE_T update;
    SDL_Mouse * mouse = SDL_GetMouse();
   
    if (mouse == NULL) {
		printf("RPI_ShowCursor cursor=%u nomouse !\n", (unsigned)cursor);		
        return -1;
    }
	
	printf("RPI_ShowCursor cursor=%u %s...\n", (unsigned)cursor, mouse->cursor_shown ? "toshow":"tohide");
	
	// Hide Cursor
	if (cursor == NULL || mouse->focus == NULL) {
		printf("RPI_ShowCursor cur_cursor=%u ... hide cur_cursor !\n", (unsigned)cursor);
		RPI_HideCursor(mouse->cur_cursor);
		// just hide current cursor
		if (cursor == NULL)
			return 0;
	}
	// Show Cursor
	if (cursor && mouse->focus)
	{
		curdata = (RPI_CursorData *) cursor->driverdata;
		if (curdata == NULL) {
			printf("RPI_ShowCursor cursor=%u no driverdata !\n", (unsigned)cursor);		
			return -1;
		}		
		if (curdata->element == DISPMANX_NO_HANDLE) {			
			SDL_VideoDisplay *display;
			SDL_DisplayData *data;
			VC_RECT_T src_rect, dst_rect;
			VC_DISPMANX_ALPHA_T alpha = {  DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS /* flags */ 
										, 255 /*opacity 0->255*/,  0 /* mask */ };
			const char *env = SDL_GetHint(SDL_HINT_RPI_VIDEO_LAYER);
			uint32_t layer = env ? SDL_atoi(env) + 1 : SDL_RPI_VIDEOLAYER + 1;
			
			display = SDL_GetDisplayForWindow(mouse->focus);
			if (display == NULL) {
				printf("RPI_ShowCursor cursor=%u ... nodisplay !\n", (unsigned)cursor);
				mouse->cursor_shown = SDL_FALSE;
				return -1;
			}

			data = (SDL_DisplayData*) display->driverdata;
			if (data == NULL) {
				printf("RPI_ShowCursor cursor=%u ... display has no driverdata !\n", (unsigned)cursor);
				mouse->cursor_shown = SDL_FALSE;
				return -1;
			}
				
			update = vc_dispmanx_update_start(10);
			vc_dispmanx_rect_set(&dst_rect, mouse->x, mouse->y, curdata->w, curdata->h);
			vc_dispmanx_rect_set(&src_rect, 0, 0, curdata->w << 16, curdata->h << 16);
			curdata->element = vc_dispmanx_element_add(update, data->dispman_display, layer, 
													&dst_rect, curdata->resource,
													&src_rect, DISPMANX_PROTECTION_NONE,
													&alpha, NULL /*clamp*/, VC_IMAGE_ROT0);				
			ret = vc_dispmanx_update_submit_sync(update);
			SDL_assert( ret == 0 );
			
			printf("RPI_ShowCursor cursor=%u layer=%d at %d,%d... ddisplay=%u OK !\n", (unsigned)cursor, layer, mouse->x, mouse->y, (unsigned)data->dispman_display);
		} else {			
			printf("RPI_ShowCursor ... delement=%u so must be shown already !\n", curdata->element);
		}
	}

    return 0;
}

/* Free a window manager cursor */
static void
RPI_FreeCursor(SDL_Cursor * cursor)
{
    int ret;
    DISPMANX_UPDATE_HANDLE_T update;
    RPI_CursorData *curdata;

	printf("RPI_FreeCursor cursor=%u ... OK !\n", (unsigned)cursor);

    if (cursor != NULL) {
        curdata = (RPI_CursorData *) cursor->driverdata;

        if (curdata != NULL) {
            if (curdata->element != DISPMANX_NO_HANDLE) {
                update = vc_dispmanx_update_start(10);
                SDL_assert(update);
                ret = vc_dispmanx_element_remove(update, curdata->element);
                SDL_assert(ret == DISPMANX_SUCCESS);
                ret = vc_dispmanx_update_submit_sync(update);
                SDL_assert(ret == DISPMANX_SUCCESS);
            }

            if (curdata->resource != DISPMANX_NO_HANDLE) {
                ret = vc_dispmanx_resource_delete(curdata->resource);
                SDL_assert(ret == DISPMANX_SUCCESS);
            }
			
            SDL_free(cursor->driverdata);
        }
        
        SDL_free(cursor);
    }
}

/* Warp the mouse to (x,y) */
static void
RPI_WarpMouse(SDL_Window * window, int x, int y)
{
    RPI_WarpMouseGlobal(x, y);
}

/* Warp the mouse to (x,y) */
static int
RPI_WarpMouseGlobal(int x, int y)
{
    RPI_CursorData *curdata;
    DISPMANX_UPDATE_HANDLE_T update;
    int ret;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;
    SDL_Mouse *mouse = SDL_GetMouse();

    if (mouse == NULL || mouse->cur_cursor == NULL || mouse->cur_cursor->driverdata == NULL) {
        return 0;
    }

    /* Update internal mouse position. */
    SDL_SendMouseMotion(mouse->focus, mouse->mouseID, 0, x, y);

    curdata = (RPI_CursorData *) mouse->cur_cursor->driverdata;
    if (curdata->element == DISPMANX_NO_HANDLE) {
        return 0;
    }

    update = vc_dispmanx_update_start(10);
    if (!update) {
        return 0;
    }

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width  = curdata->w << 16;
    src_rect.height = curdata->h << 16;
    dst_rect.x = x;
    dst_rect.y = y;
    dst_rect.width  = curdata->w;
    dst_rect.height = curdata->h;

    ret = vc_dispmanx_element_change_attributes(
        update,
        curdata->element,
        0,
        0,
        0,
        &dst_rect,
        &src_rect,
        DISPMANX_NO_HANDLE,
        DISPMANX_NO_ROTATE);
    if (ret != DISPMANX_SUCCESS) {
        return SDL_SetError("vc_dispmanx_element_change_attributes() failed");
    }

    /* Submit asynchronously, otherwise the peformance suffers a lot */
    ret = vc_dispmanx_update_submit(update, 0, NULL);
    if (ret != DISPMANX_SUCCESS) {
        return SDL_SetError("vc_dispmanx_update_submit() failed");
    }
    return 0;
}

/* Warp the mouse to (x,y) */
static int
RPI_WarpMouseGlobalGraphicOnly(int x, int y)
{
    RPI_CursorData *curdata;
    DISPMANX_UPDATE_HANDLE_T update;
    int ret;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;
    SDL_Mouse *mouse = SDL_GetMouse();
		
    if (mouse == NULL || mouse->focus == NULL ||
		mouse->cur_cursor == NULL || mouse->cur_cursor->driverdata == NULL)
        return 0;
   
    curdata = (RPI_CursorData *) mouse->cur_cursor->driverdata;
    if (curdata->element == DISPMANX_NO_HANDLE) {
        return 0;
    }
	
	// global position, so get the position of the focused window
    x += mouse->focus->x - curdata->hot_x;
    y += mouse->focus->y - curdata->hot_x;
    
    //printf("RPI_WarpMouseGlobalGraphicOnly ... %d,%d !\n", x, y);
    
    update = vc_dispmanx_update_start(0);
    if (!update) {
        return 0;
    }
	
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width  = curdata->w << 16;
    src_rect.height = curdata->h << 16;
    dst_rect.x = x;
    dst_rect.y = y;
    dst_rect.width  = curdata->w;
    dst_rect.height = curdata->h;

    ret = vc_dispmanx_element_change_attributes(
        update,
        curdata->element,
        0, //ELEMENT_CHANGE_DEST_RECT | ELEMENT_CHANGE_SRC_RECT,
        0,
        0,
        &dst_rect,
        &src_rect,
        DISPMANX_NO_HANDLE,
        DISPMANX_NO_ROTATE);
    if (ret != DISPMANX_SUCCESS) {
        return SDL_SetError("vc_dispmanx_element_change_attributes() failed");
    }

    /* Submit asynchronously, otherwise the peformance suffers a lot */
    ret = vc_dispmanx_update_submit(update, 0, NULL);
    if (ret != DISPMANX_SUCCESS) {
        return SDL_SetError("vc_dispmanx_update_submit() failed");
    }
    
    return 0;
}

void
RPI_InitMouse(_THIS)
{
    /* FIXME: Using UDEV it should be possible to scan all mice
     * but there's no point in doing so as there's no multimice support...yet!
     */
    SDL_Mouse *mouse = SDL_GetMouse();

    mouse->CreateCursor = RPI_CreateCursor;
    mouse->ShowCursor = RPI_ShowCursor;
    mouse->MoveCursor = RPI_MoveCursor;
    mouse->FreeCursor = RPI_FreeCursor;
    mouse->WarpMouse = RPI_WarpMouse;
    mouse->WarpMouseGlobal = RPI_WarpMouseGlobal;

    SDL_SetDefaultCursor(RPI_CreateDefaultCursor());
}

void
RPI_QuitMouse(_THIS)
{

}

/* This is called when a mouse motion event occurs */
static void
RPI_MoveCursor(SDL_Cursor * cursor)
{
    SDL_Mouse *mouse = SDL_GetMouse();
    /* We must NOT call SDL_SendMouseMotion() on the next call or we will enter recursivity, 
     * so we create a version of WarpMouseGlobal without it. */
    RPI_WarpMouseGlobalGraphicOnly(mouse->x, mouse->y);
    
    //printf("RPI_MoveCursor ... %d,%d !\n", mouse->x, mouse->y);
}

#endif /* SDL_VIDEO_DRIVER_RPI */

/* vi: set ts=4 sw=4 expandtab: */
