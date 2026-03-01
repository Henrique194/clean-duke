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

#include "am_overhead.h"
#include "names.h"
#include "build/engine.h"
#include "funct.h"

static i32 xvect;
static i32 yvect;
static i32 xvect2;
static i32 yvect2;



static void AM_DrawWall(const walltype* wal, i32 posx, i32 posy, u8 col) {
    i32 ox = wal->x - posx;
    i32 oy = wal->y - posy;
    i32 x1 = dmulscale16(ox, xvect, -oy, yvect) + (xdim << 11);
    i32 y1 = dmulscale16(oy, xvect2, ox, yvect2) + (ydim << 11);

    const walltype* wal2 = &wall[wal->point2];
    ox = wal2->x - posx;
    oy = wal2->y - posy;
    i32 x2 = dmulscale16(ox, xvect, -oy, yvect) + (xdim << 11);
    i32 y2 = dmulscale16(oy, xvect2, ox, yvect2) + (ydim << 11);

    drawline256(x1, y1, x2, y2, col);
}

static bool AM_CanDrawWall(const sectortype* sec, const walltype* wal) {
    if (wal->nextwall < 0) {
        return false;
    }
    if (SEC_VIS(wal->nextsector)) {
        return false;
    }
    const sectortype* next_sec = &sector[wal->nextsector];
    if (next_sec->ceilingz != sec->ceilingz) {
        return true;
    }
    if (next_sec->floorz != sec->floorz) {
        return true;
    }
    const walltype* next_wall = &wall[wal->nextwall];
    return ((wal->cstat | next_wall->cstat) & (16 + 32)) != 0;
}

static void AM_DrawSecWalls(const sectortype* sec, i32 posx, i32 posy) {
    i32 first_wall = sec->wallptr;
    i32 last_wall = sec->wallptr + sec->wallnum;
    for (i32 i = first_wall; i < last_wall; i++) {
        const walltype* wal = &wall[i];
        if (AM_CanDrawWall(sec, wal)) {
            AM_DrawWall(wal, posx, posy, 24);
        }
    }
}

static void AM_DrawRedLines(i32 posx, i32 posy) {
    for (i32 sec = 0; sec < numsectors; sec++) {
        if (SEC_VIS(sec)) {
            AM_DrawSecWalls(&sector[sec], posx, posy);
        }
    }
}


static void AM_DrawWallSprite(const spritetype* spr, i32 posx, i32 posy, u8 col) {
    if (spr->picnum != LASERLINE) {
        return;
    }

    i32 tilenum = spr->picnum;
    i32 xoff = (i32) ((i8) ((tiles[tilenum].anim_flags >> 8) & 255)) + ((i32) spr->xoffset);
    if ((spr->cstat & 4) > 0) {
        xoff = -xoff;
    }
    i32 dax = SIN(spr->ang) * spr->xrepeat;
    i32 day = SIN(spr->ang + ANG270) * spr->xrepeat;
    i32 l = tiles[tilenum].dim.width;
    i32 k = (l >> 1) + xoff;
    i32 x1 = spr->x - mulscale16(dax, k);
    i32 x2 = x1 + mulscale16(dax, l);
    i32 y1 = spr->y - mulscale16(day, k);
    i32 y2 = y1 + mulscale16(day, l);

    i32 ox = x1 - posx;
    i32 oy = y1 - posy;
    x1 = dmulscale16(ox, xvect, -oy, yvect);
    y1 = dmulscale16(oy, xvect2, ox, yvect2);

    ox = x2 - posx;
    oy = y2 - posy;
    x2 = dmulscale16(ox, xvect, -oy, yvect);
    y2 = dmulscale16(oy, xvect2, ox, yvect2);

    drawline256(x1 + (xdim << 11), y1 + (ydim << 11), x2 + (xdim << 11), y2 + (ydim << 11), col);
}

static void AM_DrawFloorSprite(const spritetype* spr, i32 posx, i32 posy, u8 col) {
    i32 tilenum = spr->picnum;
    i32 xoff = (i32) ((i8) ((tiles[tilenum].anim_flags >> 8) & 255)) + ((i32) spr->xoffset);
    i32 yoff = (i32) ((i8) ((tiles[tilenum].anim_flags >> 16) & 255)) + ((i32) spr->yoffset);
    if ((spr->cstat & 4) > 0) {
        xoff = -xoff;
    }
    if ((spr->cstat & 8) > 0) {
        yoff = -yoff;
    }

    i32 k = spr->ang;
    i32 cosang = COS(k);
    i32 sinang = SIN(k);
    i32 xspan = tiles[tilenum].dim.width;
    i32 xrepeat = spr->xrepeat;
    i32 yspan = tiles[tilenum].dim.height;
    i32 yrepeat = spr->yrepeat;

    i32 dax = ((xspan >> 1) + xoff) * xrepeat;
    i32 day = ((yspan >> 1) + yoff) * yrepeat;
    i32 x1 = spr->x + dmulscale16(sinang, dax, cosang, day);
    i32 y1 = spr->y + dmulscale16(sinang, day, -cosang, dax);
    i32 l = xspan * xrepeat;
    i32 x2 = x1 - mulscale16(sinang, l);
    i32 y2 = y1 + mulscale16(cosang, l);
    l = yspan * yrepeat;
    k = -mulscale16(cosang, l);
    i32 x3 = x2 + k;
    i32 x4 = x1 + k;
    k = -mulscale16(sinang, l);
    i32 y3 = y2 + k;
    i32 y4 = y1 + k;

    i32 ox = x1 - posx;
    i32 oy = y1 - posy;
    x1 = dmulscale16(ox, xvect, -oy, yvect);
    y1 = dmulscale16(oy, xvect2, ox, yvect2);

    ox = x2 - posx;
    oy = y2 - posy;
    x2 = dmulscale16(ox, xvect, -oy, yvect);
    y2 = dmulscale16(oy, xvect2, ox, yvect2);

    ox = x3 - posx;
    oy = y3 - posy;
    x3 = dmulscale16(ox, xvect, -oy, yvect);
    y3 = dmulscale16(oy, xvect2, ox, yvect2);

    ox = x4 - posx;
    oy = y4 - posy;
    x4 = dmulscale16(ox, xvect, -oy, yvect);
    y4 = dmulscale16(oy, xvect2, ox, yvect2);

    drawline256(x1 + (xdim << 11), y1 + (ydim << 11), x2 + (xdim << 11), y2 + (ydim << 11), col);

    drawline256(x2 + (xdim << 11), y2 + (ydim << 11), x3 + (xdim << 11), y3 + (ydim << 11), col);

    drawline256(x3 + (xdim << 11), y3 + (ydim << 11), x4 + (xdim << 11), y4 + (ydim << 11), col);

    drawline256(x4 + (xdim << 11), y4 + (ydim << 11), x1 + (xdim << 11), y1 + (ydim << 11), col);
}

