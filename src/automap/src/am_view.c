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
// automap drawing functions

#include "am_view.h"
#include "build/build.h"
#include "build/engine.h"
#include "renderer/draw.h"
#include "video/display.h"


// Warning: This depends on MAXYSAVES & MAXYDIM!
#define MAXNODESPERLINE 42


typedef struct {
    sectortype* sec;
    i32 x;
    i32 y;
    i32 num_points;
    i32 clip_mask;
    i32 bakx1;
    i32 baky1;
} polygon_t;


extern i32 beforedrawrooms;
extern i32 xyaspect;
extern i32 viewingrangerecip;

extern i32 transarea;


static short* dotp1[MAXYDIM];
static short* dotp2[MAXYDIM];

static i32 bakgxvect;
static i32 bakgyvect;
static i32 xvect;
static i32 yvect;
static i32 xvect2;
static i32 yvect2;


//
// Finds minimum and maximum Y of all polygon vertices.
//
static void AM_GetPolyBounds(i32 npoints, i32* miny, i32* maxy) {
    i32 min = 0x7fffffff;
    i32 max = 0x80000000;
    for (i32 z = npoints - 1; z >= 0; z--) {
        i32 y = pv_walls[z].cameraSpaceCoo[0][VEC_Y];
        min = SDL_min(min, y);
        max = SDL_max(max, y);
    }
    min = (min >> 12);
    max = (max >> 12);
    if (min < 0) {
        min = 0;
    }
    if (max >= ydim) {
        max = ydim - 1;
    }
    *miny = min;
    *maxy = max;
}

static void AM_SetUpAET(i32 miny, i32 maxy) {
    // They're pointers! - watch how you optimize this thing
    short* ptr = smost;
    for (i32 y = miny; y <= maxy; y++) {
        dotp1[y] = ptr;
        dotp2[y] = ptr + (MAXNODESPERLINE >> 1);
        ptr += MAXNODESPERLINE;
    }
}

static void AM_FillPoly(i32 npoints) {
    i32 miny;
    i32 maxy;
    AM_GetPolyBounds(npoints, &miny, &maxy);

    AM_SetUpAET(miny, maxy);

    for (i32 z = npoints - 1; z >= 0; z--) {
        i32 zz = pv_walls[z].screenSpaceCoo[0][VEC_COL];
        i32 y1 = pv_walls[z].cameraSpaceCoo[0][VEC_Y];
        i32 day1 = (y1 >> 12);
        i32 y2 = pv_walls[zz].cameraSpaceCoo[0][VEC_Y];
        i32 day2 = (y2 >> 12);
        if (day1 == day2) {
            // Horizontal edge.
            continue;
        }
        // Compute X slope.
        i32 x1 = pv_walls[z].cameraSpaceCoo[0][VEC_X];
        i32 x2 = pv_walls[zz].cameraSpaceCoo[0][VEC_X];
        i32 xinc = divscale12(x2 - x1, y2 - y1);
        if (day1 < day2) {
            x1 += mulscale12((day1 << 12) + 4095 - y1, xinc);
            for (i32 y = day1; y < day2; y++) {
                *dotp2[y]++ = (x1 >> 12);
                x1 += xinc;
            }
        } else {
            x2 += mulscale12((day2 << 12) + 4095 - y2, xinc);
            for (i32 y = day2; y < day1; y++) {
                *dotp1[y]++ = (x2 >> 12);
                x2 += xinc;
            }
        }
    }

    globalx1 = mulscale16(globalx1, xyaspect);
    globaly2 = mulscale16(globaly2, xyaspect);

    i32 oy = miny + 1 - (ydim >> 1);
    globalposx += oy * globalx1;
    globalposy += oy * globaly2;


    short* ptr = smost;
    for (i32 y = miny; y <= maxy; y++) {
        i32 cnt = dotp1[y] - ptr;
        short* ptr2 = ptr + (MAXNODESPERLINE >> 1);
        for (i32 z = cnt - 1; z >= 0; z--) {
            i32 day1 = 0;
            i32 day2 = 0;
            for (i32 zz = z; zz > 0; zz--) {
                if (ptr[zz] < ptr[day1])
                    day1 = zz;
                if (ptr2[zz] < ptr2[day2])
                    day2 = zz;
            }
            i32 x1 = ptr[day1];
            ptr[day1] = ptr[z];
            i32 x2 = ptr2[day2] - 1;
            ptr2[day2] = ptr2[z];
            if (x1 > x2)
                continue;

            if (globalpolytype < 1) {
                /* maphline */
                i32 ox = x2 + 1 - (xdim >> 1);
                i32 bx = ox * asm1 + globalposx;
                i32 by = ox * asm2 - globalposy;

                u8* p = ylookup[y] + x2 + frameplace;
                hlineasm4(x2 - x1, globalshade << 8, by, bx, p);
            } else {
                /* maphline */
                i32 ox = x1 + 1 - (xdim >> 1);
                i32 bx = ox * asm1 + globalposx;
                i32 by = ox * asm2 - globalposy;

                u8* p = ylookup[y] + x1 + frameplace;
                if (globalpolytype == 1)
                    mhline(globalbufplc, bx, (x2 - x1) << 16, 0L, by, p);
                else {
                    thline(globalbufplc, bx, (x2 - x1) << 16, 0L, by, p);
                    transarea += (x2 - x1);
                }
            }
        }
        globalposx += globalx1;
        globalposy += globaly2;
        ptr += MAXNODESPERLINE;
    }
    faketimerhandler();
}


