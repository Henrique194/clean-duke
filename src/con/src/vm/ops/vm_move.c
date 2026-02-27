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

void VM_Move(con_vm_t* vm) {
    actor_state_t* actor = vm->actor;
    actor->move_script = VM_READ(vm);
    actor->count = 0;

    spritetype* spr = vm->sprite;
    spr->hitag = (i16) VM_READ(vm);
    if (spr->hitag & random_angle) {
        spr->ang = TRAND & 2047;
    }
}

void VM_IfMove(con_vm_t* vm) {
    i32 move_script = VM_READ(vm);
    VM_IfElse(vm, vm->actor->move_script == move_script);
}