static void AM_DrawSprite(const spritetype* spr, i32 posx, i32 posy) {
    u8 col;
    if (spr->cstat & 1) {
        col = 234; // magenta
    } else {
        col = 71; // cyan
    }
    switch (spr->cstat & 48) {
        case 16:
            AM_DrawWallSprite(spr, posx, posy, col);
            break;
        case 32:
            AM_DrawFloorSprite(spr, posx, posy, col);
            break;
        default:
            break;
    }
}

static bool AM_CanDrawSprite(i32 spr_idx) {
    const spritetype* spr = &sprite[spr_idx];
    i32 plr_idx = ps[screenpeek].i;
    if (!(spr->cstat & 257)) {
        return false;
    }
    if (spr_idx == plr_idx) {
        return false;
    }
    return !(spr->cstat & 0x8000) && spr->cstat != 257 && spr->xrepeat;
}

static void AM_DrawSecSprites(i32 sec, i32 posx, i32 posy) {
    i32 spr = headspritesect[sec];
    while (spr >= 0) {
        if (AM_CanDrawSprite(spr)) {
            AM_DrawSprite(&sprite[spr], posx, posy);
        }
        spr = nextspritesect[spr];
    }
}

static void AM_DrawSprites(i32 posx, i32 posy) {
    for (i32 sec = 0; sec < numsectors; sec++) {
        if (SEC_VIS(sec)) {
            AM_DrawSecSprites(sec, posx, posy);
        }
    }
}


static void AM_DrawWhiteLines(i32 posx, i32 posy) {
    for (i32 i = 0; i < numsectors; i++) {
        if (!SEC_VIS(i)) {
            continue;
        }

        i32 startwall = sector[i].wallptr;
        i32 endwall = sector[i].wallptr + sector[i].wallnum;

        i32 j = startwall;
        const walltype* wal = &wall[startwall];
        for (; j < endwall; j++, wal++) {
            if (wal->nextwall >= 0) {
                continue;
            }
            if (tiles[wal->picnum].dim.width == 0) {
                continue;
            }
            if (tiles[wal->picnum].dim.height == 0) {
                continue;
            }
            AM_DrawWall(wal, posx, posy, 24);
        }
    }
}


void AM_DrawOverhead(i32 posx, i32 posy, i32 zoom, i16 ang) {
    i32 i;
    i32 j;
    i32 x1;
    i32 y1;
    i32 ox;
    i32 oy;
    i32 daang;
    short p;
    
    xvect = COS(ANG270 - ang) * zoom;
    yvect = SIN(ANG270 - ang) * zoom;
    xvect2 = mulscale16(xvect, yxaspect);
    yvect2 = mulscale16(yvect, yxaspect);

    AM_DrawRedLines(posx, posy);
    AM_DrawSprites(posx, posy);
    AM_DrawWhiteLines(posx, posy);

    for (p = connecthead; p >= 0; p = connectpoint2[p]) {
        if (ud.scrollmode && p == screenpeek)
            continue;

        ox = sprite[ps[p].i].x - posx;
        oy = sprite[ps[p].i].y - posy;
        daang = (sprite[ps[p].i].ang - ang) & 2047;
        if (p == screenpeek) {
            ox = 0;
            oy = 0;
            daang = 0;
        }
        x1 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
        y1 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

        if (p == screenpeek || ud.coop == 1) {
            if (sprite[ps[p].i].xvel > 16 && ps[p].on_ground)
                i = APLAYERTOP + ((totalclock >> 4) & 3);
            else
                i = APLAYERTOP;

            j = klabs(ps[p].truefz - ps[p].posz) >> 8;
            j = mulscale(zoom * (sprite[ps[p].i].yrepeat + j), yxaspect, 16);

            if (j < 22000)
                j = 22000;
            else if (j > (65536 << 1))
                j = (65536 << 1);

            rotatesprite((x1 << 4) + (xdim << 15), (y1 << 4) + (ydim << 15), j,
                         daang, i, sprite[ps[p].i].shade, sprite[ps[p].i].pal,
                         (sprite[ps[p].i].cstat & 2) >> 1, windowx1, windowy1,
                         windowx2, windowy2);
        }
    }
}
