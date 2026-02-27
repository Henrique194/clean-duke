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
// drawing functions

#include "renderer/renderer.h"
#include "renderer/draw.h"
#include "build/engine.h"
#include "video/display.h"
#if WIN32
#include "io.h"
#endif


// Don't forget to change this in A.ASM also!
#define BITSOFPRECISION 3


extern int32_t viewingrangerecip;
extern int32_t transarea;
extern int32_t globalpal;
extern int32_t lastx[MAXYDIM];
extern short uplc[MAXXDIM + 1];
extern short dplc[MAXXDIM + 1];
extern short radarang2[MAXXDIM + 1];
extern int32_t reciptable[2048];


static int32_t globalzx;
int32_t globalxpanning;
int32_t globalypanning;
int32_t globalzd;
int32_t globalyscale;
static int32_t globalx;
static int32_t globaly;
static int32_t globalz;

int32_t swall[MAXXDIM + 1];
int32_t lwall[MAXXDIM + 4];

short maskwall[MAXWALLSB];
short maskwallcnt;

short smost[MAXYSAVES];
short smostcnt;
int32_t smostwallcnt = -1;
short smoststart[MAXWALLSB];
uint8_t smostwalltype[MAXWALLSB];
int32_t smostwall[MAXWALLSB];

short searchit = 0;

// Search input.
int32_t searchx = -1;
int32_t searchy;
// Search output.
short searchsector;
short searchwall;
short searchstat = -1;

int16_t uwall[MAXXDIM + 1];
int16_t dwall[MAXXDIM + 1];
static int32_t swplc[MAXXDIM + 1];
static int32_t lplc[MAXXDIM + 1];

static intptr_t slopalookup[16384];

/* Textured Map variables */
uint8_t globalpolytype;

// Potentially Visible walls are stored in this stack.
pv_wall_t pv_walls[MAXWALLSB];


int32_t krecipasm(int32_t i) {
    // tanguyf: fix strict aliasing rules.
    union
    {
        float f;
        int32_t i;
    } value;

    // Ken did this
    //   float f = (float)i;
    //   i = *(int32_t *)&f;

    value.f = (float) i;
    i = value.i;
    return ((reciptable[(i >> 12) & 2047] >> (((i - 0x3f800000) >> 23) & 31)) ^
            (i >> 31));
}

/*
 FCS:

 Goal : ????
 param 1: Z is the wallID in the list of potentially visible walls.
 param 2: Only used to lookup the xrepeat attribute of the wall.

*/
void prepwall(int32_t z, walltype* wal) {
    int32_t i, l = 0, ol = 0, splc, sinc, x, topinc, top, botinc, bot, walxrepeat;
    vector_t* wallCoo = pv_walls[z].cameraSpaceCoo;

    walxrepeat = (wal->xrepeat << 3);

    /* lwall calculation */
    i = pv_walls[z].screenSpaceCoo[0][VEC_COL] - halfxdimen;

    //Let's use some of the camera space wall coordinate now.
    topinc = -(wallCoo[0][VEC_Y] >> 2);
    botinc = ((wallCoo[1][VEC_Y] - wallCoo[0][VEC_Y]) >> 8);

    top = mulscale5(wallCoo[0][VEC_X], xdimen) + mulscale2(topinc, i);
    bot = mulscale11(wallCoo[0][VEC_X] - wallCoo[1][VEC_X], xdimen) + mulscale2(
              botinc, i);

    splc = mulscale19(wallCoo[0][VEC_Y], xdimscale);
    sinc = mulscale16(wallCoo[1][VEC_Y] - wallCoo[0][VEC_Y], xdimscale);

    //X screenspce column of point Z.
    x = pv_walls[z].screenSpaceCoo[0][VEC_COL];

    if (bot != 0) {
        l = divscale12(top, bot);
        swall[x] = mulscale21(l, sinc) + splc;
        l *= walxrepeat;
        lwall[x] = (l >> 18);
    }

    //If the wall is less than 4 column wide.
    while (x + 4 <= pv_walls[z].screenSpaceCoo[1][VEC_COL]) {
        top += topinc;
        bot += botinc;
        if (bot != 0) {
            ol = l;
            l = divscale12(top, bot);
            swall[x + 4] = mulscale21(l, sinc) + splc;
            l *= walxrepeat;
            lwall[x + 4] = (l >> 18);
        }
        i = ((ol + l) >> 1);
        lwall[x + 2] = (i >> 18);
        lwall[x + 1] = ((ol + i) >> 19);
        lwall[x + 3] = ((l + i) >> 19);
        swall[x + 2] = ((swall[x] + swall[x + 4]) >> 1);
        swall[x + 1] = ((swall[x] + swall[x + 2]) >> 1);
        swall[x + 3] = ((swall[x + 4] + swall[x + 2]) >> 1);
        x += 4;
    }

    //If the wall is less than 2 columns wide.
    if (x + 2 <= pv_walls[z].screenSpaceCoo[1][VEC_COL]) {
        top += (topinc >> 1);
        bot += (botinc >> 1);
        if (bot != 0) {
            ol = l;
            l = divscale12(top, bot);
            swall[x + 2] = mulscale21(l, sinc) + splc;
            l *= walxrepeat;
            lwall[x + 2] = (l >> 18);
        }
        lwall[x + 1] = ((l + ol) >> 19);
        swall[x + 1] = ((swall[x] + swall[x + 2]) >> 1);
        x += 2;
    }

    //The wall is 1 column wide.
    if (x + 1 <= pv_walls[z].screenSpaceCoo[1][VEC_COL]) {
        bot += (botinc >> 2);
        if (bot != 0) {
            l = divscale12(top + (topinc >> 2), bot);
            swall[x + 1] = mulscale21(l, sinc) + splc;
            lwall[x + 1] = mulscale18(l, walxrepeat);
        }
    }

    if (lwall[pv_walls[z].screenSpaceCoo[0][VEC_COL]] < 0)
        lwall[pv_walls[z].screenSpaceCoo[0][VEC_COL]] = 0;

    if ((lwall[pv_walls[z].screenSpaceCoo[1][VEC_COL]] >= walxrepeat) && (
            walxrepeat))
        lwall[pv_walls[z].screenSpaceCoo[1][VEC_COL]] = walxrepeat - 1;

    if (wal->cstat & 8) {
        walxrepeat--;
        for (x = pv_walls[z].screenSpaceCoo[0][VEC_COL];
             x <= pv_walls[z].screenSpaceCoo[1][VEC_COL]; x++)
            lwall[x] = walxrepeat - lwall[x];
    }
}

int owallmost(short* mostbuf, int32_t w, int32_t z) {
    int32_t bad, inty, xcross, y, yinc;
    int32_t s1, s2, s3, s4, ix1, ix2, iy1, iy2, t;

    z <<= 7;
    s1 = mulscale20(globaluclip, pv_walls[w].screenSpaceCoo[0][VEC_DIST]);
    s2 = mulscale20(globaluclip, pv_walls[w].screenSpaceCoo[1][VEC_DIST]);
    s3 = mulscale20(globaldclip, pv_walls[w].screenSpaceCoo[0][VEC_DIST]);
    s4 = mulscale20(globaldclip, pv_walls[w].screenSpaceCoo[1][VEC_DIST]);
    bad = (z < s1) + ((z < s2) << 1) + ((z > s3) << 2) + ((z > s4) << 3);

    ix1 = pv_walls[w].screenSpaceCoo[0][VEC_COL];
    iy1 = pv_walls[w].screenSpaceCoo[0][VEC_DIST];
    ix2 = pv_walls[w].screenSpaceCoo[1][VEC_COL];
    iy2 = pv_walls[w].screenSpaceCoo[1][VEC_DIST];

    if ((bad & 3) == 3) {
        clearbufbyte(&mostbuf[ix1], (ix2 - ix1 + 1) * sizeof(mostbuf[0]), 0L);
        return (bad);
    }

    if ((bad & 12) == 12) {
        clearbufbyte(&mostbuf[ix1], (ix2 - ix1 + 1) * sizeof(mostbuf[0]),
                     ydimen + (ydimen << 16));
        return (bad);
    }

    if (bad & 3) {
        t = divscale30(z - s1, s2 - s1);
        inty = pv_walls[w].screenSpaceCoo[0][VEC_DIST] + mulscale30(
                   pv_walls[w].screenSpaceCoo[1][VEC_DIST] - pv_walls[w].
                   screenSpaceCoo[0][VEC_DIST], t);
        xcross = pv_walls[w].screenSpaceCoo[0][VEC_COL] + scale(
                     mulscale30(pv_walls[w].screenSpaceCoo[1][VEC_DIST], t),
                     pv_walls[w].screenSpaceCoo[1][VEC_COL] - pv_walls[w].
                     screenSpaceCoo[0][VEC_COL], inty);

        if ((bad & 3) == 2) {
            if (pv_walls[w].screenSpaceCoo[0][VEC_COL] <= xcross) {
                iy2 = inty;
                ix2 = xcross;
            }
            clearbufbyte(&mostbuf[xcross + 1],
                         (pv_walls[w].screenSpaceCoo[1][VEC_COL] - xcross) *
                         sizeof(mostbuf[0]), 0L);
        } else {
            if (xcross <= pv_walls[w].screenSpaceCoo[1][VEC_COL]) {
                iy1 = inty;
                ix1 = xcross;
            }
            clearbufbyte(&mostbuf[pv_walls[w].screenSpaceCoo[0][VEC_COL]],
                         (xcross - pv_walls[w].screenSpaceCoo[0][VEC_COL] + 1) *
                         sizeof(mostbuf[0]), 0L);
        }
    }

    if (bad & 12) {
        t = divscale30(z - s3, s4 - s3);
        inty = pv_walls[w].screenSpaceCoo[0][VEC_DIST] + mulscale30(
                   pv_walls[w].screenSpaceCoo[1][VEC_DIST] - pv_walls[w].
                   screenSpaceCoo[0][VEC_DIST], t);
        xcross = pv_walls[w].screenSpaceCoo[0][VEC_COL] + scale(
                     mulscale30(pv_walls[w].screenSpaceCoo[1][VEC_DIST], t),
                     pv_walls[w].screenSpaceCoo[1][VEC_COL] - pv_walls[w].
                     screenSpaceCoo[0][VEC_COL], inty);

        if ((bad & 12) == 8) {
            if (pv_walls[w].screenSpaceCoo[0][VEC_COL] <= xcross) {
                iy2 = inty;
                ix2 = xcross;
            }
            clearbufbyte(&mostbuf[xcross + 1],
                         (pv_walls[w].screenSpaceCoo[1][VEC_COL] - xcross) *
                         sizeof(mostbuf[0]), ydimen + (ydimen << 16));
        } else {
            if (xcross <= pv_walls[w].screenSpaceCoo[1][VEC_COL]) {
                iy1 = inty;
                ix1 = xcross;
            }
            clearbufbyte(&mostbuf[pv_walls[w].screenSpaceCoo[0][VEC_COL]],
                         (xcross - pv_walls[w].screenSpaceCoo[0][VEC_COL] + 1) *
                         sizeof(mostbuf[0]), ydimen + (ydimen << 16));
        }
    }

    y = (scale(z, xdimenscale, iy1) << 4);
    yinc = ((scale(z, xdimenscale, iy2) << 4) - y) / (ix2 - ix1 + 1);
    qinterpolatedown16short((int32_t*) &mostbuf[ix1], ix2 - ix1 + 1,
                            y + (globalhoriz << 16), yinc);

    if (mostbuf[ix1] < 0)
        mostbuf[ix1] = 0;
    if (mostbuf[ix1] > ydimen)
        mostbuf[ix1] = ydimen;
    if (mostbuf[ix2] < 0)
        mostbuf[ix2] = 0;
    if (mostbuf[ix2] > ydimen)
        mostbuf[ix2] = ydimen;

    return (bad);
}


