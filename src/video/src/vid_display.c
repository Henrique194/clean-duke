/*
 * Copyright (C) 1994-1995 Apogee Software, Ltd.
 * Copyright (C) 1996, 2003 - 3D Realms Entertainment
 *
 * This file is part of Duke Nukem 3D version 1.5 - Atomic Edition
 *
 * Duke Nukem 3D is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
// An SDL replacement for BUILD's VESA code.


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "build/build.h"
#include "video/display.h"
#include "build/fixedPoint_math.h"
#include "build/engine.h"
#include "build/sdl_util.h"
#include "build/timer.h"


#ifndef __DATE__
#define __DATE__ "a long, long time ago"
#endif

#define DEFAULT_MAXRESWIDTH  MAXXDIM
#define DEFAULT_MAXRESHEIGHT MAXYDIM

#define UNLOCK_SURFACE_AND_RETURN                                              \
    if (SDL_MUSTLOCK(surface))                                                 \
        SDL_UnlockSurface(surface);                                            \
    return;

#define BUILD_USERSCREENRES "BUILD_USERSCREENRES"
#define BUILD_MAXSCREENRES  "BUILD_MAXSCREENRES"

#define print_tf_state(str, val)                                               \
    printf("%s: {%s}\n", str, (val) ? "true" : "false")

#define swap_macro(tmp, x, y)                                                  \
    {                                                                          \
        tmp = x;                                                               \
        x = y;                                                                 \
        y = tmp;                                                               \
    }


/* !!! move these elsewhere? */
int32_t xres, yres, bytesperline, imageSize, maxpages;
uint8_t* frameplace;

//The frambuffer address
uint8_t* frameoffset;
uint8_t *screen;
uint8_t vesachecked;
int32_t buffermode;
int32_t origbuffermode;
int32_t linearmode;
uint8_t permanentupdate = 0;
uint8_t vgacompatible;

SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;
SDL_Surface* surface = NULL;
SDL_Surface* surface_rgba = NULL;

static int32_t mouse_relative_x = 0;
static int32_t mouse_relative_y = 0;
static short mouse_buttons = 0;
static unsigned int lastkey = 0;

static uint32_t scancodes[SDL_NUM_SCANCODES];

int32_t last_render_ticks = 0;
int32_t total_render_time = 1;
int32_t total_rendered_frames = 0;

char* titleName = NULL;



void present_framebuffer(void) {
    SDL_CHECK_SUCCESS(SDL_BlitSurface(surface, NULL, surface_rgba, NULL));
    SDL_CHECK_SUCCESS(SDL_UpdateTexture(texture, NULL, surface_rgba->pixels, surface_rgba->pitch));
    SDL_CHECK_SUCCESS(SDL_RenderClear(renderer));
    SDL_CHECK_SUCCESS(SDL_RenderCopy(renderer, texture, NULL, NULL));
    SDL_RenderPresent(renderer);
}

static int sdl_mouse_button_filter(SDL_MouseButtonEvent const* event) {
    /*
		 * What bits BUILD expects:
		 *  0 left button pressed if 1
		 *  1 right button pressed if 1
		 *  2 middle button pressed if 1
		 *
		 *   (That is, this is what Int 33h (AX=0x05) returns...)
		 *
		 *  additionally bits 3&4 are set for the mouse wheel
		 */
    Uint8 button = event->button;
    if (button >= sizeof(mouse_buttons) * 8)
        return (0);

    if (button == SDL_BUTTON_RIGHT)
        button = SDL_BUTTON_MIDDLE;
    else if (button == SDL_BUTTON_MIDDLE)
        button = SDL_BUTTON_RIGHT;

    if (((const SDL_MouseButtonEvent*) event)->state)
        mouse_buttons |= 1 << (button - 1);
    else if (button != 4 && button != 5)
        mouse_buttons ^= 1 << (button - 1);
#if 0
    Uint8 bmask = SDL_GetMouseState(NULL, NULL);
    mouse_buttons = 0;
    if (bmask & SDL_BUTTON_LMASK)
        mouse_buttons |= 1;
    if (bmask & SDL_BUTTON_RMASK)
        mouse_buttons |= 2;
    if (bmask & SDL_BUTTON_MMASK)
        mouse_buttons |= 4;
#endif

    return (0);
}


