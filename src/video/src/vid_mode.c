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


#include "video/mode.h"
#include "video/display.h"
#include "build/engine.h"
#include "build/sdl_util.h"
#include "build/icon.h"
#include "renderer/draw.h"
#include "build/timer.h"
#include "memory/cache.h"


extern int32_t last_render_ticks;
extern char* titleName;
extern SDL_Renderer* renderer;
extern SDL_Texture* texture;
extern SDL_Surface* surface;
extern SDL_Surface* surface_rgba;


static SDL_Window* window = NULL;


//
// This is almost an entire copy of the original setgamemode().
// Figure out what is needed for just 2D mode, and separate that
// out. Then, place the original setgamemode() back into engine.c,
// and remove our simple implementation (and this function.)
// Just be sure to keep the non-DOS things, like the window's
// titlebar caption.   --ryan.
//
static void init_new_res_vars() {
    static uint8_t screenalloctype = 255;

    int i = 0;
    int j = 0;

    setupmouse();

    xdim = xres = surface->w;
    ydim = yres = surface->h;

    printf("init_new_res_vars %d %d\n", xdim, ydim);

    numpages = 1; // we always expose the same surface to the drawing engine.
    bytesperline = surface->w;
    vesachecked = 1;
    vgacompatible = 1;
    linearmode = 1;
    qsetmode = surface->h;
    activepage = visualpage = 0;


    frameoffset = frameplace = (uint8_t*) surface->pixels;

    if (screen != NULL) {
        if (screenalloctype == 0)
            free((void*) screen);
        if (screenalloctype == 1)
            suckcache((int32_t*) screen);
        screen = NULL;
    } /* if */


    switch (vidoption) {
        case 1:
            i = xdim * ydim;
            break;
        case 2:
            xdim = 320;
            ydim = 200;
            i = xdim * ydim;
            break;
        default:
            assert(0);
    }
    // Leave room for horizlookup&horizlookup2.
    j = ydim * 4 * sizeof(int32_t);

    if (horizlookup)
        free(horizlookup);

    if (horizlookup2)
        free(horizlookup2);

    horizlookup = (int32_t*) malloc(j);
    horizlookup2 = (int32_t*) malloc(j);

    j = 0;

    //Build lookup table (X screespace -> frambuffer offset.
    for (i = 0; i <= ydim; i++) {
        ylookup[i] = j;
        j += bytesperline;
    }

    horizycent = ((ydim * 4) >> 1);

    /* Force drawrooms to call dosetaspect & recalculate stuff */
    oxyaspect = oxdimen = oviewingrange = -1;

    //Let the Assembly module how many pixels to skip when drawing a column
    setBytesPerLine(bytesperline);


    setview(0L, 0L, xdim - 1, ydim - 1);

    VID_SetBrightness(curbrightness, palette);

    if (searchx < 0) {
        searchx = halfxdimen;
        searchy = (ydimen >> 1);
    }
}

static void go_to_new_vid_mode(int w, int h) {
    if (window != NULL) {
        SDL_FreeSurface(surface_rgba);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        window = NULL;
    }

    SDL_CHECK_SUCCESS(SDL_InitSubSystem(SDL_INIT_VIDEO));

    window = SDL_CreateWindow(titleName, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, w, h,
                              (FullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
                              | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_CHECK_NOT_NULL(window, "create window");

    // don't override higher-res app icon on OS X or Windows
#if !PLATFORM_MACOSX && !WIN32
    SDL_Surface* image =
        SDL_LoadBMP_RW(SDL_RWFromMem(iconBMP, sizeof(iconBMP)), 1);
    Uint32 colorkey = 0; // index in this image to be transparent
    SDL_SetColorKey(image, SDL_TRUE, colorkey);
    SDL_SetWindowIcon(window, image);
    SDL_FreeSurface(image);
#endif

    getvalidvesamodes();
    SDL_ClearError();

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_CHECK_NOT_NULL(renderer, "create renderer");

    SDL_RendererInfo rendererInfo;
    SDL_CHECK_SUCCESS(SDL_GetRendererInfo(renderer, &rendererInfo));
    printf("SDL Renderer: '%s'.\n", rendererInfo.name);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    //SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    SDL_CHECK_SUCCESS(SDL_RenderSetLogicalSize(renderer, w, h));

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, w, h);
    SDL_CHECK_NOT_NULL(texture, "create texture");

    surface = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);
    SDL_CHECK_NOT_NULL(surface, "create palettized surface");

    const Uint32 rmask = 0x00ff0000;
    const Uint32 gmask = 0x0000ff00;
    const Uint32 bmask = 0x000000ff;
    const Uint32 amask = 0xff000000;

    surface_rgba = SDL_CreateRGBSurface(0, w, h, 32, rmask, gmask, bmask, amask);
    SDL_CHECK_NOT_NULL(surface_rgba, "create RGBA surface");

    init_new_res_vars();
}


void VID_SetMode(int mode) {
    if (mode == 0x3) {
        // Text mode.
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return;
    }
    printf("setvmode(0x%x) is unsupported in SDL driver.\n", mode);
}

int VID_SetGameMode(int32_t daxdim, int32_t daydim) {
    if (daxdim > MAXXDIM || daydim > MAXYDIM) {
        printf("Resolution %dx%d is too high. Changed to %dx%d\n", daxdim, daydim, MAXXDIM, MAXYDIM);
        daxdim = MAXXDIM;
        daydim = MAXYDIM;
    }

    getvalidvesamodes();

    int validated = 0;
    for (int i = 0; i < validmodecnt; i++) {
        if (validmodexdim[i] == daxdim && validmodeydim[i] == daydim) {
            validated = 1;
        }
    }
    if (!validated) {
        printf("Resolution %dx%d unsupported. Changed to 640x480\n", daxdim, daydim);
        daxdim = 640;
        daydim = 480;
    }

    go_to_new_vid_mode(daxdim, daydim);

    qsetmode = 200;
    last_render_ticks = getticks();

    return 0;
}
