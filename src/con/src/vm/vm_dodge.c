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

#include "vm_main.h"
#include "build/engine.h"

static bool VM_CanDogeProjectile(const spritetype* spr, i16 proj_idx) {
    const spritetype* proj = &sprite[proj_idx];
    if (proj->owner == proj_idx) {
        return false;
    }
    if (proj->sectnum != spr->sectnum) {
        // Not in the same sector (can't hit us).
        return false;
    }
    vec2_t spr_pos = SPR_POS(spr);
    vec2_t spr_dir = SPR_DIR(spr);
    vec2_t proj_pos = SPR_POS(proj);
    vec2_t proj_dir = SPR_DIR(proj);
    vec2_t dist = VEC2_SUB(&proj_pos, &spr_pos);
    if (VEC2_DOT(&spr_dir, &dist) < 0) {
        // Projectile is behind us, do not dodge.
        return false;
    }
    if (VEC2_DOT(&proj_dir, &dist) >= 0) {
        // Projectile moving away from us.
        return false;
    }
    // Check if projectile is heading close to our position.
    // To do this, we calculate the closest distance the
    // projectile would get to the sprite if it continues
    // on its current trajectory.
    i32 closest_dist = VEC2_CROSS(&proj_dir, &dist);
    return klabs(closest_dist) < (65536 * 64);
}

//
// Enemy bullet avoidance logic.
//
bool VM_Dodge(spritetype* spr) {
    // Weapons list.
    i16 i = headspritestat[4];
    for (; i >= 0; i = nextspritestat[i]) {
        if (VM_CanDogeProjectile(spr, i)) {
            // Turn 90 degrees left or right (randomized).
            spr->ang -= ANG90 + (TRAND & ANG180);
            return true;
        }
    }
    return false;
}
