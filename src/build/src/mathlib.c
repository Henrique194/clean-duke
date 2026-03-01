/*
 * Copyright (C) 1994-1995 Apogee Software, Ltd.
 * Copyright (C) 1996, 2003 - 3D Realms Entertainment
 * Copyright (C) Henrique Barateli, <henriquejb194@gmail.com>, et al.
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
#include "io.h"
#endif

#include "build/platform.h"
#include "build/build.h"
#include "build/engine.h"


extern short radarang[1280];

static uint16_t sqrtable[4096];
static uint16_t shlookup[4096 + 256];


void initksqrt(void) {
    int32_t i, j, k;

    j = 1;
    k = 0;
    for (i = 0; i < 4096; i++) {
        if (i >= j) {
            j <<= 2;
            k++;
        }
        sqrtable[i] = (uint16_t) (msqrtasm((i << 18) + 131072) << 1);
        shlookup[i] = (k << 1) + ((10 - k) << 8);
        if (i < 256)
            shlookup[i + 4096] = ((k + 6) << 1) + ((10 - (k + 6)) << 8);
    }
}


int krand() {
    randomseed = (randomseed * 27584621) + 1;
    return ((uint32_t) randomseed) >> 16;
}

int ksqrt(int32_t num) {
    uint32_t param = (uint32_t) num;

    uint16_t* shlookup_a = (uint16_t*) shlookup;
    uint16_t* sqrtable_a = (uint16_t*) sqrtable;
    uint16_t cx;

    if (param & 0xff000000)
        cx = shlookup_a[(param >> 24) + 4096];
    else
        cx = shlookup_a[param >> 12];

    param = param >> (cx & 0xff);
    param = ((param & 0xffff0000) | sqrtable_a[param]);
    param = param >> ((cx & 0xff00) >> 8);

    return (int32_t) param;
}


int getangle(int32_t xvect, int32_t yvect) {
    if ((xvect | yvect) == 0)
        return (0);
    if (xvect == 0)
        return (512 + ((yvect < 0) << 10));
    if (yvect == 0)
        return (((xvect < 0) << 10));
    if (xvect == yvect)
        return (256 + ((xvect < 0) << 10));
    if (xvect == -yvect)
        return (768 + ((xvect > 0) << 10));
    if (klabs(xvect) > klabs(yvect))
        return (((radarang[640 + scale(160, yvect, xvect)] >> 6) + ((xvect < 0) << 10)) & 2047);
    return (((radarang[640 - scale(160, xvect, yvect)] >> 6) + 512 + ((yvect < 0) << 10)) & 2047);
}


void rotatepoint(int32_t xpivot, int32_t ypivot, int32_t x, int32_t y,
                 short daang, int32_t* x2, int32_t* y2) {
    int32_t dacos, dasin;

    dacos = COS(daang);
    dasin = SIN(daang);
    x -= xpivot;
    y -= ypivot;
    *x2 = dmulscale14(x, dacos, -y, dasin) + xpivot;
    *y2 = dmulscale14(y, dacos, x, dasin) + ypivot;
}


int nextsectorneighborz(short sectnum, int32_t thez,
                        short topbottom, short direction) {
    walltype* wal;
    int32_t i, testz, nextz;
    short sectortouse;

    if (direction == 1)
        nextz = 0x7fffffff;
    else
        nextz = 0x80000000;

    sectortouse = -1;

    wal = &wall[sector[sectnum].wallptr];
    i = sector[sectnum].wallnum;
    do {
        if (wal->nextsector >= 0) {
            if (topbottom == 1) {
                testz = sector[wal->nextsector].floorz;
                if (direction == 1) {
                    if ((testz > thez) && (testz < nextz)) {
                        nextz = testz;
                        sectortouse = wal->nextsector;
                    }
                } else {
                    if ((testz < thez) && (testz > nextz)) {
                        nextz = testz;
                        sectortouse = wal->nextsector;
                    }
                }
            } else {
                testz = sector[wal->nextsector].ceilingz;
                if (direction == 1) {
                    if ((testz > thez) && (testz < nextz)) {
                        nextz = testz;
                        sectortouse = wal->nextsector;
                    }
                } else {
                    if ((testz < thez) && (testz > nextz)) {
                        nextz = testz;
                        sectortouse = wal->nextsector;
                    }
                }
            }
        }
        wal++;
        i--;
    } while (i != 0);

    return (sectortouse);
}


int getceilzofslope(short sectnum, int32_t dax, int32_t day) {
    int32_t dx, dy, i, j;
    walltype* wal;

    if (!(sector[sectnum].ceilingstat & 2))
        return (sector[sectnum].ceilingz);
    wal = &wall[sector[sectnum].wallptr];
    dx = wall[wal->point2].x - wal->x;
    dy = wall[wal->point2].y - wal->y;
    i = (ksqrt(dx * dx + dy * dy) << 5);
    if (i == 0)
        return (sector[sectnum].ceilingz);
    j = dmulscale3(dx, day - wal->y, -dy, dax - wal->x);
    return (sector[sectnum].ceilingz + scale(sector[sectnum].ceilingheinum, j, i));
}


int getflorzofslope(short sectnum, int32_t dax, int32_t day) {
    int32_t dx, dy, i, j;
    walltype* wal;

    if (!(sector[sectnum].floorstat & 2))
        return (sector[sectnum].floorz);

    wal = &wall[sector[sectnum].wallptr];
    dx = wall[wal->point2].x - wal->x;
    dy = wall[wal->point2].y - wal->y;
    i = (ksqrt(dx * dx + dy * dy) << 5);

    if (i == 0)
        return (sector[sectnum].floorz);

    j = dmulscale3(dx, day - wal->y, -dy, dax - wal->x);

    return (sector[sectnum].floorz + scale(sector[sectnum].floorheinum, j, i));
}


//
// Output the ceiling and floor Z coordinate in the two last parameters for given:
// sectorNumber and worldspace (coordinate X,Y).
//
// If the sector is flat, this is jsut a lookup. But if either the floor/ceiling have
// a slope it requires more calculation
//
void getzsofslope(short sectnum, int32_t dax, int32_t day, int32_t* ceilz,
                  int32_t* florz) {
    int32_t dx, dy, i, j;
    walltype *wal, *wal2;
    sectortype* sec;

    sec = &sector[sectnum];
    *ceilz = sec->ceilingz;
    *florz = sec->floorz;

    //If the sector has a slopped ceiling or a slopped floor then it needs more calculation.
    if ((sec->ceilingstat | sec->floorstat) & 2) {
        wal = &wall[sec->wallptr];
        wal2 = &wall[wal->point2];
        dx = wal2->x - wal->x;
        dy = wal2->y - wal->y;
        i = (ksqrt(dx * dx + dy * dy) << 5);
        if (i == 0)
            return;
        j = dmulscale3(dx, day - wal->y, -dy, dax - wal->x);

        if (sec->ceilingstat & 2)
            *ceilz = (*ceilz) + scale(sec->ceilingheinum, j, i);
        if (sec->floorstat & 2)
            *florz = (*florz) + scale(sec->floorheinum, j, i);
    }
}


void alignceilslope(short dasect, int32_t x, int32_t y, int32_t z) {
    int32_t i, dax, day;
    walltype* wal;

    wal = &wall[sector[dasect].wallptr];
    dax = wall[wal->point2].x - wal->x;
    day = wall[wal->point2].y - wal->y;

    i = (y - wal->y) * dax - (x - wal->x) * day;
    if (i == 0)
        return;
    sector[dasect].ceilingheinum = scale((z - sector[dasect].ceilingz) << 8,
                                         ksqrt(dax * dax + day * day), i);

    if (sector[dasect].ceilingheinum == 0)
        sector[dasect].ceilingstat &= ~2;
    else
        sector[dasect].ceilingstat |= 2;
}


void alignflorslope(short dasect, int32_t x, int32_t y, int32_t z) {
    int32_t i, dax, day;
    walltype* wal;

    wal = &wall[sector[dasect].wallptr];
    dax = wall[wal->point2].x - wal->x;
    day = wall[wal->point2].y - wal->y;

    i = (y - wal->y) * dax - (x - wal->x) * day;
    if (i == 0)
        return;
    sector[dasect].floorheinum = scale((z - sector[dasect].floorz) << 8,
                                       ksqrt(dax * dax + day * day), i);

    if (sector[dasect].floorheinum == 0)
        sector[dasect].floorstat &= ~2;
    else
        sector[dasect].floorstat |= 2;
}