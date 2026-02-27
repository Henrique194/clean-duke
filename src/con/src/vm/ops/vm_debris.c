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
#include "names.h"

//
// Loot table used when BLIMP scrap debris spawns pickups.
//
static short blimp_loot[15] = {
    RPGSPRITE, CHAINGUNSPRITE, DEVISTATORAMMO, RPGAMMO,      RPGAMMO,
    JETPACK,   SHIELD,         FIRSTAID,       STEROIDS,     RPGAMMO,
    RPGAMMO,   RPGSPRITE,      RPGAMMO,        FREEZESPRITE, FREEZEAMMO,
};

static i16 CON_GetRandType(const spritetype* spr, i16 type) {
    if (spr->picnum == BLIMP && type == SCRAP1) {
        return type;
    }
    return type + (TRAND % 3);
}

static i16 CON_SpawnDebris(const con_vm_t* vm, i16 type) {
    const spritetype* spr = vm->sprite;
    i16 sect = spr->sectnum;
    i32 x = spr->x + (TRAND & 255) - 128;
    i32 y = spr->y + (TRAND & 255) - 128;
    i32 z = spr->z - (8 << 8) - (TRAND & 8191);
    i16 pn = CON_GetRandType(spr, type);
    i8 s = spr->shade;
    i8 xr = 32 + (TRAND & 15);
    i8 yr = 32 + (TRAND & 15);
    i16 a = TRAND & 2047;
    i16 ve = 32 + (TRAND & 127);
    i32 zv = -(TRAND & 2047);
    i16 ow = vm->sprite_idx;
    i8 stat = 5;
    return EGS(sect, x, y, z, pn, s, xr, yr, a, ve, zv, ow, stat);
}

static void CON_ConfigDebris(const con_vm_t* vm, i32 i, i32 debris, i16 type) {
    const spritetype* spr = vm->sprite;
    if (spr->picnum == BLIMP && type == SCRAP1) {
        // Debris can spawn weapon/ammo for pickup later.
        sprite[debris].yvel = blimp_loot[i % 14];
    } else {
        sprite[debris].yvel = -1;
    }
    sprite[debris].pal = spr->pal;
}

void VM_Debris(con_vm_t* vm) {
    i16 type = (i16) VM_READ(vm);
    i16 amount = (i16) VM_READ(vm);
    const spritetype* spr = vm->sprite;
    if (spr->sectnum < 0 || spr->sectnum >= MAXSECTORS) {
        return;
    }
    for (i32 i = amount - 1; i >= 0; i--) {
        i32 debris = CON_SpawnDebris(vm, type);
        CON_ConfigDebris(vm, i, debris, type);
    }
}