static bool AM_SetPic(i16 picnum) {
    globalpicnum = picnum;
    if ((u32) globalpicnum >= (u32) MAXTILES) {
        globalpicnum = 0;
    }
    setgotpic(globalpicnum);

    const tile_t* tile = &tiles[globalpicnum];
    if (tile->dim.width <= 0 || tile->dim.height <= 0) {
        return false;
    }
    if ((tile->anim_flags & 192) != 0) {
        globalpicnum += animateoffs(globalpicnum);
    }
    TILE_MakeAvailable(globalpicnum);
    globalbufplc = tile->data;

    return true;
}

/*
================================================================================

POLYGON CLIPPING

================================================================================
*/

static i32 AM_GetPointClipMask(i32 x, i32 y) {
    i32 res = 0;
    if (y >= wy2) {
        // Need to clip bottom.
        res |= 1;
    }
    if (y < wy1) {
        // Need to clip top.
        res |= 2;
    }
    if (x >= wx2) {
        // Need to clip right.
        res |= 4;
    }
    if (x < wx1) {
        // Need to clip left.
        res |= 8;
    }
    res |= ((res << 4) ^ 0xf0);
    return res;
}

static int AM_ClipPoly(i32 npoints, i32 clipstat) {
    i32 z, zz, s1, s2, t, npoints2, start2, z1, z2, z3, z4, splitcnt;

    i32 cx1 = (windowx1 << 12);
    i32 cy1 = (windowy1 << 12);
    i32 cx2 = ((windowx2 + 1) << 12);
    i32 cy2 = ((windowy2 + 1) << 12);

    if (clipstat & 0xa) {
        /* Need to clip top or left */
        npoints2 = 0;
        start2 = 0;
        z = 0;
        splitcnt = 0;
        do {
            s2 = cx1 - pv_walls[z].cameraSpaceCoo[0][VEC_X];
            do {
                zz = pv_walls[z].screenSpaceCoo[0][VEC_COL];
                pv_walls[z].screenSpaceCoo[0][VEC_COL] = -1;
                s1 = s2;
                s2 = cx1 - pv_walls[zz].cameraSpaceCoo[0][VEC_X];

                if (s1 < 0) {
                    pv_walls[npoints2].cameraSpaceCoo[1][VEC_X] = pv_walls[z].
                        cameraSpaceCoo[0][VEC_X];
                    pv_walls[npoints2].cameraSpaceCoo[1][VEC_Y] = pv_walls[z].
                        cameraSpaceCoo[0][VEC_Y];
                    pv_walls[npoints2].screenSpaceCoo[1][VEC_COL] = npoints2 + 1;
                    npoints2++;
                }

                if ((s1 ^ s2) < 0) {
                    pv_walls[npoints2].cameraSpaceCoo[1][VEC_X] =
                        pv_walls[z].cameraSpaceCoo[0][VEC_X] + scale(
                            pv_walls[zz].cameraSpaceCoo[0][VEC_X] - pv_walls[z].
                            cameraSpaceCoo[0][VEC_X], s1, s1 - s2);
                    pv_walls[npoints2].cameraSpaceCoo[1][VEC_Y] =
                        pv_walls[z].cameraSpaceCoo[0][VEC_Y] + scale(
                            pv_walls[zz].cameraSpaceCoo[0][VEC_Y] - pv_walls[z].
                            cameraSpaceCoo[0][VEC_Y], s1, s1 - s2);

                    if (s1 < 0)
                        bunchWallsList[splitcnt++] = npoints2;

                    pv_walls[npoints2].screenSpaceCoo[1][VEC_COL] = npoints2 + 1;
                    npoints2++;
                }
                z = zz;
            } while (pv_walls[z].screenSpaceCoo[0][VEC_COL] >= 0);

            if (npoints2 >= start2 + 3)
                pv_walls[npoints2 - 1].screenSpaceCoo[1][VEC_COL] = start2,
                    start2 = npoints2;
            else
                npoints2 = start2;

            z = 1;
            while ((z < npoints) && (pv_walls[z].screenSpaceCoo[0][VEC_COL] < 0))
                z++;
        } while (z < npoints);
        if (npoints2 <= 2)
            return (0);

        for (z = 1; z < splitcnt; z++)
            for (zz = 0; zz < z; zz++) {
                z1 = bunchWallsList[z];
                z2 = pv_walls[z1].screenSpaceCoo[1][VEC_COL];
                z3 = bunchWallsList[zz];
                z4 = pv_walls[z3].screenSpaceCoo[1][VEC_COL];
                s1 = klabs(pv_walls[z1].cameraSpaceCoo[1][VEC_X] - pv_walls[z2].
                           cameraSpaceCoo[1][VEC_X]) + klabs(
                         pv_walls[z1].cameraSpaceCoo[1][VEC_Y] - pv_walls[z2].
                         cameraSpaceCoo[1][VEC_Y]);
                s1 += klabs(
                    pv_walls[z3].cameraSpaceCoo[1][VEC_X] - pv_walls[z4].
                    cameraSpaceCoo[1][VEC_X]) + klabs(
                    pv_walls[z3].cameraSpaceCoo[1][VEC_Y] - pv_walls[z4].
                    cameraSpaceCoo[1][VEC_Y]);
                s2 = klabs(pv_walls[z1].cameraSpaceCoo[1][VEC_X] - pv_walls[z4].
                           cameraSpaceCoo[1][VEC_X]) + klabs(
                         pv_walls[z1].cameraSpaceCoo[1][VEC_Y] - pv_walls[z4].
                         cameraSpaceCoo[1][VEC_Y]);
                s2 += klabs(
                    pv_walls[z3].cameraSpaceCoo[1][VEC_X] - pv_walls[z2].
                    cameraSpaceCoo[1][VEC_X]) + klabs(
                    pv_walls[z3].cameraSpaceCoo[1][VEC_Y] - pv_walls[z2].
                    cameraSpaceCoo[1][VEC_Y]);
                if (s2 < s1) {
                    t = pv_walls[bunchWallsList[z]].screenSpaceCoo[1][VEC_COL];
                    pv_walls[bunchWallsList[z]].screenSpaceCoo[1][VEC_COL] =
                        pv_walls[bunchWallsList[zz]].screenSpaceCoo[1][VEC_COL];
                    pv_walls[bunchWallsList[zz]].screenSpaceCoo[1][VEC_COL] = t;
                }
            }


        npoints = 0;
        start2 = 0;
        z = 0;
        splitcnt = 0;
        do {
            s2 = cy1 - pv_walls[z].cameraSpaceCoo[1][VEC_Y];
            do {
                zz = pv_walls[z].screenSpaceCoo[1][VEC_COL];
                pv_walls[z].screenSpaceCoo[1][VEC_COL] = -1;
                s1 = s2;
                s2 = cy1 - pv_walls[zz].cameraSpaceCoo[1][VEC_Y];
                if (s1 < 0) {
                    pv_walls[npoints].cameraSpaceCoo[0][VEC_X] = pv_walls[z].
                        cameraSpaceCoo[1][VEC_X];
                    pv_walls[npoints].cameraSpaceCoo[0][VEC_Y] = pv_walls[z].
                        cameraSpaceCoo[1][VEC_Y];
                    pv_walls[npoints].screenSpaceCoo[0][VEC_COL] = npoints + 1;
                    npoints++;
                }
                if ((s1 ^ s2) < 0) {
                    pv_walls[npoints].cameraSpaceCoo[0][VEC_X] =
                        pv_walls[z].cameraSpaceCoo[1][VEC_X] + scale(
                            pv_walls[zz].cameraSpaceCoo[1][VEC_X] - pv_walls[z].
                            cameraSpaceCoo[1][VEC_X], s1, s1 - s2);
                    pv_walls[npoints].cameraSpaceCoo[0][VEC_Y] =
                        pv_walls[z].cameraSpaceCoo[1][VEC_Y] + scale(
                            pv_walls[zz].cameraSpaceCoo[1][VEC_Y] - pv_walls[z].
                            cameraSpaceCoo[1][VEC_Y], s1, s1 - s2);
                    if (s1 < 0)
                        bunchWallsList[splitcnt++] = npoints;
                    pv_walls[npoints].screenSpaceCoo[0][VEC_COL] = npoints + 1;
                    npoints++;
                }
                z = zz;
            } while (pv_walls[z].screenSpaceCoo[1][VEC_COL] >= 0);

            if (npoints >= start2 + 3)
                pv_walls[npoints - 1].screenSpaceCoo[0][VEC_COL] = start2, start2
                    = npoints;
            else
                npoints = start2;

            z = 1;
            while ((z < npoints2) && (
                       pv_walls[z].screenSpaceCoo[1][VEC_COL] < 0))
                z++;
        } while (z < npoints2);
        if (npoints <= 2)
            return (0);

        for (z = 1; z < splitcnt; z++)
            for (zz = 0; zz < z; zz++) {
                z1 = bunchWallsList[z];
                z2 = pv_walls[z1].screenSpaceCoo[0][VEC_COL];
                z3 = bunchWallsList[zz];
                z4 = pv_walls[z3].screenSpaceCoo[0][VEC_COL];
                s1 = klabs(pv_walls[z1].cameraSpaceCoo[0][VEC_X] - pv_walls[z2].
                           cameraSpaceCoo[0][VEC_X]) + klabs(
                         pv_walls[z1].cameraSpaceCoo[0][VEC_Y] - pv_walls[z2].
                         cameraSpaceCoo[0][VEC_Y]);
                s1 += klabs(
                    pv_walls[z3].cameraSpaceCoo[0][VEC_X] - pv_walls[z4].
                    cameraSpaceCoo[0][VEC_X]) + klabs(
                    pv_walls[z3].cameraSpaceCoo[0][VEC_Y] - pv_walls[z4].
                    cameraSpaceCoo[0][VEC_Y]);
                s2 = klabs(pv_walls[z1].cameraSpaceCoo[0][VEC_X] - pv_walls[z4].
                           cameraSpaceCoo[0][VEC_X]) + klabs(
                         pv_walls[z1].cameraSpaceCoo[0][VEC_Y] - pv_walls[z4].
                         cameraSpaceCoo[0][VEC_Y]);
                s2 += klabs(
                    pv_walls[z3].cameraSpaceCoo[0][VEC_X] - pv_walls[z2].
                    cameraSpaceCoo[0][VEC_X]) + klabs(
                    pv_walls[z3].cameraSpaceCoo[0][VEC_Y] - pv_walls[z2].
                    cameraSpaceCoo[0][VEC_Y]);
                if (s2 < s1) {
                    t = pv_walls[bunchWallsList[z]].screenSpaceCoo[0][VEC_COL];
                    pv_walls[bunchWallsList[z]].screenSpaceCoo[0][VEC_COL] =
                        pv_walls[bunchWallsList[zz]].screenSpaceCoo[0][VEC_COL];
                    pv_walls[bunchWallsList[zz]].screenSpaceCoo[0][VEC_COL] = t;
                }
            }
    }
    if (clipstat & 0x5) {
        /* Need to clip bottom or right */
        npoints2 = 0;
        start2 = 0;
        z = 0;
        splitcnt = 0;
        do {
            s2 = pv_walls[z].cameraSpaceCoo[0][VEC_X] - cx2;
            do {
                zz = pv_walls[z].screenSpaceCoo[0][VEC_COL];
                pv_walls[z].screenSpaceCoo[0][VEC_COL] = -1;
                s1 = s2;
                s2 = pv_walls[zz].cameraSpaceCoo[0][VEC_X] - cx2;
                if (s1 < 0) {
                    pv_walls[npoints2].cameraSpaceCoo[1][VEC_X] = pv_walls[z].
                        cameraSpaceCoo[0][VEC_X];
                    pv_walls[npoints2].cameraSpaceCoo[1][VEC_Y] = pv_walls[z].
                        cameraSpaceCoo[0][VEC_Y];
                    pv_walls[npoints2].screenSpaceCoo[1][VEC_COL] = npoints2 + 1;
                    npoints2++;
                }
                if ((s1 ^ s2) < 0) {
                    pv_walls[npoints2].cameraSpaceCoo[1][VEC_X] =
                        pv_walls[z].cameraSpaceCoo[0][VEC_X] + scale(
                            pv_walls[zz].cameraSpaceCoo[0][VEC_X] - pv_walls[z].
                            cameraSpaceCoo[0][VEC_X], s1, s1 - s2);
                    pv_walls[npoints2].cameraSpaceCoo[1][VEC_Y] =
                        pv_walls[z].cameraSpaceCoo[0][VEC_Y] + scale(
                            pv_walls[zz].cameraSpaceCoo[0][VEC_Y] - pv_walls[z].
                            cameraSpaceCoo[0][VEC_Y], s1, s1 - s2);
                    if (s1 < 0)
                        bunchWallsList[splitcnt++] = npoints2;
                    pv_walls[npoints2].screenSpaceCoo[1][VEC_COL] = npoints2 + 1;
                    npoints2++;
                }
                z = zz;
            } while (pv_walls[z].screenSpaceCoo[0][VEC_COL] >= 0);

            if (npoints2 >= start2 + 3)
                pv_walls[npoints2 - 1].screenSpaceCoo[1][VEC_COL] = start2,
                    start2 = npoints2;
            else
                npoints2 = start2;

            z = 1;
            while ((z < npoints) && (pv_walls[z].screenSpaceCoo[0][VEC_COL] < 0))
                z++;
        } while (z < npoints);
        if (npoints2 <= 2)
            return (0);

        for (z = 1; z < splitcnt; z++)
            for (zz = 0; zz < z; zz++) {
                z1 = bunchWallsList[z];
                z2 = pv_walls[z1].screenSpaceCoo[1][VEC_COL];
                z3 = bunchWallsList[zz];
                z4 = pv_walls[z3].screenSpaceCoo[1][VEC_COL];
                s1 = klabs(pv_walls[z1].cameraSpaceCoo[1][VEC_X] - pv_walls[z2].
                           cameraSpaceCoo[1][VEC_X]) + klabs(
                         pv_walls[z1].cameraSpaceCoo[1][VEC_Y] - pv_walls[z2].
                         cameraSpaceCoo[1][VEC_Y]);
                s1 += klabs(
                    pv_walls[z3].cameraSpaceCoo[1][VEC_X] - pv_walls[z4].
                    cameraSpaceCoo[1][VEC_X]) + klabs(
                    pv_walls[z3].cameraSpaceCoo[1][VEC_Y] - pv_walls[z4].
                    cameraSpaceCoo[1][VEC_Y]);
                s2 = klabs(pv_walls[z1].cameraSpaceCoo[1][VEC_X] - pv_walls[z4].
                           cameraSpaceCoo[1][VEC_X]) + klabs(
                         pv_walls[z1].cameraSpaceCoo[1][VEC_Y] - pv_walls[z4].
                         cameraSpaceCoo[1][VEC_Y]);
                s2 += klabs(
                    pv_walls[z3].cameraSpaceCoo[1][VEC_X] - pv_walls[z2].
                    cameraSpaceCoo[1][VEC_X]) + klabs(
                    pv_walls[z3].cameraSpaceCoo[1][VEC_Y] - pv_walls[z2].
                    cameraSpaceCoo[1][VEC_Y]);
                if (s2 < s1) {
                    t = pv_walls[bunchWallsList[z]].screenSpaceCoo[1][VEC_COL];
                    pv_walls[bunchWallsList[z]].screenSpaceCoo[1][VEC_COL] =
                        pv_walls[bunchWallsList[zz]].screenSpaceCoo[1][VEC_COL];
                    pv_walls[bunchWallsList[zz]].screenSpaceCoo[1][VEC_COL] = t;
                }
            }


        npoints = 0;
        start2 = 0;
        z = 0;
        splitcnt = 0;
        do {
            s2 = pv_walls[z].cameraSpaceCoo[1][VEC_Y] - cy2;
            do {
                zz = pv_walls[z].screenSpaceCoo[1][VEC_COL];
                pv_walls[z].screenSpaceCoo[1][VEC_COL] = -1;
                s1 = s2;
                s2 = pv_walls[zz].cameraSpaceCoo[1][VEC_Y] - cy2;
                if (s1 < 0) {
                    pv_walls[npoints].cameraSpaceCoo[0][VEC_X] = pv_walls[z].
                        cameraSpaceCoo[1][VEC_X];
                    pv_walls[npoints].cameraSpaceCoo[0][VEC_Y] = pv_walls[z].
                        cameraSpaceCoo[1][VEC_Y];
                    pv_walls[npoints].screenSpaceCoo[0][VEC_COL] = npoints + 1;
                    npoints++;
                }
                if ((s1 ^ s2) < 0) {
                    pv_walls[npoints].cameraSpaceCoo[0][VEC_X] =
                        pv_walls[z].cameraSpaceCoo[1][VEC_X] + scale(
                            pv_walls[zz].cameraSpaceCoo[1][VEC_X] - pv_walls[z].
                            cameraSpaceCoo[1][VEC_X], s1, s1 - s2);
                    pv_walls[npoints].cameraSpaceCoo[0][VEC_Y] =
                        pv_walls[z].cameraSpaceCoo[1][VEC_Y] + scale(
                            pv_walls[zz].cameraSpaceCoo[1][VEC_Y] - pv_walls[z].
                            cameraSpaceCoo[1][VEC_Y], s1, s1 - s2);
                    if (s1 < 0)
                        bunchWallsList[splitcnt++] = npoints;
                    pv_walls[npoints].screenSpaceCoo[0][VEC_COL] = npoints + 1;
                    npoints++;
                }
                z = zz;
            } while (pv_walls[z].screenSpaceCoo[1][VEC_COL] >= 0);

            if (npoints >= start2 + 3)
                pv_walls[npoints - 1].screenSpaceCoo[0][VEC_COL] = start2, start2
                    = npoints;
            else
                npoints = start2;

            z = 1;
            while ((z < npoints2) && (
                       pv_walls[z].screenSpaceCoo[1][VEC_COL] < 0))
                z++;
        } while (z < npoints2);
        if (npoints <= 2)
            return (0);

        for (z = 1; z < splitcnt; z++)
            for (zz = 0; zz < z; zz++) {
                z1 = bunchWallsList[z];
                z2 = pv_walls[z1].screenSpaceCoo[0][VEC_COL];
                z3 = bunchWallsList[zz];
                z4 = pv_walls[z3].screenSpaceCoo[0][VEC_COL];
                s1 = klabs(pv_walls[z1].cameraSpaceCoo[0][VEC_X] - pv_walls[z2].
                           cameraSpaceCoo[0][VEC_X]) + klabs(
                         pv_walls[z1].cameraSpaceCoo[0][VEC_Y] - pv_walls[z2].
                         cameraSpaceCoo[0][VEC_Y]);
                s1 += klabs(
                    pv_walls[z3].cameraSpaceCoo[0][VEC_X] - pv_walls[z4].
                    cameraSpaceCoo[0][VEC_X]) + klabs(
                    pv_walls[z3].cameraSpaceCoo[0][VEC_Y] - pv_walls[z4].
                    cameraSpaceCoo[0][VEC_Y]);
                s2 = klabs(pv_walls[z1].cameraSpaceCoo[0][VEC_X] - pv_walls[z4].
                           cameraSpaceCoo[0][VEC_X]) + klabs(
                         pv_walls[z1].cameraSpaceCoo[0][VEC_Y] - pv_walls[z4].
                         cameraSpaceCoo[0][VEC_Y]);
                s2 += klabs(
                    pv_walls[z3].cameraSpaceCoo[0][VEC_X] - pv_walls[z2].
                    cameraSpaceCoo[0][VEC_X]) + klabs(
                    pv_walls[z3].cameraSpaceCoo[0][VEC_Y] - pv_walls[z2].
                    cameraSpaceCoo[0][VEC_Y]);
                if (s2 < s1) {
                    t = pv_walls[bunchWallsList[z]].screenSpaceCoo[0][VEC_COL];
                    pv_walls[bunchWallsList[z]].screenSpaceCoo[0][VEC_COL] =
                        pv_walls[bunchWallsList[zz]].screenSpaceCoo[0][VEC_COL];
                    pv_walls[bunchWallsList[zz]].screenSpaceCoo[0][VEC_COL] = t;
                }
            }
    }

    return npoints;
}

