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

void VM_PlrStomp(con_vm_t* vm) {
    const spritetype* spr = vm->sprite;
    player_t* p = vm->player;
    if (p->knee_incs || sprite[p->i].xrepeat < 40) {
        // If player is kicking or shrunk, do not stomp.
        return;
    }
    i32 x1 = spr->x;
    i32 y1 = spr->y;
    i32 z1 = spr->z - (4 << 8);
    i16 sect1 = spr->sectnum;
    i32 x2 = p->posx;
    i32 y2 = p->posy;
    i32 z2 = p->posz + (16 << 8);
    i16 sect2 = sprite[p->i].sectnum;
    if (!cansee(x1, y1, z1, sect1, x2, y2, z2, sect2)) {
        // Player cannot see actor, so do not stomp it.
        return;
    }
    p->knee_incs = 1;
    if (!p->weapon_pos) {
        p->weapon_pos = -1;
    }
    p->actorsqu = vm->sprite_idx;
}