static int sdl_mouse_motion_filter(SDL_Event const* event) {
    if (surface == NULL)
        return (0);

    if (event->type == SDL_JOYBALLMOTION) {
        mouse_relative_x = event->jball.xrel / 100;
        mouse_relative_y = event->jball.yrel / 100;
    } else {
        if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
            mouse_relative_x += event->motion.xrel;
            mouse_relative_y += event->motion.yrel;
            //printf("sdl_mouse_motion_filter: mrx=%d, mry=%d, mx=%d, my=%d\n",
            //	mouse_relative_x, mouse_relative_y, event->motion.xrel, event->motion.yrel);

            // mouse_relative_* is already reset in
            // readmousexy(). It must not be
            // reset here because calling this function does not mean
            // we always handle the mouse.
            // FIX_00001: Mouse speed is uneven and slower in windowed mode vs fullscreen mode.
        } else
            mouse_relative_x = mouse_relative_y = 0;
    }

    return (0);
}


/*
 * The windib driver can't alert us to the keypad enter key, which
 *  Ken's code depends on heavily. It sends it as the same key as the
 *  regular return key. These users will have to hit SHIFT-ENTER,
 *  which we check for explicitly, and give the engine a keypad enter
 *  enter event.
*/
static int handle_keypad_enter_hack(const SDL_Event* event) {
    static int kp_enter_hack = 0;
    int retval = 0;

    if (event->key.keysym.sym == SDLK_RETURN) {
        if (event->key.state == SDL_PRESSED) {
            if (event->key.keysym.mod & KMOD_SHIFT) {
                kp_enter_hack = 1;
                lastkey = SDL_SCANCODE_KP_ENTER;
                retval = 1;
            } /* if */
        }     /* if */

        else /* key released */
        {
            if (kp_enter_hack) {
                kp_enter_hack = 0;
                lastkey = SDL_SCANCODE_KP_ENTER;
                retval = 1;
            } /* if */
        }     /* if */
    }         /* if */

    return (retval);
} /* handle_keypad_enter_hack */

void fullscreen_toggle_and_change_driver(void) {

    //  FIX_00002: New Toggle Windowed/FullScreen system now simpler and will
    //  dynamically change for Windib or Directx driver. Windowed/Fullscreen
    //  toggle also made available from menu.
    //  Replace attempt_fullscreen_toggle(SDL_Surface **surface, Uint32 *flags)

    int32_t x, y;
    x = surface->w;
    y = surface->h;

    FullScreen = !FullScreen;
    _platform_init(0, NULL, "Duke Nukem 3D", "Duke3D");
    VID_SetGameMode(x, y);
    //vscrn();

    return;
}

static int sdl_key_filter(const SDL_Event* event) {
    int extended;

    if ((event->key.keysym.sym == SDLK_m) && (event->key.state == SDL_PRESSED)
        && (event->key.keysym.mod & KMOD_CTRL)) {
        // FIX_00005: Mouse pointer can be toggled on/off (see mouse menu or use CTRL-M)
        // This is usefull to move the duke window when playing in window mode.

        if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
            SDL_CHECK_SUCCESS(SDL_SetRelativeMouseMode(SDL_FALSE));
        } else {
            SDL_CHECK_SUCCESS(SDL_SetRelativeMouseMode(SDL_TRUE));
        }

        return (0);
    }

    if (((event->key.keysym.sym == SDLK_RETURN)
         || (event->key.keysym.sym == SDLK_KP_ENTER))
        && (event->key.state == SDL_PRESSED)
        && (event->key.keysym.mod & KMOD_ALT)) {
        fullscreen_toggle_and_change_driver();

        // hack to discard the ALT key...
        lastkey = scancodes[SDL_SCANCODE_RALT] >> 8; // extended
        keyhandler();
        lastkey =
            (scancodes[SDL_SCANCODE_RALT] & 0xff) + 0x80; // Simulating Key up
        keyhandler();
        lastkey = (scancodes[SDL_SCANCODE_LALT] & 0xff)
                  + 0x80; // Simulating Key up (not extended)
        keyhandler();
        SDL_SetModState(
            KMOD_NONE); // SDL doesnt see we are releasing the ALT-ENTER keys

        return (0);
    }

    if (!handle_keypad_enter_hack(event))
        lastkey = scancodes[event->key.keysym.scancode];

    //	printf("key.keysym.sym=%d\n", event->key.keysym.sym);

    if (lastkey == 0x0000) /* No DOS equivalent defined. */
        return (0);

    extended = ((lastkey & 0xFF00) >> 8);
    if (extended != 0) {
        lastkey = extended;
        keyhandler();
        lastkey = (scancodes[event->key.keysym.scancode] & 0xFF);
    } /* if */

    if (event->key.state == SDL_RELEASED)
        lastkey += 128; /* +128 signifies that the key is released in DOS. */

    keyhandler();
    return (0);
}


