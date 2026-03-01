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
// movement collision code

#include "physics/physics.h"
#include "build/build.h"
#include "build/engine.h"
#include "build/tiles.h"
#if WIN32
#include "io.h"
#endif


#define MAXCLIPNUM 512
#define MAXCLIPDIST 1024

#define addclipline(dax1, day1, dax2, day2, daoval)                            \
    {                                                                          \
        clipit[clipnum].x1 = dax1;                                             \
        clipit[clipnum].y1 = day1;                                             \
        clipit[clipnum].x2 = dax2;                                             \
        clipit[clipnum].y2 = day2;                                             \
        clipobjectval[clipnum] = daoval;                                       \
        clipnum++;                                                             \
    }


typedef struct {
    i32 x1;
    i32 y1;
    i32 x2;
    i32 y2;
} linetype;


extern i32 rxi[8];
extern i32 ryi[8];
extern i32 rzi[8];
extern i32 rxi2[8];
extern i32 ryi2[8];
extern i32 rzi2[8];
extern i32 xsi[8];
extern i32 ysi[8];


static short editstatus = 0;

static linetype clipit[MAXCLIPNUM];
static short clipsectorlist[MAXCLIPNUM];
static short clipsectnum;
static short clipobjectval[MAXCLIPNUM];

static i16 clipnum;
static i16 hitwalls[4];

static i32 hitscangoalx = (1 << 29) - 1;
static i32 hitscangoaly = (1 << 29) - 1;


//
// Line intersect.
// If l1 be the line segment defined by the points p1 = (x1, y1)
// and p2 = (x2, y2), and l2 be the line segment defined by the
// points p3 = (x3, y3) and p4 = (x4, y4), Then this function
// checks if l1 crosses l2.
//
static int lintersect(
    i32 x1,
    i32 y1,
    i32 z1,
    i32 x2,
    i32 y2,
    i32 z2,
    i32 x3,
    i32 y3,
    i32 x4,
    i32 y4,
    i32* intx,
    i32* inty,
    i32* intz
) {
    i32 x21 = x2 - x1;
    i32 y21 = y2 - y1;
    i32 x34 = x3 - x4;
    i32 y34 = y3 - y4;
    i32 bot = x21 * y34 - y21 * x34;
    if (bot == 0) {
        return 0;
    }
    i32 x31 = x3 - x1;
    i32 y31 = y3 - y1;
    i32 topt = x31 * y34 - y31 * x34;
    if (bot > 0) {
        if (topt < 0 || topt >= bot) {
            return 0;
        }
        i32 topu = x21 * y31 - y21 * x31;
        if (topu < 0 || topu >= bot) {
            return 0;
        }
    } else {
        if (topt > 0 || topt <= bot) {
            return 0;
        }
        i32 topu = x21 * y31 - y21 * x31;
        if (topu > 0 || topu <= bot) {
            return 0;
        }
    }
    i32 t = divscale24(topt, bot);
    *intx = x1 + mulscale24(x21, t);
    *inty = y1 + mulscale24(y21, t);
    *intz = z1 + mulscale24(z2 - z1, t);
    return 1;
}


//
// Ray intersect.
// p1 towards p2 is a ray.
//
static int rintersect(
    i32 x1,
    i32 y1,
    i32 z1,
    i32 vx,
    i32 vy,
    i32 vz,
    i32 x3,
    i32 y3,
    i32 x4,
    i32 y4,
    i32* intx,
    i32* inty,
    i32* intz
) {
    i32 x34 = x3 - x4;
    i32 y34 = y3 - y4;
    i32 bot = vx * y34 - vy * x34;
    if (bot == 0) {
        return 0;
    }
    i32 x31 = x3 - x1;
    i32 y31 = y3 - y1;
    i32 topt = x31 * y34 - y31 * x34;
    if (bot > 0) {
        if (topt < 0) {
            return 0;
        }
        i32 topu = vx * y31 - vy * x31;
        if (topu < 0 || topu >= bot) {
            return 0;
        }
    } else {
        if (topt > 0) {
            return 0;
        }
        i32 topu = vx * y31 - vy * x31;
        if (topu > 0 || topu <= bot) {
            return 0;
        }
    }
    i32 t = divscale16(topt, bot);
    *intx = x1 + mulscale16(vx, t);
    *inty = y1 + mulscale16(vy, t);
    *intz = z1 + mulscale16(vz, t);
    return 1;
}


static void keepaway(i32* x, i32* y, i32 w) {
    i32 dx, dy, ox, oy, x1, y1;
    u8 first;

    x1 = clipit[w].x1;
    dx = clipit[w].x2 - x1;
    y1 = clipit[w].y1;
    dy = clipit[w].y2 - y1;
    ox = ksgn(-dy);
    oy = ksgn(dx);
    first = (klabs(dx) <= klabs(dy));
    while (1) {
        if (dx * (*y - y1) > (*x - x1) * dy)
            return;
        if (first == 0)
            *x += ox;
        else
            *y += oy;
        first ^= 1;
    }
}


static int raytrace(i32 x3, i32 y3, i32* x4, i32* y4) {
    i32 x1, y1, x2, y2, bot, topu, nintx, ninty, cnt, z, hitwall;
    i32 x21, y21, x43, y43;

    hitwall = -1;
    for (z = clipnum - 1; z >= 0; z--) {
        x1 = clipit[z].x1;
        x2 = clipit[z].x2;
        x21 = x2 - x1;
        y1 = clipit[z].y1;
        y2 = clipit[z].y2;
        y21 = y2 - y1;

        topu = x21 * (y3 - y1) - (x3 - x1) * y21;
        if (topu <= 0)
            continue;
        if (x21 * (*y4 - y1) > (*x4 - x1) * y21)
            continue;
        x43 = *x4 - x3;
        y43 = *y4 - y3;
        if (x43 * (y1 - y3) > (x1 - x3) * y43)
            continue;
        if (x43 * (y2 - y3) <= (x2 - x3) * y43)
            continue;
        bot = x43 * y21 - x21 * y43;
        if (bot == 0)
            continue;

        cnt = 256;
        do {
            cnt--;
            if (cnt < 0) {
                *x4 = x3;
                *y4 = y3;
                return (z);
            }
            nintx = x3 + scale(x43, topu, bot);
            ninty = y3 + scale(y43, topu, bot);
            topu--;
        } while (x21 * (ninty - y1) <= (nintx - x1) * y21);

        if (klabs(x3 - nintx) + klabs(y3 - ninty) < klabs(x3 - *x4) + klabs(
                y3 - *y4)) {
            *x4 = nintx;
            *y4 = ninty;
            hitwall = z;
        }
    }
    return (hitwall);
}


static int clipinsideboxline(
    i32 x,
    i32 y,
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2,
    i32 walldist
) {
    i32 r;

    r = (walldist << 1);

    x1 += walldist - x;
    x2 += walldist - x;
    if ((x1 < 0) && (x2 < 0))
        return (0);
    if ((x1 >= r) && (x2 >= r))
        return (0);

    y1 += walldist - y;
    y2 += walldist - y;
    if ((y1 < 0) && (y2 < 0))
        return (0);
    if ((y1 >= r) && (y2 >= r))
        return (0);

    x2 -= x1;
    y2 -= y1;
    if (x2 * (walldist - y1) >= y2 * (walldist - x1))
    {
        // Front.
        if (x2 > 0)
            x2 *= (0 - y1);
        else
            x2 *= (r - y1);
        if (y2 > 0)
            y2 *= (r - x1);
        else
            y2 *= (0 - x1);
        return (x2 < y2);
    }
    if (x2 > 0)
        x2 *= (r - y1);
    else
        x2 *= (0 - y1);
    if (y2 > 0)
        y2 *= (0 - x1);
    else
        y2 *= (r - x1);
    return ((x2 >= y2) << 1);
}


static int lastwall(short point) {
    if (point > 0 && wall[point - 1].point2 == point) {
        return point - 1;
    }
    i32 i = point;
    i32 cnt = MAXWALLS;
    do {
        i32 j = wall[i].point2;
        if (j == point) {
            return i;
        }
        i = j;
        cnt--;
    } while (cnt > 0);
    return (point);
}