static void AM_ProjectCameraSpace(
    const polygon_t* poly,
    const walltype* wal,
    i32* x,
    i32* y
) {
    // First translate point, so player is at origin.
    const i32 xt = wal->x - poly->x;
    const i32 yt = wal->y - poly->y;

    // Now rotate point so player is always facing up.
    // Also scale by zoom factor and screen aspect ratio.
    *x = dmulscale16(xt, xvect, -yt, yvect) + (xdim << 11);
    *y = dmulscale16(xt, yvect2, yt, xvect2) + (ydim << 11);
}

static void AM_SetPolyClipMask(polygon_t* poly) {
    const sectortype* s = poly->sec;
    const i32 first_wall = s->wallptr;
    const i32 num_walls = s->wallnum;

    for (i32 i = 0; i < num_walls; i++) {
        const walltype* wal = &wall[first_wall + i];
        i32 x;
        i32 y;
        AM_ProjectCameraSpace(poly, wal, &x, &y);

        // Add wall clip mask.
        poly->clip_mask |= AM_GetPointClipMask(x, y);

        // Update potential visible walls.
        pv_walls[i].cameraSpaceCoo[0][VEC_X] = x;
        pv_walls[i].cameraSpaceCoo[0][VEC_Y] = y;
        pv_walls[i].screenSpaceCoo[0][VEC_COL] = wal->point2 - first_wall;
    }
}

