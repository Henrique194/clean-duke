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
#include "build/fixedPoint_math.h"

void VM_IfOnWater(con_vm_t* vm) {
    const spritetype* spr = vm->sprite;
    const sectortype* sec = &sector[spr->sectnum];
    if (sec->lotag != 1) {
        // Not water sector.
        VM_IfElse(vm, false);
        return;
    }
    i32 height = klabs(spr->z - sec->floorz);
    VM_IfElse(vm, height < (32 << 8));
}


void VM_IfInWater(con_vm_t* vm) {
    const spritetype* spr = vm->sprite;
    VM_IfElse(vm, sector[spr->sectnum].lotag == 2);
}