int PHYS_ClipMove(
    i32* x,
    i32* y,
    i32* z,
    short* sectnum,
    i32 xvect,
    i32 yvect,
    i32 walldist,
    i32 ceildist,
    i32 flordist,
    u32 cliptype
) {
    // !!! ugh...move this var into clipmove as a parameter, and update build2.txt!
    static i32 clipmoveboxtracenum = 3;

    walltype *wal, *wal2;
    spritetype* spr;
    sectortype *sec, *sec2;
    i32 i, j, templong1, templong2;
    i32 oxvect, oyvect, goalx, goaly, intx, inty, lx, ly, retval;
    i32 k, l, clipsectcnt, startwall, endwall, cstat, dasect;
    i32 x1, y1, x2, y2, cx, cy, rad, xmin, ymin, xmax, ymax, daz, daz2;
    i32 bsz, dax, day, xoff, yoff, xspan, yspan, cosang, sinang, tilenum;
    i32 xrepeat, yrepeat, gx, gy, dx, dy, dasprclipmask, dawalclipmask;
    i32 hitwall, cnt, clipyou;

    if (((xvect | yvect) == 0) || (*sectnum < 0))
        return (0);
    retval = 0;

    oxvect = xvect;
    oyvect = yvect;

    goalx = (*x) + (xvect >> 14);
    goaly = (*y) + (yvect >> 14);


    clipnum = 0;

    cx = (((*x) + goalx) >> 1);
    cy = (((*y) + goaly) >> 1);
    // Extra walldist for sprites on sector lines.
    gx = goalx - (*x);
    gy = goaly - (*y);
    rad = ksqrt(gx * gx + gy * gy) + MAXCLIPDIST + walldist + 8;
    xmin = cx - rad;
    ymin = cy - rad;
    xmax = cx + rad;
    ymax = cy + rad;

    dawalclipmask = (cliptype & 65535); // CLIPMASK0 = 0x00010001
    dasprclipmask = (cliptype >> 16);   // CLIPMASK1 = 0x01000040

    clipsectorlist[0] = (*sectnum);
    clipsectcnt = 0;
    clipsectnum = 1;
    do {
        dasect = clipsectorlist[clipsectcnt++];
        sec = &sector[dasect];
        startwall = sec->wallptr;
        endwall = startwall + sec->wallnum;
        for (j = startwall, wal = &wall[startwall]; j < endwall; j++, wal++) {
            wal2 = &wall[wal->point2];
            if ((wal->x < xmin) && (wal2->x < xmin))
                continue;
            if ((wal->x > xmax) && (wal2->x > xmax))
                continue;
            if ((wal->y < ymin) && (wal2->y < ymin))
                continue;
            if ((wal->y > ymax) && (wal2->y > ymax))
                continue;

            x1 = wal->x;
            y1 = wal->y;
            x2 = wal2->x;
            y2 = wal2->y;

            dx = x2 - x1;
            dy = y2 - y1;
            if (dx * ((*y) - y1) < ((*x) - x1) * dy)
                continue; // If wall's not facing you.

            if (dx > 0)
                dax = dx * (ymin - y1);
            else
                dax = dx * (ymax - y1);
            if (dy > 0)
                day = dy * (xmax - x1);
            else
                day = dy * (xmin - x1);
            if (dax >= day)
                continue;

            clipyou = 0;
            if ((wal->nextsector < 0) || (wal->cstat & dawalclipmask))
                clipyou = 1;
            else if (editstatus == 0) {
                if (rintersect(*x, *y, 0, gx, gy, 0, x1, y1, x2, y2, &dax, &day,
                               &daz) == 0)
                    dax = *x, day = *y;
                daz = getflorzofslope((short) dasect, dax, day);
                daz2 = getflorzofslope(wal->nextsector, dax, day);

                sec2 = &sector[wal->nextsector];
                if (daz2 < daz - (1 << 8))
                    if ((sec2->floorstat & 1) == 0)
                        if ((*z) >= daz2 - (flordist - 1))
                            clipyou = 1;
                if (clipyou == 0) {
                    daz = getceilzofslope((short) dasect, dax, day);
                    daz2 = getceilzofslope(wal->nextsector, dax, day);
                    if (daz2 > daz + (1 << 8))
                        if ((sec2->ceilingstat & 1) == 0)
                            if ((*z) <= daz2 + (ceildist - 1))
                                clipyou = 1;
                }
            }

            if (clipyou) {
                // Add 2 boxes at endpoints.
                bsz = walldist;
                if (gx < 0)
                    bsz = -bsz;
                addclipline(x1-bsz, y1-bsz, x1-bsz, y1+bsz, (short)j+32768);
                addclipline(x2-bsz, y2-bsz, x2-bsz, y2+bsz, (short)j+32768);
                bsz = walldist;
                if (gy < 0)
                    bsz = -bsz;
                addclipline(x1+bsz, y1-bsz, x1-bsz, y1-bsz, (short)j+32768);
                addclipline(x2+bsz, y2-bsz, x2-bsz, y2-bsz, (short)j+32768);

                dax = walldist;
                if (dy > 0)
                    dax = -dax;
                day = walldist;
                if (dx < 0)
                    day = -day;
                addclipline(x1+dax, y1+day, x2+dax, y2+day, (short)j+32768);
            } else {
                for (i = clipsectnum - 1; i >= 0; i--)
                    if (wal->nextsector == clipsectorlist[i])
                        break;
                if (i < 0)
                    clipsectorlist[clipsectnum++] = wal->nextsector;
            }
        }

        for (j = headspritesect[dasect]; j >= 0; j = nextspritesect[j]) {
            spr = &sprite[j];
            cstat = spr->cstat;
            if ((cstat & dasprclipmask) == 0)
                continue;
            x1 = spr->x;
            y1 = spr->y;
            switch (cstat & 48) {
                case 0:
                    if ((x1 >= xmin) && (x1 <= xmax) && (y1 >= ymin) && (
                            y1 <= ymax)) {
                        k = ((tiles[spr->picnum].dim.height * spr->yrepeat) <<
                             2);
                        if (cstat & 128)
                            daz = spr->z + (k >> 1);
                        else
                            daz = spr->z;

                        if (tiles[spr->picnum].anim_flags & 0x00ff0000)
                            daz -= (
                                (i32) ((i8) (
                                    (tiles[spr->picnum].anim_flags >> 16) & 255))
                                * spr->yrepeat << 2);

                        if (((*z) < daz + ceildist) && (
                                (*z) > daz - k - flordist)) {
                            bsz = (spr->clipdist << 2) + walldist;
                            if (gx < 0)
                                bsz = -bsz;
                            addclipline(x1-bsz, y1-bsz, x1-bsz, y1+bsz,
                                        (short)j+49152);
                            bsz = (spr->clipdist << 2) + walldist;
                            if (gy < 0)
                                bsz = -bsz;
                            addclipline(x1+bsz, y1-bsz, x1-bsz, y1-bsz,
                                        (short)j+49152);
                        }
                    }
                    break;
                case 16:
                    k = ((tiles[spr->picnum].dim.height * spr->yrepeat) << 2);

                    if (cstat & 128)
                        daz = spr->z + (k >> 1);
                    else
                        daz = spr->z;

                    if (tiles[spr->picnum].anim_flags & 0x00ff0000)
                        daz -= ((i32) ((i8) (
                                    (tiles[spr->picnum].anim_flags >> 16) & 255))
                                * spr->yrepeat << 2);
                    daz2 = daz - k;
                    daz += ceildist;
                    daz2 -= flordist;
                    if (((*z) < daz) && ((*z) > daz2)) {
                        // These lines get the 2 points of the rotated sprite
                        // Given: (x1, y1) starts out as the center point
                        tilenum = spr->picnum;
                        xoff = (i32) ((i8) (
                                   (tiles[tilenum].anim_flags >> 8) & 255)) + ((
                                   i32) spr->xoffset);
                        if ((cstat & 4) > 0)
                            xoff = -xoff;
                        k = spr->ang;
                        l = spr->xrepeat;
                        dax = SIN(k) * l;
                        day = SIN(k + ANG270) * l;
                        l = tiles[tilenum].dim.width;
                        k = (l >> 1) + xoff;
                        x1 -= mulscale16(dax, k);
                        x2 = x1 + mulscale16(dax, l);
                        y1 -= mulscale16(day, k);
                        y2 = y1 + mulscale16(day, l);
                        if (clipinsideboxline(cx, cy, x1, y1, x2, y2, rad) !=
                            0) {
                            dax = mulscale14(
                                COS(spr->ang + ANG45),
                                walldist);
                            day = mulscale14(SIN(spr->ang + ANG45),
                                             walldist);

                            if ((x1 - (*x)) * (y2 - (*y)) >= (x2 - (*x)) * (
                                    y1 - (*y))) // Front
                            {
                                addclipline(x1+dax, y1+day, x2+day, y2-dax,
                                            (short)j+49152);
                            } else {
                                if ((cstat & 64) != 0)
                                    continue;
                                addclipline(x2-dax, y2-day, x1-day, y1+dax,
                                            (short)j+49152);
                            }

                            // Side blocker
                            if ((x2 - x1) * ((*x) - x1) + (y2 - y1) * (
                                    (*y) - y1) < 0) {
                                addclipline(x1-day, y1+dax, x1+dax, y1+day,
                                            (short)j+49152);
                            } else if (
                                (x1 - x2) * ((*x) - x2) + (y1 - y2) * (
                                    (*y) - y2) < 0) {
                                addclipline(x2+day, y2-dax, x2-dax, y2-day,
                                            (short)j+49152);
                            }
                        }
                    }
                    break;
                case 32:
                    daz = spr->z + ceildist;
                    daz2 = spr->z - flordist;
                    if (((*z) < daz) && ((*z) > daz2)) {
                        if ((cstat & 64) != 0)
                            if (((*z) > spr->z) == ((cstat & 8) == 0))
                                continue;

                        tilenum = spr->picnum;
                        xoff = (i32) ((i8) (
                                   (tiles[tilenum].anim_flags >> 8) & 255)) + ((
                                   i32) spr->xoffset);
                        yoff = (i32) ((i8) (
                                   (tiles[tilenum].anim_flags >> 16) & 255)) + ((
                                   i32) spr->yoffset);
                        if ((cstat & 4) > 0)
                            xoff = -xoff;
                        if ((cstat & 8) > 0)
                            yoff = -yoff;

                        k = spr->ang;
                        cosang = COS(k);
                        sinang = SIN(k);
                        xspan = tiles[tilenum].dim.width;
                        xrepeat = spr->xrepeat;
                        yspan = tiles[tilenum].dim.height;
                        yrepeat = spr->yrepeat;

                        dax = ((xspan >> 1) + xoff) * xrepeat;
                        day = ((yspan >> 1) + yoff) * yrepeat;
                        rxi[0] = x1 + dmulscale16(sinang, dax, cosang, day);
                        ryi[0] = y1 + dmulscale16(sinang, day, -cosang, dax);
                        l = xspan * xrepeat;
                        rxi[1] = rxi[0] - mulscale16(sinang, l);
                        ryi[1] = ryi[0] + mulscale16(cosang, l);
                        l = yspan * yrepeat;
                        k = -mulscale16(cosang, l);
                        rxi[2] = rxi[1] + k;
                        rxi[3] = rxi[0] + k;
                        k = -mulscale16(sinang, l);
                        ryi[2] = ryi[1] + k;
                        ryi[3] = ryi[0] + k;

                        dax = mulscale14(
                            COS(spr->ang - ANG45), walldist);
                        day = mulscale14(SIN(spr->ang - ANG45),
                                         walldist);

                        if ((rxi[0] - (*x)) * (ryi[1] - (*y)) < (rxi[1] - (*x))
                            * (ryi[0] - (*y))) {
                            if (clipinsideboxline(
                                    cx, cy, rxi[1], ryi[1], rxi[0], ryi[0],
                                    rad) != 0)
                                addclipline(rxi[1]-day, ryi[1]+dax, rxi[0]+dax,
                                        ryi[0]+day, (short)j+49152);
                        } else if (
                            (rxi[2] - (*x)) * (ryi[3] - (*y)) < (rxi[3] - (*x))
                            * (ryi[2] - (*y))) {
                            if (clipinsideboxline(
                                    cx, cy, rxi[3], ryi[3], rxi[2], ryi[2],
                                    rad) != 0)
                                addclipline(rxi[3]+day, ryi[3]-dax, rxi[2]-dax,
                                        ryi[2]-day, (short)j+49152);
                        }

                        if ((rxi[1] - (*x)) * (ryi[2] - (*y)) < (rxi[2] - (*x))
                            * (ryi[1] - (*y))) {
                            if (clipinsideboxline(
                                    cx, cy, rxi[2], ryi[2], rxi[1], ryi[1],
                                    rad) != 0)
                                addclipline(rxi[2]-dax, ryi[2]-day, rxi[1]-day,
                                        ryi[1]+dax, (short)j+49152);
                        } else if (
                            (rxi[3] - (*x)) * (ryi[0] - (*y)) < (rxi[0] - (*x))
                            * (ryi[3] - (*y))) {
                            if (clipinsideboxline(
                                    cx, cy, rxi[0], ryi[0], rxi[3], ryi[3],
                                    rad) != 0)
                                addclipline(rxi[0]+dax, ryi[0]+day, rxi[3]+day,
                                        ryi[3]-dax, (short)j+49152);
                        }
                    }
                    break;
            }
        }
    } while (clipsectcnt < clipsectnum);


    hitwall = 0;
    cnt = clipmoveboxtracenum;
    do {
        intx = goalx;
        inty = goaly;
        if ((hitwall = raytrace(*x, *y, &intx, &inty)) >= 0) {
            lx = clipit[hitwall].x2 - clipit[hitwall].x1;
            ly = clipit[hitwall].y2 - clipit[hitwall].y1;
            templong2 = lx * lx + ly * ly;
            if (templong2 > 0) {
                templong1 = (goalx - intx) * lx + (goaly - inty) * ly;

                if ((klabs(templong1) >> 11) < templong2)
                    i = divscale20(templong1, templong2);
                else
                    i = 0;
                goalx = mulscale20(lx, i) + intx;
                goaly = mulscale20(ly, i) + inty;
            }

            templong1 = dmulscale6(lx, oxvect, ly, oyvect);
            for (i = cnt + 1; i <= clipmoveboxtracenum; i++) {
                j = hitwalls[i];
                templong2 = dmulscale6(clipit[j].x2 - clipit[j].x1, oxvect,
                                       clipit[j].y2 - clipit[j].y1, oyvect);
                if ((templong1 ^ templong2) < 0) {
                    PHYS_UpdateSector(*x, *y, sectnum);
                    return (retval);
                }
            }

            keepaway(&goalx, &goaly, hitwall);
            xvect = ((goalx - intx) << 14);
            yvect = ((goaly - inty) << 14);

            if (cnt == clipmoveboxtracenum)
                retval = clipobjectval[hitwall];
            hitwalls[cnt] = hitwall;
        }
        cnt--;

        *x = intx;
        *y = inty;
    } while (((xvect | yvect) != 0) && (hitwall >= 0) && (cnt > 0));

    for (j = 0; j < clipsectnum; j++)
        if (PHYS_Inside(*x, *y, clipsectorlist[j]) == 1) {
            *sectnum = clipsectorlist[j];
            return (retval);
        }

    *sectnum = -1;
    templong1 = 0x7fffffff;
    for (j = numsectors - 1; j >= 0; j--)
        if (PHYS_Inside(*x, *y, j) == 1) {
            if (sector[j].ceilingstat & 2)
                templong2 = (getceilzofslope((short) j, *x, *y) - (*z));
            else
                templong2 = (sector[j].ceilingz - (*z));

            if (templong2 > 0) {
                if (templong2 < templong1) {
                    *sectnum = j;
                    templong1 = templong2;
                }
            } else {
                if (sector[j].floorstat & 2)
                    templong2 = ((*z) - getflorzofslope((short) j, *x, *y));
                else
                    templong2 = ((*z) - sector[j].floorz);

                if (templong2 <= 0) {
                    *sectnum = j;
                    return (retval);
                }
                if (templong2 < templong1) {
                    *sectnum = j;
                    templong1 = templong2;
                }
            }
        }

    return (retval);
}


