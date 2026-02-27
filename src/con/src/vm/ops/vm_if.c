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
#include "com_keyword.h"
#include "names.h"
#include "build/engine.h"
#include "actors/actors.h"

void VM_IfElse(con_vm_t* vm, bool condition) {
    if (condition) {
        // Execute if branch.
        vm->ip++;
        parse(vm);
        return;
    }
    // Check to see if we have else branch.
    // If we do, execute it.
    vm->ip = CON_DecodeScript(vm->ip[0]);
    if (vm->ip[0] == CK_ELSE) {
        vm->ip += 2;
        parse(vm);
    }
}

void VM_Else(con_vm_t* vm) {
    // If we have found an else opcode, that means
    // we have just executed its associated if branch.
    // Thus, all we have to do is skip the else branch.
    i32 skip_offset = vm->ip[0];
    vm->ip = CON_DecodeScript(skip_offset);
}

void VM_IfRnd(con_vm_t* vm) {
    i32 val = VM_READ(vm);
    bool rand_condition = rnd(val);
    VM_IfElse(vm, rand_condition);
}

void VM_IfDead(con_vm_t* vm) {
    const spritetype* spr = vm->sprite;
    i32 health = spr->extra;
    if (spr->picnum == APLAYER) {
        health--;
    }
    VM_IfElse(vm, health < 0);
}

void VM_IfSquished(con_vm_t* vm) {
    i16 spr = vm->sprite_idx;
    i16 player = vm->player_idx;
    bool was_squished = (P_IfSquished(spr, player) == 1);
    VM_IfElse(vm, was_squished);
}

void VM_IfHitSpace(con_vm_t* vm) {
    VM_IfElse(vm, sync[vm->player_idx].bits & (1 << 29));
}

void VM_IfOutside(con_vm_t* vm) {
    VM_IfElse(vm, sector[vm->sprite->sectnum].ceilingstat & 1);
}

void VM_IfMultiplayer(con_vm_t* vm) {
    VM_IfElse(vm, ud.multimode > 1);
}

void VM_IfInSpace(con_vm_t* vm) {
    bool in_space = P_CeilingSpace(vm->sprite->sectnum);
    VM_IfElse(vm, in_space);
}

void VM_IfBulletNear(con_vm_t* vm) {
    VM_IfElse(vm, VM_Dodge(vm->sprite));
}

void VM_IfAwayFromWall(con_vm_t* vm) {
    const spritetype* spr = vm->sprite;
    i16 sec = spr->sectnum;
    updatesector(spr->x + 108, spr->y + 108, &sec);
    if (sec != spr->sectnum) {
        VM_IfElse(vm, false);
        return;
    }
    updatesector(spr->x - 108, spr->y - 108, &sec);
    if (sec != spr->sectnum) {
        VM_IfElse(vm, false);
        return;
    }
    updatesector(spr->x + 108, spr->y - 108, &sec);
    if (sec != spr->sectnum) {
        VM_IfElse(vm, false);
        return;
    }
    updatesector(spr->x - 108, spr->y + 108, &sec);
    if (sec != spr->sectnum) {
        VM_IfElse(vm, false);
        return;
    }
    VM_IfElse(vm, true);
}

void VM_IfInOuterSpace(con_vm_t* vm) {
    bool in_outerspace = P_FloorSpace(vm->sprite->sectnum);
    VM_IfElse(vm, in_outerspace);
}

void VM_IfNotMoving(con_vm_t* vm) {
    VM_IfElse(vm, (hittype[vm->sprite_idx].movflag & 49152) > 16384);
}

void VM_IfAngDiffLt(con_vm_t* vm) {
    i32 ang = VM_READ(vm);
    i32 j = klabs(getincangle(vm->player->ang, vm->sprite->ang));
    VM_IfElse(vm, j <= ang);
}
