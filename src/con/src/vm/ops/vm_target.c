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
#include "build/engine.h"
#include "funct.h"

static void CON_GetShotParams(con_vm_t* vm, short* clip_dist, short* ang_dif) {
    if (badguy(vm->sprite) && vm->sprite->xrepeat > 56) {
        *clip_dist = 3084;
        *ang_dif = 48;
        return;
    }
    *clip_dist = 768;
    *ang_dif = 16;
}

static bool CON_ShotClear(const con_vm_t* vm, i32 dist, short clip_dist, short hit) {
    if (dist <= clip_dist) {
        // Shot clipped, so it is blocked.
        return false;
    }
    if (hit >= 0 && sprite[hit].picnum == vm->sprite->picnum) {
        // Shot hit sprite of same type, so block it.
        return false;
    }
    return true;
}

static bool CON_AngShotClear(con_vm_t* vm, short clip_dist, short ang_dif) {
    short hit;
    vm->sprite->ang += ang_dif;
    i32 dist = P_HitSprite(vm->sprite_idx, &hit);
    vm->sprite->ang -= ang_dif;
    return CON_ShotClear(vm, dist, clip_dist, hit);
}

static bool CON_CanShoot(con_vm_t* vm) {
    if (vm->player_dist <= 1024) {
        // Player is close, so we can shoot it.
        return true;
    }
    short clip_dist;
    short ang_dif;
    short hit;
    CON_GetShotParams(vm, &clip_dist, &ang_dif);
    i32 dist = P_HitSprite(vm->sprite_idx, &hit);
    if (dist == (1 << 30)) {
        return true;
    }
    // Simulate projectile thickness by testing
    // additional angular offsets.
    return CON_ShotClear(vm, dist, clip_dist, hit)
            && CON_AngShotClear(vm, clip_dist, ang_dif)
            && CON_AngShotClear(vm, 768, ang_dif);
}

void VM_IfCanShootTarget(con_vm_t* vm) {
    bool can_shoot = CON_CanShoot(vm);
    VM_IfElse(vm, can_shoot);
}


void VM_IfCanSeeTarget(con_vm_t* vm) {
    const player_t* plr = vm->player;
    const spritetype* spr = vm->sprite;
    i32 x1 = spr->x;
    i32 y1 = spr->y;
    i32 z1 = spr->z - ((TRAND & 41) << 8);
    short sect1 = spr->sectnum;
    i32 x2 = plr->posx;
    i32 y2 = plr->posy;
    i32 z2 = plr->posz;
    short sect2 = sprite[plr->i].sectnum;
    bool can_see = cansee(x1, y1, z1, sect1, x2, y2, z2, sect2);
    VM_IfElse(vm, can_see);
    if (can_see) {
        vm->sprite_hit->timetosleep = SLEEPTIME;
    }
}