int PHYS_PushMove(
    i32* x,
    i32* y,
    i32* z,
    short* sectnum,
    i32 walldist,
    i32 ceildist,
    i32 flordist,
    u32 cliptype
) {
    sectortype *sec, *sec2;
    walltype* wal;
    i32 i, j, k, t, dx, dy, dax, day, daz, daz2, bad, dir;
    //    i32 dasprclipmask;
    i32 dawalclipmask;
    short startwall, endwall, clipsectcnt;
    u8 bad2;

    if ((*sectnum) < 0)
        return (-1);

    dawalclipmask = (cliptype & 65535);
    //    dasprclipmask = (cliptype>>16);

    k = 32;
    dir = 1;
    do {
        bad = 0;

        clipsectorlist[0] = *sectnum;
        clipsectcnt = 0;
        clipsectnum = 1;
        do {
            sec = &sector[clipsectorlist[clipsectcnt]];
            if (dir > 0)
                startwall = sec->wallptr, endwall = startwall + sec->wallnum;
            else
                endwall = sec->wallptr, startwall = endwall + sec->wallnum;

            for (i = startwall, wal = &wall[startwall]; i != endwall;
                 i += dir, wal += dir)
                if (PHYS_ClipInsideBox(*x, *y, i, walldist - 4) == 1) {
                    j = 0;
                    if (wal->nextsector < 0)
                        j = 1;
                    if (wal->cstat & dawalclipmask)
                        j = 1;
                    if (j == 0) {
                        sec2 = &sector[wal->nextsector];


                        // Find the closest point on wall (dax, day) to (*x, *y)
                        dax = wall[wal->point2].x - wal->x;
                        day = wall[wal->point2].y - wal->y;
                        daz = dax * ((*x) - wal->x) + day * ((*y) - wal->y);
                        if (daz <= 0)
                            t = 0;
                        else {
                            daz2 = dax * dax + day * day;
                            if (daz >= daz2)
                                t = (1 << 30);
                            else
                                t = divscale30(daz, daz2);
                        }
                        dax = wal->x + mulscale30(dax, t);
                        day = wal->y + mulscale30(day, t);


                        daz = getflorzofslope(clipsectorlist[clipsectcnt], dax,
                                              day);
                        daz2 = getflorzofslope(wal->nextsector, dax, day);
                        if ((daz2 < daz - (1 << 8)) && (
                                (sec2->floorstat & 1) == 0))
                            if (*z >= daz2 - (flordist - 1))
                                j = 1;

                        daz = getceilzofslope(clipsectorlist[clipsectcnt], dax,
                                              day);
                        daz2 = getceilzofslope(wal->nextsector, dax, day);
                        if ((daz2 > daz + (1 << 8)) && (
                                (sec2->ceilingstat & 1) == 0))
                            if (*z <= daz2 + (ceildist - 1))
                                j = 1;
                    }
                    if (j != 0) {
                        j = getangle(wall[wal->point2].x - wal->x,
                                     wall[wal->point2].y - wal->y);
                        dx = (SIN(j + ANG180) >> 11);
                        dy = (COS(j) >> 11);
                        bad2 = 16;
                        do {
                            *x = (*x) + dx;
                            *y = (*y) + dy;
                            bad2--;
                            if (bad2 == 0)
                                break;
                        } while (PHYS_ClipInsideBox(*x, *y, i, walldist - 4) != 0);
                        bad = -1;
                        k--;
                        if (k <= 0)
                            return (bad);
                        PHYS_UpdateSector(*x, *y, sectnum);
                    } else {
                        for (j = clipsectnum - 1; j >= 0; j--)
                            if (wal->nextsector == clipsectorlist[j])
                                break;
                        if (j < 0)
                            clipsectorlist[clipsectnum++] = wal->nextsector;
                    }
                }

            clipsectcnt++;
        } while (clipsectcnt < clipsectnum);
        dir = -dir;
    } while (bad != 0);

    return (bad);
}


