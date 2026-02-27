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
#include "player.h"
#include "build/engine.h"

static void CON_ExitCameraView(player_t* plr) {
    // Exit camera.
    plr->newowner = -1;
    plr->posx = plr->oposx;
    plr->posy = plr->oposy;
    plr->posz = plr->oposz;
    plr->ang = plr->oang;
    updatesector(plr->posx, plr->posy, &plr->cursectnum);
    P_SetPal(plr);

    // Stop all cameras motion.
    i32 j = headspritestat[1];
    while (j >= 0) {
        if (sprite[j].picnum == CAMERA1) {
            sprite[j].yvel = 0;
        }
        j = nextspritestat[j];
    }
}

static bool CON_CanUpdateHealth(const con_vm_t* vm, i32 amount) {
    const player_t* plr = vm->player;
    const spritetype* spr = vm->sprite;
    if (ud.god) {
        return false;
    }
    if (spr->picnum == ATOMICHEALTH) {
        // Atomic health actor can boost the player's
        // health value over the max value.
        return true;
    }
    i32 health = sprite[plr->i].extra;
    return health <= max_player_health || amount <= 0;
}

static i32 CON_CalcNewHealth(const con_vm_t* vm, i32 amount) {
    const player_t* plr = vm->player;
    const spritetype* spr = vm->sprite;
    i32 health = sprite[plr->i].extra;
    if (health > 0) {
        health += amount;
    }
    if (spr->picnum == ATOMICHEALTH) {
        if (health > (max_player_health << 1)) {
            return max_player_health << 1;
        }

        return max(health, 0);
    }
    if (health > max_player_health && amount > 0) {
        return max_player_health;
    }
    return max(health, 0);
}

static void CON_UpdateLastHealth(player_t* p, i32 new_health, i32 amount) {
    i32 old_health = new_health - amount;
    i32 low_health = (max_player_health >> 2);
    if (old_health < low_health && new_health >= low_health) {
        // Got health when you really needed it.
        spritesound(DUKE_GOTHEALTHATLOW, p->i);
    }
    p->last_extra = (i16) new_health;
}

void VM_AddPlrHealth(con_vm_t* vm) {
    i32 amount = VM_READ(vm);
    player_t* plr = vm->player;
    if (plr->newowner >= 0) {
        // Kick you out of camera view.
        CON_ExitCameraView(plr);
    }
    if (!CON_CanUpdateHealth(vm, amount)) {
        return;
    }
    i32 new_health = CON_CalcNewHealth(vm, amount);
    if (amount > 0) {
        CON_UpdateLastHealth(plr, new_health, amount);
    }
    sprite[plr->i].extra = (i16) new_health;
}


void VM_IfPlrHealthLt(con_vm_t* vm) {
    i32 health = VM_READ(vm);
    VM_IfElse(vm, sprite[vm->player->i].extra < health);
}