int wallmost(short* mostbuf, int32_t w, int32_t sectnum, uint8_t dastat) {
    int32_t bad, i, j, t, y, z, inty, intz, xcross, yinc, fw;
    int32_t x1, y1, z1, x2, y2, z2, xv, yv, dx, dy, dasqr, oz1, oz2;
    int32_t s1, s2, s3, s4, ix1, ix2, iy1, iy2;

    if (dastat == 0) {
        z = sector[sectnum].ceilingz - globalposz;
        if ((sector[sectnum].ceilingstat & 2) == 0)
            return (owallmost(mostbuf, w, z));
    } else {
        z = sector[sectnum].floorz - globalposz;
        if ((sector[sectnum].floorstat & 2) == 0)
            return (owallmost(mostbuf, w, z));
    }

    i = pv_walls[w].worldWallId;
    if (i == sector[sectnum].wallptr)
        return (owallmost(mostbuf, w, z));

    x1 = wall[i].x;
    x2 = wall[wall[i].point2].x - x1;
    y1 = wall[i].y;
    y2 = wall[wall[i].point2].y - y1;

    fw = sector[sectnum].wallptr;
    i = wall[fw].point2;
    dx = wall[i].x - wall[fw].x;
    dy = wall[i].y - wall[fw].y;
    dasqr = krecipasm(ksqrt(dx * dx + dy * dy));

    if (pv_walls[w].screenSpaceCoo[0][VEC_COL] == 0) {
        xv = cosglobalang + sinviewingrangeglobalang;
        yv = singlobalang - cosviewingrangeglobalang;
    } else {
        xv = x1 - globalposx;
        yv = y1 - globalposy;
    }
    i = xv * (y1 - globalposy) - yv * (x1 - globalposx);
    j = yv * x2 - xv * y2;

    if (klabs(j) > klabs(i >> 3))
        i = divscale28(i, j);

    if (dastat == 0) {
        t = mulscale15(sector[sectnum].ceilingheinum, dasqr);
        z1 = sector[sectnum].ceilingz;
    } else {
        t = mulscale15(sector[sectnum].floorheinum, dasqr);
        z1 = sector[sectnum].floorz;
    }

    z1 = dmulscale24(dx * t, mulscale20(y2, i) + ((y1 - wall[fw].y) << 8),
                     -dy * t,
                     mulscale20(x2, i) + ((x1 - wall[fw].x) << 8)) + (
             (z1 - globalposz) << 7);


    if (pv_walls[w].screenSpaceCoo[1][VEC_COL] == xdimen - 1) {
        xv = cosglobalang - sinviewingrangeglobalang;
        yv = singlobalang + cosviewingrangeglobalang;
    } else {
        xv = (x2 + x1) - globalposx;
        yv = (y2 + y1) - globalposy;
    }

    i = xv * (y1 - globalposy) - yv * (x1 - globalposx);
    j = yv * x2 - xv * y2;

    if (klabs(j) > klabs(i >> 3))
        i = divscale28(i, j);

    if (dastat == 0) {
        t = mulscale15(sector[sectnum].ceilingheinum, dasqr);
        z2 = sector[sectnum].ceilingz;
    } else {
        t = mulscale15(sector[sectnum].floorheinum, dasqr);
        z2 = sector[sectnum].floorz;
    }

    z2 = dmulscale24(dx * t, mulscale20(y2, i) + ((y1 - wall[fw].y) << 8),
                     -dy * t,
                     mulscale20(x2, i) + ((x1 - wall[fw].x) << 8)) + (
             (z2 - globalposz) << 7);


    s1 = mulscale20(globaluclip, pv_walls[w].screenSpaceCoo[0][VEC_DIST]);
    s2 = mulscale20(globaluclip, pv_walls[w].screenSpaceCoo[1][VEC_DIST]);
    s3 = mulscale20(globaldclip, pv_walls[w].screenSpaceCoo[0][VEC_DIST]);
    s4 = mulscale20(globaldclip, pv_walls[w].screenSpaceCoo[1][VEC_DIST]);
    bad = (z1 < s1) + ((z2 < s2) << 1) + ((z1 > s3) << 2) + ((z2 > s4) << 3);

    ix1 = pv_walls[w].screenSpaceCoo[0][VEC_COL];
    ix2 = pv_walls[w].screenSpaceCoo[1][VEC_COL];
    iy1 = pv_walls[w].screenSpaceCoo[0][VEC_DIST];
    iy2 = pv_walls[w].screenSpaceCoo[1][VEC_DIST];
    oz1 = z1;
    oz2 = z2;

    if ((bad & 3) == 3) {
        clearbufbyte(&mostbuf[ix1], (ix2 - ix1 + 1) * sizeof(mostbuf[0]), 0L);
        return (bad);
    }

    if ((bad & 12) == 12) {
        clearbufbyte(&mostbuf[ix1], (ix2 - ix1 + 1) * sizeof(mostbuf[0]),
                     ydimen + (ydimen << 16));
        return (bad);
    }

    if (bad & 3) {
        /* inty = intz / (globaluclip>>16) */
        t = divscale30(oz1 - s1, s2 - s1 + oz1 - oz2);
        inty = pv_walls[w].screenSpaceCoo[0][VEC_DIST] + mulscale30(
                   pv_walls[w].screenSpaceCoo[1][VEC_DIST] - pv_walls[w].
                   screenSpaceCoo[0][VEC_DIST], t);
        intz = oz1 + mulscale30(oz2 - oz1, t);
        xcross = pv_walls[w].screenSpaceCoo[0][VEC_COL] + scale(
                     mulscale30(pv_walls[w].screenSpaceCoo[1][VEC_DIST], t),
                     pv_walls[w].screenSpaceCoo[1][VEC_COL] - pv_walls[w].
                     screenSpaceCoo[0][VEC_COL], inty);

        if ((bad & 3) == 2) {
            if (pv_walls[w].screenSpaceCoo[0][VEC_COL] <= xcross) {
                z2 = intz;
                iy2 = inty;
                ix2 = xcross;
            }
            clearbufbyte(&mostbuf[xcross + 1],
                         (pv_walls[w].screenSpaceCoo[1][VEC_COL] - xcross) *
                         sizeof(mostbuf[0]), 0L);
        } else {
            if (xcross <= pv_walls[w].screenSpaceCoo[1][VEC_COL]) {
                z1 = intz;
                iy1 = inty;
                ix1 = xcross;
            }
            clearbufbyte(&mostbuf[pv_walls[w].screenSpaceCoo[0][VEC_COL]],
                         (xcross - pv_walls[w].screenSpaceCoo[0][VEC_COL] + 1) *
                         sizeof(mostbuf[0]), 0L);
        }
    }

    if (bad & 12) {
        /* inty = intz / (globaldclip>>16) */
        t = divscale30(oz1 - s3, s4 - s3 + oz1 - oz2);
        inty = pv_walls[w].screenSpaceCoo[0][VEC_DIST] + mulscale30(
                   pv_walls[w].screenSpaceCoo[1][VEC_DIST] - pv_walls[w].
                   screenSpaceCoo[0][VEC_DIST], t);
        intz = oz1 + mulscale30(oz2 - oz1, t);
        xcross = pv_walls[w].screenSpaceCoo[0][VEC_COL] + scale(
                     mulscale30(pv_walls[w].screenSpaceCoo[1][VEC_DIST], t),
                     pv_walls[w].screenSpaceCoo[1][VEC_COL] - pv_walls[w].
                     screenSpaceCoo[0][VEC_COL], inty);

        if ((bad & 12) == 8) {
            if (pv_walls[w].screenSpaceCoo[0][VEC_COL] <= xcross) {
                z2 = intz;
                iy2 = inty;
                ix2 = xcross;
            }
            clearbufbyte(&mostbuf[xcross + 1],
                         (pv_walls[w].screenSpaceCoo[1][VEC_COL] - xcross) *
                         sizeof(mostbuf[0]), ydimen + (ydimen << 16));
        } else {
            if (xcross <= pv_walls[w].screenSpaceCoo[1][VEC_COL]) {
                z1 = intz;
                iy1 = inty;
                ix1 = xcross;
            }
            clearbufbyte(&mostbuf[pv_walls[w].screenSpaceCoo[0][VEC_COL]],
                         (xcross - pv_walls[w].screenSpaceCoo[0][VEC_COL] + 1) *
                         sizeof(mostbuf[0]), ydimen + (ydimen << 16));
        }
    }

    y = (scale(z1, xdimenscale, iy1) << 4);
    yinc = ((scale(z2, xdimenscale, iy2) << 4) - y) / (ix2 - ix1 + 1);
    qinterpolatedown16short((int32_t*) &mostbuf[ix1], ix2 - ix1 + 1,
                            y + (globalhoriz << 16), yinc);

    if (mostbuf[ix1] < 0)
        mostbuf[ix1] = 0;
    if (mostbuf[ix1] > ydimen)
        mostbuf[ix1] = ydimen;
    if (mostbuf[ix2] < 0)
        mostbuf[ix2] = 0;
    if (mostbuf[ix2] > ydimen)
        mostbuf[ix2] = ydimen;

    return (bad);
}



