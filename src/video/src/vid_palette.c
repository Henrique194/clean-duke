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


#include "video/palette.h"
#include "video/display.h"
#include "build/engine.h"
#include "build/sdl_util.h"
#include "renderer/draw.h"
#include "memory/cache.h"


#define FASTPALGRIDSIZ 8


void present_framebuffer(void);

extern uint8_t paletteloaded;
extern int32_t globalpal;
uint8_t britable[16][64];

extern SDL_Surface* surface;


static uint8_t lastPalette[768];

static uint8_t colhere[((FASTPALGRIDSIZ + 2) * (FASTPALGRIDSIZ + 2) * (FASTPALGRIDSIZ + 2)) >> 3];
static uint8_t colhead[(FASTPALGRIDSIZ + 2) * (FASTPALGRIDSIZ + 2) * (FASTPALGRIDSIZ + 2)];

static int32_t rdist[129];
static int32_t gdist[129];
static int32_t bdist[129];

static int32_t colnext[256];
static uint8_t coldist[8] = {0, 1, 2, 3, 4, 3, 2, 1};
static int32_t colscan[27];


static void initfastcolorlookup(int32_t rscale, int32_t gscale, int32_t bscale) {
    int32_t i, j, x, y, z;
    uint8_t* pal1;

    j = 0;
    for (i = 64; i >= 0; i--) {
        /*j = (i-64)*(i-64);*/
        rdist[i] = rdist[128 - i] = j * rscale;
        gdist[i] = gdist[128 - i] = j * gscale;
        bdist[i] = bdist[128 - i] = j * bscale;
        j += 129 - (i << 1);
    }

    clearbufbyte(colhere, sizeof(colhere), 0L);
    clearbufbyte(colhead, sizeof(colhead), 0L);

    pal1 = &palette[768 - 3];
    for (i = 255; i >= 0; i--, pal1 -= 3) {
        j = (pal1[0] >> 3) * FASTPALGRIDSIZ * FASTPALGRIDSIZ + (pal1[1] >> 3) *
            FASTPALGRIDSIZ + (pal1[2] >> 3) + FASTPALGRIDSIZ * FASTPALGRIDSIZ +
            FASTPALGRIDSIZ + 1;
        if (colhere[j >> 3] & pow2char[j & 7])
            colnext[i] = colhead[j];
        else
            colnext[i] = -1;
        colhead[j] = i;
        colhere[j >> 3] |= pow2char[j & 7];
    }

    i = 0;
    for (x = -FASTPALGRIDSIZ * FASTPALGRIDSIZ;
         x <= FASTPALGRIDSIZ * FASTPALGRIDSIZ;
         x += FASTPALGRIDSIZ * FASTPALGRIDSIZ)
        for (y = -FASTPALGRIDSIZ; y <= FASTPALGRIDSIZ; y += FASTPALGRIDSIZ)
            for (z = -1; z <= 1; z++)
                colscan[i++] = x + y + z;
    i = colscan[13];
    colscan[13] = colscan[26];
    colscan[26] = i;
}


static int getclosestcol(int32_t r, int32_t g, int32_t b) {
    int32_t i, j, k, dist, mindist, retcol;
    uint8_t* pal1;

    j = (r >> 3) * FASTPALGRIDSIZ * FASTPALGRIDSIZ + (g >> 3) * FASTPALGRIDSIZ +
        (b >> 3) + FASTPALGRIDSIZ * FASTPALGRIDSIZ + FASTPALGRIDSIZ + 1;
    mindist = min(rdist[coldist[r&7]+64+8], gdist[coldist[g&7]+64+8]);
    mindist = min(mindist, bdist[coldist[b&7]+64+8]);
    mindist++;

    r = 64 - r;
    g = 64 - g;
    b = 64 - b;

    retcol = -1;
    for (k = 26; k >= 0; k--) {
        i = colscan[k] + j;
        if ((colhere[i >> 3] & pow2char[i & 7]) == 0)
            continue;
        i = colhead[i];
        do {
            pal1 = (uint8_t*) &palette[i * 3];
            dist = gdist[pal1[1] + g];
            if (dist < mindist) {
                dist += rdist[pal1[0] + r];
                if (dist < mindist) {
                    dist += bdist[pal1[2] + b];
                    if (dist < mindist) {
                        mindist = dist;
                        retcol = i;
                    }
                }
            }
            i = colnext[i];
        } while (i >= 0);
    }
    if (retcol >= 0)
        return (retcol);

    mindist = 0x7fffffff;
    pal1 = (uint8_t*) &palette[768 - 3];
    for (i = 255; i >= 0; i--, pal1 -= 3) {
        dist = gdist[pal1[1] + g];
        if (dist >= mindist)
            continue;
        dist += rdist[pal1[0] + r];
        if (dist >= mindist)
            continue;
        dist += bdist[pal1[2] + b];
        if (dist >= mindist)
            continue;
        mindist = dist;
        retcol = i;
    }
    return (retcol);
}