static bool AM_ClipPoly2(polygon_t* poly) {
    AM_SetPolyClipMask(poly);
    if ((poly->clip_mask & 0xf0) != 0xf0) {
        // Fully clipped:
        // all points of polygon are outside viewing window.
        return false;
    }
    poly->bakx1 = pv_walls[0].cameraSpaceCoo[0][VEC_X];
    poly->baky1 = mulscale16(pv_walls[0].cameraSpaceCoo[0][VEC_Y] - (ydim << 11), xyaspect) + (ydim << 11);
    if ((poly->clip_mask & 0x0f) == 0) {
        // No clipping needed:
        // all points of polygon are inside viewing window.
        return true;
    }
    poly->num_points = AM_ClipPoly(poly->num_points, poly->clip_mask);
    return poly->num_points >= 3;
}

//==============================================================================


/*
================================================================================

SPRITE DRAWER

================================================================================
*/

static void AM_DrawSprites(i32 dax, i32 day, i32 sort_num) {
    for (i32 s = sort_num - 1; s >= 0; s--) {
        const spritetype* spr = &sprite[tsprite[s].owner];
        if ((spr->cstat & 48) != 32) {
            continue;
        }
        i32 tilenum = spr->picnum;
        i32 xoff = (i32) ((int8_t) ((tiles[tilenum].anim_flags >> 8) & 255))  + ((i32) spr->xoffset);
        i32 yoff = (i32) ((int8_t) ((tiles[tilenum].anim_flags >> 16) & 255)) + ((i32) spr->yoffset);

        if ((spr->cstat & 4) > 0) {
            xoff = -xoff;
        }
        if ((spr->cstat & 8) > 0) {
            yoff = -yoff;
        }

        i32 k = spr->ang;
        i32 cosang = sintable[(k + 512) & 2047];
        i32 sinang = sintable[k];
        i32 xspan = tiles[tilenum].dim.width;
        i32 xrepeat = spr->xrepeat;
        i32 yspan = tiles[tilenum].dim.height;
        i32 yrepeat = spr->yrepeat;

        i32 ox = ((xspan >> 1) + xoff) * xrepeat;
        i32 oy = ((yspan >> 1) + yoff) * yrepeat;
        i32 x1 = spr->x + mulscale(sinang, ox, 16) + mulscale(cosang, oy, 16);
        i32 y1 = spr->y + mulscale(sinang, oy, 16) - mulscale(cosang, ox, 16);
        i32 l = xspan * xrepeat;
        i32 x2 = x1 - mulscale(sinang, l, 16);
        i32 y2 = y1 + mulscale(cosang, l, 16);
        l = yspan * yrepeat;
        k = -mulscale(cosang, l, 16);
        i32 x3 = x2 + k;
        i32 x4 = x1 + k;
        k = -mulscale(sinang, l, 16);
        i32 y3 = y2 + k;
        i32 y4 = y1 + k;

        pv_walls[0].screenSpaceCoo[0][VEC_COL] = 1;
        pv_walls[1].screenSpaceCoo[0][VEC_COL] = 2;
        pv_walls[2].screenSpaceCoo[0][VEC_COL] = 3;
        pv_walls[3].screenSpaceCoo[0][VEC_COL] = 0;
        i32 npoints = 4;

        i32 i = 0;

        ox = x1 - dax;
        oy = y1 - day;
        i32 x = dmulscale16(ox, xvect, -oy, yvect) + (xdim << 11);
        i32 y = dmulscale16(oy, xvect2, ox, yvect2) + (ydim << 11);
        i |= AM_GetPointClipMask(x, y);
        pv_walls[0].cameraSpaceCoo[0][VEC_X] = x;
        pv_walls[0].cameraSpaceCoo[0][VEC_Y] = y;

        ox = x2 - dax;
        oy = y2 - day;
        x = dmulscale16(ox, xvect, -oy, yvect) + (xdim << 11);
        y = dmulscale16(oy, xvect2, ox, yvect2) + (ydim << 11);
        i |= AM_GetPointClipMask(x, y);
        pv_walls[1].cameraSpaceCoo[0][VEC_X] = x;
        pv_walls[1].cameraSpaceCoo[0][VEC_Y] = y;

        ox = x3 - dax;
        oy = y3 - day;
        x = dmulscale16(ox, xvect, -oy, yvect) + (xdim << 11);
        y = dmulscale16(oy, xvect2, ox, yvect2) + (ydim << 11);
        i |= AM_GetPointClipMask(x, y);
        pv_walls[2].cameraSpaceCoo[0][VEC_X] = x;
        pv_walls[2].cameraSpaceCoo[0][VEC_Y] = y;

        x = pv_walls[0].cameraSpaceCoo[0][VEC_X]
            + pv_walls[2].cameraSpaceCoo[0][VEC_X]
            - pv_walls[1].cameraSpaceCoo[0][VEC_X];

        y = pv_walls[0].cameraSpaceCoo[0][VEC_Y]
            + pv_walls[2].cameraSpaceCoo[0][VEC_Y]
            - pv_walls[1].cameraSpaceCoo[0][VEC_Y];

        i |= AM_GetPointClipMask(x, y);
        pv_walls[3].cameraSpaceCoo[0][VEC_X] = x;
        pv_walls[3].cameraSpaceCoo[0][VEC_Y] = y;

        if ((i & 0xf0) != 0xf0) {
            continue;
        }
        i32 bakx1 = pv_walls[0].cameraSpaceCoo[0][VEC_X];
        i32 baky1 = mulscale16(
                    pv_walls[0].cameraSpaceCoo[0][VEC_Y] - (ydim << 11),
                    xyaspect) + (ydim << 11);
        if (i & 0x0f) {
            npoints = AM_ClipPoly(npoints, i);
            if (npoints < 3) {
                continue;
            }
        }

        if (!AM_SetPic(spr->picnum)) {
            continue;
        }

        if ((sector[spr->sectnum].ceilingstat & 1) > 0)
            globalshade = ((i32) sector[spr->sectnum].ceilingshade);
        else
            globalshade = ((i32) sector[spr->sectnum].floorshade);
        globalshade = max(min(globalshade+spr->shade+6,numpalookups-1), 0);
        asm3 = (intptr_t) palookup[spr->pal] + (globalshade << 8);
        globvis = globalhisibility;
        globalpolytype = ((spr->cstat & 2) >> 1) + 1;

        /* relative alignment stuff */
        ox = x2 - x1;
        oy = y2 - y1;
        i = ox * ox + oy * oy;
        if (i == 0)
            continue;
        i = (65536 * 16384) / i;
        globalx1 = mulscale10(dmulscale10(ox, bakgxvect, oy, bakgyvect), i);
        globaly1 = mulscale10(dmulscale10(ox, bakgyvect, -oy, bakgxvect), i);
        ox = y1 - y4;
        oy = x4 - x1;
        i = ox * ox + oy * oy;
        if (i == 0)
            continue;
        i = (65536 * 16384) / i;
        globalx2 = mulscale10(dmulscale10(ox, bakgxvect, oy, bakgyvect), i);
        globaly2 = mulscale10(dmulscale10(ox, bakgyvect, -oy, bakgxvect), i);

        ox = picsiz[globalpicnum];
        oy = ((ox >> 4) & 15);
        ox &= 15;
        if (pow2long[ox] != xspan) {
            ox++;
            globalx1 = mulscale(globalx1, xspan, ox);
            globaly1 = mulscale(globaly1, xspan, ox);
        }

        bakx1 = (bakx1 >> 4) - (xdim << 7);
        baky1 = (baky1 >> 4) - (ydim << 7);
        globalposx = dmulscale28(-baky1, globalx1, -bakx1, globaly1);
        globalposy = dmulscale28(bakx1, globalx2, -baky1, globaly2);

        if ((spr->cstat & 2) == 0) {
            msethlineshift(ox, oy);
        } else {
            tsethlineshift(ox, oy);
        }

        if ((spr->cstat & 0x4) > 0) {
            globalx1 = -globalx1;
            globaly1 = -globaly1;
            globalposx = -globalposx;
        }
        asm1 = (globaly1 << 2);
        globalx1 <<= 2;
        globalposx <<= (20 + 2);
        asm2 = (globalx2 << 2);
        globaly2 <<= 2;
        globalposy <<= (20 + 2);

        AM_FillPoly(npoints);
    }
}