static int root_sdl_event_filter(const SDL_Event* event) {
    switch (event->type) {
        case SDL_KEYUP:
            // FIX_00003: Pause mode is now fully responsive - (Thx to Jonathon Fowler tips)
            if (event->key.keysym.sym == SDLK_PAUSE)
                break;
            if (event->key.keysym.scancode == SDL_SCANCODE_CAPSLOCK)
                break;
        case SDL_KEYDOWN:
            return (sdl_key_filter(event));
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP: {
            //Do Nothing

            //printf("Joybutton UP/DOWN\n");
            //return(sdl_joystick_button_filter((const SDL_MouseButtonEvent*)event));
            return 0;
        }
        case SDL_JOYBALLMOTION:
        case SDL_MOUSEMOTION:
            return (sdl_mouse_motion_filter(event));
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
            return (
                sdl_mouse_button_filter((const SDL_MouseButtonEvent*) event));
        case SDL_MOUSEWHEEL:
            if (event->wheel.y > 0) {
                mouse_buttons ^= 8;
            } else if (event->wheel.y < 0) {
                mouse_buttons ^= 16;
            }
            break;
        case SDL_QUIT:
            /* !!! rcg TEMP */
            Error(EXIT_SUCCESS, "Exit through SDL\n");
        default:
            //printf("This event is not handled: %d\n",event->type);
            break;
    } /* switch */

    return (1);
}


static void handle_events(void) {
    SDL_Event event;

    while (SDL_PollEvent(&event))
        root_sdl_event_filter(&event);
} /* handle_events */


/* bleh...public version... */
void _handle_events(void) {
    handle_events();
}


uint8_t _readlastkeyhit(void) {
    return (lastkey);
} /* _readlastkeyhit */


static void output_sdl_versions(void) {
    SDL_version linked_ver;
    SDL_GetVersion(&linked_ver);

    SDL_version compiled_ver;
    SDL_VERSION(&compiled_ver);

    printf("SDL Display driver for the BUILD engine initializing.\n");
    printf("SDL Compiled %s against SDL version %d.%d.%d ...\n", __DATE__,
           compiled_ver.major, compiled_ver.minor, compiled_ver.patch);
    printf("SDL Linked against SDL version %d.%d.%d ...\n", linked_ver.major,
           linked_ver.minor, linked_ver.patch);
}

/* lousy -ansi flag.  :) */
static char* string_dupe(const char* str) {
    char* retval = malloc(strlen(str) + 1);
    if (retval != NULL)
        strcpy(retval, str);
    return (retval);
}

void _platform_init(int argc, char** argv, const char* title,
                    const char* iconName) {
    int64_t timeElapsed;

    // FIX_00061: "ERROR: Two players have the same random ID" too frequent cuz of internet windows times
    timeElapsed = SDL_GetTicks();
    srand(timeElapsed & 0xFFFFFFFF);

    SDL_CHECK_SUCCESS(SDL_Init(SDL_INIT_VIDEO));

    if (title == NULL)
        title = "BUILD";

    titleName = string_dupe(title);
    set_sdl_scancodes(scancodes, sizeof(scancodes) / sizeof(scancodes[0]));

    output_sdl_versions();

    printf("SDL Video Driver: '%s'.\n", SDL_GetCurrentVideoDriver());
}

// Capture BMP of the current frame
void screencapture(char* filename) {
    //  FIX_00006: better naming system for screenshots + message when pic is taken.
    //  Use ./screenshots folder. Screenshot code rerwritten. Faster and
    //  makes smaller files. Doesn't freeze or lag the game anymore.
    if (SDL_SaveBMP(surface, filename)) {
        printf("SDL failed to save screenshot '%s' (%s)\n", filename, SDL_GetError());
    }
}


static int get_dimensions_from_str(const char* str, int32_t* _w, int32_t* _h) {
    char* xptr = NULL;
    char* ptr = NULL;
    int32_t w = -1;
    int32_t h = -1;

    if (str == NULL)
        return (0);

    xptr = strchr(str, 'x');
    if (xptr == NULL)
        return (0);

    w = strtol(str, &ptr, 10);
    if (ptr != xptr)
        return (0);

    xptr++;
    h = strtol(xptr, &ptr, 10);
    if ((*xptr == '\0') || (*ptr != '\0'))
        return (0);

    if ((w <= 1) || (h <= 1))
        return (0);

    if (_w != NULL)
        *_w = w;

    if (_h != NULL)
        *_h = h;

    return (1);
}


