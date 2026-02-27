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

#ifndef BUILD_SPRITE_H
#define BUILD_SPRITE_H

#include "build/build.h"

extern short headspritesect[MAXSECTORS + 1];
extern short headspritestat[MAXSTATUS + 1];
extern short prevspritesect[MAXSPRITES];
extern short prevspritestat[MAXSPRITES];
extern short nextspritesect[MAXSPRITES];
extern short nextspritestat[MAXSPRITES];

//
// Returns (short)spritenum;
//
// Whenever you insert a sprite, you must pass it the sector
// number, and a status number (statnum). The status number can
// be any number from 0 to MAXSTATUS-1. This functions works like
// a memory allocation function and returns the sprite number.
//
int B_InsertSprite(short sectnum, short statnum);

//
// Deletes the sprite.
//
int deletesprite(short spritenum);

//
// This function simply sets the sprite's position to a specified
// coordinate (newx, newy, newz) without any checking to see
// whether the position is valid or not.  You could directly
// modify the sprite[].x, sprite[].y, and sprite[].z values,
// but if you use my function, the sprite is guaranteed to be
// in the right sector.
//
int B_SetSprite(short spritenum, int32_t newx, int32_t newy, int32_t newz);

//
// Changes the sector of sprite (spritenum) to the
// newsector (newsectnum). This function may become
// internal to the engine in the movesprite function.
// But this function is necessary since all the sectors
// have their own doubly-linked lists of sprites.
//
int B_ChangeSpriteSect(short spritenum, short newsectnum);

//
// Changes the status of sprite (spritenum) to status (newstatus).
// Newstatus can be any number from 0 to MAXSTATUS-1. You can use
// this function to put a monster on a list of active sprites
// when it first sees you.
//
int B_ChangeSpriteStat(short spritenum, short newstatnum);

void B_InitSpriteLists(void);

#endif
