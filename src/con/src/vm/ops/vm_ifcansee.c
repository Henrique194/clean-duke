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

//
// Finds a visible point near a target that the actor can see.
// Used when:
//   - Direct line of sight to player fails
//   - Enemy tries to reposition
//
static short furthestcanseepoint(const con_vm_t* vm, spritetype* ts, i32* dax, i32* day) {
    short j;
    short hitsect;
    short hitwall;
    short hitspr;
    short angincs;
    i32 hx;
    i32 hy;
    i32 hz;
    i32 d;
    i32 da; //, d, cd, ca,tempx,tempy,cx,cy;
    spritetype* s = &sprite[vm->sprite_idx];

    if ((vm->actor->count & 63)) {
        return -1;
    }

    if (ud.multimode < 2 && ud.player_skill < 3) {
        angincs = 2048 / 2;
    } else {
        angincs = 2048 / (1 + (TRAND & 1));
    }

    for (j = ts->ang; j < (2048 + ts->ang); j += (angincs - (TRAND & 511))) {
        hitscan(ts->x, ts->y, ts->z - (16 << 8), ts->sectnum,
                sintable[(j + 512) & 2047], sintable[j & 2047],
                16384 - (TRAND & 32767), &hitsect, &hitwall, &hitspr, &hx, &hy,
                &hz, CLIPMASK1);

        d = klabs(hx - ts->x) + klabs(hy - ts->y);
        da = klabs(hx - s->x) + klabs(hy - s->y);

        if (d < da) {
            if (cansee(hx, hy, hz, hitsect, s->x, s->y, s->z - (16 << 8), s->sectnum)) {
                *dax = hx;
                *day = hy;
                return hitsect;
           }
        }
    }
    return -1;
}

static bool CON_CanSeeHoloDuke(const con_vm_t* vm) {
    const spritetype* s = &sprite[vm->player->holoduke_on];
    const spritetype* spr = vm->sprite;
    i32 x1 = spr->x;
    i32 y1 = spr->y;
    i32 z1 = spr->z - (TRAND & ((32 << 8) - 1));
    short sect1 = spr->sectnum;
    i32 x2 = s->x;
    i32 y2 = s->y;
    i32 z2 = s->z;
    short sect2 = s->sectnum;
    return cansee(x1, y1, z1, sect1, x2, y2, z2, sect2);
}

static spritetype* CON_GetPlayerSprite(const con_vm_t* vm) {
    const player_t* plr = vm->player;
    if (plr->holoduke_on < 0) {
        return &sprite[plr->i];
    }
    if (CON_CanSeeHoloDuke(vm)) {
        return &sprite[plr->holoduke_on];
    }
    return &sprite[plr->i];
}

static bool CON_CanSeePlayerSprite(const con_vm_t* vm, spritetype* player) {
    weapon_hit_t* hit = vm->sprite_hit;
    const spritetype* spr = vm->sprite;
    i32 x1 = spr->x;
    i32 y1 = spr->y;
    i32 z1 = spr->z - (TRAND & (47 << 8));
    short sect1 = spr->sectnum;
    i32 x2 = player->x;
    i32 y2 = player->y;
    i32 z2 = player->z - (24 << 8);
    short sect2 = player->sectnum;
    if (cansee(x1, y1, z1, sect1, x2, y2, z2, sect2)) {
        // Update last visible player location.
        hit->lastvx = player->x;
        hit->lastvy = player->y;
        return true;
    }
    i32 sect = furthestcanseepoint(vm, player, &hit->lastvx, &hit->lastvy);
    return sect != -1;
}

void VM_IfCanSee(con_vm_t* vm) {
    spritetype* player = CON_GetPlayerSprite(vm);
    bool can_see = CON_CanSeePlayerSprite(vm, player);
    const spritetype* spr = vm->sprite;
    if (can_see && (spr->statnum == 1 || spr->statnum == 6)) {
        // Actor can see player, so keep it awake.
        vm->sprite_hit->timetosleep = SLEEPTIME;
    }
    VM_IfElse(vm, can_see);
}
