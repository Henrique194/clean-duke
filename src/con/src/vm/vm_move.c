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
#include "names.h"
#include "build/engine.h"
#include "funct.h"


static i16 VM_GetTarget(const con_vm_t* vm) {
    const player_t* plr = vm->player;
    const spritetype* spr = vm->sprite;
    i16 j = plr->holoduke_on;
    if (j < 0) {
        // No Holoduke, so chase player.
        return plr->i;
    }
    const spritetype* holoduke = &sprite[j];
    i32 x1 = holoduke->x;
    i32 y1 = holoduke->y;
    i32 z1 = holoduke->z;
    i16 sect1 = holoduke->sectnum;
    i32 x2 = spr->x;
    i32 y2 = spr->y;
    i32 z2 = spr->z;
    i16 sect2 = spr->sectnum;
    if (cansee(x1, y1, z1, sect1, x2, y2, z2, sect2)) {
        // Can see Holoduke, so chase it.
        return j;
    }
    return plr->i;
}

static i32 VM_GetTargetAng(const con_vm_t* vm) {
    const spritetype* spr = vm->sprite;
    const weapon_hit_t* spr_hit = vm->sprite_hit;
    const spritetype* target = &sprite[spr->owner];
    i32 xvect;
    i32 yvect;
    if (target->picnum == APLAYER) {
        xvect = spr_hit->lastvx - spr->x;
        yvect = spr_hit->lastvy - spr->y;
    } else {
        xvect = target->x - spr->x;
        yvect = target->y - spr->y;
    }
    return getangle(xvect, yvect);
}

static void VM_FaceTarget(const con_vm_t* vm) {
    i16 spr_idx = vm->sprite_idx;
    spritetype* spr = vm->sprite;
    i16 target_ang = (i16) VM_GetTargetAng(vm);
    if (!spr->xvel || spr->picnum == DRONE) {
        spr->ang = target_ang;
        return;
    }
    short ang_dif = getincangle(spr->ang, target_ang);
    i32 tics_elapsed = (vm->actor->count) & 31;
    if (tics_elapsed < 2) {
        if (klabs(ang_dif) < ANG45) {
            i16 j = ANG22 - (TRAND & ANG45);
            spr->ang += j;
            if (P_Hits(spr_idx) < 844) {
                spr->ang -= j;
            }
        }
        return;
    }
    if (tics_elapsed > 18 && tics_elapsed < 26) {
        // choose
        if (klabs(ang_dif >> 2) < ANG22) {
            spr->ang = target_ang;
        } else {
            spr->ang += (ang_dif >> 2);
        }
    }
}

static void VM_SeekPlayer(con_vm_t* vm) {
    vm->sprite->owner = VM_GetTarget(vm);
    VM_FaceTarget(vm);
}


static void VM_FurthestDir(const con_vm_t* vm) {
    i16 spr_idx = vm->sprite_idx;
    spritetype* spr = vm->sprite;
    spr->ang = furthestangle(spr_idx, 2);
    spr->owner = vm->player->i;
}


static void VM_FleeEnemy(const con_vm_t* vm) {
    vm->sprite->ang = furthestangle(vm->sprite_idx, 2);
}


static void VM_FacePlayer(const con_vm_t* vm) {
    const player_t* plr = vm->player;
    spritetype* spr = vm->sprite;
    i32 xvect;
    i32 yvect;
    if (plr->newowner >= 0) {
        xvect = plr->oposx - spr->x;
        yvect = plr->oposy - spr->y;
    } else {
        xvect = plr->posx - spr->x;
        yvect = plr->posy - spr->y;
    }
    i16 goal_ang = (i16) getangle(xvect, yvect);
    i16 ang_dif = (i16) (getincangle(spr->ang, goal_ang) >> 2);
    if (ang_dif > -8 && ang_dif < 0) {
        ang_dif = 0;
    }
    spr->ang += ang_dif;
}


static void VM_FacePlayerSlow(const con_vm_t* vm) {
    const player_t* plr = vm->player;
    spritetype* spr = vm->sprite;
    i32 xvect;
    i32 yvect;
    if (plr->newowner >= 0) {
        xvect = plr->oposx - spr->x;
        yvect = plr->oposy - spr->y;
    } else {
        xvect = plr->posx - spr->x;
        yvect = plr->posy - spr->y;
    }
    i16 goal_ang = (i16) getangle(xvect, yvect);
    i16 ang_dif = (i16) (ksgn(getincangle(spr->ang, goal_ang)) << 5);
    if (ang_dif > -32 && ang_dif < 0) {
        ang_dif = 0;
        spr->ang = goal_ang;
    }
    spr->ang += ang_dif;
}