static void get_max_screen_res(int32_t* max_w, int32_t* max_h) {
    int32_t w = DEFAULT_MAXRESWIDTH;
    int32_t h = DEFAULT_MAXRESHEIGHT;
    const char* envr = getenv(BUILD_MAXSCREENRES);

    if (envr != NULL) {
        if (!get_dimensions_from_str(envr, &w, &h)) {
            printf("User's resolution ceiling [%s] is bogus!\n", envr);
            w = DEFAULT_MAXRESWIDTH;
            h = DEFAULT_MAXRESHEIGHT;
        } /* if */
    }     /* if */

    if (max_w != NULL)
        *max_w = w;

    if (max_h != NULL)
        *max_h = h;
}


static void add_vesa_mode(int w, int h) {
    validmode[validmodecnt] = validmodecnt;
    validmodexdim[validmodecnt] = w;
    validmodeydim[validmodecnt] = h;
    validmodecnt++;
}

/* Let the user specify a specific mode via environment variable. */
static void add_user_defined_resolution(void) {
    int32_t w;
    int32_t h;
    const char* envr = getenv(BUILD_USERSCREENRES);

    if (envr == NULL)
        return;

    if (get_dimensions_from_str(envr, &w, &h))
        add_vesa_mode(w, h);
    else
        printf("User defined resolution [%s] is bogus!\n", envr);
}

static void remove_vesa_mode(int index, const char* reason) {
    int i;

    assert(index < validmodecnt);
    //printf("Removing resolution #%d, %dx%d [%s].\n",index, validmodexdim[index], validmodeydim[index], reason);

    for (i = index; i < validmodecnt - 1; i++) {
        validmode[i] = validmode[i + 1];
        validmodexdim[i] = validmodexdim[i + 1];
        validmodeydim[i] = validmodeydim[i + 1];
    } /* for */

    validmodecnt--;
}


static void cull_large_vesa_modes(void) {
    int32_t max_w;
    int32_t max_h;
    int i;

    get_max_screen_res(&max_w, &max_h);
    printf("Setting resolution ceiling to (%dx%d).\n", max_w, max_h);

    for (i = 0; i < validmodecnt; i++) {
        if ((validmodexdim[i] > max_w) || (validmodeydim[i] > max_h)) {
            remove_vesa_mode(i, "above resolution ceiling");
            i--; /* list shrinks. */
        }        /* if */
    }            /* for */
}


static void cull_duplicate_vesa_modes(void) {
    int i;
    int j;

    for (i = 0; i < validmodecnt; i++) {
        for (j = i + 1; j < validmodecnt; j++) {
            if ((validmodexdim[i] == validmodexdim[j])
                && (validmodeydim[i] == validmodeydim[j])) {
                remove_vesa_mode(j, "duplicate");
                j--; /* list shrinks. */
            }
        }
    }
}


/* be sure to call cull_duplicate_vesa_modes() before calling this. */
static void sort_vesa_modelist(void) {
    int i;
    int sorted;
    int32_t tmp;

    do {
        sorted = 1;
        for (i = 0; i < validmodecnt - 1; i++) {
            if ((validmodexdim[i] >= validmodexdim[i + 1])
                && (validmodeydim[i] >= validmodeydim[i + 1])) {
                sorted = 0;
                swap_macro(tmp, validmode[i], validmode[i + 1]);
                swap_macro(tmp, validmodexdim[i], validmodexdim[i + 1]);
                swap_macro(tmp, validmodeydim[i], validmodeydim[i + 1]);
            } /* if */
        }     /* for */
    } while (!sorted);
}


static void cleanup_vesa_modelist(void) {
    cull_large_vesa_modes();
    cull_duplicate_vesa_modes();
    sort_vesa_modelist();
}


static void output_vesa_modelist(void) {
    char buffer[512];
    char numbuf[20];
    int i;

    buffer[0] = '\0';

    for (i = 0; i < validmodecnt; i++) {
        sprintf(numbuf, " (%dx%d)", (int32_t) validmodexdim[i],
                (int32_t) validmodeydim[i]);

        if ((strlen(buffer) + strlen(numbuf)) >= (sizeof(buffer) - 1))
            strcpy(buffer + (sizeof(buffer) - 5), " ...");
        else
            strcat(buffer, numbuf);
    } /* for */

    printf("Final sorted modelist:%s\n", buffer);
}