static void hline(int32_t xr, int32_t yp) {
    int32_t xl, r, s;

    xl = lastx[yp];

    if (xl > xr)
        return;

    r = horizlookup2[yp - globalhoriz + horizycent];
    asm1 = globalx1 * r;
    asm2 = globaly2 * r;
    s = (getpalookup(mulscale16(r, globvis), globalshade) << 8);

    hlineasm4(xr - xl, s, globalx2 * r + globalypanning,
              globaly1 * r + globalxpanning, ylookup[yp] + xr + frameoffset);
}


static void slowhline(int32_t xr, int32_t yp) {
    int32_t xl, r;

    xl = lastx[yp];
    if (xl > xr)
        return;
    r = horizlookup2[yp - globalhoriz + horizycent];
    asm1 = globalx1 * r;
    asm2 = globaly2 * r;

    asm3 = (int32_t) globalpalwritten + (getpalookup(mulscale16(r, globvis),
                                             globalshade) << 8);
    if (!(globalorientation & 256)) {
        mhline(globalbufplc, globaly1 * r + globalxpanning - asm1 * (xr - xl),
               (xr - xl) << 16, 0L,
               globalx2 * r + globalypanning - asm2 * (xr - xl),
               ylookup[yp] + xl + frameoffset);
        return;
    }
    thline(globalbufplc, globaly1 * r + globalxpanning - asm1 * (xr - xl),
           (xr - xl) << 16, 0L,
           globalx2 * r + globalypanning - asm2 * (xr - xl),
           ylookup[yp] + xl + frameoffset);
    transarea += (xr - xl);
}


/* renders non-parallaxed ceilings. --ryan. */
static void ceilscan(int32_t x1, int32_t x2, int32_t sectnum) {
    int32_t i, j, ox, oy, x, y1, y2, twall, bwall;
    sectortype* sec;

    sec = &sector[sectnum];

    if (palookup[sec->ceilingpal] != globalpalwritten)
        globalpalwritten = palookup[sec->ceilingpal];


    globalzd = sec->ceilingz - globalposz;


    if (globalzd > 0)
        return;


    globalpicnum = sec->ceilingpicnum;

    if ((uint32_t) globalpicnum >= (uint32_t) MAXTILES)
        globalpicnum = 0;

    setgotpic(globalpicnum);

    //Check the tile dimension are valid.
    if ((tiles[globalpicnum].dim.width <= 0) ||
        (tiles[globalpicnum].dim.height <= 0))
        return;

    if (tiles[globalpicnum].anim_flags & 192)
        globalpicnum += animateoffs(globalpicnum);

    TILE_MakeAvailable(globalpicnum);

    globalbufplc = tiles[globalpicnum].data;

    globalshade = (int32_t) sec->ceilingshade;
    globvis = globalcisibility;
    if (sec->visibility != 0)
        globvis = mulscale4(globvis, (int32_t) ((uint8_t) (sec->visibility + 16)));

    globalorientation = (int32_t) sec->ceilingstat;


    if ((globalorientation & 64) == 0) {
        globalx1 = singlobalang;
        globalx2 = singlobalang;
        globaly1 = cosglobalang;
        globaly2 = cosglobalang;
        globalxpanning = (globalposx << 20);
        globalypanning = -(globalposy << 20);
    } else {
        j = sec->wallptr;
        ox = wall[wall[j].point2].x - wall[j].x;
        oy = wall[wall[j].point2].y - wall[j].y;
        i = ksqrt(ox * ox + oy * oy);

        if (i == 0)
            i = 1024;
        else
            i = 1048576 / i;

        globalx1 = mulscale10(dmulscale10(ox, singlobalang, -oy, cosglobalang),
                              i);
        globaly1 = mulscale10(dmulscale10(ox, cosglobalang, oy, singlobalang),
                              i);
        globalx2 = -globalx1;
        globaly2 = -globaly1;

        ox = ((wall[j].x - globalposx) << 6);
        oy = ((wall[j].y - globalposy) << 6);
        i = dmulscale14(oy, cosglobalang, -ox, singlobalang);
        j = dmulscale14(ox, cosglobalang, oy, singlobalang);
        ox = i;
        oy = j;
        globalxpanning = globalx1 * ox - globaly1 * oy;
        globalypanning = globaly2 * ox + globalx2 * oy;
    }

    globalx2 = mulscale16(globalx2, viewingrangerecip);
    globaly1 = mulscale16(globaly1, viewingrangerecip);
    globalxshift = (8 - (picsiz[globalpicnum] & 15));
    globalyshift = (8 - (picsiz[globalpicnum] >> 4));
    if (globalorientation & 8) {
        globalxshift++;
        globalyshift++;
    }

    if ((globalorientation & 0x4) > 0) {
        i = globalxpanning;
        globalxpanning = globalypanning;
        globalypanning = i;
        i = globalx2;
        globalx2 = -globaly1;
        globaly1 = -i;
        i = globalx1;
        globalx1 = globaly2;
        globaly2 = i;
    }
    if ((globalorientation & 0x10) > 0) {
        globalx1 = -globalx1;
        globaly1 = -globaly1;
        globalxpanning = -globalxpanning;
    }
    if ((globalorientation & 0x20) > 0) {
        globalx2 = -globalx2;
        globaly2 = -globaly2;
        globalypanning = -globalypanning;
    }

    globalx1 <<= globalxshift;
    globaly1 <<= globalxshift;
    globalx2 <<= globalyshift;
    globaly2 <<= globalyshift;
    globalxpanning <<= globalxshift;
    globalypanning <<= globalyshift;
    globalxpanning += (((int32_t) sec->ceilingxpanning) << 24);
    globalypanning += (((int32_t) sec->ceilingypanning) << 24);
    globaly1 = (-globalx1 - globaly1) * halfxdimen;
    globalx2 = (globalx2 - globaly2) * halfxdimen;

    sethlinesizes(picsiz[globalpicnum] & 15, picsiz[globalpicnum] >> 4,
                  globalbufplc);

    globalx2 += globaly2 * (x1 - 1);
    globaly1 += globalx1 * (x1 - 1);
    globalx1 = mulscale16(globalx1, globalzd);
    globalx2 = mulscale16(globalx2, globalzd);
    globaly1 = mulscale16(globaly1, globalzd);
    globaly2 = mulscale16(globaly2, globalzd);
    globvis = klabs(mulscale10(globvis, globalzd));

    if (!(globalorientation & 0x180)) {
        y1 = umost[x1];
        y2 = y1;
        for (x = x1; x <= x2; x++) {
            twall = umost[x] - 1;
            bwall = min(uplc[x], dmost[x]);
            if (twall < bwall - 1) {
                if (twall >= y2) {
                    while (y1 < y2 - 1)
                        hline(x - 1, ++y1);
                    y1 = twall;
                } else {
                    while (y1 < twall)
                        hline(x - 1, ++y1);
                    while (y1 > twall)
                        lastx[y1--] = x;
                }
                while (y2 > bwall)
                    hline(x - 1, --y2);
                while (y2 < bwall)
                    lastx[y2++] = x;
            } else {
                while (y1 < y2 - 1)
                    hline(x - 1, ++y1);
                if (x == x2) {
                    globalx2 += globaly2;
                    globaly1 += globalx1;
                    break;
                }
                y1 = umost[x + 1];
                y2 = y1;
            }
            globalx2 += globaly2;
            globaly1 += globalx1;
        }
        while (y1 < y2 - 1)
            hline(x2, ++y1);
        faketimerhandler();
        return;
    }

    switch (globalorientation & 0x180) {
        case 128:
            msethlineshift(picsiz[globalpicnum] & 15,
                           picsiz[globalpicnum] >> 4);
            break;
        case 256:
            settrans(TRANS_NORMAL);
            tsethlineshift(picsiz[globalpicnum] & 15,
                           picsiz[globalpicnum] >> 4);
            break;
        case 384:
            settrans(TRANS_REVERSE);
            tsethlineshift(picsiz[globalpicnum] & 15,
                           picsiz[globalpicnum] >> 4);
            break;
    }

    y1 = umost[x1];
    y2 = y1;
    for (x = x1; x <= x2; x++) {
        twall = umost[x] - 1;
        bwall = min(uplc[x], dmost[x]);
        if (twall < bwall - 1) {
            if (twall >= y2) {
                while (y1 < y2 - 1)
                    slowhline(x - 1, ++y1);
                y1 = twall;
            } else {
                while (y1 < twall)
                    slowhline(x - 1, ++y1);
                while (y1 > twall)
                    lastx[y1--] = x;
            }
            while (y2 > bwall)
                slowhline(x - 1, --y2);
            while (y2 < bwall)
                lastx[y2++] = x;
        } else {
            while (y1 < y2 - 1)
                slowhline(x - 1, ++y1);
            if (x == x2) {
                globalx2 += globaly2;
                globaly1 += globalx1;
                break;
            }
            y1 = umost[x + 1];
            y2 = y1;
        }
        globalx2 += globaly2;
        globaly1 += globalx1;
    }
    while (y1 < y2 - 1)
        slowhline(x2, ++y1);
    faketimerhandler();
}