static void AM_SortSprites(i32 sort_num) {
    i32 gap = 1;
    while (gap < sort_num) {
        gap = (gap << 1) + 1;
    }
    for (gap >>= 1; gap > 0; gap >>= 1) {
        for (i32 i = 0; i < sort_num - gap; i++) {
            for (i32 j = i; j >= 0; j -= gap) {
                if (sprite[tsprite[j].owner].z <= sprite[tsprite[j + gap].owner].z) {
                    break;
                }
                swapshort(&tsprite[j].owner, &tsprite[j + gap].owner);
            }
        }
    }
}

//
// Collect floor sprites to draw.
//
static i32 AM_CollectFloorSprites(i32 s, i32 sort_num) {
    for (i16 i = headspritesect[s]; i >= 0; i = nextspritesect[i]) {
        const spritetype* spr = &sprite[i];
        if (CSTAT_HAS(spr, SC_WALL) || !CSTAT_HAS(spr, SC_FLOOR)) {
            // Not floor sprite.
            continue;
        }
        if (CSTAT_HAS(spr, SC_YFLIP + SC_ONE_SIDE)) {
            // 1-sided and y-flipped sprite.
            continue;
        }
        tsprite[sort_num].owner = i;
        sort_num++;
    }
    return sort_num;
}

//==============================================================================


