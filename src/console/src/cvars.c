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

#include "console/cvars.h"
#include "console/cvar_defs.h"
#include "build/platform.h"
#include <stdlib.h>
#include <string.h>

#define MAX_CVARS 32

static cvar_t cvar_list[MAX_CVARS];
static int num_cvar = 0;


void CVAR_RegisterCvar(
    const char* varname,
    const char* varhelp,
    void* variable,
    function_t function
) {
    if (!function) {
        return;
    }
    cvar_t* cvar = &cvar_list[num_cvar];
    num_cvar++;
    cvar->variable = variable;
    cvar->function = function;
    memset(cvar->name, 0, 64);
    strncpy(cvar->name, varname, 63);
    memset(cvar->help, 0, 64);
    strncpy(cvar->help, varhelp, 63);
}

int CVAR_GetNumCvars(void) {
    return num_cvar;
}

cvar_t* CVAR_GetCvar(int nBinding) {
    if (nBinding > num_cvar - 1) {
        return NULL;
    }
    return &cvar_list[nBinding];
}

// Bind all standard CVars here
void CVAR_RegisterDefaultCvarBindings(void) {
    CVARDEFS_Init();
}

void CVAR_Render(void) {
    CVARDEFS_Render();
}

cvar_t* CVAR_FindVar(const char* name) {
    if (!name || *name == '\0') {
        return NULL;
    }
    const int num_cvars = CVAR_GetNumCvars();
    for (int i = 0; i < num_cvars; i++) {
        cvar_t* cvar = CVAR_GetCvar(i);
        if (strcmpi(name, cvar->name) == 0) {
            return cvar;
        }
    }
    return NULL;
}
