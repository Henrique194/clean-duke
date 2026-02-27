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
#include "soundefs.h"
#include "actors/actors.h"
#include "build/engine.h"

//
// true if the player is standing (and not moving).
//
static bool VM_IsStanding(const con_vm_t* vm, i32 condition) {
    if (!(condition & pstanding)) {
        return false;
    }
    return vm->sprite->xvel >= 0 && vm->sprite->xvel < 8;
}

//
// true if the player is walking.
//
static bool VM_IsWalking(const con_vm_t* vm, i32 condition) {
    if (!(condition & pwalking)) {
        return false;
    }
    return vm->sprite->xvel >= 8 && !(sync[vm->player_idx].bits & (1 << 5));
}

//
// true if the player is running.
//
static bool VM_IsRunning(const con_vm_t* vm, i32 condition) {
    if (!(condition & prunning)) {
        return false;
    }
    return vm->sprite->xvel >= 8 && (sync[vm->player_idx].bits & (1 << 5));
}

//
// true if the player is crouching.
//
static bool VM_IsDucking(const con_vm_t* vm, i32 condition) {
    if (!(condition & pducking)) {
        return false;
    }
    return vm->player->on_ground && (sync[vm->player_idx].bits & 2);
}

//
// true if the player is falling.
//
static bool VM_IsFalling(const con_vm_t* vm, i32 condition) {
    if (!(condition & pfalling)) {
        return false;
    }
    const player_t* p = vm->player;
    return !p->jumping_counter && !p->on_ground && p->poszv > 2048;
}

//
// true if the player is jumping.
//
static bool VM_IsJumping(const con_vm_t* vm, i32 condition) {
    if (!(condition & pjumping)) {
        return false;
    }
    return vm->player->jumping_counter > 348;
}

//
// true if the player is higher than the actor calling the code.
//
static bool VM_IsHigher(const con_vm_t* vm, i32 condition) {
    if (!(condition & phigher)) {
        return false;
    }
    return vm->player->posz < (vm->sprite->z - (48 << 8));
}

//
// true if the player is walking backwards.
//
static bool VM_IsWalkingBack(const con_vm_t* vm, i32 condition) {
    if (!(condition & pwalkingback)) {
        return false;
    }
    return vm->sprite->xvel <= -8 && !(sync[vm->player_idx].bits & (1 << 5));
}

//
// true if the player is running backwards.
//
static bool VM_IsRunningBack(const con_vm_t* vm, i32 condition) {
    if (!(condition & prunningback)) {
        return false;
    }
    return vm->sprite->xvel <= -8 && (sync[vm->player_idx].bits & (1 << 5));
}

//
// true if the player is using the kick.
//
static bool VM_IsKicking(const con_vm_t* vm, i32 condition) {
    if (!(condition & pkicking)) {
        return false;
    }
    const player_t* p = vm->player;
    if (p->quick_kick > 0) {
        return true;
    }
    return p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 0;
}

//
// true if the player is smaller than normal size.
//
static bool VM_IsShrunk(const con_vm_t* vm, i32 condition) {
    if (!(condition & pshrunk)) {
        return false;
    }
    i32 player_sprite = vm->player->i;
    return sprite[player_sprite].xrepeat < 32;
}

//
// true if the player is using the jetpack.
//
static bool VM_IsUsingJetpack(const con_vm_t* vm, i32 condition) {
    if (!(condition & pjetpack)) {
        return false;
    }
    return vm->player->jetpack_on;
}

//
// true if the player is using steroids.
//
static bool VM_IsOnSteroids(const con_vm_t* vm, i32 condition) {
    if (!(condition & ponsteroids)) {
        return false;
    }
    const player_t* p = vm->player;
    return p->steroids_amount > 0 && p->steroids_amount < 400;
}

//
// true if the player is on a solid surface (including sprite constructions).
//
static bool VM_IsOnGround(const con_vm_t* vm, i32 condition) {
    if (!(condition & ponground)) {
        return false;
    }
    return vm->player->on_ground;
}

//
// true if the player is alive.
//
static bool VM_IsAlive(const con_vm_t* vm, i32 condition) {
    if (!(condition & palive)) {
        return false;
    }
    const spritetype* player = &sprite[vm->player->i];
    return player->xrepeat > 32
            && player->extra > 0
            && !vm->player->timebeforeexit;
}

//
// true if the player is dead.
//
static bool VM_IsDead(const con_vm_t* vm, i32 condition) {
    if (!(condition & pdead)) {
        return false;
    }
    return sprite[vm->player->i].extra <= 0;
}

//
// true if the player is facing the current actor.
//
static bool VM_IsFacing(const con_vm_t* vm, i32 condition) {
    if (!(condition & pfacing)) {
        return false;
    }
    const spritetype* spr = vm->sprite;
    const player_t* p = vm->player;
    i16 ang;
    i32 xvect;
    i32 yvect;
    if (spr->picnum == APLAYER && ud.multimode > 1) {
        ang = ps[otherp].ang;
        xvect = p->posx - ps[otherp].posx;
        yvect = p->posy - ps[otherp].posy;
    } else {
        ang = p->ang;
        xvect = spr->x - p->posx;
        yvect = spr->y - p->posy;
    }
    i16 na = (i16) getangle(xvect, yvect);
    i32 j = getincangle(ang, na);
    return j > -128 && j < 128;
}

static bool VM_CheckCondition(const con_vm_t* vm, i32 condition) {
    return VM_IsStanding(vm, condition)
            || VM_IsWalking(vm, condition)
            || VM_IsRunning(vm, condition)
            || VM_IsDucking(vm, condition)
            || VM_IsFalling(vm, condition)
            || VM_IsJumping(vm, condition)
            || VM_IsHigher(vm, condition)
            || VM_IsWalkingBack(vm, condition)
            || VM_IsRunningBack(vm, condition)
            || VM_IsKicking(vm, condition)
            || VM_IsShrunk(vm, condition)
            || VM_IsUsingJetpack(vm, condition)
            || VM_IsOnSteroids(vm, condition)
            || VM_IsOnGround(vm, condition)
            || VM_IsAlive(vm, condition)
            || VM_IsDead(vm, condition)
            || VM_IsFacing(vm, condition);
}

void VM_IfPlr(con_vm_t* vm) {
    i32 condition = VM_READ(vm);
    bool plr_cond = VM_CheckCondition(vm, condition);
    VM_IfElse(vm, plr_cond);
}