/*
================================================================================

POLYGON DRAWER

================================================================================
*/

static void AM_ApplyOrientation(void) {
    if ((globalorientation & 0x4) > 0) {
        swaplong(&globalposx, &globalposy);
        swaplong(&globalx1, &globaly2);
        swaplong(&globalx2, &globaly1);
        globalposx = -globalposx;
        globalposy = -globalposy;
        globalx1 = -globalx1;
        globaly2 = -globaly2;
    }
    if ((globalorientation & 0x10) > 0) {
        globalx1 = -globalx1;
        globaly1 = -globaly1;
        globalposx = -globalposx;
    }
    if ((globalorientation & 0x20) > 0) {
        globalx2 = -globalx2;
        globaly2 = -globaly2;
        globalposy = -globalposy;
    }
}

static void AM_SetGlobalShift(const polygon_t* poly) {
    globalxshift = (8 - (picsiz[globalpicnum] & 15));
    globalyshift = (8 - (picsiz[globalpicnum] >> 4));
    if (globalorientation & 8) {
        globalxshift++;
        globalyshift++;
    }
    asm1 = (globaly1 << globalxshift);
    asm2 = (globalx2 << globalyshift);
    globalx1 <<= globalxshift;
    globaly2 <<= globalyshift;
    const sectortype* s = poly->sec;
    globalposx = (globalposx << (20 + globalxshift)) + (((i32) s->floorxpanning) << 24);
    globalposy = (globalposy << (20 + globalyshift)) - (((i32) s->floorypanning) << 24);
}

