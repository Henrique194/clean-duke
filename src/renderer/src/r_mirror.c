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
// drawing functions

#include "renderer/renderer.h"
#include "build/engine.h"
#include "video/display.h"
#if WIN32
#include "io.h"
#endif

extern int32_t transarea;

uint8_t inpreparemirror = 0;
int32_t mirrorsx1;
int32_t mirrorsy1;
int32_t mirrorsx2;
int32_t mirrorsy2;


void preparemirror(
    int32_t dax,
    int32_t day,
    short daang,
    short dawall,
    int32_t* tposx,
    int32_t* tposy,
    short* tang
) {
    const walltype* wal1 = &wall[dawall];
    const walltype* wal2 = &wall[wal1->point2];

    int32_t x = wal1->x;
    int32_t dx = wal2->x - x;
    int32_t y = wal1->y;
    int32_t dy = wal2->y - y;
    int32_t j = dx * dx + dy * dy;
    if (j == 0) {
        return;
    }
    int32_t i = (((dax - x) * dx + (day - y) * dy) << 1);
    *tposx = (x << 1) + scale(dx, i, j) - dax;
    *tposy = (y << 1) + scale(dy, i, j) - day;
    *tang = (((getangle(dx, dy) << 1) - daang) & 2047);

    inpreparemirror = 1;
}

//
// Reverse screen x-wise in this function.
//
void completemirror(void) {
    // Can't reverse with uninitialized data.
    if (inpreparemirror) {
        inpreparemirror = 0;
        return;
    }
    if (mirrorsx1 > 0) {
        mirrorsx1--;
    }
    if (mirrorsx2 < windowx2 - windowx1 - 1) {
        mirrorsx2++;
    }
    if (mirrorsx2 < mirrorsx1) {
        return;
    }

    transarea += (mirrorsx2 - mirrorsx1) * (windowy2 - windowy1);

    intptr_t p = (intptr_t) (frameplace + ylookup[windowy1 + mirrorsy1] + windowx1 + mirrorsx1);
    int32_t i = windowx2 - windowx1 - mirrorsx2 - mirrorsx1;
    mirrorsx2 -= mirrorsx1;
    // FIX_00085: Optimized Video driver. FPS increases by +20%.
    for (int32_t dy = mirrorsy2 - mirrorsy1 - 1; dy >= 0; dy--) {
        copybufbyte((void*) (p), r_tempbuf, mirrorsx2 + 1);
        r_tempbuf[mirrorsx2] = r_tempbuf[mirrorsx2 - 1];
        copybufreverse(&r_tempbuf[mirrorsx2], (void*) (p + i), mirrorsx2 + 1);
        p += ylookup[1];
        faketimerhandler();
    }
}