/* renders non-parallaxed floors. --ryan. */
static void florscan(int32_t x1, int32_t x2, int32_t sectnum) {
    int32_t i, j, ox, oy, x, y1, y2, twall, bwall;
    sectortype* sec;

    //Retrieve the sector object
    sec = &sector[sectnum];

    //Retrieve the floor palette.
    if (palookup[sec->floorpal] != globalpalwritten)
        globalpalwritten = palookup[sec->floorpal];

    globalzd = globalposz - sec->floorz;

    //We are UNDER the floor: Do NOT render anything.
    if (globalzd > 0)
        return;

    //Retrive the floor texture.
    globalpicnum = sec->floorpicnum;
    if ((uint32_t) globalpicnum >= (uint32_t) MAXTILES)
        globalpicnum = 0;

    //Lock the floor texture
    setgotpic(globalpicnum);


    //This tile has unvalid dimensions ( negative)
    if ((tiles[globalpicnum].dim.width <= 0) ||
        (tiles[globalpicnum].dim.height <= 0))
        return;

    //If this is an animated texture: Animate it.
    if (tiles[globalpicnum].anim_flags & 192)
        globalpicnum += animateoffs(globalpicnum);

    //If the texture is not in RAM: Load it !!
    TILE_MakeAvailable(globalpicnum);

    //Check where is the texture in RAM
    globalbufplc = tiles[globalpicnum].data;

    //Retrieve the shade of the sector (illumination level).
    globalshade = (int32_t) sec->floorshade;

    globvis = globalcisibility;
    if (sec->visibility != 0)
        globvis = mulscale4(globvis,
                            (int32_t) ((uint8_t) (sec->visibility + 16)));


    globalorientation = (int32_t) sec->floorstat;


    if ((globalorientation & 64) == 0) {
        globalx1 = singlobalang;
        globalx2 = singlobalang;
        globaly1 = cosglobalang;
        globaly2 = cosglobalang;
        globalxpanning = (globalposx << 20);
        globalypanning = -(globalposy << 20);
    } else {
        j = sec->wallptr;
        ox = wall[wall[j].point2].x - wall[j].x;
        oy = wall[wall[j].point2].y - wall[j].y;
        i = ksqrt(ox * ox + oy * oy);
        if (i == 0)
            i = 1024;
        else
            i = 1048576 / i;
        globalx1 = mulscale10(dmulscale10(ox, singlobalang, -oy, cosglobalang),
                              i);
        globaly1 = mulscale10(dmulscale10(ox, cosglobalang, oy, singlobalang),
                              i);
        globalx2 = -globalx1;
        globaly2 = -globaly1;

        ox = ((wall[j].x - globalposx) << 6);
        oy = ((wall[j].y - globalposy) << 6);
        i = dmulscale14(oy, cosglobalang, -ox, singlobalang);
        j = dmulscale14(ox, cosglobalang, oy, singlobalang);
        ox = i;
        oy = j;
        globalxpanning = globalx1 * ox - globaly1 * oy;
        globalypanning = globaly2 * ox + globalx2 * oy;
    }


    globalx2 = mulscale16(globalx2, viewingrangerecip);
    globaly1 = mulscale16(globaly1, viewingrangerecip);
    globalxshift = (8 - (picsiz[globalpicnum] & 15));
    globalyshift = (8 - (picsiz[globalpicnum] >> 4));
    if (globalorientation & 8) {
        globalxshift++;
        globalyshift++;
    }

    if ((globalorientation & 0x4) > 0) {
        i = globalxpanning;
        globalxpanning = globalypanning;
        globalypanning = i;
        i = globalx2;
        globalx2 = -globaly1;
        globaly1 = -i;
        i = globalx1;
        globalx1 = globaly2;
        globaly2 = i;
    }


    if ((globalorientation & 0x10) > 0) {
        globalx1 = -globalx1;
        globaly1 = -globaly1;
        globalxpanning = -globalxpanning;
    }

    if ((globalorientation & 0x20) > 0) {
        globalx2 = -globalx2;
        globaly2 = -globaly2;
        globalypanning = -globalypanning;
    }


    globalx1 <<= globalxshift;
    globaly1 <<= globalxshift;
    globalx2 <<= globalyshift;
    globaly2 <<= globalyshift;
    globalxpanning <<= globalxshift;
    globalypanning <<= globalyshift;
    globalxpanning += (((int32_t) sec->floorxpanning) << 24);
    globalypanning += (((int32_t) sec->floorypanning) << 24);
    globaly1 = (-globalx1 - globaly1) * halfxdimen;
    globalx2 = (globalx2 - globaly2) * halfxdimen;

    //Setup the drawing routine paramters
    sethlinesizes(picsiz[globalpicnum] & 15, picsiz[globalpicnum] >> 4,
                  globalbufplc);

    globalx2 += globaly2 * (x1 - 1);
    globaly1 += globalx1 * (x1 - 1);
    globalx1 = mulscale16(globalx1, globalzd);
    globalx2 = mulscale16(globalx2, globalzd);
    globaly1 = mulscale16(globaly1, globalzd);
    globaly2 = mulscale16(globaly2, globalzd);
    globvis = klabs(mulscale10(globvis, globalzd));

    if (!(globalorientation & 0x180)) {
        y1 = max(dplc[x1], umost[x1]);
        y2 = y1;
        for (x = x1; x <= x2; x++) {
            twall = max(dplc[x], umost[x]) - 1;
            bwall = dmost[x];
            if (twall < bwall - 1) {
                if (twall >= y2) {
                    while (y1 < y2 - 1)
                        hline(x - 1, ++y1);
                    y1 = twall;
                } else {
                    while (y1 < twall)
                        hline(x - 1, ++y1);
                    while (y1 > twall)
                        lastx[y1--] = x;
                }
                while (y2 > bwall)
                    hline(x - 1, --y2);
                while (y2 < bwall)
                    lastx[y2++] = x;
            } else {
                while (y1 < y2 - 1)
                    hline(x - 1, ++y1);
                if (x == x2) {
                    globalx2 += globaly2;
                    globaly1 += globalx1;
                    break;
                }
                y1 = max(dplc[x+1], umost[x+1]);
                y2 = y1;
            }
            globalx2 += globaly2;
            globaly1 += globalx1;
        }
        while (y1 < y2 - 1)
            hline(x2, ++y1);

        faketimerhandler();
        return;
    }

    switch (globalorientation & 0x180) {
        case 128:
            msethlineshift(picsiz[globalpicnum] & 15,
                           picsiz[globalpicnum] >> 4);
            break;
        case 256:
            settrans(TRANS_NORMAL);
            tsethlineshift(picsiz[globalpicnum] & 15,
                           picsiz[globalpicnum] >> 4);
            break;
        case 384:
            settrans(TRANS_REVERSE);
            tsethlineshift(picsiz[globalpicnum] & 15,
                           picsiz[globalpicnum] >> 4);
            break;
    }

    y1 = max(dplc[x1], umost[x1]);
    y2 = y1;
    for (x = x1; x <= x2; x++) {
        twall = max(dplc[x], umost[x]) - 1;
        bwall = dmost[x];
        if (twall < bwall - 1) {
            if (twall >= y2) {
                while (y1 < y2 - 1)
                    slowhline(x - 1, ++y1);
                y1 = twall;
            } else {
                while (y1 < twall)
                    slowhline(x - 1, ++y1);
                while (y1 > twall)
                    lastx[y1--] = x;
            }
            while (y2 > bwall)
                slowhline(x - 1, --y2);
            while (y2 < bwall)
                lastx[y2++] = x;
        } else {
            while (y1 < y2 - 1)
                slowhline(x - 1, ++y1);
            if (x == x2) {
                globalx2 += globaly2;
                globaly1 += globalx1;
                break;
            }
            y1 = max(dplc[x+1], umost[x+1]);
            y2 = y1;
        }
        globalx2 += globaly2;
        globaly1 += globalx1;
    }
    while (y1 < y2 - 1)
        slowhline(x2, ++y1);

    faketimerhandler();
}


/*
 * renders walls and parallaxed skies/floors. Look at parascan() for the
 *  higher level of parallaxing.
 *
 *    x1 == offset of leftmost pixel of wall. 0 is left of surface.
 *    x2 == offset of rightmost pixel of wall. 0 is left of surface.
 *
 *  apparently, walls are always vertical; there are sloping functions
 *   (!!!) er...elsewhere. Only the sides need be vertical, as the top and
 *   bottom of the polygon will need to be angled as the camera perspective
 *   shifts (user spins in a circle, etc.)
 *
 *  uwal is an array of the upper most pixels, and dwal are the lower most.
 *   This must be a list, as the top and bottom of the polygon are not
 *   necessarily horizontal lines.
 *
 *   So, the screen coordinate of the top left of a wall is specified by
 *   uwal[x1], the bottom left by dwal[x1], the top right by uwal[x2], and
 *   the bottom right by dwal[x2]. Every physical point on the edge of the
 *   wall in between is specified by traversing those arrays, one pixel per
 *   element.
 *
 *  --ryan.
 */
