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
#include "con/con.h"
#include "build/engine.h"
#include "soundefs.h"
#include "funct.h"

static con_vm_t vm;


//
// Scans multiple angles using hitscan().
// Finds the direction with the longest unobstructed path.
// Used for:
//   - Flee behavior
//   - Random wandering
//
short furthestangle(short i, short angs) {
    short j;
    short hitsect;
    short hitwall;
    short hitspr;
    short furthest_angle;
    short angincs;
    i32 hx;
    i32 hy;
    i32 hz;
    i32 d;
    i32 greatestd;
    spritetype* s = &sprite[i];

    i32* g_t = (i32*) vm.actor;

    greatestd = -(1 << 30);
    angincs = 2048 / angs;

    if (s->picnum != APLAYER)
        if ((g_t[0] & 63) > 2)
            return (s->ang + 1024);

    for (j = s->ang; j < (2048 + s->ang); j += angincs) {
        hitscan(s->x, s->y, s->z - (8 << 8), s->sectnum,
                sintable[(j + 512) & 2047], sintable[j & 2047], 0, &hitsect,
                &hitwall, &hitspr, &hx, &hy, &hz, CLIPMASK1);

        d = klabs(hx - s->x) + klabs(hy - s->y);

        if (d > greatestd) {
            greatestd = d;
            furthest_angle = j;
        }
    }
    return (furthest_angle & 2047);
}


static bool VM_SetUp(short i, short p, i32 x) {
    vm.sprite_idx = i;
    vm.sprite = &sprite[i];
    vm.sprite_hit = &hittype[i];

    i32* actor_script = actorscrptr[vm.sprite->picnum];
    if (!actor_script) {
        // Actor has no script.
        return false;
    }
    vm.ip = actor_script + 4;

    vm.player_idx = p;
    vm.player = &ps[p];

    vm.actor = (actor_state_t*) vm.sprite_hit->temp_data;
    vm.player_dist = x;
    vm.killit_flag = 0;

    return true;
}

static void VM_ExecuteAction(void) {
    spritetype* spr = vm.sprite;
    actor_state_t* actor = vm.actor;
    if (!actor->action_script) {
        // Actor has no action.
        return;
    }
    const i32* action_scrp = CON_DecodeScript(actor->action_script);
    i32 frames = action_scrp[1];
    i32 inc_val = action_scrp[3];
    i32 delay = action_scrp[4];
    spr->lotag += TICSPERFRAME;
    if (spr->lotag > delay) {
        actor->action_count++;
        spr->lotag = 0;
        actor->anim_offset += inc_val;
    }
    if (klabs(actor->anim_offset) >= klabs(frames * inc_val)) {
        actor->anim_offset = 0;
    }
}

void execute(short i, short p, i32 x) {
    if (!VM_SetUp(i, p, x)) {
        return;
    }

    i16 g_i = vm.sprite_idx;
    i16 g_p = vm.player_idx;
    spritetype* g_sp = vm.sprite;

    if (g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS) {
        if (badguy(g_sp)) {
            ps[g_p].actors_killed++;
        }
        deletesprite(g_i);
        return;
    }

    VM_ExecuteAction();
    u8 done;
    do {
        done = parse(&vm);
    } while (done == 0);

    if (vm.killit_flag == 1) {
        if (ps[g_p].actorsqu == g_i) {
            ps[g_p].actorsqu = -1;
        }
        deletesprite(g_i);
    }
    else {
        move(&vm);

        if (g_sp->statnum == 1) {
            if (badguy(g_sp)) {
                if (g_sp->xrepeat > 60) {
                    return;
                }
                if (ud.respawn_monsters == 1 && g_sp->extra <= 0) {
                    return;
                }
            }
            else if (ud.respawn_items == 1 && (g_sp->cstat & 32768)) {
                return;
            }

            if (hittype[g_i].timetosleep > 1) {
                hittype[g_i].timetosleep--;
            }
            else if (hittype[g_i].timetosleep == 1) {
                B_ChangeSpriteStat(g_i, 2);
            }
        }
        else if (g_sp->statnum == 6) {
            switch (g_sp->picnum) {
                case RUBBERCAN:
                case EXPLODINGBARREL:
                case WOODENHORSE:
                case HORSEONSIDE:
                case CANWITHSOMETHING:
                case FIREBARREL:
                case NUKEBARREL:
                case NUKEBARRELDENTED:
                case NUKEBARRELLEAKED:
                case TRIPBOMB:
                case EGG:
                    if (hittype[g_i].timetosleep > 1) {
                        hittype[g_i].timetosleep--;
                    }
                    else if (hittype[g_i].timetosleep == 1) {
                        B_ChangeSpriteStat(g_i, 2);
                    }
                    break;
            }
        }
    }
}
