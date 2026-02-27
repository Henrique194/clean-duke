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

#ifndef RENDERER_H
#define RENDERER_H

#include "types.h"
#include "build/build.h"


#define MAXWALLSB 2048

#define MAXYSAVES ((MAXXDIM * MAXSPRITES) >> 7)

i32 getpalookup(i32 davis, i32 dashade);

void scansector(short sectnum);

int wallfront(i32 pvWallID1, i32 pvWallID2);

void drawrooms(
    i32 posx,
    i32 posy,
    i32 posz,
    i16 ang,
    i32 horiz,
    i16 cursectnum
);

extern i32 globalposx;
extern i32 globalposy;
extern i32 globalposz;
extern i32 globalhoriz;
extern i32 globalx1;
extern i32 globaly1;
extern i32 globalx2;
extern i32 globaly2;
extern i32 globalx3;
extern i32 globaly3;
extern i32 globaluclip;
extern i32 globaldclip;
extern i32 globvis;
extern i32 globalshade;
extern i32 globalorientation;
extern i32 globalvisibility;
extern i32 globalhisibility;
extern i32 globalpisibility;
extern i32 globalcisibility;
extern i16 globalang;
extern i16 globalpicnum;
extern u8 globalxshift;
extern u8 globalyshift;
extern u8 globparaceilclip;
extern u8 globparaflorclip;
extern u8* globalbufplc;
extern short umost[MAXXDIM + 1];
extern short dmost[MAXXDIM + 1];
extern i32 cosglobalang;
extern i32 singlobalang;
extern i32 cosviewingrangeglobalang;
extern i32 sinviewingrangeglobalang;
extern short bunchWallsList[MAXWALLSB];
extern short bunchfirst[MAXWALLSB];
extern short bunchlast[MAXWALLSB];
extern char r_tempbuf[MAXWALLS];
extern short numhits;


void preparemirror(
    i32 dax,
    i32 day,
    i16 daang,
    i16 dawall,
    i32* tposx,
    i32* tposy,
    short* tang
);

void completemirror(void);

extern u8 inpreparemirror;
extern i32 mirrorsx1;
extern i32 mirrorsy1;
extern i32 mirrorsx2;
extern i32 mirrorsy2;


void dorotatesprite(
    i32 sx,
    i32 sy,
    i32 z,
    short a,
    short picnum,
    i8 dashade,
    u8 dapalnum,
    u8 dastat,
    i32 cx1,
    i32 cy1,
    i32 cx2,
    i32 cy2
);

void drawmasks(void);

void rotatesprite(
    i32 sx,
    i32 sy,
    i32 z,
    i16 a,
    i16 picnum,
    i8 dashade,
    u8 dapalnum,
    u8 dastat,
    i32 cx1,
    i32 cy1,
    i32 cx2,
    i32 cy2
);


enum vector_index_e {
    VEC_X = 0,
    VEC_Y = 1,
};

enum screenSpaceCoo_index_e {
    VEC_COL = 0,
    VEC_DIST = 1,
};

typedef i32 vector_t[2];
typedef i32 coo2D_t[2];

//
// This is the structure emitted for each wall that is potentially visible.
// A stack of those is populated when the sectors are scanned.
//
typedef struct {
    // Camera space coordinates of the wall endpoints. Access with vector_index_e.
    vector_t cameraSpaceCoo[2];
    // The index of the sector this wall belongs to in the map database.
    i16 sectorId;
    // The index of the wall in the map database.
    i16 worldWallId;
    // Screen space coordinate of the wall endpoints. Access with screenSpaceCoo_index_e.
    coo2D_t screenSpaceCoo[2];
} pv_wall_t;

i32 krecipasm(i32 i);

void prepwall(i32 z, walltype* wal);

int owallmost(short* mostbuf, i32 w, i32 z);

int wallmost(short* mostbuf, i32 w, i32 sectnum, u8 dastat);

void drawalls(i32 bunch);

extern i32 globalxpanning;
extern i32 globalypanning;
extern i32 globalzd;
extern i32 globalyscale;
extern i32 swall[MAXXDIM + 1];
extern i32 lwall[MAXXDIM + 4];
extern short maskwall[MAXWALLSB];
extern short maskwallcnt;
extern short smost[MAXYSAVES];
extern short smostcnt;
extern short smoststart[MAXWALLSB];
extern u8 smostwalltype[MAXWALLSB];
extern i32 smostwall[MAXWALLSB];
extern i32 smostwallcnt;
extern short searchsector;
extern short searchwall;
extern short searchstat;
extern short searchit;
extern i32 searchx;
extern i32 searchy;
extern i16 uwall[MAXXDIM + 1];
extern i16 dwall[MAXXDIM + 1];
extern u8 globalpolytype;
extern pv_wall_t pv_walls[MAXWALLSB];

#endif
