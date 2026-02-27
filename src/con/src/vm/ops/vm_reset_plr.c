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
#include "player.h"
#include "premap.h"
#include "build/engine.h"
#include "input/control.h"
#include "menu/menu.h"

static void CON_ResetSinglePlayer(con_vm_t* vm) {
    player_t* p = vm->player;
    if (lastsavedpos >= 0 && ud.recstat != 2) {
        // Save game exists, so show load menu.
        p->gm = MODE_MENU;
        KB_ClearKeyDown(sc_Space);
        cmenu(15000);
    } else {
        // Restart level.
        p->gm = MODE_RESTART;
    }
    vm->killit_flag = 2;
}

static void CON_ResetPositions(con_vm_t* vm) {
    player_t* p = vm->player;
    spritetype* spr = vm->sprite;

    spr->x = p->posx;
    hittype[vm->sprite_idx].bposx = p->posx;
    p->bobposx = p->posx;
    p->oposx = p->posx;

    spr->y = p->posy;
    hittype[vm->sprite_idx].bposy = p->posy;
    p->bobposy = p->posy;
    p->oposy = p->posy;

    spr->z = p->posz;
    hittype[vm->sprite_idx].bposy = p->posz;
    p->oposz = p->posz;

    // Re-links sprite into correct sector.
    updatesector(p->posx, p->posy, &p->cursectnum);
    B_SetSprite(p->i, p->posx, p->posy, p->posz + PHEIGHT);
}

static void CON_ResetPlayerState(con_vm_t* vm) {
    player_t* p = vm->player;
    p->last_extra = (i16) max_player_health;
    p->wantweaponfire = -1;
    p->horiz = 100;
    p->on_crane = -1;
    p->frag_ps = vm->sprite_idx;
    p->horizoff = 0;
    p->opyoff = 0;
    p->wackedbyactor = -1;
    p->shield_amount = (i16) max_armour_amount;
    p->dead_flag = 0;
    p->pals_time = 0;
    p->footprintcount = 0;
    p->weapreccnt = 0;
    p->fta = 0;
    p->ftq = 0;
    p->posxv = 0;
    p->posyv = 0;
    p->rotscrnang = 0;
    p->falling_counter = 0;
}

static void CON_ResetSprite(con_vm_t* vm) {
    const player_t* p = vm->player;
    spritetype* spr = vm->sprite;
    spr->cstat = 257;
    spr->shade = -12;
    spr->clipdist = 64;
    spr->xrepeat = 42;
    spr->yrepeat = 36;
    spr->owner = vm->sprite_idx;
    spr->xoffset = 0;
    spr->extra = p->last_extra;
    spr->pal = p->palookup;
}

static void CON_ResetHitType(const con_vm_t* vm) {
    const player_t* p = vm->player;
    weapon_hit_t* hit = &hittype[vm->sprite_idx];
    hit->extra = -1;
    hit->owner = vm->sprite_idx;
    hit->cgg = 0;
    hit->movflag = 0;
    hit->tempang = 0;
    hit->actorstayput = -1;
    hit->dispicnum = 0;
    hit->owner = p->i;
}

static void CON_ResetCamera(void) {
    cameradist = 0;
    cameraclock = totalclock;
}

static void CON_ResetMultiPlayer(con_vm_t* vm) {
    P_PickRandomSpot(vm->player_idx);
    CON_ResetPositions(vm);
    CON_ResetPlayerState(vm);
    CON_ResetSprite(vm);
    CON_ResetHitType(vm);
    P_ResetInventory(vm->player_idx);
    P_ResetWeapons(vm->player_idx);
    CON_ResetCamera();
}

void VM_ResetPlayer(con_vm_t* vm) {
    if (ud.multimode < 2) {
        CON_ResetSinglePlayer(vm);
    } else {
        CON_ResetMultiPlayer(vm);
    }
    P_SetPal(vm->player);
}