static void VM_FacePlayerSmart(const con_vm_t* vm) {
    const player_t* plr = vm->player;
    spritetype* spr = vm->sprite;
    i32 newx = plr->posx + (plr->posxv / 768);
    i32 newy = plr->posy + (plr->posyv / 768);
    i32 x_vec = newx - spr->x;
    i32 y_vec = newy - spr->y;
    i16 goal_ang = (i16) getangle(x_vec, y_vec);
    i16 angdif = (i16) (getincangle(spr->ang, goal_ang) >> 2);
    if (angdif > -8 && angdif < 0) {
        angdif = 0;
    }
    spr->ang += angdif;
}


static void VM_Spin(const con_vm_t* vm) {
    i32 ang = (vm->actor->count << 3);
    vm->sprite->ang += (SIN(ang) >> 6);
}


static void VM_Jump(const con_vm_t* vm) {
    spritetype* g_sp = vm->sprite;
    i32 count = vm->actor->count;
    if (count < 16) {
        i32 ang = ANG90 + (count << 4);
        g_sp->zvel -= (SIN(ang) >> 5);
    }
}


static void VM_UpdateVel(con_vm_t* vm) {
    const actor_state_t* actor = vm->actor;
    const i32* move_script = CON_DecodeScript(actor->move_script);
    i32 xvel = move_script[0];
    i32 yvel = move_script[1];

    spritetype* spr = vm->sprite;
    spr->xvel += (xvel - spr->xvel) / 5;
    if (spr->zvel < 648) {
        spr->zvel += ((yvel << 4) - spr->zvel) / 5;
    }
}

static void alterang(con_vm_t* vm, short a) {
    i32 tics_elapsed = (vm->actor->count) & 31;
    VM_UpdateVel(vm);
    if (a & seekplayer) {
        VM_SeekPlayer(vm);
    }
    if (tics_elapsed < 1) {
        if (a & furthestdir) {
            VM_FurthestDir(vm);
        }
        if (a & fleeenemy) {
            VM_FleeEnemy(vm);
        }
    }
}


static void VM_MoveEnemy(con_vm_t* vm) {
    i32 l;
    i16 g_i = vm->sprite_idx;
    spritetype* spr = vm->sprite;

    if ((spr->picnum == DRONE || spr->picnum == COMMANDER) && spr->extra > 0) {
        if (spr->picnum == COMMANDER) {
            hittype[g_i].floorz = l = getflorzofslope(spr->sectnum, spr->x, spr->y);
            if (spr->z > (l - (8 << 8))) {
                if (spr->z > (l - (8 << 8))) {
                    spr->z = l - (8 << 8);
                }
                spr->zvel = 0;
            }

            hittype[g_i].ceilingz = l = getceilzofslope(spr->sectnum, spr->x, spr->y);
            if ((spr->z - l) < (80 << 8)) {
                spr->z = l + (80 << 8);
                spr->zvel = 0;
            }
        }
        else {
            if (spr->zvel > 0) {
                hittype[g_i].floorz = l = getflorzofslope(spr->sectnum, spr->x, spr->y);
                if (spr->z > (l - (30 << 8))) {
                    spr->z = l - (30 << 8);
                }
            }
            else {
                hittype[g_i].ceilingz = l = getceilzofslope(spr->sectnum, spr->x, spr->y);
                if ((spr->z - l) < (50 << 8)) {
                    spr->z = l + (50 << 8);
                    spr->zvel = 0;
                }
            }
        }
    }
    else if (spr->picnum != ORGANTIC) {
        if (spr->zvel > 0 && hittype[g_i].floorz < spr->z) {
            spr->z = hittype[g_i].floorz;
        }
        if (spr->zvel < 0) {
            l = getceilzofslope(spr->sectnum, spr->x, spr->y);
            if ((spr->z - l) < (66 << 8)) {
                spr->z = l + (66 << 8);
                spr->zvel >>= 1;
            }
        }
    }
}

