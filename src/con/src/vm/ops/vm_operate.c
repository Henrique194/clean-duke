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
#include "sector.h"
#include "build/engine.h"

static i16 CON_GetNearSector(const spritetype* spr) {
    i32 xs = spr->x;
    i32 ys = spr->y;
    i32 zs = spr->z - (32 << 8);
    short sect = spr->sectnum;
    short angle = spr->ang;
    i32 tag_range = 768L;
    u8 tag_search = 1;
    neartag(
        xs,
        ys,
        zs,
        sect,
        angle,
        &neartagsector,
        &neartagwall,
        &neartagsprite,
        &neartaghitdist,
        tag_range,
        tag_search
    );
    if (neartagsector < 0) {
        // Could not find any near sector.
        return -1;
    }
    if (!P_IsNearOperator(sector[neartagsector].lotag)) {
        // Not near sector.
        return -1;
    }
    return neartagsector;
}

static bool CON_HasActivator(i16 sec_tag) {
    i32 i = headspritesect[sec_tag];
    while (i >= 0) {
        if (sprite[i].picnum == ACTIVATOR) {
            return true;
        }
        i = nextspritesect[i];
    }
    return false;
}

static bool CON_CanOperateSector(i16 sec_tag) {
    if (sec_tag < 0) {
        return false;
    }
    const sectortype* sec = &sector[sec_tag];
    if ((sec->lotag & 0xff) != 23 && sec->floorz != sec->ceilingz) {
        return false;
    }
    if (sec->lotag & 16384) {
        return false;
    }
    if (sec->lotag & 32768) {
        return false;
    }
    return !CON_HasActivator(sec_tag);
}

//
// Causes the current actor to open a nearby door.
//
void VM_Operate(con_vm_t* vm) {
    if (sector[vm->sprite->sectnum].lotag) {
        // In special sector, so do not open any door.
        return;
    }
    i16 sec_tag = CON_GetNearSector(vm->sprite);
    if (CON_CanOperateSector(sec_tag)) {
        P_OperateSectors(sec_tag, vm->sprite_idx);
    }
}