void getvalidvesamodes(void) {
    static int already_checked = 0;
    int i;
    int stdres[][2] = {{320, 240}, {640, 480}, {800, 600}, {1024, 768},
                       {1280, 960}, {1440, 1080}, {1600, 1200}, {1920, 1440},
                       {2560, 1920}, {2880, 2160}};

    if (already_checked)
        return;

    already_checked = 1;
    validmodecnt = 0;
    vidoption = 1; /* !!! tmp */

    // Fill in the standard 4:3 resolutions that the display supports
    int numModes = SDL_GetNumDisplayModes(0);
    int maxWidth = 0;
    int maxHeight = 0;

    for (i = 0; i < numModes; ++i) {
        SDL_DisplayMode mode;
        SDL_CHECK_SUCCESS(SDL_GetDisplayMode(0, i, &mode));

        maxWidth = max(maxWidth, mode.w);
        maxHeight = max(maxHeight, mode.h);
    }

    for (i = 0; i < sizeof(stdres) / sizeof(stdres[0]); i++) {
        assert(stdres[i][0] / 4 == stdres[i][1] / 3);

        if (stdres[i][0] <= maxWidth && stdres[i][1] <= maxHeight) {
            add_vesa_mode(stdres[i][0], stdres[i][1]);
        }
    }

    /* Now add specific resolutions that the user wants... */
    add_user_defined_resolution();

    /* get rid of dupes and bogus resolutions... */
    cleanup_vesa_modelist();

    /* print it out for debugging purposes... */
    output_vesa_modelist();
}


void _uninitengine(void) {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

int setupmouse(void) {

    SDL_Event event;

    if (surface == NULL)
        return (0);

    SDL_CHECK_SUCCESS(SDL_SetRelativeMouseMode(SDL_TRUE));

    mouse_relative_x = mouse_relative_y = 0;

    /*
	 * this global usually gets set by BUILD, but it's a one-shot
	 *  deal, and we may not have an SDL surface at that point. --ryan.
	 */
    moustat = 1;

    // FIX_00063: Duke's angle changing or incorrect when using toggle fullscreen/window mode
    while (SDL_PollEvent(&event));
    // Empying the various pending events (especially the mouse one)

    //SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

    return (1);
}


void readmousexy(short* x, short* y) {
    if (x)
        *x = mouse_relative_x << 2;
    if (y)
        *y = mouse_relative_y << 2;

    mouse_relative_x = mouse_relative_y = 0;
}


void readmousebstatus(short* bstatus) {
    if (bstatus)
        *bstatus = mouse_buttons;

    // special wheel treatment: make it like a button click
    if (mouse_buttons & 8)
        mouse_buttons ^= 8;
    if (mouse_buttons & 16)
        mouse_buttons ^= 16;

}


void _updateScreenRect(int32_t x, int32_t y, int32_t w, int32_t h) {
    SDL_Rect rect = {x, y, w, h};

    SDL_CHECK_SUCCESS(SDL_BlitSurface(surface, &rect, surface_rgba, &rect));
    SDL_CHECK_SUCCESS(SDL_UpdateTexture(texture, &rect, surface_rgba->pixels,
        surface_rgba->pitch));
    SDL_CHECK_SUCCESS(SDL_RenderCopy(renderer, texture, &rect, &rect));
    SDL_RenderPresent(renderer);
}

void _nextpage(void) {
    Uint32 ticks;

    _handle_events();

    present_framebuffer();

    //sprintf(bmpName,"%d.bmp",counter++);
    //SDL_SaveBMP(surface,bmpName);

    //if (CLEAR_FRAMEBUFFER)
    //    SDL_FillRect(surface,NULL,0);

    ticks = getticks();
    total_render_time = (ticks - last_render_ticks);
    if (total_render_time > 1000) {
        total_rendered_frames = 0;
        total_render_time = 1;
        last_render_ticks = ticks;
    }
    total_rendered_frames++;
}

uint8_t readpixel(uint8_t* offset) {
    return *offset;
}

void drawpixel(uint8_t* location, uint8_t pixel) {
    *location = pixel;
}


void clear2dscreen(void) {
    SDL_Rect rect;

    rect.x = rect.y = 0;
    rect.w = surface->w;
    rect.h = surface->h;

    if (qsetmode == 350)
        rect.h = 350;
    else if (qsetmode == 480) {
        if (ydim16 <= 336)
            rect.h = 336;
        else
            rect.h = 480;
    } /* else if */

    SDL_FillRect(surface, &rect, 0);
}


void _idle(void) {
    if (surface) {
        _handle_events();
    }
    SDL_Delay(1);
}