static void VM_UpdateEnemyShade(con_vm_t* vm) {
    spritetype* spr = vm->sprite;
    const sectortype* sec = &sector[spr->sectnum];
    i8 shade;
    if (sec->ceilingstat & 1) {
        shade = sec->ceilingshade;
    } else {
        shade = sec->floorshade;
    }
    spr->shade += (shade - spr->shade) >> 1;
}

void move(con_vm_t* vm) {
    i32 *moveptr;
    short angdif;
    i32 daxvel;

    i16 g_i = vm->sprite_idx;
    i16 g_p = vm->player_idx;
    i32 g_x = vm->player_dist;
    i32* g_t = (i32*) vm->actor;
    spritetype* spr = vm->sprite;

    i16 a = spr->hitag;

    if (a == -1) {
        a = 0;
    }

    g_t[0]++;

    if (a & face_player) {
        VM_FacePlayer(vm);
    }
    if (a & spin) {
        VM_Spin(vm);
    }
    if (a & face_player_slow) {
        VM_FacePlayerSlow(vm);
    }
    if ((a & jumptoplayer) == jumptoplayer) {
        VM_Jump(vm);
    }
    if (a & face_player_smart) {
        VM_FacePlayerSmart(vm);
    }

    if (g_t[1] == 0 || a == 0) {
        if ((badguy(spr) && spr->extra <= 0)
            || (hittype[g_i].bposx != spr->x)
            || (hittype[g_i].bposy != spr->y)) {
            hittype[g_i].bposx = spr->x;
            hittype[g_i].bposy = spr->y;
            B_SetSprite(g_i, spr->x, spr->y, spr->z);
        }
        return;
    }

    moveptr = CON_DecodeScript(g_t[1]);

    if (a & geth)
        spr->xvel += (*moveptr - spr->xvel) >> 1;
    if (a & getv)
        spr->zvel += ((*(moveptr + 1) << 4) - spr->zvel) >> 1;
    if (a & dodgebullet)
        VM_Dodge(spr);
    if (spr->picnum != APLAYER)
        alterang(vm, a);

    if (spr->xvel > -6 && spr->xvel < 6)
        spr->xvel = 0;

    a = badguy(spr);

    if (spr->xvel || spr->zvel) {
        if (a && spr->picnum != ROTATEGUN) {
            VM_MoveEnemy(vm);
        }
        else if (spr->picnum == APLAYER) {
            if ((spr->z - hittype[g_i].ceilingz) < (32 << 8)) {
                spr->z = hittype[g_i].ceilingz + (32 << 8);
            }
        }

        daxvel = spr->xvel;
        angdif = spr->ang;

        if (a && spr->picnum != ROTATEGUN) {
            if (g_x < 960 && spr->xrepeat > 16) {
                daxvel = -(1024 - g_x);
                angdif = getangle(ps[g_p].posx - spr->x, ps[g_p].posy - spr->y);
                if (g_x < 512) {
                    ps[g_p].posxv = 0;
                    ps[g_p].posyv = 0;
                } else {
                    ps[g_p].posxv = mulscale(ps[g_p].posxv, dukefriction - 0x2000, 16);
                    ps[g_p].posyv = mulscale(ps[g_p].posyv, dukefriction - 0x2000, 16);
                }
            }
            else if (spr->picnum != DRONE && spr->picnum != SHARK && spr->picnum != COMMANDER) {
                if (hittype[g_i].bposz != spr->z || (ud.multimode < 2 && ud.player_skill < 2)) {
                    if ((g_t[0] & 1) || ps[g_p].actorsqu == g_i)
                        return;
                    else
                        daxvel <<= 1;
                } else {
                    if ((g_t[0] & 3) || ps[g_p].actorsqu == g_i)
                        return;
                    else
                        daxvel <<= 2;
                }
            }
        }
        hittype[g_i].movflag = P_MoveSprite(g_i, (daxvel * (sintable[(angdif + 512) & 2047])) >> 14, (daxvel * (sintable[angdif & 2047])) >> 14, spr->zvel, CLIPMASK0);
    }

    if (a) {
        VM_UpdateEnemyShade(vm);
        if (sector[spr->sectnum].floorpicnum == MIRROR) {
            deletesprite(g_i);
        }
    }
}