void VID_LoadPalette(void) {
    int32_t k, fil;


    if (paletteloaded != 0)
        return;

    if ((fil = TCkopen4load("palette.dat", 0)) == -1)
        return;

    kread(fil, palette, 768);

    //CODE EXPLORATION
    //WritePaletteToFile(palette,"palette.tga",16, 16);
    memcpy(lastPalette, palette, 768);


    kread16(fil, &numpalookups);

    //CODE EXPLORATION
    //printf("Num palettes lookup: %d.\n",numpalookups);

    if ((palookup[0] = (uint8_t*) malloc(numpalookups << 8)) == NULL)
        allocache(&palookup[0], numpalookups << 8, &permanentlock);

    //Transluctent pallete is 65KB.
    if ((transluc = (uint8_t*) malloc(65536)) == NULL)
        allocache(&transluc, 65536, &permanentlock);

    globalpalwritten = palookup[0];
    globalpal = 0;


    kread(fil, palookup[globalpal], numpalookups << 8);


    /*kread(fil,transluc,65536);*/
    for (k = 0; k < (65536 / 4); k++)
        kread32(fil, ((int32_t*) transluc) + k);


    kclose(fil);

    initfastcolorlookup(30L, 59L, 11L);

    paletteloaded = 1;

}


void VID_SetPalette(const uint8_t* pal) {
    SDL_Color fmt_swap[256];
    SDL_Color* sdlp = fmt_swap;

    // CODE EXPLORATION
    // Used only to write the last palette to file.
    memcpy(lastPalette, pal, 768);

    // Naturally, the bytes are in the reverse order that SDL wants them...
    for (int i = 0; i < 256; i++) {
        // SDL wants the color elements in a range from 0-255,
        // so we do a conversion.
        sdlp->r = pal[2] * 4;
        sdlp->g = pal[1] * 4;
        sdlp->b = pal[0] * 4;
        sdlp->a = SDL_ALPHA_OPAQUE;

        sdlp++;
        pal += 4;
    }

    int res = SDL_SetPaletteColors(surface->format->palette, fmt_swap, 0, 256);
    if (res != 0) {
        Error(EXIT_FAILURE, "Failed to set palette: %s\n", SDL_GetError());
    }
}


void VID_MakePalLookup(
    int32_t palnum,
    uint8_t* remapbuf,
    int8_t r,
    int8_t g,
    int8_t b,
    uint8_t dastat
) {
    int32_t i, j, palscale;
    uint8_t *ptr, *ptr2;

    if (paletteloaded == 0)
        return;

    if (palookup[palnum] == NULL) {
        /* Allocate palookup buffer */
        if ((palookup[palnum] = (uint8_t*) malloc(numpalookups << 8)) == NULL)
            allocache(&palookup[palnum], numpalookups << 8, &permanentlock);
    }

    if (dastat == 0)
        return;
    if ((r | g | b | 63) != 63)
        return;

    if ((r | g | b) == 0) {
        for (i = 0; i < 256; i++) {
            ptr = palookup[0] + remapbuf[i];
            ptr2 = palookup[palnum] + i;
            for (j = 0; j < numpalookups; j++) {
                *ptr2 = *ptr;
                ptr += 256;
                ptr2 += 256;
            }
        }
    } else {
        ptr2 = palookup[palnum];
        for (i = 0; i < numpalookups; i++) {
            palscale = divscale16(i, numpalookups);
            for (j = 0; j < 256; j++) {
                ptr = (uint8_t*) &palette[remapbuf[j] * 3];
                *ptr2++ = getclosestcol(
                    (int32_t) ptr[0] + mulscale16(r - ptr[0], palscale),
                    (int32_t) ptr[1] + mulscale16(g - ptr[1], palscale),
                    (int32_t) ptr[2] + mulscale16(b - ptr[2], palscale));
            }
        }
    }
}


void VID_SetBrightness(uint8_t brightness, const uint8_t* pal) {
    // Clamp brightness to [0-15].
    curbrightness = SDL_min(brightness, 15);

    uint8_t new_palette[256 * 4];
    int32_t k = 0;
    for (int32_t i = 0; i < 256; i++) {
        new_palette[k++] = britable[curbrightness][pal[i * 3 + 2]];
        new_palette[k++] = britable[curbrightness][pal[i * 3 + 1]];
        new_palette[k++] = britable[curbrightness][pal[i * 3 + 0]];
        new_palette[k++] = 0;
    }

    VID_SetPalette(new_palette);
}


//
// Updating the palette is not immediate with a buffered surface, screen needs updating as well.
// Call this function if nextpage() is not called. E.g. static intro logo.
//
void VID_PresentPalette() {
    present_framebuffer();
}