static void wallscan(int32_t x1, int32_t x2,
                     int16_t* uwal, int16_t* dwal,
                     int32_t* swal, int32_t* lwal) {
    int32_t x, xnice, ynice;
    intptr_t i;
    uint8_t* fpalookup;
    int32_t y1ve[4], y2ve[4], u4, d4, z, tileWidth, tsizy;
    uint8_t bad;

    tileWidth = tiles[globalpicnum].dim.width;
    tsizy = tiles[globalpicnum].dim.height;

    setgotpic(globalpicnum);

    if ((tileWidth <= 0) || (tsizy <= 0))
        return;

    if ((uwal[x1] > ydimen) && (uwal[x2] > ydimen))
        return;

    if ((dwal[x1] < 0) && (dwal[x2] < 0))
        return;

    TILE_MakeAvailable(globalpicnum);

    xnice = (pow2long[picsiz[globalpicnum] & 15] == tileWidth);
    if (xnice)
        tileWidth--;

    ynice = (pow2long[picsiz[globalpicnum] >> 4] == tsizy);
    if (ynice)
        tsizy = (picsiz[globalpicnum] >> 4);

    fpalookup = palookup[globalpal];

    setupvlineasm(globalshiftval);

    //Starting on the left column of the wall, check the occlusion arrays.
    x = x1;
    while ((umost[x] > dmost[x]) && (x <= x2))
        x++;

    for (; (x <= x2) && ((x + frameoffset - (uint8_t*) NULL) & 3); x++) {
        y1ve[0] = max(uwal[x], umost[x]);
        y2ve[0] = min(dwal[x], dmost[x]);
        if (y2ve[0] <= y1ve[0])
            continue;

        palookupoffse[0] = fpalookup + (getpalookup(
                                            (int32_t) mulscale16(
                                                swal[x], globvis),
                                            globalshade) << 8);

        bufplce[0] = lwal[x] + globalxpanning;

        if (bufplce[0] >= tileWidth) {
            if (xnice == 0)
                bufplce[0] %= tileWidth;
            else
                bufplce[0] &= tileWidth;
        }

        if (ynice == 0)
            bufplce[0] *= tsizy;
        else
            bufplce[0] <<= tsizy;

        vince[0] = swal[x] * globalyscale;
        vplce[0] = globalzd + vince[0] * (y1ve[0] - globalhoriz + 1);

        vlineasm1(vince[0], palookupoffse[0], y2ve[0] - y1ve[0] - 1, vplce[0],
                  bufplce[0] + tiles[globalpicnum].data,
                  x + frameoffset + ylookup[y1ve[0]]);
    }

    for (; x <= x2 - 3; x += 4) {
        bad = 0;
        for (z = 3; z >= 0; z--) {
            y1ve[z] = max(uwal[x+z], umost[x+z]);
            y2ve[z] = min(dwal[x+z], dmost[x+z]) - 1;

            if (y2ve[z] < y1ve[z]) {
                bad += pow2char[z];
                continue;
            }

            i = lwal[x + z] + globalxpanning;
            if (i >= tileWidth) {
                if (xnice == 0)
                    i %= tileWidth;
                else
                    i &= tileWidth;
            }
            if (ynice == 0)
                i *= tsizy;
            else
                i <<= tsizy;
            bufplce[z] = (intptr_t) tiles[globalpicnum].data + i;

            vince[z] = swal[x + z] * globalyscale;
            vplce[z] = globalzd + vince[z] * (y1ve[z] - globalhoriz + 1);
        }

        if (bad == 15)
            continue;

        palookupoffse[0] = fpalookup + (getpalookup(
                                            (int32_t) mulscale16(
                                                swal[x], globvis),
                                            globalshade) << 8);
        palookupoffse[3] = fpalookup + (getpalookup(
                                            (int32_t) mulscale16(
                                                swal[x + 3], globvis),
                                            globalshade) << 8);

        if ((palookupoffse[0] == palookupoffse[3]) && ((bad & 0x9) == 0)) {
            palookupoffse[1] = palookupoffse[0];
            palookupoffse[2] = palookupoffse[0];
        } else {
            palookupoffse[1] = fpalookup + (getpalookup(
                                                (int32_t) mulscale16(
                                                    swal[x + 1], globvis),
                                                globalshade) << 8);
            palookupoffse[2] = fpalookup + (getpalookup(
                                                (int32_t) mulscale16(
                                                    swal[x + 2], globvis),
                                                globalshade) << 8);
        }

        u4 = max(max(y1ve[0],y1ve[1]), max(y1ve[2],y1ve[3]));
        d4 = min(min(y2ve[0],y2ve[1]), min(y2ve[2],y2ve[3]));

        if ((bad != 0) || (u4 >= d4)) {
            if (!(bad & 1))
                prevlineasm1(vince[0], palookupoffse[0], y2ve[0] - y1ve[0],
                             vplce[0], (uint8_t*) bufplce[0],
                             ylookup[y1ve[0]] + x + frameoffset + 0);
            if (!(bad & 2))
                prevlineasm1(vince[1], palookupoffse[1], y2ve[1] - y1ve[1],
                             vplce[1], (uint8_t*) bufplce[1],
                             ylookup[y1ve[1]] + x + frameoffset + 1);
            if (!(bad & 4))
                prevlineasm1(vince[2], palookupoffse[2], y2ve[2] - y1ve[2],
                             vplce[2], (uint8_t*) bufplce[2],
                             ylookup[y1ve[2]] + x + frameoffset + 2);
            if (!(bad & 8))
                prevlineasm1(vince[3], palookupoffse[3], y2ve[3] - y1ve[3],
                             vplce[3], (uint8_t*) bufplce[3],
                             ylookup[y1ve[3]] + x + frameoffset + 3);
            continue;
        }

        if (u4 > y1ve[0])
            vplce[0] = prevlineasm1(vince[0], palookupoffse[0],
                                    u4 - y1ve[0] - 1, vplce[0],
                                    (uint8_t*) bufplce[0],
                                    ylookup[y1ve[0]] + x + frameoffset + 0);
        if (u4 > y1ve[1])
            vplce[1] = prevlineasm1(vince[1], palookupoffse[1],
                                    u4 - y1ve[1] - 1, vplce[1],
                                    (uint8_t*) bufplce[1],
                                    ylookup[y1ve[1]] + x + frameoffset + 1);
        if (u4 > y1ve[2])
            vplce[2] = prevlineasm1(vince[2], palookupoffse[2],
                                    u4 - y1ve[2] - 1, vplce[2],
                                    (uint8_t*) bufplce[2],
                                    ylookup[y1ve[2]] + x + frameoffset + 2);
        if (u4 > y1ve[3])
            vplce[3] = prevlineasm1(vince[3], palookupoffse[3],
                                    u4 - y1ve[3] - 1, vplce[3],
                                    (uint8_t*) bufplce[3],
                                    ylookup[y1ve[3]] + x + frameoffset + 3);

        if (d4 >= u4)
            vlineasm4(d4 - u4 + 1, ylookup[u4] + x + frameoffset);

        i = (intptr_t) (x + frameoffset + ylookup[d4 + 1]);

        if (y2ve[0] > d4)
            prevlineasm1(vince[0], palookupoffse[0], y2ve[0] - d4 - 1, vplce[0],
                         (uint8_t*) bufplce[0], (uint8_t*) i + 0);
        if (y2ve[1] > d4)
            prevlineasm1(vince[1], palookupoffse[1], y2ve[1] - d4 - 1, vplce[1],
                         (uint8_t*) bufplce[1], (uint8_t*) i + 1);
        if (y2ve[2] > d4)
            prevlineasm1(vince[2], palookupoffse[2], y2ve[2] - d4 - 1, vplce[2],
                         (uint8_t*) bufplce[2], (uint8_t*) i + 2);
        if (y2ve[3] > d4)
            prevlineasm1(vince[3], palookupoffse[3], y2ve[3] - d4 - 1, vplce[3],
                         (uint8_t*) bufplce[3], (uint8_t*) i + 3);
    }
    for (; x <= x2; x++) {
        y1ve[0] = max(uwal[x], umost[x]);
        y2ve[0] = min(dwal[x], dmost[x]);
        if (y2ve[0] <= y1ve[0])
            continue;

        palookupoffse[0] = fpalookup + (getpalookup(
                                            (int32_t) mulscale16(
                                                swal[x], globvis),
                                            globalshade) << 8);

        bufplce[0] = lwal[x] + globalxpanning;
        if (bufplce[0] >= tileWidth) {
            if (xnice == 0)
                bufplce[0] %= tileWidth;
            else
                bufplce[0] &= tileWidth;
        }

        if (ynice == 0)
            bufplce[0]
                *= tsizy;
        else
            bufplce[0] <<= tsizy;

        vince[0] = swal[x] * globalyscale;
        vplce[0] = globalzd + vince[0] * (y1ve[0] - globalhoriz + 1);

        vlineasm1(vince[0], palookupoffse[0], y2ve[0] - y1ve[0] - 1, vplce[0],
                  bufplce[0] + tiles[globalpicnum].data,
                  x + frameoffset + ylookup[y1ve[0]]);
    }
    faketimerhandler();
}