void PHYS_GetZRange(
    i32 x,
    i32 y,
    i32 z,
    short sectnum,
    i32* ceilz,
    i32* ceilhit,
    i32* florz,
    i32* florhit,
    i32 walldist,
    u32 cliptype
) {
    sectortype* sec;
    walltype *wal, *wal2;
    spritetype* spr;
    i32 clipsectcnt, startwall, endwall, tilenum, xoff, yoff, dax, day;
    i32 xmin, ymin, xmax, ymax, i, j, k, l, daz, daz2, dx, dy;
    i32 x1, y1, x2, y2, x3, y3, x4, y4, ang, cosang, sinang;
    i32 xspan, yspan, xrepeat, yrepeat, dasprclipmask, dawalclipmask;
    short cstat;
    u8 clipyou;

    if (sectnum < 0) {
        *ceilz = 0x80000000;
        *ceilhit = -1;
        *florz = 0x7fffffff;
        *florhit = -1;
        return;
    }

    // Extra walldist for sprites on sector lines.
    i = walldist + MAXCLIPDIST + 1;
    xmin = x - i;
    ymin = y - i;
    xmax = x + i;
    ymax = y + i;

    getzsofslope(sectnum, x, y, ceilz, florz);
    *ceilhit = sectnum + 16384;
    *florhit = sectnum + 16384;

    dawalclipmask = (cliptype & 65535);
    dasprclipmask = (cliptype >> 16);

    clipsectorlist[0] = sectnum;
    clipsectcnt = 0;
    clipsectnum = 1;

    // Collect sectors inside your square first.
    do
    {
        sec = &sector[clipsectorlist[clipsectcnt]];
        startwall = sec->wallptr;
        endwall = startwall + sec->wallnum;
        for (j = startwall, wal = &wall[startwall]; j < endwall; j++, wal++) {
            k = wal->nextsector;
            if (k >= 0) {
                wal2 = &wall[wal->point2];
                x1 = wal->x;
                x2 = wal2->x;
                if ((x1 < xmin) && (x2 < xmin))
                    continue;
                if ((x1 > xmax) && (x2 > xmax))
                    continue;
                y1 = wal->y;
                y2 = wal2->y;
                if ((y1 < ymin) && (y2 < ymin))
                    continue;
                if ((y1 > ymax) && (y2 > ymax))
                    continue;

                dx = x2 - x1;
                dy = y2 - y1;
                if (dx * (y - y1) < (x - x1) * dy)
                    continue; // back.
                if (dx > 0)
                    dax = dx * (ymin - y1);
                else
                    dax = dx * (ymax - y1);
                if (dy > 0)
                    day = dy * (xmax - x1);
                else
                    day = dy * (xmin - x1);
                if (dax >= day)
                    continue;

                if (wal->cstat & dawalclipmask)
                    continue;
                sec = &sector[k];
                if (editstatus == 0) {
                    if (((sec->ceilingstat & 1) == 0) && (
                            z <= sec->ceilingz + (3 << 8)))
                        continue;
                    if (((sec->floorstat & 1) == 0) && (
                            z >= sec->floorz - (3 << 8)))
                        continue;
                }

                for (i = clipsectnum - 1; i >= 0; i--)
                    if (clipsectorlist[i] == k)
                        break;
                if (i < 0)
                    clipsectorlist[clipsectnum++] = k;

                if ((x1 < xmin + MAXCLIPDIST) && (x2 < xmin + MAXCLIPDIST))
                    continue;
                if ((x1 > xmax - MAXCLIPDIST) && (x2 > xmax - MAXCLIPDIST))
                    continue;
                if ((y1 < ymin + MAXCLIPDIST) && (y2 < ymin + MAXCLIPDIST))
                    continue;
                if ((y1 > ymax - MAXCLIPDIST) && (y2 > ymax - MAXCLIPDIST))
                    continue;
                if (dx > 0)
                    dax += dx * MAXCLIPDIST;
                else
                    dax -= dx * MAXCLIPDIST;
                if (dy > 0)
                    day -= dy * MAXCLIPDIST;
                else
                    day += dy * MAXCLIPDIST;
                if (dax >= day)
                    continue;

                // It actually got here, through all the continue's.
                getzsofslope((short) k, x, y, &daz, &daz2);
                if (daz > *ceilz) {
                    *ceilz = daz;
                    *ceilhit = k + 16384;
                }
                if (daz2 < *florz) {
                    *florz = daz2;
                    *florhit = k + 16384;
                }
            }
        }
        clipsectcnt++;
    } while (clipsectcnt < clipsectnum);

    for (i = 0; i < clipsectnum; i++) {
        for (j = headspritesect[clipsectorlist[i]]; j >= 0;
             j = nextspritesect[j]) {
            spr = &sprite[j];
            cstat = spr->cstat;
            if (cstat & dasprclipmask) {
                x1 = spr->x;
                y1 = spr->y;

                clipyou = 0;
                switch (cstat & 48) {
                    case 0:
                        k = walldist + (spr->clipdist << 2) + 1;
                        if ((klabs(x1 - x) <= k) && (klabs(y1 - y) <= k)) {
                            daz = spr->z;
                            k = ((tiles[spr->picnum].dim.height * spr->yrepeat)
                                 << 1);
                            if (cstat & 128)
                                daz += k;
                            if (tiles[spr->picnum].anim_flags & 0x00ff0000)
                                daz -= (
                                    (i32) ((i8) (
                                        (tiles[spr->picnum].anim_flags >> 16) &
                                        255)) * spr->yrepeat << 2);
                            daz2 = daz - (k << 1);
                            clipyou = 1;
                        }
                        break;
                    case 16:
                        tilenum = spr->picnum;
                        xoff = (i32) ((i8) (
                                   (tiles[tilenum].anim_flags >> 8) & 255)) + ((
                                   i32) spr->xoffset);
                        if ((cstat & 4) > 0)
                            xoff = -xoff;
                        k = spr->ang;
                        l = spr->xrepeat;
                        dax = SIN(k) * l;
                        day = SIN(k + ANG270) * l;
                        l = tiles[tilenum].dim.width;
                        k = (l >> 1) + xoff;
                        x1 -= mulscale16(dax, k);
                        x2 = x1 + mulscale16(dax, l);
                        y1 -= mulscale16(day, k);
                        y2 = y1 + mulscale16(day, l);
                        if (clipinsideboxline(x, y, x1, y1, x2, y2,
                                              walldist + 1) != 0) {
                            daz = spr->z;
                            k = ((tiles[spr->picnum].dim.height * spr->yrepeat)
                                 << 1);
                            if (cstat & 128)
                                daz += k;

                            if (tiles[spr->picnum].anim_flags & 0x00ff0000)
                                daz -= (
                                    (i32) ((i8) (
                                        (tiles[spr->picnum].anim_flags >> 16) &
                                        255)) * spr->yrepeat << 2);

                            daz2 = daz - (k << 1);
                            clipyou = 1;
                        }
                        break;
                    case 32:
                        daz = spr->z;
                        daz2 = daz;

                        if ((cstat & 64) != 0)
                            if ((z > daz) == ((cstat & 8) == 0))
                                continue;

                        tilenum = spr->picnum;
                        xoff = (i32) ((i8) (
                                   (tiles[tilenum].anim_flags >> 8) & 255)) + ((
                                   i32) spr->xoffset);
                        yoff = (i32) ((i8) (
                                   (tiles[tilenum].anim_flags >> 16) & 255)) + ((
                                   i32) spr->yoffset);
                        if ((cstat & 4) > 0)
                            xoff = -xoff;
                        if ((cstat & 8) > 0)
                            yoff = -yoff;

                        ang = spr->ang;
                        cosang = COS(ang);
                        sinang = SIN(ang);
                        xspan = tiles[tilenum].dim.width;
                        xrepeat = spr->xrepeat;
                        yspan = tiles[tilenum].dim.height;
                        yrepeat = spr->yrepeat;

                        dax = ((xspan >> 1) + xoff) * xrepeat;
                        day = ((yspan >> 1) + yoff) * yrepeat;
                        x1 += dmulscale16(sinang, dax, cosang, day) - x;
                        y1 += dmulscale16(sinang, day, -cosang, dax) - y;
                        l = xspan * xrepeat;
                        x2 = x1 - mulscale16(sinang, l);
                        y2 = y1 + mulscale16(cosang, l);
                        l = yspan * yrepeat;
                        k = -mulscale16(cosang, l);
                        x3 = x2 + k;
                        x4 = x1 + k;
                        k = -mulscale16(sinang, l);
                        y3 = y2 + k;
                        y4 = y1 + k;

                        dax = mulscale14(
                            COS(spr->ang - ANG45),
                            walldist + 4);
                        day = mulscale14(SIN(spr->ang - ANG45),
                                         walldist + 4);
                        x1 += dax;
                        x2 -= day;
                        x3 -= dax;
                        x4 += day;
                        y1 += day;
                        y2 += dax;
                        y3 -= day;
                        y4 -= dax;

                        if ((y1 ^ y2) < 0) {
                            if ((x1 ^ x2) < 0)
                                clipyou ^= (x1 * y2 < x2 * y1) ^ (y1 < y2);
                            else
                                if (x1 >= 0)
                                    clipyou ^= 1;
                        }
                        if ((y2 ^ y3) < 0) {
                            if ((x2 ^ x3) < 0)
                                clipyou ^= (x2 * y3 < x3 * y2) ^ (y2 < y3);
                            else
                                if (x2 >= 0)
                                    clipyou ^= 1;
                        }
                        if ((y3 ^ y4) < 0) {
                            if ((x3 ^ x4) < 0)
                                clipyou ^= (x3 * y4 < x4 * y3) ^ (y3 < y4);
                            else
                                if (x3 >= 0)
                                    clipyou ^= 1;
                        }
                        if ((y4 ^ y1) < 0) {
                            if ((x4 ^ x1) < 0)
                                clipyou ^= (x4 * y1 < x1 * y4) ^ (y4 < y1);
                            else
                                if (x4 >= 0)
                                    clipyou ^= 1;
                        }
                        break;
                }

                if (clipyou != 0) {
                    if ((z > daz) && (daz > *ceilz)) {
                        *ceilz = daz;
                        *ceilhit = j + 49152;
                    }
                    if ((z < daz2) && (daz2 < *florz)) {
                        *florz = daz2;
                        *florhit = j + 49152;
                    }
                }
            }
        }
    }
}


