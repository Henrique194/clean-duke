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

#if WIN32
#include <conio.h>
#endif

#include <SDL_stdinc.h>
#include "duke3d.h"
#include "gamedefs.h"
#include "con/con.h"
#include "build/engine.h"
#include "input/keyboard.h"
#include "soundefs.h"
#include "funct.h"
#include "actors/actors.h"


short getincangle(short a, short na) {
    a &= 2047;
    na &= 2047;
    if (klabs(a - na) < 1024) {
        return (na - a);
    }
    if (na > 1024) {
        na -= 2048;
    }
    if (a > 1024) {
        a -= 2048;
    }
    na -= 2048;
    a -= 2048;
    return (na - a);
}

void getglobalz(short i) {
    int32_t hz, lz, zr;

    spritetype* s = &sprite[i];

    if (s->statnum == 10 || s->statnum == 6 || s->statnum == 2 || s->statnum == 1 || s->statnum == 4) {
        if (s->statnum == 4)
            zr = 4L;
        else
            zr = 127L;

        getzrange(s->x, s->y, s->z - (FOURSLEIGHT), s->sectnum,
                  &hittype[i].ceilingz, &hz, &hittype[i].floorz, &lz, zr,
                  CLIPMASK0);

        if ((lz & 49152) == 49152 && (sprite[lz & (MAXSPRITES - 1)].cstat & 48) == 0) {
            lz &= (MAXSPRITES - 1);
            if (badguy(&sprite[lz]) && sprite[lz].pal != 1) {
                if (s->statnum != 4) {
                    hittype[i].dispicnum = -4; // No shadows on actors
                    s->xvel = -256;
                    P_SetSprite(i, CLIPMASK0);
                }
            } else if (sprite[lz].picnum == APLAYER && badguy(s)) {
                hittype[i].dispicnum = -4; // No shadows on actors
                s->xvel = -256;
                P_SetSprite(i, CLIPMASK0);
            } else if (s->statnum == 4 && sprite[lz].picnum == APLAYER)
                if (s->owner == lz) {
                    hittype[i].ceilingz = sector[s->sectnum].ceilingz;
                    hittype[i].floorz = sector[s->sectnum].floorz;
                }
        }
    } else {
        hittype[i].ceilingz = sector[s->sectnum].ceilingz;
        hittype[i].floorz = sector[s->sectnum].floorz;
    }
}


void makeitfall(short i) {
    spritetype* s = &sprite[i];
    int32_t hz, lz, c;

    if (P_FloorSpace(s->sectnum))
        c = 0;
    else {
        if (P_CeilingSpace(s->sectnum) || sector[s->sectnum].lotag == 2)
            c = gc / 6;
        else
            c = gc;
    }

    if ((s->statnum == 1 || s->statnum == 10 || s->statnum == 2
         || s->statnum == 6))
        getzrange(s->x, s->y, s->z - (FOURSLEIGHT), s->sectnum,
                  &hittype[i].ceilingz, &hz, &hittype[i].floorz, &lz, 127L,
                  CLIPMASK0);
    else {
        hittype[i].ceilingz = sector[s->sectnum].ceilingz;
        hittype[i].floorz = sector[s->sectnum].floorz;
    }

    if (s->z < hittype[i].floorz - (FOURSLEIGHT)) {
        if (sector[s->sectnum].lotag == 2 && s->zvel > 3122)
            s->zvel = 3144;
        if (s->zvel < 6144)
            s->zvel += c;
        else
            s->zvel = 6144;
        s->z += s->zvel;
    }
    if (s->z >= hittype[i].floorz - (FOURSLEIGHT)) {
        s->z = hittype[i].floorz - FOURSLEIGHT;
        s->zvel = 0;
    }
}


/*
================================================================================

LOADING CON FILE

================================================================================
*/

//
// Reload normal GRP internal *.CON files.
//
static void CON_LoadGrpFile(char* file) {
    const int base_grp_crc = getGRPcrc32(0);
    if (base_grp_crc == CRC_BASE_GRP_SHAREWARE_13
        || base_grp_crc == CRC_BASE_GRP_FULL_13) {
        conVersion = 13;
    }
    // Force GRP con files.
    CON_LoadFile(file, 1);
}

static const char* CON_GetBaseGrpVersion(void) {
    const int base_crc = getGRPcrc32(0);
    switch (base_crc) {
        case CRC_BASE_GRP_SHAREWARE_13:
        case CRC_BASE_GRP_FULL_13:
            return "v1.3 Shareware/Full";
        case CRC_BASE_GRP_PLUTONIUM_14:
            return "v1.4 PLUTONIUM";
        case CRC_BASE_GRP_ATOMIC_15:
            return "v1.5 ATOMIC";
        default:
            return NULL;
    }
}

static const char* CON_GetVersion(void) {
    switch (conVersion) {
        case 13:
            return "v1.3";
        case 14:
            return "v1.4";
        case 15:
            return "v1.5";
        default:
            return NULL;
    }
}

//
// Warns the user that the CON file has an incompatible version
// with the GRP file, and then asks if the user wants to
// use the CON file from the base GRP file instead.
//
static int CON_WarnInvalidVersion(void) {
    printf("\nYou are trying to use a %s *.GRP with %s\n"
           "external *.CON files. You may run in troubles by doing so and/or get\n"
           "Out Of Synch errors. You can safely delete those files so xDuke will\n"
           "always use the GRP internal CON files.\n"
           "\nReload normal GRP internal *.CON files? (Y/N) : ",
           CON_GetBaseGrpVersion(),
           CON_GetVersion()
        );
    uint8_t kbdKey;
    do {
        kbdKey = (uint8_t) (getch() | ' ');
    } while (kbdKey != 'y' && kbdKey != 'n');
    printf("%c\n", kbdKey);
    return kbdKey == 'y';
}

//
// Checks if the compiled CON file is compatible with the base GRP file.
//
static int CON_CheckVersion(void) {
    const int grp_crc = getGRPcrc32(1);
    if (grp_crc != 0) {
        return 1;
    }
    const int base_grp_crc = getGRPcrc32(0);
    switch (base_grp_crc) {
        case CRC_BASE_GRP_SHAREWARE_13:
        case CRC_BASE_GRP_FULL_13:
            return conVersion == 13;
        case CRC_BASE_GRP_PLUTONIUM_14:
            return conVersion == 14;
        case CRC_BASE_GRP_ATOMIC_15:
            return conVersion == 15;
        default:
            Error(EXIT_SUCCESS, "Unknown base GRP version\n");
            return 0;
    }
}

void CON_LoadFile(char* file, int read_grp) {
    if (!COM_CompileFile(file, read_grp)) {
        return;
    }
    if (CON_CheckVersion()) {
        return;
    }
    if (CON_WarnInvalidVersion()) {
        CON_LoadGrpFile(file);
    }
}
