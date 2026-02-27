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
#include "names.h"
#include "build/fixedPoint_math.h"
#include "build/tiles.h"

static void CON_RescaleX(con_vm_t* vm, i32 xrepeat) {
    spritetype* spr = vm->sprite;
    i32 j = (xrepeat - spr->xrepeat) << 1;
    spr->xrepeat += ksgn(j);
}

static void CON_RescaleY(con_vm_t* vm, i32 yrepeat) {
    spritetype* spr = vm->sprite;
    i32 j = (yrepeat - spr->yrepeat) << 1;
    spr->yrepeat += ksgn(j);
}

static bool CON_CanRescaleY(const con_vm_t* vm, i32 new_yrepeat) {
    const spritetype* spr = vm->sprite;
    const weapon_hit_t* spr_hit = vm->sprite_hit;
    u8 yrepeat = spr->yrepeat;
    if (spr->picnum == APLAYER && yrepeat < 36) {
        // Allow if it's the player sprite and it's too small.
        return true;
    }
    if (new_yrepeat < yrepeat) {
        // Allow shrinking.
        return true;
    }
    // Allow growing only if it fits in the sector.
    i32 base_height = tiles[spr->picnum].dim.height;
    // Use some safety padding for sprite height.
    i32 sprite_height = base_height + 8;
    i32 scaled_height = (yrepeat * sprite_height << 2);
    i32 sec_height = spr_hit->floorz - spr_hit->ceilingz;
    return scaled_height < sec_height;
}

void VM_SizeTo(con_vm_t* vm) {
    i32 xrepeat = VM_READ(vm);
    i32 yrepeat = VM_READ(vm);
    CON_RescaleX(vm, xrepeat);
    if (CON_CanRescaleY(vm, yrepeat)) {
        CON_RescaleY(vm, yrepeat);
    }
}


void VM_SizeAt(con_vm_t* vm) {
    vm->sprite->xrepeat = (u8) VM_READ(vm);
    vm->sprite->yrepeat = (u8) VM_READ(vm);
}
