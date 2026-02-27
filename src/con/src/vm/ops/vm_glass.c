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

#include "vm_ops.h"
#include "funct.h"
#include "names.h"
#include "build/engine.h"

static void CON_SpawnGlass(i16 owner, i32 pic_offset) {
    const spritetype* spr_owner = &sprite[owner];
    i16 sec = spr_owner->sectnum;
    i32 x = spr_owner->x;
    i32 y = spr_owner->y;
    i32 z = spr_owner->z - ((TRAND & 16) << 8);
    i16 pn = GLASSPIECES + (pic_offset % 3);
    i8 shade = TRAND & 15;
    i8 xr = 36;
    i8 yr = 36;
    i16 ang = TRAND & 2047;
    i16 ve = 32 + (TRAND & 63);
    i32 zv = -512 - (TRAND & 2047);
    i8 stat = 5;
    i32 i = EGS(sec, x, y, z, pn, shade, xr, yr, ang, ve, zv, owner, stat);
    sprite[i].pal = spr_owner->pal;
}

void VM_LotsOfGlass(con_vm_t* vm) {
    i16 num_glass = (i16) VM_READ(vm);
    for (i32 i = num_glass; i > 0; i--) {
        CON_SpawnGlass(vm->sprite_idx, i);
    }
}
