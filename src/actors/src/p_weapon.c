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

#include "duke3d.h"
#include "actors/actors.h"
#include "build/engine.h"
#include "soundefs.h"
#include "audiolib/sounds.h"
#include "premap.h"


static bool P_PlayingOldDemo(void) {
    switch (ud.playing_demo_rev) {
        case BYTEVERSION_27:
        case BYTEVERSION_28:
        case BYTEVERSION_116:
        case BYTEVERSION_117:
            return true;
        default:
            return false;
    }
}

//
// Register weapon as owned by the player and returns
// if we are allowed to switch to this weapon.
//
static bool P_GiveWeapon(player_t* p, i16 weapon) {
    if (p->gotweapon[weapon]) {
        // Player already got weapon, so just switch to it.
        return true;
    }
    p->gotweapon[weapon] = 1;
    if (weapon == SHRINKER_WEAPON) {
        p->gotweapon[GROW_WEAPON] = 1;
    }
    if (P_PlayingOldDemo()) {
        // Always switch weapon on 1st pick up if playing an old demo.
        return true;
    }
    // FIX_00012:
    //   Added "weapon autoswitch" toggle allowing to turn the
    //   autoswitch off when picking up new weapons. The weapon
    //   sound on pickup will remain on, to not affect the
    //   opponent's gameplay (so he can still hear you picking
    //   up new weapons).
    return !p->weaponautoswitch;
}

static void P_ChangeWeapon(player_t* p, i16 weapon) {
    p->random_club_frame = 0;
    if (p->holster_weapon) {
        p->weapon_pos = 10;
        p->holster_weapon = 0;
        p->last_weapon = -1;
    } else {
        p->weapon_pos = -1;
        p->last_weapon = p->curr_weapon;
    }
    p->kickback_pic = 0;
    p->curr_weapon = weapon;
}

static void P_WeaponAddedSound(const player_t* p, i16 weapon) {
    switch (weapon) {
        case KNEE_WEAPON:
        case TRIPBOMB_WEAPON:
        case HANDREMOTE_WEAPON:
        case HANDBOMB_WEAPON:
            break;
        case SHOTGUN_WEAPON:
            spritesound(SHOTGUN_COCK, p->i);
            break;
        case PISTOL_WEAPON:
            spritesound(INSERT_CLIP, p->i);
            break;
        default:
            spritesound(SELECT_WEAPON, p->i);
            break;
    }
}

void P_AddWeapon(player_t* p, int16_t weapon) {
    bool switch_weapon = P_GiveWeapon(p, weapon);
    if (switch_weapon) {
        // Always allow change on 1st pick up if playing an old demo.
        P_ChangeWeapon(p, weapon);
    }
    P_WeaponAddedSound(p, weapon);
    // FIX_00056:
    //   Refresh issue w/FPS, small Weapon and custom FTA,
    //   when screen resized down
    P_ViewChanged();
}


void P_CheckAvailWeapon(struct player_struct* p) {
    if (p->wantweaponfire >= 0) {
        short weap = p->wantweaponfire;
        p->wantweaponfire = -1;
        if (weap == p->curr_weapon) {
            return;
        }
        if (p->gotweapon[weap] && p->ammo_amount[weap] > 0) {
            P_AddWeapon(p, weap);
            return;
        }
    }

    int32 weap = p->curr_weapon;
    if (p->gotweapon[weap] && p->ammo_amount[weap] > 0) {
        return;
    }

    short snum = sprite[p->i].yvel;
    short i;
    for (i = 0; i < 10; i++) {
        weap = ud.wchoice[snum][i];
        if (VOLUMEONE && weap > 6) {
            continue;
        }
        if (weap == 0) {
            weap = 9;
        } else {
            weap--;
        }
        if (weap == 0 || (p->gotweapon[weap] && p->ammo_amount[weap] > 0)) {
            break;
        }
    }
    if (i == 10) {
        weap = 0;
    }

    // Found the weapon
    p->last_weapon = p->curr_weapon;
    p->random_club_frame = 0;
    p->curr_weapon = weap;
    p->kickback_pic = 0;
    if (p->holster_weapon == 1) {
        p->holster_weapon = 0;
        p->weapon_pos = 10;
    } else {
        p->weapon_pos = -1;
    }
}


void P_AddAmmo(int16_t weapon, player_t* p, int16_t amount) {
    p->ammo_amount[weapon] += amount;
    if (p->ammo_amount[weapon] > max_ammo_amount[weapon]) {
        p->ammo_amount[weapon] = max_ammo_amount[weapon];
    }
}