int PHYS_Hitscan(
    i32 xs,
    i32 ys,
    i32 zs,
    short sectnum,
    i32 vx,
    i32 vy,
    i32 vz,
    short* hitsect,
    short* hitwall,
    short* hitsprite,
    i32* hitx,
    i32* hity,
    i32* hitz,
    u32 cliptype
) {
    sectortype* sec;
    walltype *wal, *wal2;
    spritetype* spr;
    i32 z, zz, x1, y1 = 0, z1 = 0, x2, y2, x3, y3, x4, y4, intx, inty, intz;
    i32 topt, topu, bot, dist, offx, offy, cstat;
    i32 i, j, k, l, tilenum, xoff, yoff, dax, day, daz, daz2;
    i32 ang, cosang, sinang, xspan, yspan, xrepeat, yrepeat;
    i32 dawalclipmask, dasprclipmask;
    short tempshortcnt, tempshortnum, dasector, startwall, endwall;
    short nextsector;
    u8 clipyou;

    *hitsect = -1;
    *hitwall = -1;
    *hitsprite = -1;
    if (sectnum < 0)
        return (-1);

    *hitx = hitscangoalx;
    *hity = hitscangoaly;

    dawalclipmask = (cliptype & 65535);
    dasprclipmask = (cliptype >> 16);

    clipsectorlist[0] = sectnum;
    tempshortcnt = 0;
    tempshortnum = 1;
    do {
        dasector = clipsectorlist[tempshortcnt];
        sec = &sector[dasector];

        x1 = 0x7fffffff;
        if (sec->ceilingstat & 2) {
            wal = &wall[sec->wallptr];
            wal2 = &wall[wal->point2];
            dax = wal2->x - wal->x;
            day = wal2->y - wal->y;
            i = ksqrt(dax * dax + day * day);
            if (i == 0)
                continue;
            i = divscale15(sec->ceilingheinum, i);
            dax *= i;
            day *= i;

            j = (vz << 8) - dmulscale15(dax, vy, -day, vx);
            if (j != 0) {
                i = ((sec->ceilingz - zs) << 8) + dmulscale15(
                        dax, ys - wal->y, -day, xs - wal->x);
                if (((i ^ j) >= 0) && ((klabs(i) >> 1) < klabs(j))) {
                    i = divscale30(i, j);
                    x1 = xs + mulscale30(vx, i);
                    y1 = ys + mulscale30(vy, i);
                    z1 = zs + mulscale30(vz, i);
                }
            }
        } else if ((vz < 0) && (zs >= sec->ceilingz)) {
            z1 = sec->ceilingz;
            i = z1 - zs;
            if ((klabs(i) >> 1) < -vz) {
                i = divscale30(i, vz);
                x1 = xs + mulscale30(vx, i);
                y1 = ys + mulscale30(vy, i);
            }
        }
        if ((x1 != 0x7fffffff) && (
                klabs(x1 - xs) + klabs(y1 - ys) < klabs((*hitx) - xs) + klabs(
                    (*hity) - ys)))
            if (PHYS_Inside(x1, y1, dasector) != 0) {
                *hitsect = dasector;
                *hitwall = -1;
                *hitsprite = -1;
                *hitx = x1;
                *hity = y1;
                *hitz = z1;
            }

        x1 = 0x7fffffff;
        if (sec->floorstat & 2) {
            wal = &wall[sec->wallptr];
            wal2 = &wall[wal->point2];
            dax = wal2->x - wal->x;
            day = wal2->y - wal->y;
            i = ksqrt(dax * dax + day * day);
            if (i == 0)
                continue;
            i = divscale15(sec->floorheinum, i);
            dax *= i;
            day *= i;

            j = (vz << 8) - dmulscale15(dax, vy, -day, vx);
            if (j != 0) {
                i = ((sec->floorz - zs) << 8) + dmulscale15(
                        dax, ys - wal->y, -day, xs - wal->x);
                if (((i ^ j) >= 0) && ((klabs(i) >> 1) < klabs(j))) {
                    i = divscale30(i, j);
                    x1 = xs + mulscale30(vx, i);
                    y1 = ys + mulscale30(vy, i);
                    z1 = zs + mulscale30(vz, i);
                }
            }
        } else if ((vz > 0) && (zs <= sec->floorz)) {
            z1 = sec->floorz;
            i = z1 - zs;
            if ((klabs(i) >> 1) < vz) {
                i = divscale30(i, vz);
                x1 = xs + mulscale30(vx, i);
                y1 = ys + mulscale30(vy, i);
            }
        }
        if ((x1 != 0x7fffffff) && (
                klabs(x1 - xs) + klabs(y1 - ys) < klabs((*hitx) - xs) + klabs(
                    (*hity) - ys)))
            if (PHYS_Inside(x1, y1, dasector) != 0) {
                *hitsect = dasector;
                *hitwall = -1;
                *hitsprite = -1;
                *hitx = x1;
                *hity = y1;
                *hitz = z1;
            }

        startwall = sec->wallptr;
        endwall = startwall + sec->wallnum;
        for (z = startwall, wal = &wall[startwall]; z < endwall; z++, wal++) {
            wal2 = &wall[wal->point2];
            x1 = wal->x;
            y1 = wal->y;
            x2 = wal2->x;
            y2 = wal2->y;

            if ((x1 - xs) * (y2 - ys) < (x2 - xs) * (y1 - ys))
                continue;
            if (rintersect(xs, ys, zs, vx, vy, vz, x1, y1, x2, y2, &intx, &inty,
                           &intz) == 0)
                continue;

            if (klabs(intx - xs) + klabs(inty - ys) >= klabs((*hitx) - xs) +
                klabs((*hity) - ys))
                continue;

            nextsector = wal->nextsector;
            if ((nextsector < 0) || (wal->cstat & dawalclipmask)) {
                *hitsect = dasector;
                *hitwall = z;
                *hitsprite = -1;
                *hitx = intx;
                *hity = inty;
                *hitz = intz;
                continue;
            }
            getzsofslope(nextsector, intx, inty, &daz, &daz2);
            if ((intz <= daz) || (intz >= daz2)) {
                *hitsect = dasector;
                *hitwall = z;
                *hitsprite = -1;
                *hitx = intx;
                *hity = inty;
                *hitz = intz;
                continue;
            }

            for (zz = tempshortnum - 1; zz >= 0; zz--)
                if (clipsectorlist[zz] == nextsector)
                    break;
            if (zz < 0)
                clipsectorlist[tempshortnum++] = nextsector;
        }

        for (z = headspritesect[dasector]; z >= 0; z = nextspritesect[z]) {
            spr = &sprite[z];
            cstat = spr->cstat;
            if ((cstat & dasprclipmask) == 0)
                continue;

            x1 = spr->x;
            y1 = spr->y;
            z1 = spr->z;
            switch (cstat & 48) {
                case 0:
                    topt = vx * (x1 - xs) + vy * (y1 - ys);
                    if (topt <= 0)
                        continue;
                    bot = vx * vx + vy * vy;
                    if (bot == 0)
                        continue;

                    intz = zs + scale(vz, topt, bot);

                    i = (tiles[spr->picnum].dim.height * spr->yrepeat << 2);

                    if (cstat & 128)
                        z1 += (i >> 1);

                    if (tiles[spr->picnum].anim_flags & 0x00ff0000)
                        z1 -= ((i32) ((i8) (
                                   (tiles[spr->picnum].anim_flags >> 16) & 255))
                               * spr->yrepeat << 2);

                    if ((intz > z1) || (intz < z1 - i))
                        continue;
                    topu = vx * (y1 - ys) - vy * (x1 - xs);

                    offx = scale(vx, topu, bot);
                    offy = scale(vy, topu, bot);
                    dist = offx * offx + offy * offy;
                    i = tiles[spr->picnum].dim.width * spr->xrepeat;
                    i *= i;
                    if (dist > (i >> 7))
                        continue;
                    intx = xs + scale(vx, topt, bot);
                    inty = ys + scale(vy, topt, bot);

                    if (klabs(intx - xs) + klabs(inty - ys) > klabs(
                            (*hitx) - xs) + klabs((*hity) - ys))
                        continue;

                    *hitsect = dasector;
                    *hitwall = -1;
                    *hitsprite = z;
                    *hitx = intx;
                    *hity = inty;
                    *hitz = intz;
                    break;
                case 16:
                    // These lines get the 2 points of the rotated sprite
                    // Given: (x1, y1) starts out as the center point
                    tilenum = spr->picnum;
                    xoff = (i32) ((i8) (
                               (tiles[tilenum].anim_flags >> 8) & 255)) + ((
                               i32) spr->xoffset);
                    if ((cstat & 4) > 0)
                        xoff = -xoff;
                    k = spr->ang;
                    l = spr->xrepeat;
                    dax = SIN(k) * l;
                    day = SIN(k + ANG270) * l;
                    l = tiles[tilenum].dim.width;
                    k = (l >> 1) + xoff;
                    x1 -= mulscale16(dax, k);
                    x2 = x1 + mulscale16(dax, l);
                    y1 -= mulscale16(day, k);
                    y2 = y1 + mulscale16(day, l);

                    if ((cstat & 64) != 0) // back side of 1-way sprite
                        if ((x1 - xs) * (y2 - ys) < (x2 - xs) * (y1 - ys))
                            continue;

                    if (rintersect(xs, ys, zs, vx, vy, vz, x1, y1, x2, y2,
                                   &intx, &inty, &intz) == 0)
                        continue;

                    if (klabs(intx - xs) + klabs(inty - ys) > klabs(
                            (*hitx) - xs) + klabs((*hity) - ys))
                        continue;

                    k = ((tiles[spr->picnum].dim.height * spr->yrepeat) << 2);
                    if (cstat & 128)
                        daz = spr->z + (k >> 1);
                    else
                        daz = spr->z;

                    if (tiles[spr->picnum].anim_flags & 0x00ff0000)
                        daz -= ((i32) ((i8) (
                                    (tiles[spr->picnum].anim_flags >> 16) & 255))
                                * spr->yrepeat << 2);

                    if ((intz < daz) && (intz > daz - k)) {
                        *hitsect = dasector;
                        *hitwall = -1;
                        *hitsprite = z;
                        *hitx = intx;
                        *hity = inty;
                        *hitz = intz;
                    }
                    break;
                case 32:
                    if (vz == 0)
                        continue;
                    intz = z1;
                    if (((intz - zs) ^ vz) < 0)
                        continue;
                    if ((cstat & 64) != 0)
                        if ((zs > intz) == ((cstat & 8) == 0))
                            continue;

                    intx = xs + scale(intz - zs, vx, vz);
                    inty = ys + scale(intz - zs, vy, vz);

                    if (klabs(intx - xs) + klabs(inty - ys) > klabs(
                            (*hitx) - xs) + klabs((*hity) - ys))
                        continue;

                    tilenum = spr->picnum;
                    xoff = (i32) ((i8) (
                               (tiles[tilenum].anim_flags >> 8) & 255)) + ((
                               i32) spr->xoffset);
                    yoff = (i32) ((i8) (
                               (tiles[tilenum].anim_flags >> 16) & 255)) + ((
                               i32) spr->yoffset);
                    if ((cstat & 4) > 0)
                        xoff = -xoff;
                    if ((cstat & 8) > 0)
                        yoff = -yoff;

                    ang = spr->ang;
                    cosang = COS(ang);
                    sinang = SIN(ang);
                    xspan = tiles[tilenum].dim.width;
                    xrepeat = spr->xrepeat;
                    yspan = tiles[tilenum].dim.height;
                    yrepeat = spr->yrepeat;

                    dax = ((xspan >> 1) + xoff) * xrepeat;
                    day = ((yspan >> 1) + yoff) * yrepeat;
                    x1 += dmulscale16(sinang, dax, cosang, day) - intx;
                    y1 += dmulscale16(sinang, day, -cosang, dax) - inty;
                    l = xspan * xrepeat;
                    x2 = x1 - mulscale16(sinang, l);
                    y2 = y1 + mulscale16(cosang, l);
                    l = yspan * yrepeat;
                    k = -mulscale16(cosang, l);
                    x3 = x2 + k;
                    x4 = x1 + k;
                    k = -mulscale16(sinang, l);
                    y3 = y2 + k;
                    y4 = y1 + k;

                    clipyou = 0;
                    if ((y1 ^ y2) < 0) {
                        if ((x1 ^ x2) < 0)
                            clipyou ^= (x1 * y2 < x2 * y1) ^ (y1 < y2);
                        else
                            if (x1 >= 0)
                                clipyou ^= 1;
                    }
                    if ((y2 ^ y3) < 0) {
                        if ((x2 ^ x3) < 0)
                            clipyou ^= (x2 * y3 < x3 * y2) ^ (y2 < y3);
                        else
                            if (x2 >= 0)
                                clipyou ^= 1;
                    }
                    if ((y3 ^ y4) < 0) {
                        if ((x3 ^ x4) < 0)
                            clipyou ^= (x3 * y4 < x4 * y3) ^ (y3 < y4);
                        else
                            if (x3 >= 0)
                                clipyou ^= 1;
                    }
                    if ((y4 ^ y1) < 0) {
                        if ((x4 ^ x1) < 0)
                            clipyou ^= (x4 * y1 < x1 * y4) ^ (y4 < y1);
                        else
                            if (x4 >= 0)
                                clipyou ^= 1;
                    }

                    if (clipyou != 0) {
                        *hitsect = dasector;
                        *hitwall = -1;
                        *hitsprite = z;
                        *hitx = intx;
                        *hity = inty;
                        *hitz = intz;
                    }
                    break;
            }
        }
        tempshortcnt++;
    } while (tempshortcnt < tempshortnum);
    return (0);
}