/* renders parallaxed skies/floors  --ryan. */
static void parascan(int32_t dax1, int32_t dax2, int32_t sectnum,
                     uint8_t dastat, int32_t bunch) {
    sectortype* sec;
    int32_t j, k, l, m, n, x, z, wallnum, nextsectnum, globalhorizbak;
    short *topptr, *botptr;

    sectnum = pv_walls[bunchfirst[bunch]].sectorId;
    sec = &sector[sectnum];

    globalhorizbak = globalhoriz;
    if (parallaxyscale != 65536)
        globalhoriz = mulscale16(globalhoriz - (ydimen >> 1), parallaxyscale) +
                      (ydimen >> 1);
    globvis = globalpisibility;
    /* globalorientation = 0L; */
    if (sec->visibility != 0)
        globvis = mulscale4(globvis,
                            (int32_t) ((uint8_t) (sec->visibility + 16)));

    if (dastat == 0) {
        globalpal = sec->ceilingpal;
        globalpicnum = sec->ceilingpicnum;
        globalshade = (int32_t) sec->ceilingshade;
        globalxpanning = (int32_t) sec->ceilingxpanning;
        globalypanning = (int32_t) sec->ceilingypanning;
        topptr = umost;
        botptr = uplc;
    } else {
        globalpal = sec->floorpal;
        globalpicnum = sec->floorpicnum;
        globalshade = (int32_t) sec->floorshade;
        globalxpanning = (int32_t) sec->floorxpanning;
        globalypanning = (int32_t) sec->floorypanning;
        topptr = dplc;
        botptr = dmost;
    }

    if ((uint32_t) globalpicnum >= (uint32_t) MAXTILES)
        globalpicnum = 0;

    if (tiles[globalpicnum].anim_flags & 192)
        globalpicnum += animateoffs(globalpicnum);

    globalshiftval = (picsiz[globalpicnum] >> 4);

    if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height)
        globalshiftval++;
    globalshiftval = 32 - globalshiftval;
    globalzd = (((tiles[globalpicnum].dim.height >> 1) + parallaxyoffs) <<
                globalshiftval) + (globalypanning << 24);
    globalyscale = (8 << (globalshiftval - 19));
    /*if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;*/

    k = 11 - (picsiz[globalpicnum] & 15) - pskybits;
    x = -1;

    for (z = bunchfirst[bunch]; z >= 0; z = bunchWallsList[z]) {
        wallnum = pv_walls[z].worldWallId;
        nextsectnum = wall[wallnum].nextsector;

        if (dastat == 0)
            j = sector[nextsectnum].ceilingstat;
        else
            j = sector[nextsectnum].floorstat;

        if ((nextsectnum < 0) || (wall[wallnum].cstat & 32) || ((j & 1) == 0)) {
            if (x == -1)
                x = pv_walls[z].screenSpaceCoo[0][VEC_COL];

            if (parallaxtype == 0) {
                n = mulscale16(xdimenrecip, viewingrange);
                for (j = pv_walls[z].screenSpaceCoo[0][VEC_COL];
                     j <= pv_walls[z].screenSpaceCoo[1][VEC_COL]; j++)
                    lplc[j] = (((mulscale23(j - halfxdimen, n) + globalang) &
                                2047) >> k);
            } else {
                for (j = pv_walls[z].screenSpaceCoo[0][VEC_COL];
                     j <= pv_walls[z].screenSpaceCoo[1][VEC_COL]; j++)
                    lplc[j] = ((((int32_t) radarang2[j] + globalang) & 2047) >>
                               k);
            }
            if (parallaxtype == 2) {
                n = mulscale16(xdimscale, viewingrange);
                for (j = pv_walls[z].screenSpaceCoo[0][VEC_COL];
                     j <= pv_walls[z].screenSpaceCoo[1][VEC_COL]; j++)
                    swplc[j] = mulscale14(
                        sintable[((int32_t) radarang2[j] + 512) & 2047], n);
            } else
                clearbuf(&swplc[pv_walls[z].screenSpaceCoo[0][VEC_COL]],
                         pv_walls[z].screenSpaceCoo[1][VEC_COL] - pv_walls[z].
                         screenSpaceCoo[0][VEC_COL] + 1,
                         mulscale16(xdimscale, viewingrange));
        } else if (x >= 0) {
            l = globalpicnum;
            m = (picsiz[globalpicnum] & 15);
            globalpicnum = l + pskyoff[lplc[x] >> m];

            if (((lplc[x] ^ lplc[pv_walls[z].screenSpaceCoo[0][VEC_COL] - 1]) >>
                 m) == 0)
                wallscan(x, pv_walls[z].screenSpaceCoo[0][VEC_COL] - 1, topptr,
                         botptr, swplc, lplc);
            else {
                j = x;
                while (x < pv_walls[z].screenSpaceCoo[0][VEC_COL]) {
                    n = l + pskyoff[lplc[x] >> m];
                    if (n != globalpicnum) {
                        wallscan(j, x - 1, topptr, botptr, swplc, lplc);
                        j = x;
                        globalpicnum = n;
                    }
                    x++;
                }
                if (j < x)
                    wallscan(j, x - 1, topptr, botptr, swplc, lplc);
            }

            globalpicnum = l;
            x = -1;
        }
    }

    if (x >= 0) {
        l = globalpicnum;
        m = (picsiz[globalpicnum] & 15);
        globalpicnum = l + pskyoff[lplc[x] >> m];

        if (((lplc[x] ^ lplc[pv_walls[bunchlast[bunch]].screenSpaceCoo[1][
                  VEC_COL]]) >> m) == 0)
            wallscan(x, pv_walls[bunchlast[bunch]].screenSpaceCoo[1][VEC_COL],
                     topptr, botptr, swplc, lplc);
        else {
            j = x;
            while (x <= pv_walls[bunchlast[bunch]].screenSpaceCoo[1][VEC_COL]) {
                n = l + pskyoff[lplc[x] >> m];
                if (n != globalpicnum) {
                    wallscan(j, x - 1, topptr, botptr, swplc, lplc);
                    j = x;
                    globalpicnum = n;
                }
                x++;
            }
            if (j <= x)
                wallscan(j, x, topptr, botptr, swplc, lplc);
        }
        globalpicnum = l;
    }
    globalhoriz = globalhorizbak;
}


static void grouscan(int32_t dax1, int32_t dax2, int32_t sectnum,
                     uint8_t dastat) {
    int32_t i, l, x, y, dx, dy, wx, wy, y1, y2, daz;
    int32_t daslope, dasqr;
    intptr_t j, shoffs, shinc, m1, m2, *mptr1, *mptr2, *nptr1, *nptr2;
    walltype* wal;
    sectortype* sec;

    sec = &sector[sectnum];

    if (dastat == 0) {
        if (globalposz <= getceilzofslope((short) sectnum, globalposx,
                                          globalposy))
            return; /* Back-face culling */
        globalorientation = sec->ceilingstat;
        globalpicnum = sec->ceilingpicnum;
        globalshade = sec->ceilingshade;
        globalpal = sec->ceilingpal;
        daslope = sec->ceilingheinum;
        daz = sec->ceilingz;
    } else {
        if (globalposz >= getflorzofslope((short) sectnum, globalposx,
                                          globalposy))
            return; /* Back-face culling */
        globalorientation = sec->floorstat;
        globalpicnum = sec->floorpicnum;
        globalshade = sec->floorshade;
        globalpal = sec->floorpal;
        daslope = sec->floorheinum;
        daz = sec->floorz;
    }

    if ((tiles[globalpicnum].anim_flags & 192) != 0)
        globalpicnum += animateoffs(globalpicnum);

    setgotpic(globalpicnum);

    if ((tiles[globalpicnum].dim.width <= 0) ||
        (tiles[globalpicnum].dim.height <= 0))
        return;

    TILE_MakeAvailable(globalpicnum);

    wal = &wall[sec->wallptr];
    wx = wall[wal->point2].x - wal->x;
    wy = wall[wal->point2].y - wal->y;
    dasqr = krecipasm(ksqrt(wx * wx + wy * wy));
    i = mulscale21(daslope, dasqr);
    wx *= i;
    wy *= i;

    globalx = -mulscale19(singlobalang, xdimenrecip);
    globaly = mulscale19(cosglobalang, xdimenrecip);
    globalx1 = (globalposx << 8);
    globaly1 = -(globalposy << 8);
    i = (dax1 - halfxdimen) * xdimenrecip;
    globalx2 = mulscale16(cosglobalang << 4, viewingrangerecip) - mulscale27(
                   singlobalang, i);
    globaly2 = mulscale16(singlobalang << 4, viewingrangerecip) + mulscale27(
                   cosglobalang, i);
    globalzd = (xdimscale << 9);
    globalzx = -dmulscale17(wx, globaly2, -wy, globalx2) + mulscale10(
                   1 - globalhoriz, globalzd);
    globalz = -dmulscale25(wx, globaly, -wy, globalx);

    if (globalorientation & 64) /* Relative alignment */
    {
        dx = mulscale14(wall[wal->point2].x - wal->x, dasqr);
        dy = mulscale14(wall[wal->point2].y - wal->y, dasqr);

        i = ksqrt(daslope * daslope + 16777216);

        x = globalx;
        y = globaly;
        globalx = dmulscale16(x, dx, y, dy);
        globaly = mulscale12(dmulscale16(-y, dx, x, dy), i);

        x = ((wal->x - globalposx) << 8);
        y = ((wal->y - globalposy) << 8);
        globalx1 = dmulscale16(-x, dx, -y, dy);
        globaly1 = mulscale12(dmulscale16(-y, dx, x, dy), i);

        x = globalx2;
        y = globaly2;
        globalx2 = dmulscale16(x, dx, y, dy);
        globaly2 = mulscale12(dmulscale16(-y, dx, x, dy), i);
    }
    if (globalorientation & 0x4) {
        i = globalx;
        globalx = -globaly;
        globaly = -i;
        i = globalx1;
        globalx1 = globaly1;
        globaly1 = i;
        i = globalx2;
        globalx2 = -globaly2;
        globaly2 = -i;
    }
    if (globalorientation & 0x10) {
        globalx1 = -globalx1, globalx2 = -globalx2, globalx = -globalx;
    }
    if (globalorientation & 0x20) {
        globaly1 = -globaly1, globaly2 = -globaly2, globaly = -globaly;
    }

    daz = dmulscale9(wx, globalposy - wal->y, -wy, globalposx - wal->x) + (
              (daz - globalposz) << 8);
    globalx2 = mulscale20(globalx2, daz);
    globalx = mulscale28(globalx, daz);
    globaly2 = mulscale20(globaly2, -daz);
    globaly = mulscale28(globaly, -daz);

    i = 8 - (picsiz[globalpicnum] & 15);
    j = 8 - (picsiz[globalpicnum] >> 4);
    if (globalorientation & 8) {
        i++;
        j++;
    }
    globalx1 <<= (i + 12);
    globalx2 <<= i;
    globalx <<= i;
    globaly1 <<= (j + 12);
    globaly2 <<= j;
    globaly <<= j;

    if (dastat == 0) {
        globalx1 += (((int32_t) sec->ceilingxpanning) << 24);
        globaly1 += (((int32_t) sec->ceilingypanning) << 24);
    } else {
        globalx1 += (((int32_t) sec->floorxpanning) << 24);
        globaly1 += (((int32_t) sec->floorypanning) << 24);
    }

    asm1 = -(globalzd >> (16 - BITSOFPRECISION));

    globvis = globalvisibility;
    if (sec->visibility != 0)
        globvis = mulscale4(globvis,
                            (int32_t) ((uint8_t) (sec->visibility + 16)));
    globvis = mulscale13(globvis, daz);
    globvis = mulscale16(globvis, xdimscale);
    j = (intptr_t) palookup[globalpal];

    setupslopevlin(
        ((int32_t) (picsiz[globalpicnum] & 15)) + (
            ((int32_t) (picsiz[globalpicnum] >> 4)) << 8),
        (intptr_t) (tiles[globalpicnum].data), -ylookup[1]);

    l = (globalzd >> 16);

    shinc = mulscale16(globalz, xdimenscale);
    if (shinc > 0)
        shoffs = (4 << 15);
    else
        shoffs = ((16380 - ydimen) << 15); // JBF: was 2044     16380
    if (dastat == 0)
        y1 = umost[dax1];
    else
        y1 = max(umost[dax1], dplc[dax1]);
    m1 = mulscale16(y1, globalzd) + (globalzx >> 6);
    /* Avoid visibility overflow by crossing horizon */
    if (globalzd > 0)
        m1 += (globalzd >> 16);
    else
        m1 -= (globalzd >> 16);
    m2 = m1 + l;
    mptr1 = (intptr_t*) &slopalookup[y1 + (shoffs >> 15)];
    mptr2 = mptr1 + 1;

    for (x = dax1; x <= dax2; x++) {
        if (dastat == 0) {
            y1 = umost[x];
            y2 = min(dmost[x], uplc[x]) - 1;
        } else {
            y1 = max(umost[x], dplc[x]);
            y2 = dmost[x] - 1;
        }
        if (y1 <= y2) {
            nptr1 = (intptr_t*) &slopalookup[y1 + (shoffs >> 15)];
            nptr2 = (intptr_t*) &slopalookup[y2 + (shoffs >> 15)];
            while (nptr1 <= mptr1) {
                *mptr1-- = j + (getpalookup(
                                    (int32_t)
                                    mulscale24(krecipasm(m1), globvis),
                                    globalshade) << 8);
                m1 -= l;
            }
            while (nptr2 >= mptr2) {
                *mptr2++ = j + (getpalookup(
                                    (int32_t)
                                    mulscale24(krecipasm(m2), globvis),
                                    globalshade) << 8);
                m2 += l;
            }

            globalx3 = (globalx2 >> 10);
            globaly3 = (globaly2 >> 10);
            asm3 = mulscale16(y2, globalzd) + (globalzx >> 6);
            slopevlin((intptr_t) (ylookup[y2] + x + frameoffset),
                      krecipasm(asm3 >> 3), slopalookup, y2 + (shoffs >> 15),
                      y2 - y1 + 1, globalx1, globaly1);

            if ((x & 15) == 0)
                faketimerhandler();
        }
        globalx2 += globalx;
        globaly2 += globaly;
        globalzx += globalz;
        shoffs += shinc;
    }
}


