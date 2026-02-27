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

#include "duke3d.h"
#include "actors/actors.h"
#include "build/engine.h"
#include "funct.h"

int32_t numinterpolations = 0;
int32_t bakipos[MAXINTERPOLATIONS];
int32_t* curipos[MAXINTERPOLATIONS];
static int32_t oldipos[MAXINTERPOLATIONS];

//
// Stick at beginning of domovethings
//
void P_UpdateInterpolations(void) {
    for (i32 i = numinterpolations - 1; i >= 0; i--) {
        oldipos[i] = *curipos[i];
    }
}


void P_SetInterpolation(int32_t* posptr) {
    if (numinterpolations >= MAXINTERPOLATIONS) {
        return;
    }
    for (i32 i = numinterpolations - 1; i >= 0; i--) {
        if (curipos[i] == posptr) {
            return;
        }
    }
    curipos[numinterpolations] = posptr;
    oldipos[numinterpolations] = *posptr;
    numinterpolations++;
}

void P_StopInterpolation(const i32* posptr) {
    for (i32 i = numinterpolations - 1; i >= 0; i--)
        if (curipos[i] == posptr) {
            numinterpolations--;
            oldipos[i] = oldipos[numinterpolations];
            bakipos[i] = bakipos[numinterpolations];
            curipos[i] = curipos[numinterpolations];
        }
}

//
// Stick at beginning of drawscreen.
//
void P_DoInterpolations(int32_t smoothratio) {
    i32 j = 0;
    i32 ndelta = 0;
    i32 i = numinterpolations - 1;
    for (; i >= 0; i--) {
        bakipos[i] = *curipos[i];
        i32 odelta = ndelta;
        ndelta = (*curipos[i]) - oldipos[i];
        if (odelta != ndelta) {
            j = mulscale16(ndelta, smoothratio);
        }
        *curipos[i] = oldipos[i] + j;
    }
}

//
// Stick at end of drawscreen.
//
void P_RestoreInterpolations(void) {
    for (i32 i = numinterpolations - 1; i >= 0; i--) {
        *curipos[i] = bakipos[i];
    }
}
