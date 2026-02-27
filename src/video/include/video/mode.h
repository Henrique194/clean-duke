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

#ifndef VID_MODE_H
#define VID_MODE_H

#include "build/platform.h"

//
// All this function is doing is setting the video mode.
//
void VID_SetMode(int mode);

//
// This function sets the video mode to 320*200*256color graphics.
// Since BUILD supports several different modes including mode x,
// mode 13h, and other special modes, I don't expect you to write
// any graphics output functions.  (Soon I have all the necessary
// functions)  If for some reason, you use your own graphics mode,
// you must call this function again before using the BUILD drawing
// functions.
//
// resolution inits. sdl_driver.c ...
//
int32_t VID_SetGameMode(int32_t daxdim, int32_t daydim);

#endif