void drawalls(int32_t bunch) {
    sectortype *sec, *nextsec;
    walltype* wal;
    int32_t i, x, x1, x2, cz[5], fz[5];
    int32_t z, wallnum, sectnum, nextsectnum;
    int32_t startsmostwallcnt, startsmostcnt, gotswall;
    uint8_t andwstat1, andwstat2;

    z = bunchfirst[bunch];
    sectnum = pv_walls[z].sectorId;
    sec = &sector[sectnum];

    andwstat1 = 0xff;
    andwstat2 = 0xff;
    for (; z >= 0; z = bunchWallsList[z]) {
        /* uplc/dplc calculation */

        andwstat1 &= wallmost(uplc, z, sectnum, (uint8_t) 0);
        andwstat2 &= wallmost(dplc, z, sectnum, (uint8_t) 1);
    }

    /* draw ceilings */
    if ((andwstat1 & 3) != 3) {
        if ((sec->ceilingstat & 3) == 2)
            grouscan(pv_walls[bunchfirst[bunch]].screenSpaceCoo[0][VEC_COL],
                     pv_walls[bunchlast[bunch]].screenSpaceCoo[1][VEC_COL],
                     sectnum, 0);
        else if ((sec->ceilingstat & 1) == 0)
            ceilscan(pv_walls[bunchfirst[bunch]].screenSpaceCoo[0][VEC_COL],
                     pv_walls[bunchlast[bunch]].screenSpaceCoo[1][VEC_COL],
                     sectnum);
        else
            parascan(pv_walls[bunchfirst[bunch]].screenSpaceCoo[0][VEC_COL],
                     pv_walls[bunchlast[bunch]].screenSpaceCoo[1][VEC_COL],
                     sectnum, 0, bunch);
    }

    /* draw floors */
    if ((andwstat2 & 12) != 12) {
        if ((sec->floorstat & 3) == 2)
            grouscan(pv_walls[bunchfirst[bunch]].screenSpaceCoo[0][VEC_COL],
                     pv_walls[bunchlast[bunch]].screenSpaceCoo[1][VEC_COL],
                     sectnum, 1);
        else if ((sec->floorstat & 1) == 0)
            florscan(pv_walls[bunchfirst[bunch]].screenSpaceCoo[0][VEC_COL],
                     pv_walls[bunchlast[bunch]].screenSpaceCoo[1][VEC_COL],
                     sectnum);
        else
            parascan(pv_walls[bunchfirst[bunch]].screenSpaceCoo[0][VEC_COL],
                     pv_walls[bunchlast[bunch]].screenSpaceCoo[1][VEC_COL],
                     sectnum, 1, bunch);
    }

    /* DRAW WALLS SECTION! */
    for (z = bunchfirst[bunch]; z >= 0; z = bunchWallsList[z]) {

        x1 = pv_walls[z].screenSpaceCoo[0][VEC_COL];
        x2 = pv_walls[z].screenSpaceCoo[1][VEC_COL];
        if (umost[x2] >= dmost[x2]) {

            for (x = x1; x < x2; x++)
                if (umost[x] < dmost[x])
                    break;

            if (x >= x2) {
                smostwall[smostwallcnt] = z;
                smostwalltype[smostwallcnt] = 0;
                smostwallcnt++;
                continue;
            }
        }

        wallnum = pv_walls[z].worldWallId;
        wal = &wall[wallnum];
        nextsectnum = wal->nextsector;
        nextsec = &sector[nextsectnum];

        gotswall = 0;

        startsmostwallcnt = smostwallcnt;
        startsmostcnt = smostcnt;

        if ((searchit == 2) && (searchx >= x1) && (searchx <= x2)) {
            if (searchy <= uplc[searchx]) {
                /* ceiling */
                searchsector = sectnum;
                searchwall = wallnum;
                searchstat = 1;
                searchit = 1;
            } else if (searchy >= dplc[searchx]) {
                /* floor */
                searchsector = sectnum;
                searchwall = wallnum;
                searchstat = 2;
                searchit = 1;
            }
        }

        if (nextsectnum >= 0) {
            getzsofslope((short) sectnum, wal->x, wal->y, &cz[0], &fz[0]);
            getzsofslope((short) sectnum, wall[wal->point2].x,
                         wall[wal->point2].y, &cz[1], &fz[1]);
            getzsofslope((short) nextsectnum, wal->x, wal->y, &cz[2], &fz[2]);
            getzsofslope((short) nextsectnum, wall[wal->point2].x,
                         wall[wal->point2].y, &cz[3], &fz[3]);
            getzsofslope((short) nextsectnum, globalposx, globalposy, &cz[4],
                         &fz[4]);

            if ((wal->cstat & 48) == 16)
                maskwall[maskwallcnt++] = z;

            if (((sec->ceilingstat & 1) == 0) || (
                    (nextsec->ceilingstat & 1) == 0)) {
                if ((cz[2] <= cz[0]) && (cz[3] <= cz[1])) {
                    if (globparaceilclip)
                        for (x = x1; x <= x2; x++)
                            if (uplc[x] > umost[x])
                                if (umost[x] <= dmost[x]) {
                                    umost[x] = uplc[x];
                                    if (umost[x] > dmost[x])
                                        numhits--;
                                }
                } else {
                    wallmost(dwall, z, nextsectnum, (uint8_t) 0);
                    if ((cz[2] > fz[0]) || (cz[3] > fz[1]))
                        for (i = x1; i <= x2; i++)
                            if (dwall[i] > dplc[i])
                                dwall[i] = dplc[i];

                    if ((searchit == 2) && (searchx >= x1) && (searchx <= x2))
                        if (searchy <= dwall[searchx]) /* wall */ {
                            searchsector = sectnum;
                            searchwall = wallnum;
                            searchstat = 0;
                            searchit = 1;
                        }

                    globalorientation = (int32_t) wal->cstat;
                    globalpicnum = wal->picnum;
                    if ((uint32_t) globalpicnum >= (uint32_t) MAXTILES)
                        globalpicnum = 0;
                    globalxpanning = (int32_t) wal->xpanning;
                    globalypanning = (int32_t) wal->ypanning;
                    globalshiftval = (picsiz[globalpicnum] >> 4);
                    if (pow2long[globalshiftval] != tiles[globalpicnum].dim.
                        height)
                        globalshiftval++;
                    globalshiftval = 32 - globalshiftval;

                    //Animated
                    if (tiles[globalpicnum].anim_flags & 192)
                        globalpicnum += animateoffs(globalpicnum);

                    globalshade = (int32_t) wal->shade;
                    globvis = globalvisibility;
                    if (sec->visibility != 0)
                        globvis = mulscale4(globvis,
                                            (int32_t) ((uint8_t) (
                                                sec->visibility + 16)));
                    globalpal = (int32_t) wal->pal;
                    globalyscale = (wal->yrepeat << (globalshiftval - 19));
                    if ((globalorientation & 4) == 0)
                        globalzd = (
                            ((globalposz - nextsec->ceilingz) * globalyscale) <<
                            8);
                    else
                        globalzd = (
                            ((globalposz - sec->ceilingz) * globalyscale) << 8);
                    globalzd += (globalypanning << 24);
                    if (globalorientation & 256)
                        globalyscale = -globalyscale, globalzd = -globalzd;

                    if (gotswall == 0) {
                        gotswall = 1;
                        prepwall(z, wal);
                    }
                    wallscan(x1, x2, uplc, dwall, swall, lwall);

                    if ((cz[2] >= cz[0]) && (cz[3] >= cz[1])) {
                        for (x = x1; x <= x2; x++)
                            if (dwall[x] > umost[x])
                                if (umost[x] <= dmost[x]) {
                                    umost[x] = dwall[x];
                                    if (umost[x] > dmost[x])
                                        numhits--;
                                }
                    } else {
                        for (x = x1; x <= x2; x++)
                            if (umost[x] <= dmost[x]) {
                                i = max(uplc[x], dwall[x]);
                                if (i > umost[x]) {
                                    umost[x] = i;
                                    if (umost[x] > dmost[x])
                                        numhits--;
                                }
                            }
                    }
                }
                if ((cz[2] < cz[0]) || (cz[3] < cz[1]) || (
                        globalposz < cz[4])) {
                    i = x2 - x1 + 1;
                    if (smostcnt + i < MAXYSAVES) {
                        smoststart[smostwallcnt] = smostcnt;
                        smostwall[smostwallcnt] = z;
                        smostwalltype[smostwallcnt] = 1; /* 1 for umost */
                        smostwallcnt++;
                        copybufbyte((int32_t*) &umost[x1],
                                    (int32_t*) &smost[smostcnt],
                                    i * sizeof(smost[0]));
                        smostcnt += i;
                    }
                }
            }
            if (((sec->floorstat & 1) == 0) || (
                    (nextsec->floorstat & 1) == 0)) {
                if ((fz[2] >= fz[0]) && (fz[3] >= fz[1])) {
                    if (globparaflorclip)
                        for (x = x1; x <= x2; x++)
                            if (dplc[x] < dmost[x])
                                if (umost[x] <= dmost[x]) {
                                    dmost[x] = dplc[x];
                                    if (umost[x] > dmost[x])
                                        numhits--;
                                }
                } else {
                    wallmost(uwall, z, nextsectnum, (uint8_t) 1);
                    if ((fz[2] < cz[0]) || (fz[3] < cz[1]))
                        for (i = x1; i <= x2; i++)
                            if (uwall[i] < uplc[i])
                                uwall[i] = uplc[i];

                    if ((searchit == 2) && (searchx >= x1) && (searchx <= x2))
                        if (searchy >= uwall[searchx]) /* wall */ {
                            searchsector = sectnum;
                            searchwall = wallnum;
                            if ((wal->cstat & 2) > 0)
                                searchwall = wal->nextwall;
                            searchstat = 0;
                            searchit = 1;
                        }

                    if ((wal->cstat & 2) > 0) {
                        wallnum = wal->nextwall;
                        wal = &wall[wallnum];
                        globalorientation = (int32_t) wal->cstat;
                        globalpicnum = wal->picnum;
                        if ((uint32_t) globalpicnum >= (uint32_t) MAXTILES)
                            globalpicnum = 0;
                        globalxpanning = (int32_t) wal->xpanning;
                        globalypanning = (int32_t) wal->ypanning;

                        if (tiles[globalpicnum].anim_flags & 192)
                            globalpicnum += animateoffs(globalpicnum);

                        globalshade = (int32_t) wal->shade;
                        globalpal = (int32_t) wal->pal;
                        wallnum = pv_walls[z].worldWallId;
                        wal = &wall[wallnum];
                    } else {
                        globalorientation = (int32_t) wal->cstat;
                        globalpicnum = wal->picnum;

                        if ((uint32_t) globalpicnum >= (uint32_t) MAXTILES)
                            globalpicnum = 0;

                        globalxpanning = (int32_t) wal->xpanning;
                        globalypanning = (int32_t) wal->ypanning;

                        if (tiles[globalpicnum].anim_flags & 192)
                            globalpicnum += animateoffs(globalpicnum);
                        globalshade = (int32_t) wal->shade;
                        globalpal = (int32_t) wal->pal;
                    }
                    globvis = globalvisibility;
                    if (sec->visibility != 0)
                        globvis = mulscale4(globvis,
                                            (int32_t) ((uint8_t) (
                                                sec->visibility + 16)));
                    globalshiftval = (picsiz[globalpicnum] >> 4);

                    if (pow2long[globalshiftval] != tiles[globalpicnum].dim.
                        height)
                        globalshiftval++;

                    globalshiftval = 32 - globalshiftval;
                    globalyscale = (wal->yrepeat << (globalshiftval - 19));

                    if ((globalorientation & 4) == 0)
                        globalzd = (
                            ((globalposz - nextsec->floorz) * globalyscale) <<
                            8);
                    else
                        globalzd = (
                            ((globalposz - sec->ceilingz) * globalyscale) << 8);

                    globalzd += (globalypanning << 24);
                    if (globalorientation & 256)
                        globalyscale = -globalyscale, globalzd = -globalzd;

                    if (gotswall == 0) {
                        gotswall = 1;
                        prepwall(z, wal);
                    }
                    wallscan(x1, x2, uwall, dplc, swall, lwall);

                    if ((fz[2] <= fz[0]) && (fz[3] <= fz[1])) {
                        for (x = x1; x <= x2; x++)
                            if (uwall[x] < dmost[x])
                                if (umost[x] <= dmost[x]) {
                                    dmost[x] = uwall[x];
                                    if (umost[x] > dmost[x])
                                        numhits--;
                                }
                    } else {
                        for (x = x1; x <= x2; x++)
                            if (umost[x] <= dmost[x]) {
                                i = min(dplc[x], uwall[x]);
                                if (i < dmost[x]) {
                                    dmost[x] = i;
                                    if (umost[x] > dmost[x])
                                        numhits--;
                                }
                            }
                    }
                }
                if ((fz[2] > fz[0]) || (fz[3] > fz[1]) || (
                        globalposz > fz[4])) {
                    i = x2 - x1 + 1;
                    if (smostcnt + i < MAXYSAVES) {
                        smoststart[smostwallcnt] = smostcnt;
                        smostwall[smostwallcnt] = z;
                        smostwalltype[smostwallcnt] = 2; /* 2 for dmost */
                        smostwallcnt++;
                        copybufbyte((int32_t*) &dmost[x1],
                                    (int32_t*) &smost[smostcnt],
                                    i * sizeof(smost[0]));
                        smostcnt += i;
                    }
                }
            }
            if (numhits < 0)
                return;
            if ((!(wal->cstat & 32)) && (
                    (visitedSectors[nextsectnum >> 3] & pow2char[
                         nextsectnum & 7]) == 0)) {
                if (umost[x2] < dmost[x2])
                    scansector((short) nextsectnum);
                else {
                    for (x = x1; x < x2; x++)
                        if (umost[x] < dmost[x]) {
                            scansector((short) nextsectnum);
                            break;
                        }

                    /*
                     * If can't see sector beyond, then cancel smost array and just
                     *  store wall!
                     */
                    if (x == x2) {
                        smostwallcnt = startsmostwallcnt;
                        smostcnt = startsmostcnt;
                        smostwall[smostwallcnt] = z;
                        smostwalltype[smostwallcnt] = 0;
                        smostwallcnt++;
                    }
                }
            }
        }
        if ((nextsectnum < 0) || (wal->cstat & 32)) /* White/1-way wall */
        {
            globalorientation = (int32_t) wal->cstat;
            if (nextsectnum < 0)
                globalpicnum = wal->picnum;
            else
                globalpicnum = wal->overpicnum;

            if ((uint32_t) globalpicnum >= (uint32_t) MAXTILES)
                globalpicnum = 0;

            globalxpanning = (int32_t) wal->xpanning;
            globalypanning = (int32_t) wal->ypanning;

            if (tiles[globalpicnum].anim_flags & 192)
                globalpicnum += animateoffs(globalpicnum);

            globalshade = (int32_t) wal->shade;
            globvis = globalvisibility;
            if (sec->visibility != 0)
                globvis = mulscale4(globvis,
                                    (int32_t) ((uint8_t) (
                                        sec->visibility + 16)));

            globalpal = (int32_t) wal->pal;
            globalshiftval = (picsiz[globalpicnum] >> 4);
            if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height)
                globalshiftval++;

            globalshiftval = 32 - globalshiftval;
            globalyscale = (wal->yrepeat << (globalshiftval - 19));
            if (nextsectnum >= 0) {
                if ((globalorientation & 4) == 0)
                    globalzd = globalposz - nextsec->ceilingz;
                else
                    globalzd = globalposz - sec->ceilingz;
            } else {
                if ((globalorientation & 4) == 0)
                    globalzd = globalposz - sec->ceilingz;
                else
                    globalzd = globalposz - sec->floorz;
            }
            globalzd = ((globalzd * globalyscale) << 8) + (
                           globalypanning << 24);

            if (globalorientation & 256) {
                globalyscale = -globalyscale;
                globalzd = -globalzd;
            }

            if (gotswall == 0) {
                gotswall = 1;
                prepwall(z, wal);
            }

            wallscan(x1, x2, uplc, dplc, swall, lwall);

            for (x = x1; x <= x2; x++)
                if (umost[x] <= dmost[x]) {
                    umost[x] = 1;
                    dmost[x] = 0;
                    numhits--;
                }
            smostwall[smostwallcnt] = z;
            smostwalltype[smostwallcnt] = 0;
            smostwallcnt++;

            if ((searchit == 2) && (searchx >= x1) && (searchx <= x2)) {
                searchit = 1;
                searchsector = sectnum;
                searchwall = wallnum;
                if (nextsectnum < 0)
                    searchstat = 0;
                else
                    searchstat = 4;
            }
        }
    }
}
