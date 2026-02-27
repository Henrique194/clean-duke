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
#include "build/engine.h"
#include "soundefs.h"


static void CON_UpdatePos(con_vm_t* vm) {
    spritetype* spr = vm->sprite;
    spr->xoffset = 0;
    spr->yoffset = 0;

    const sectortype* sec = &sector[spr->sectnum];
    weapon_hit_t* hit = vm->sprite_hit;
    if (hit->cgg <= 0 || (sec->floorstat & 2)) {
        getglobalz(vm->sprite_idx);
        hit->cgg = 6;
        return;
    }
    hit->cgg--;
}


static bool CON_IsFalling(const con_vm_t* vm) {
    const spritetype* spr = vm->sprite;
    return spr->z < (vm->sprite_hit->floorz - FOURSLEIGHT);
}

static i32 CON_GetGravity(const con_vm_t* vm) {
    i16 sec = vm->sprite->sectnum;
    if (P_FloorSpace(sec)) {
        return 0;
    }
    if (P_CeilingSpace(sec) || sector[sec].lotag == 2) {
        return gc / 6;
    }
    return gc;
}

static void CON_AddGravity(con_vm_t* vm) {
    spritetype* spr = vm->sprite;
    i32 gravity = CON_GetGravity(vm);
    spr->zvel += gravity;
    spr->z += spr->zvel;
    if (spr->zvel > 6144) {
        spr->zvel = 6144;
    }
}


static bool CON_CanGib(const con_vm_t* vm) {
    const spritetype* spr = vm->sprite;
    if (spr->pal == 1 || spr->picnum == DRONE) {
        return false;
    }
    return spr->picnum != APLAYER || spr->extra <= 0;
}

static void CON_Gibs(con_vm_t* vm) {
    P_Guts(vm->sprite, JIBS6, 15, vm->player_idx);
    spritesound(SQUISHED, vm->sprite_idx);
    spawn(vm->sprite_idx, BLOODPOOL);
}

static void CON_HardLanding(con_vm_t* vm) {
    if (CON_CanGib(vm)) {
        CON_Gibs(vm);
    }
    vm->sprite_hit->picnum = SHOTSPARK1;
    vm->sprite_hit->extra = 1;
    vm->sprite->zvel = 0;
}

static void CON_SoftLanding(con_vm_t* vm) {
    spritetype* spr = vm->sprite;
    i32* x = &spr->x;
    i32* y = &spr->y;
    i32* z = &spr->z;
    i16 sec = spr->sectnum;
    i32 walldist = 128L;
    i32 ceildist = (4L << 8);
    i32 flordist = (4L << 8);
    u32 cliptype = CLIPMASK0;
    pushmove(x, y, z, &sec, walldist, ceildist, flordist, cliptype);
    if (sec != spr->sectnum && sec >= 0 && sec < MAXSECTORS) {
        B_ChangeSpriteSect(vm->sprite_idx, sec);
    }
    spritesound(THUD, vm->sprite_idx);
}

static void CON_LandFloor(con_vm_t* vm) {
    // Position sprite at floor height.
    spritetype* spr = vm->sprite;
    spr->z = vm->sprite_hit->floorz - FOURSLEIGHT;
    // Apply landing impact.
    if (badguy(spr) || (spr->picnum == APLAYER && spr->owner >= 0)) {
        if (spr->zvel > 3084 && spr->extra <= 1) {
            CON_HardLanding(vm);
        } else if (spr->zvel > 2048 && sector[spr->sectnum].lotag != 1) {
            CON_SoftLanding(vm);
        }
    }
}

static void CON_ReactFloor(con_vm_t* vm) {
    spritetype* spr = vm->sprite;
    if (sector[spr->sectnum].lotag != 1) {
        spr->zvel = 0;
        return;
    }
    switch (spr->picnum) {
        case OCTABRAIN:
        case COMMANDER:
        case DRONE:
            break;
        default:
            spr->z += (24 << 8);
            break;
    }
}

static void CON_FloorCollision(con_vm_t* vm) {
    CON_LandFloor(vm);
    CON_ReactFloor(vm);
}


void VM_Fall(con_vm_t* vm) {
    CON_UpdatePos(vm);
    if (CON_IsFalling(vm)) {
        CON_AddGravity(vm);
        return;
    }
    // Hit floor.
    CON_FloorCollision(vm);
}