int PHYS_NearTag(
    i32 xs,
    i32 ys,
    i32 zs,
    short sectnum,
    short ange,
    short* neartagsector,
    short* neartagwall,
    short* neartagsprite,
    i32* neartaghitdist,
    i32 neartagrange,
    u8 tagsearch
) {
    walltype *wal, *wal2;
    spritetype* spr;
    i32 i, z, zz, xe, ye, ze, x1, y1, z1, x2, y2, intx, inty, intz;
    i32 topt, topu, bot, dist, offx, offy, vx, vy, vz;
    short tempshortcnt, tempshortnum, dasector, startwall, endwall;
    short nextsector, good;

    *neartagsector = -1;
    *neartagwall = -1;
    *neartagsprite = -1;
    *neartaghitdist = 0;

    if (sectnum < 0)
        return (0);
    if ((tagsearch < 1) || (tagsearch > 3))
        return (0);

    vx = mulscale14(COS(ange), neartagrange);
    xe = xs + vx;
    vy = mulscale14(SIN(ange), neartagrange);
    ye = ys + vy;
    vz = 0;
    ze = 0;

    clipsectorlist[0] = sectnum;
    tempshortcnt = 0;
    tempshortnum = 1;

    do {
        dasector = clipsectorlist[tempshortcnt];

        startwall = sector[dasector].wallptr;
        endwall = startwall + sector[dasector].wallnum - 1;
        for (z = startwall, wal = &wall[startwall]; z <= endwall; z++, wal++) {
            wal2 = &wall[wal->point2];
            x1 = wal->x;
            y1 = wal->y;
            x2 = wal2->x;
            y2 = wal2->y;

            nextsector = wal->nextsector;

            good = 0;
            if (nextsector >= 0) {
                if ((tagsearch & 1) && sector[nextsector].lotag)
                    good |= 1;
                if ((tagsearch & 2) && sector[nextsector].hitag)
                    good |= 1;
            }
            if ((tagsearch & 1) && wal->lotag)
                good |= 2;
            if ((tagsearch & 2) && wal->hitag)
                good |= 2;

            if ((good == 0) && (nextsector < 0))
                continue;
            if ((x1 - xs) * (y2 - ys) < (x2 - xs) * (y1 - ys))
                continue;

            if (lintersect(xs, ys, zs, xe, ye, ze, x1, y1, x2, y2, &intx, &inty,
                           &intz) == 1) {
                if (good != 0) {
                    if (good & 1)
                        *neartagsector = nextsector;
                    if (good & 2)
                        *neartagwall = z;
                    *neartaghitdist = dmulscale14(
                        intx - xs, COS(ange), inty - ys,
                        SIN(ange));
                    xe = intx;
                    ye = inty;
                    ze = intz;
                }
                if (nextsector >= 0) {
                    for (zz = tempshortnum - 1; zz >= 0; zz--)
                        if (clipsectorlist[zz] == nextsector)
                            break;
                    if (zz < 0)
                        clipsectorlist[tempshortnum++] = nextsector;
                }
            }
        }

        for (z = headspritesect[dasector]; z >= 0; z = nextspritesect[z]) {
            spr = &sprite[z];

            good = 0;
            if ((tagsearch & 1) && spr->lotag)
                good |= 1;
            if ((tagsearch & 2) && spr->hitag)
                good |= 1;
            if (good != 0) {
                x1 = spr->x;
                y1 = spr->y;
                z1 = spr->z;

                topt = vx * (x1 - xs) + vy * (y1 - ys);
                if (topt > 0) {
                    bot = vx * vx + vy * vy;
                    if (bot != 0) {
                        intz = zs + scale(vz, topt, bot);
                        i = tiles[spr->picnum].dim.height * spr->yrepeat;
                        if (spr->cstat & 128)
                            z1 += (i << 1);
                        if (tiles[spr->picnum].anim_flags & 0x00ff0000)
                            z1 -= ((i32) ((i8) (
                                       (tiles[spr->picnum].anim_flags >> 16) &
                                       255)) * spr->yrepeat << 2);
                        if ((intz <= z1) && (intz >= z1 - (i << 2))) {
                            topu = vx * (y1 - ys) - vy * (x1 - xs);

                            offx = scale(vx, topu, bot);
                            offy = scale(vy, topu, bot);
                            dist = offx * offx + offy * offy;
                            i = (tiles[spr->picnum].dim.width * spr->xrepeat);
                            i *= i;
                            if (dist <= (i >> 7)) {
                                intx = xs + scale(vx, topt, bot);
                                inty = ys + scale(vy, topt, bot);
                                if (klabs(intx - xs) + klabs(inty - ys) < klabs(
                                        xe - xs) + klabs(ye - ys)) {
                                    *neartagsprite = z;
                                    *neartaghitdist = dmulscale14(
                                        intx - xs,
                                        COS(ange),
                                        inty - ys,
                                        SIN(ange));
                                    xe = intx;
                                    ye = inty;
                                    ze = intz;
                                }
                            }
                        }
                    }
                }
            }
        }

        tempshortcnt++;
    } while (tempshortcnt < tempshortnum);
    return (0);
}