static bool AM_SetGlobalPos(const polygon_t* poly) {
    if ((globalorientation & 64) == 0) {
        globalposx = poly->x;
        globalposy = poly->y;
        globalx1 = bakgxvect;
        globaly1 = bakgyvect;
        globalx2 = bakgxvect;
        globaly2 = bakgyvect;
        return true;
    }

    const sectortype* s = poly->sec;
    const walltype* wal1 = &wall[s->wallptr];
    const walltype* wal2 = &wall[wal1->point2];
    i32 dx = wal2->x - wal1->x;
    i32 dy = wal2->y - wal1->y;
    i32 i = ksqrt(dx * dx + dy * dy);
    if (i == 0) {
        return false;
    }
    // We know 1048576 = (1 << 20).
    i = 1048576 / i;
    globalx1 = mulscale10(dmulscale10(dx, bakgxvect, dy, bakgyvect), i);
    globaly1 = mulscale10(dmulscale10(dx, bakgyvect, -dy, bakgxvect), i);
    dx = (poly->bakx1 >> 4) - (xdim << 7);
    dy = (poly->baky1 >> 4) - (ydim << 7);
    globalposx = dmulscale28(-dy, globalx1, -dx, globaly1);
    globalposy = dmulscale28(-dx, globalx1, dy, globaly1);
    globalx2 = -globalx1;
    globaly2 = -globaly1;

    const i32 slope = s->floorheinum;
    // cosine reciprocal: 1/cos = sqrt(tan * tan + 1)
    i = ksqrt(slope * slope + 16777216);
    globalposy = mulscale12(globalposy, i);
    globalx2 = mulscale12(globalx2, i);
    globaly2 = mulscale12(globaly2, i);

    return true;
}

