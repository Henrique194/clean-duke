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

#ifndef VID_PALETTE_H
#define VID_PALETTE_H

#include "build/platform.h"

void VID_LoadPalette(void);

//
// Set (num) palette palette entries starting at (start)
// palette entries are in a 4-byte format in this order:
//   0: Blue (0-63)
//   1: Green (0-63)
//   2: Red (0-63)
//   3: Reserved
//
void VID_SetPalette(const uint8_t* pal);

void VID_PresentPalette(void);

void VID_MakePalLookup(
    int32_t palnum,
    uint8_t* remapbuf,
    int8_t r,
    int8_t g,
    int8_t b,
    uint8_t dastat
);

void VID_SetBrightness(uint8_t brightness, const uint8_t* pal);

#endif