int PHYS_CanSee(
    i32 x1,
    i32 y1,
    i32 z1,
    short sect1,
    i32 x2,
    i32 y2,
    i32 z2,
    short sect2
) {
    sectortype* sec;
    walltype *wal, *wal2;
    i32 i, cnt, nexts, x, y, z, cz, fz, dasectnum, dacnt, danum;
    i32 x21, y21, z21, x31, y31, x34, y34, bot, t;

    if ((x1 == x2) && (y1 == y2))
        return (sect1 == sect2);

    x21 = x2 - x1;
    y21 = y2 - y1;
    z21 = z2 - z1;

    clipsectorlist[0] = sect1;
    danum = 1;
    for (dacnt = 0; dacnt < danum; dacnt++) {
        dasectnum = clipsectorlist[dacnt];
        sec = &sector[dasectnum];
        for (cnt = sec->wallnum, wal = &wall[sec->wallptr]; cnt > 0;
             cnt--, wal++) {
            wal2 = &wall[wal->point2];
            x31 = wal->x - x1;
            x34 = wal->x - wal2->x;
            y31 = wal->y - y1;
            y34 = wal->y - wal2->y;

            bot = y21 * x34 - x21 * y34;
            if (bot <= 0)
                continue;

            t = y21 * x31 - x21 * y31;
            if ((u32) t >= (u32) bot)
                continue;
            t = y31 * x34 - x31 * y34;
            if ((u32) t >= (u32) bot)
                continue;

            nexts = wal->nextsector;
            if ((nexts < 0) || (wal->cstat & 32))
                return (0);

            t = divscale24(t, bot);
            x = x1 + mulscale24(x21, t);
            y = y1 + mulscale24(y21, t);
            z = z1 + mulscale24(z21, t);

            getzsofslope((short) dasectnum, x, y, &cz, &fz);
            if ((z <= cz) || (z >= fz))
                return (0);
            getzsofslope((short) nexts, x, y, &cz, &fz);
            if ((z <= cz) || (z >= fz))
                return (0);

            for (i = danum - 1; i >= 0; i--)
                if (clipsectorlist[i] == nexts)
                    break;
            if (i < 0)
                clipsectorlist[danum++] = nexts;
        }
    }
    for (i = danum - 1; i >= 0; i--)
        if (clipsectorlist[i] == sect2)
            return (1);
    return (0);
}