static void AM_SetPolyVisibility(const polygon_t* poly) {
    const sectortype* sec = poly->sec;
    globvis = globalhisibility;
    if (sec->visibility) {
        globvis = mulscale4(globvis, (u8) (sec->visibility + 16));
    }
}

static void AM_SetPolyShade(const polygon_t* poly) {
    const sectortype* sec = poly->sec;
    globalshade = (i32) sec->floorshade;
    globalshade = SDL_clamp(globalshade, 0, numpalookups - 1);
}

static bool AM_SetPolyPic(const polygon_t* poly) {
    const sectortype* sec = poly->sec;
    return AM_SetPic(sec->floorpicnum);
}

static void AM_SetPolyPalette(const polygon_t* poly) {
    const sectortype* s = poly->sec;
    if (globalpalwritten != palookup[s->floorpal]) {
        globalpalwritten = palookup[s->floorpal];
    }
}

static bool AM_SetGlobalOrientation(const polygon_t* poly) {
    const sectortype* s = poly->sec;
    globalorientation = (i32) s->floorstat;
    return (globalorientation & 1) == 0;
}

static bool AM_SetupPolyDrawer(const polygon_t* poly) {
    globalpolytype = 0;
    if (!AM_SetGlobalOrientation(poly)) {
        return false;
    }
    AM_SetPolyPalette(poly);
    if (!AM_SetPolyPic(poly)) {
        return false;
    }
    AM_SetPolyShade(poly);
    AM_SetPolyVisibility(poly);
    if (!AM_SetGlobalPos(poly)) {
        return false;
    }
    sethlinesizes(picsiz[globalpicnum] & 15, picsiz[globalpicnum] >> 4, globalbufplc);
    AM_ApplyOrientation();
    AM_SetGlobalShift(poly);
    return true;
}

static polygon_t AM_GetPoly(i32 s, i32 x, i32 y) {
    polygon_t poly = {};
    poly.sec = &sector[s];
    poly.x = x;
    poly.y = y;
    // In 2D, number of vertices equals number of edges.
    poly.num_points = poly.sec->wallnum;
    return poly;
}

static bool AM_IsSectorVisible(i32 sec) {
    return (show2dsector[sec >> 3] & pow2char[sec & 7]) != 0;
}

static i32 AM_DrawPolygons(i32 x, i32 y) {
    i32 sort_num = 0;

    for (i32 s = 0; s < numsectors; s++) {
        if (!AM_IsSectorVisible(s)) {
            continue;
        }
        polygon_t poly = AM_GetPoly(s, x, y);
        if (!AM_ClipPoly2(&poly)) {
            continue;
        }
        sort_num = AM_CollectFloorSprites(s, sort_num);
        if (AM_SetupPolyDrawer(&poly)) {
            AM_FillPoly(poly.num_points);
        }
    }

    return sort_num;
}


//==============================================================================


static void AM_PrepareDraw(i32 zoom, i16 angle) {
    beforedrawrooms = 0;

    zoom <<= 8;

    // "bak" is the vector pointing to the back of where the player is facing.
    i16 cos = COS(angle + ANG180);
    i16 sin = SIN(angle + ANG180);
    bakgxvect = divscale28(cos, zoom);
    bakgyvect = divscale28(sin, zoom);

    // Rotation by (270 - angle) + scale by zoom.
    cos = COS(ANG270 - angle);
    sin = SIN(ANG270 - angle);
    xvect = mulscale8(cos, zoom);
    yvect = mulscale8(sin, zoom);

    // Rotation by (270 - angle) + scale by zoom + scale by yxaspect.
    xvect2 = mulscale16(xvect, yxaspect);
    yvect2 = mulscale16(yvect, yxaspect);
}

void AM_DrawMapView(i32 x, i32 y, i32 zoom, i16 ang) {
    AM_PrepareDraw(zoom, ang);
    i32 sort_num = AM_DrawPolygons(x, y);
    AM_SortSprites(sort_num);
    AM_DrawSprites(x, y, sort_num);
}