//
// x and y are the new position of the entity that has just moved:
// lastKnownSector is a hint (the last known sectorID of the entity).
//
// Thanks to the "hint", the algorithm check:
// 1. Is (x,y) inside sectors[sectnum].
// 2. Flood in sectnum portal and check again if (x,y) is inside.
// 3. Do a linear search on sectors[sectnum] from 0 to numSectors.
//
// Note: Inside uses cross product and return as soon as the point
// switch from one side to the other.
//
void PHYS_UpdateSector(i32 x, i32 y, short* lastKnownSector) {
    walltype* wal;
    i32 i, j;

    // First check the last sector where (old_x,old_y) was
    // before being updated to (x,y).
    if (PHYS_Inside(x, y, *lastKnownSector) == 1) {
        // We found it and (x,y) is still in the same sector,
        // so nothing to update!
        return;
    }

    // Seems (x,y) moved into another sector.
    // Hopefully one connected via a portal.
    // Let's flood in each portal.
    if ((*lastKnownSector >= 0) && (*lastKnownSector < numsectors)) {
        wal = &wall[sector[*lastKnownSector].wallptr];
        j = sector[*lastKnownSector].wallnum;
        do {
            i = wal->nextsector;
            if (i >= 0)
                if (PHYS_Inside(x, y, (short) i) == 1) {
                    *lastKnownSector = i;
                    return;
                }
            wal++;
            j--;
        } while (j != 0);
    }

    // Damn that is a BIG move, still cannot find which sector
    // (x,y) belongs to. Let's search via linear search.
    for (i = numsectors - 1; i >= 0; i--) {
        if (PHYS_Inside(x, y, (short) i) == 1) {
            *lastKnownSector = i;
            return;
        }
    }

    // (x,y) is contained in NO sector.
    // (x,y) is likely out of the map.
    *lastKnownSector = -1;
}


//
// Return true if the point (x, y) is inside the sector sectnum.
// Note that a sector is closed (but can be concave) so the answer is always 0 or 1.
//
// Algorithm: This is an optimized raycasting inside polygon test:
// http://en.wikipedia.org/wiki/Point_in_polygon#Ray_casting_algorithm
// The goal is to follow an ***horizontal*** ray passing by (x,y) and count how many
// wall are being crossed.
// If it is an odd number of time: (x,y) is inside the sector.
// If it is an even nymber of time:(x,y) is outside the sector.
//
int PHYS_Inside(i32 x, i32 y, short sectnum) {
    walltype* wal;
    i32 i, x1, y1, x2, y2;
    u32 wallCrossed;

    // Quick check if the sector ID is valid.
    if ((sectnum < 0) || (sectnum >= numsectors))
        return (-1);

    wallCrossed = 0;
    wal = &wall[sector[sectnum].wallptr];
    i = sector[sectnum].wallnum;
    do {
        y1 = wal->y - y;
        y2 = wall[wal->point2].y - y;

        // Compare the sign of y1 and y2.
        // If (y1^y2) < 0:
        //   y1 and y2 have different sign bit, so y is between
        //   wal->y and wall[wal->point2].y.
        // The goal is to not take into consideration any wall that
        // is totally above or totally under the point [x,y].
        if ((y1 ^ y2) < 0) {
            x1 = wal->x - x;
            x2 = wall[wal->point2].x - x;

            // If (x1^x2) >= 0 x1 and x2 have identical sign bit:
            //   x is on the left or the right of both
            //   wal->x and wall[wal->point2].x.
            if ((x1 ^ x2) >= 0) {
                // If (x,y) is totally on the left or on the right, just
                // count x1 (which indicate if we are on the left or on the right).
                wallCrossed ^= x1;
            } else {
                // This is the most complicated case:
                // x is between x1 and x2, we need a fine-grained test.
                // We need to know exactly if it is on the left or on
                // the right in order to know if the ray is crossing the
                // wall or not.
                // The sign of the cross-product can answer this case.
                wallCrossed ^= (x1 * y2 - x2 * y1) ^ y2;
            }
        }

        wal++;
        i--;

    } while (i);

    // Just return the sign.
    // If the position vector cut the sector walls an odd number
    // of time it is inside. Otherwise (even) it is outside.
    return (wallCrossed >> 31);
}


int PHYS_ClipInsideBox(i32 x, i32 y, short wallnum, i32 walldist) {
    walltype* wal;
    i32 x1, y1, x2, y2, r;

    r = (walldist << 1);
    wal = &wall[wallnum];
    x1 = wal->x + walldist - x;
    y1 = wal->y + walldist - y;
    wal = &wall[wal->point2];
    x2 = wal->x + walldist - x;
    y2 = wal->y + walldist - y;

    if ((x1 < 0) && (x2 < 0))
        return (0);
    if ((y1 < 0) && (y2 < 0))
        return (0);
    if ((x1 >= r) && (x2 >= r))
        return (0);
    if ((y1 >= r) && (y2 >= r))
        return (0);

    x2 -= x1;
    y2 -= y1;
    if (x2 * (walldist - y1) >= y2 * (walldist - x1)) // Front.
    {
        if (x2 > 0)
            x2 *= (0 - y1);
        else
            x2 *= (r - y1);
        if (y2 > 0)
            y2 *= (r - x1);
        else
            y2 *= (0 - x1);
        return (x2 < y2);
    }
    if (x2 > 0)
        x2 *= (r - y1);
    else
        x2 *= (0 - y1);
    if (y2 > 0)
        y2 *= (0 - x1);
    else
        y2 *= (r - x1);
    return ((x2 >= y2) << 1);
}


void PHYS_DragPoint(short pointhighlight, i32 dax, i32 day) {
    short cnt, tempshort;

    wall[pointhighlight].x = dax;
    wall[pointhighlight].y = day;

    cnt = MAXWALLS;
    tempshort = pointhighlight; // search points CCW.
    do {
        if (wall[tempshort].nextwall >= 0) {
            tempshort = wall[wall[tempshort].nextwall].point2;
            wall[tempshort].x = dax;
            wall[tempshort].y = day;
        } else {
            tempshort = pointhighlight;
            // search points CW if not searched all the way around.
            do {
                if (wall[lastwall(tempshort)].nextwall >= 0) {
                    tempshort = wall[lastwall(tempshort)].nextwall;
                    wall[tempshort].x = dax;
                    wall[tempshort].y = day;
                } else {
                    break;
                }
                cnt--;
            } while ((tempshort != pointhighlight) && (cnt > 0));
            break;
        }
        cnt--;
    } while ((tempshort != pointhighlight) && (cnt > 0));
}
