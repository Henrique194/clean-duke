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


extern int32_t xyaspect;
extern int32_t viewingrangerecip;
extern int32_t transarea;
extern int32_t globalpal;
extern int32_t lastx[MAXYDIM];
extern int32_t rxi[8];
extern int32_t ryi[8];
extern int32_t rzi[8];
extern int32_t rxi2[8];
extern int32_t ryi2[8];
extern int32_t rzi2[8];
extern int32_t xsi[8];
extern int32_t ysi[8];

extern short uplc[MAXXDIM + 1];
extern short dplc[MAXXDIM + 1];

extern int32_t beforedrawrooms;


static int32_t spritesx[MAXSPRITESONSCREEN];
static int32_t spritesy[MAXSPRITESONSCREEN+1];
static int32_t spritesz[MAXSPRITESONSCREEN];
static spritetype* tspriteptr[MAXSPRITESONSCREEN];


/* Assume npoints=4 with polygon on &rx1,&ry1 */
// FCS This is horrible to read: I hate you.
static int clippoly4(int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2) {
    int32_t n, nn, z, zz, x, x1, x2, y, y1, y2, t;

    nn = 0;
    z = 0;
    do {
        zz = ((z + 1) & 3);


        x1 = pv_walls[z].cameraSpaceCoo[0][VEC_X];
        x2 = pv_walls[zz].cameraSpaceCoo[0][VEC_X] - x1;

        if ((cx1 <= x1) && (x1 <= cx2)) {
            pv_walls[nn].cameraSpaceCoo[1][VEC_X] = x1;
            pv_walls[nn].cameraSpaceCoo[1][VEC_Y] = pv_walls[z].cameraSpaceCoo[0][
                VEC_Y];
            nn++;
        }

        if (x2 <= 0)
            x = cx2;
        else
            x = cx1;

        t = x - x1;

        if (((t - x2) ^ t) < 0) {
            pv_walls[nn].cameraSpaceCoo[1][VEC_X] = x;
            pv_walls[nn].cameraSpaceCoo[1][VEC_Y] =
                pv_walls[z].cameraSpaceCoo[0][VEC_Y] +
                scale(t, pv_walls[zz].cameraSpaceCoo[0][VEC_Y] - pv_walls[z].
                         cameraSpaceCoo[0][VEC_Y], x2);
            nn++;
        }

        if (x2 <= 0)
            x = cx1;
        else
            x = cx2;

        t = x - x1;

        if (((t - x2) ^ t) < 0) {
            pv_walls[nn].cameraSpaceCoo[1][VEC_X] = x;
            pv_walls[nn].cameraSpaceCoo[1][VEC_Y] =
                pv_walls[z].cameraSpaceCoo[0][VEC_Y] +
                scale(t, pv_walls[zz].cameraSpaceCoo[0][VEC_Y] - pv_walls[z].
                         cameraSpaceCoo[0][VEC_Y], x2);
            nn++;
        }
        z = zz;
    } while (z != 0);
    if (nn < 3)
        return (0);

    n = 0;
    z = 0;
    do {
        zz = z + 1;
        if (zz == nn)
            zz = 0;

        y1 = pv_walls[z].cameraSpaceCoo[1][VEC_Y];
        y2 = pv_walls[zz].cameraSpaceCoo[1][VEC_Y] - y1;

        if ((cy1 <= y1) && (y1 <= cy2)) {
            pv_walls[n].cameraSpaceCoo[0][VEC_Y] = y1;
            pv_walls[n].cameraSpaceCoo[0][VEC_X] = pv_walls[z].cameraSpaceCoo[1][
                VEC_X];
            n++;
        }
        if (y2 <= 0)
            y = cy2;
        else
            y = cy1;
        t = y - y1;
        if (((t - y2) ^ t) < 0) {
            pv_walls[n].cameraSpaceCoo[0][VEC_Y] = y;
            pv_walls[n].cameraSpaceCoo[0][VEC_X] =
                pv_walls[z].cameraSpaceCoo[1][VEC_X] + scale(t,
                    pv_walls[zz].cameraSpaceCoo[1][VEC_X] -
                    pv_walls[z].cameraSpaceCoo[1][VEC_X], y2);
            n++;
        }

        if (y2 <= 0)
            y = cy1;
        else
            y = cy2;
        t = y - y1;
        if (((t - y2) ^ t) < 0) {
            pv_walls[n].cameraSpaceCoo[0][VEC_Y] = y;
            pv_walls[n].cameraSpaceCoo[0][VEC_X] =
                pv_walls[z].cameraSpaceCoo[1][VEC_X] + scale(t,
                    pv_walls[zz].cameraSpaceCoo[1][VEC_X] -
                    pv_walls[z].cameraSpaceCoo[1][VEC_X], y2);
            n++;
        }
        z = zz;
    } while (z != 0);
    return (n);
}


void dorotatesprite(
    int32_t sx,
    int32_t sy,
    int32_t z,
    short a,
    short picnum,
    int8_t dashade,
    uint8_t dapalnum,
    uint8_t dastat,
    int32_t cx1,
    int32_t cy1,
    int32_t cx2,
    int32_t cy2
) {
    int32_t cosang, sinang, v, nextv, dax1, dax2, oy, bx, by, ny1, ny2;
    int32_t i, x, y, x1, y1, x2, y2, gx1, gy1;
    uint8_t* bufplc;
    uint8_t* palookupoffs;
    uint8_t* p;
    int32_t xoff, yoff, npoints, yplc, yinc, lx, rx, xx, xend;
    int32_t xv, yv, xv2, yv2, obuffermode = 0, qlinemode = 0, y1ve[4], y2ve[4],
            u4, d4;
    uint8_t bad;

    short tileWidht, tileHeight;

    tileWidht = tiles[picnum].dim.width;
    tileHeight = tiles[picnum].dim.height;

    if (dastat & 16) {
        xoff = 0;
        yoff = 0;
    } else {
        xoff = (int32_t) ((int8_t) ((tiles[picnum].anim_flags >> 8) & 255)) + (
                   tileWidht >> 1);
        yoff = (int32_t) ((int8_t) ((tiles[picnum].anim_flags >> 16) & 255)) + (
                   tileHeight >> 1);
    }

    if (dastat & 4)
        yoff = tileHeight - yoff;

    cosang = sintable[(a + 512) & 2047];
    sinang = sintable[a & 2047];

    if ((dastat & 2) != 0) /* Auto window size scaling */
    {
        if ((dastat & 8) == 0) {
            x = xdimenscale; /* = scale(xdimen,yxaspect,320); */
            if (stereomode)
                x = scale(windowx2 - windowx1 + 1, yxaspect, 320);
            sx = ((cx1 + cx2 + 2) << 15) + scale(sx - (320 << 15), xdimen, 320);
            sy = ((cy1 + cy2 + 2) << 15) + mulscale16(sy - (200 << 15), x);
        } else {
            /*
             * If not clipping to startmosts, & auto-scaling on, as a
             *  hard-coded bonus, scale to full screen instead
             */
            x = scale(xdim, yxaspect, 320);
            sx = (xdim << 15) + 32768 + scale(sx - (320 << 15), xdim, 320);
            sy = (ydim << 15) + 32768 + mulscale16(sy - (200 << 15), x);
        }
        z = mulscale16(z, x);
    }

    xv = mulscale14(cosang, z);
    yv = mulscale14(sinang, z);
    if (((dastat & 2) != 0) || ((dastat & 8) == 0))
    /* Don't aspect unscaled perms */
    {
        xv2 = mulscale16(xv, xyaspect);
        yv2 = mulscale16(yv, xyaspect);
    } else {
        xv2 = xv;
        yv2 = yv;
    }

    //Taking care of the Y coordinates.
    pv_walls[0].cameraSpaceCoo[0][VEC_Y] = sy - (yv * xoff + xv * yoff);
    pv_walls[1].cameraSpaceCoo[0][VEC_Y] =
        pv_walls[0].cameraSpaceCoo[0][VEC_Y] + yv * tileWidht;
    pv_walls[3].cameraSpaceCoo[0][VEC_Y] =
        pv_walls[0].cameraSpaceCoo[0][VEC_Y] + xv * tileHeight;

    pv_walls[2].cameraSpaceCoo[0][VEC_Y] = pv_walls[1].cameraSpaceCoo[0][VEC_Y] +
                                          pv_walls[3].cameraSpaceCoo[0][VEC_Y] -
                                          pv_walls[0].cameraSpaceCoo[0][VEC_Y];

    i = (cy1 << 16);

    if ((pv_walls[0].cameraSpaceCoo[0][VEC_Y] < i) &&
        (pv_walls[1].cameraSpaceCoo[0][VEC_Y] < i) &&
        (pv_walls[2].cameraSpaceCoo[0][VEC_Y] < i) &&
        (pv_walls[3].cameraSpaceCoo[0][VEC_Y] < i))
        return;

    i = (cy2 << 16);

    if ((pv_walls[0].cameraSpaceCoo[0][VEC_Y] > i) &&
        (pv_walls[1].cameraSpaceCoo[0][VEC_Y] > i) &&
        (pv_walls[2].cameraSpaceCoo[0][VEC_Y] > i) &&
        (pv_walls[3].cameraSpaceCoo[0][VEC_Y] > i))
        return;


    //Taking care of the X coordinates.
    pv_walls[0].cameraSpaceCoo[0][VEC_X] = sx - (xv2 * xoff - yv2 * yoff);
    pv_walls[1].cameraSpaceCoo[0][VEC_X] =
        pv_walls[0].cameraSpaceCoo[0][VEC_X] + xv2 * tileWidht;
    pv_walls[3].cameraSpaceCoo[0][VEC_X] =
        pv_walls[0].cameraSpaceCoo[0][VEC_X] - yv2 * tileHeight;
    pv_walls[2].cameraSpaceCoo[0][VEC_X] = pv_walls[1].cameraSpaceCoo[0][VEC_X] +
                                          pv_walls[3].cameraSpaceCoo[0][VEC_X] -
                                          pv_walls[0].cameraSpaceCoo[0][VEC_X];

    i = (cx1 << 16);
    if ((pv_walls[0].cameraSpaceCoo[0][VEC_X] < i) &&
        (pv_walls[1].cameraSpaceCoo[0][VEC_X] < i) &&
        (pv_walls[2].cameraSpaceCoo[0][VEC_X] < i) &&
        (pv_walls[3].cameraSpaceCoo[0][VEC_X] < i))
        return;

    i = (cx2 << 16);
    if ((pv_walls[0].cameraSpaceCoo[0][VEC_X] > i) &&
        (pv_walls[1].cameraSpaceCoo[0][VEC_X] > i) &&
        (pv_walls[2].cameraSpaceCoo[0][VEC_X] > i) &&
        (pv_walls[3].cameraSpaceCoo[0][VEC_X] > i))
        return;

    gx1 = pv_walls[0].cameraSpaceCoo[0][VEC_X];
    gy1 = pv_walls[0].cameraSpaceCoo[0][VEC_Y];
    /* back up these before clipping */

    if ((npoints = clippoly4(cx1 << 16, cy1 << 16, (cx2 + 1) << 16,
                             (cy2 + 1) << 16)) < 3)
        return;

    lx = pv_walls[0].cameraSpaceCoo[0][VEC_X];
    rx = pv_walls[0].cameraSpaceCoo[0][VEC_X];

    nextv = 0;
    for (v = npoints - 1; v >= 0; v--) {
        x1 = pv_walls[v].cameraSpaceCoo[0][VEC_X];
        x2 = pv_walls[nextv].cameraSpaceCoo[0][VEC_X];
        dax1 = (x1 >> 16);
        if (x1 < lx)
            lx = x1;
        dax2 = (x2 >> 16);
        if (x1 > rx)
            rx = x1;
        if (dax1 != dax2) {
            y1 = pv_walls[v].cameraSpaceCoo[0][VEC_Y];
            y2 = pv_walls[nextv].cameraSpaceCoo[0][VEC_Y];
            yinc = divscale16(y2 - y1, x2 - x1);
            if (dax2 > dax1) {
                yplc = y1 + mulscale16((dax1 << 16) + 65535 - x1, yinc);
                qinterpolatedown16short((int32_t*) (&uplc[dax1]), dax2 - dax1,
                                        yplc, yinc);
            } else {
                yplc = y2 + mulscale16((dax2 << 16) + 65535 - x2, yinc);
                qinterpolatedown16short((int32_t*) (&dplc[dax2]), dax1 - dax2,
                                        yplc, yinc);
            }
        }
        nextv = v;
    }

    TILE_MakeAvailable(picnum);

    setgotpic(picnum);
    bufplc = tiles[picnum].data;

    palookupoffs = palookup[dapalnum] + (
                       getpalookup(0L, (int32_t) dashade) << 8);

    i = divscale32(1L, z);
    xv = mulscale14(sinang, i);
    yv = mulscale14(cosang, i);
    if (((dastat & 2) != 0) || ((dastat & 8) == 0))
    /* Don't aspect unscaled perms */
    {
        yv2 = mulscale16(-xv, yxaspect);
        xv2 = mulscale16(yv, yxaspect);
    } else {
        yv2 = -xv;
        xv2 = yv;
    }

    x1 = (lx >> 16);
    x2 = (rx >> 16);

    oy = 0;
    x = (x1 << 16) - 1 - gx1;
    y = (oy << 16) + 65535 - gy1;
    bx = dmulscale16(x, xv2, y, xv);
    by = dmulscale16(x, yv2, y, yv);
    if (dastat & 4) {
        yv = -yv;
        yv2 = -yv2;
        by = (tileHeight << 16) - 1 - by;
    }

    if ((vidoption == 1) && (origbuffermode == 0)) {
        if (dastat & 128) {
            obuffermode = buffermode;
            buffermode = 0;

        }
    } else if (dastat & 8)
        permanentupdate = 1;

    if ((dastat & 1) == 0) {
        if (((a & 1023) == 0) && (tileHeight <= 256))
        /* vlineasm4 has 256 high limit! */
        {
            if (dastat & 64)
                setupvlineasm(24L);
            else
                setupmvlineasm(24L);

            by <<= 8;
            yv <<= 8;
            yv2 <<= 8;

            palookupoffse[0] = palookupoffse[1] =
                               palookupoffse[2] =
                               palookupoffse[3] = palookupoffs;
            vince[0] = vince[1] = vince[2] = vince[3] = yv;

            for (x = x1; x < x2; x += 4) {
                bad = 15;
                xend = min(x2-x, 4);
                for (xx = 0; xx < xend; xx++) {
                    bx += xv2;

                    y1 = uplc[x + xx];
                    y2 = dplc[x + xx];
                    if ((dastat & 8) == 0) {
                        if (startumost[x + xx] > y1)
                            y1 = startumost[x + xx];
                        if (startdmost[x + xx] < y2)
                            y2 = startdmost[x + xx];
                    }
                    if (y2 <= y1)
                        continue;

                    by += yv * (y1 - oy);
                    oy = y1;

                    bufplce[xx] = (intptr_t) ((bx >> 16) * tileHeight + bufplc);
                    vplce[xx] = by;
                    y1ve[xx] = y1;
                    y2ve[xx] = y2 - 1;
                    bad &= ~pow2char[xx];
                }

                p = x + frameplace;

                u4 = max(max(y1ve[0],y1ve[1]), max(y1ve[2],y1ve[3]));
                d4 = min(min(y2ve[0],y2ve[1]), min(y2ve[2],y2ve[3]));

                if (dastat & 64) {
                    if ((bad != 0) || (u4 >= d4)) {
                        if (!(bad & 1))
                            prevlineasm1(vince[0], palookupoffse[0],
                                         y2ve[0] - y1ve[0], vplce[0],
                                         (const uint8_t*) bufplce[0],
                                         ylookup[y1ve[0]] + p + 0);
                        if (!(bad & 2))
                            prevlineasm1(vince[1], palookupoffse[1],
                                         y2ve[1] - y1ve[1], vplce[1],
                                         (const uint8_t*) bufplce[1],
                                         ylookup[y1ve[1]] + p + 1);
                        if (!(bad & 4))
                            prevlineasm1(vince[2], palookupoffse[2],
                                         y2ve[2] - y1ve[2], vplce[2],
                                         (const uint8_t*) bufplce[2],
                                         ylookup[y1ve[2]] + p + 2);
                        if (!(bad & 8))
                            prevlineasm1(vince[3], palookupoffse[3],
                                         y2ve[3] - y1ve[3], vplce[3],
                                         (const uint8_t*) bufplce[3],
                                         ylookup[y1ve[3]] + p + 3);
                        continue;
                    }

                    if (u4 > y1ve[0])
                        vplce[0] = prevlineasm1(vince[0], palookupoffse[0],
                                                u4 - y1ve[0] - 1, vplce[0],
                                                (const uint8_t*) bufplce[0],
                                                ylookup[y1ve[0]] + p + 0);
                    if (u4 > y1ve[1])
                        vplce[1] = prevlineasm1(vince[1], palookupoffse[1],
                                                u4 - y1ve[1] - 1, vplce[1],
                                                (const uint8_t*) bufplce[1],
                                                ylookup[y1ve[1]] + p + 1);
                    if (u4 > y1ve[2])
                        vplce[2] = prevlineasm1(vince[2], palookupoffse[2],
                                                u4 - y1ve[2] - 1, vplce[2],
                                                (const uint8_t*) bufplce[2],
                                                ylookup[y1ve[2]] + p + 2);
                    if (u4 > y1ve[3])
                        vplce[3] = prevlineasm1(vince[3], palookupoffse[3],
                                                u4 - y1ve[3] - 1, vplce[3],
                                                (const uint8_t*) bufplce[3],
                                                ylookup[y1ve[3]] + p + 3);

                    if (d4 >= u4)
                        vlineasm4(d4 - u4 + 1, ylookup[u4] + p);

                    i = (intptr_t) (p + ylookup[d4 + 1]);
                    if (y2ve[0] > d4)
                        prevlineasm1(vince[0], palookupoffse[0],
                                     y2ve[0] - d4 - 1, vplce[0],
                                     (const uint8_t*) bufplce[0],
                                     (uint8_t*) i + 0);
                    if (y2ve[1] > d4)
                        prevlineasm1(vince[1], palookupoffse[1],
                                     y2ve[1] - d4 - 1, vplce[1],
                                     (const uint8_t*) bufplce[1],
                                     (uint8_t*) i + 1);
                    if (y2ve[2] > d4)
                        prevlineasm1(vince[2], palookupoffse[2],
                                     y2ve[2] - d4 - 1, vplce[2],
                                     (const uint8_t*) bufplce[2],
                                     (uint8_t*) i + 2);
                    if (y2ve[3] > d4)
                        prevlineasm1(vince[3], palookupoffse[3],
                                     y2ve[3] - d4 - 1, vplce[3],
                                     (const uint8_t*) bufplce[3],
                                     (uint8_t*) i + 3);
                } else {
                    if ((bad != 0) || (u4 >= d4)) {
                        if (!(bad & 1))
                            mvlineasm1(vince[0], palookupoffse[0],
                                       y2ve[0] - y1ve[0], vplce[0],
                                       (const uint8_t*) bufplce[0],
                                       ylookup[y1ve[0]] + p + 0);
                        if (!(bad & 2))
                            mvlineasm1(vince[1], palookupoffse[1],
                                       y2ve[1] - y1ve[1], vplce[1],
                                       (const uint8_t*) bufplce[1],
                                       ylookup[y1ve[1]] + p + 1);
                        if (!(bad & 4))
                            mvlineasm1(vince[2], palookupoffse[2],
                                       y2ve[2] - y1ve[2], vplce[2],
                                       (const uint8_t*) bufplce[2],
                                       ylookup[y1ve[2]] + p + 2);
                        if (!(bad & 8))
                            mvlineasm1(vince[3], palookupoffse[3],
                                       y2ve[3] - y1ve[3], vplce[3],
                                       (const uint8_t*) bufplce[3],
                                       ylookup[y1ve[3]] + p + 3);
                        continue;
                    }

                    if (u4 > y1ve[0])
                        vplce[0] = mvlineasm1(vince[0], palookupoffse[0],
                                              u4 - y1ve[0] - 1, vplce[0],
                                              (const uint8_t*) bufplce[0],
                                              ylookup[y1ve[0]] + p + 0);
                    if (u4 > y1ve[1])
                        vplce[1] = mvlineasm1(vince[1], palookupoffse[1],
                                              u4 - y1ve[1] - 1, vplce[1],
                                              (const uint8_t*) bufplce[1],
                                              ylookup[y1ve[1]] + p + 1);
                    if (u4 > y1ve[2])
                        vplce[2] = mvlineasm1(vince[2], palookupoffse[2],
                                              u4 - y1ve[2] - 1, vplce[2],
                                              (const uint8_t*) bufplce[2],
                                              ylookup[y1ve[2]] + p + 2);
                    if (u4 > y1ve[3])
                        vplce[3] = mvlineasm1(vince[3], palookupoffse[3],
                                              u4 - y1ve[3] - 1, vplce[3],
                                              (const uint8_t*) bufplce[3],
                                              ylookup[y1ve[3]] + p + 3);

                    if (d4 >= u4)
                        mvlineasm4(d4 - u4 + 1, ylookup[u4] + p);

                    i = (intptr_t) (p + ylookup[d4 + 1]);
                    if (y2ve[0] > d4)
                        mvlineasm1(vince[0], palookupoffse[0], y2ve[0] - d4 - 1,
                                   vplce[0], (const uint8_t*) bufplce[0],
                                   (uint8_t*) i + 0);
                    if (y2ve[1] > d4)
                        mvlineasm1(vince[1], palookupoffse[1], y2ve[1] - d4 - 1,
                                   vplce[1], (const uint8_t*) bufplce[1],
                                   (uint8_t*) i + 1);
                    if (y2ve[2] > d4)
                        mvlineasm1(vince[2], palookupoffse[2], y2ve[2] - d4 - 1,
                                   vplce[2], (const uint8_t*) bufplce[2],
                                   (uint8_t*) i + 2);
                    if (y2ve[3] > d4)
                        mvlineasm1(vince[3], palookupoffse[3], y2ve[3] - d4 - 1,
                                   vplce[3], (const uint8_t*) bufplce[3],
                                   (uint8_t*) i + 3);
                }

                faketimerhandler();
            }
        } else {
            if (dastat & 64) {
                if ((xv2 & 0x0000ffff) == 0) {
                    qlinemode = 1;
                    setuprhlineasm4(0L, yv2 << 16,
                                    (xv2 >> 16) * tileHeight + (yv2 >> 16),
                                    palookupoffs, 0L, 0L);
                } else {
                    qlinemode = 0;
                    setuprhlineasm4(xv2 << 16, yv2 << 16,
                                    (xv2 >> 16) * tileHeight + (yv2 >> 16),
                                    palookupoffs, tileHeight, 0L);
                }
            } else
                setuprmhlineasm4(xv2 << 16, yv2 << 16,
                                 (xv2 >> 16) * tileHeight + (yv2 >> 16),
                                 palookupoffs, tileHeight, 0L);

            y1 = uplc[x1];
            if (((dastat & 8) == 0) && (startumost[x1] > y1))
                y1 = startumost[x1];
            y2 = y1;
            for (x = x1; x < x2; x++) {
                ny1 = uplc[x] - 1;
                ny2 = dplc[x];
                if ((dastat & 8) == 0) {
                    if (startumost[x] - 1 > ny1)
                        ny1 = startumost[x] - 1;
                    if (startdmost[x] < ny2)
                        ny2 = startdmost[x];
                }

                if (ny1 < ny2 - 1) {
                    if (ny1 >= y2) {
                        while (y1 < y2 - 1) {
                            y1++;
                            if ((y1 & 31) == 0)
                                faketimerhandler();

                            /* x,y1 */
                            bx += xv * (y1 - oy);
                            by += yv * (y1 - oy);
                            oy = y1;
                            if (dastat & 64) {
                                if (qlinemode)
                                    rhlineasm4(x - lastx[y1],
                                               (bx >> 16) * tileHeight + (
                                                   by >> 16) + bufplc, 0L, 0L,
                                               by << 16,
                                               ylookup[y1] + x + frameplace);
                                else
                                    rhlineasm4(x - lastx[y1],
                                               (bx >> 16) * tileHeight + (
                                                   by >> 16) + bufplc, 0L,
                                               bx << 16, by << 16,
                                               ylookup[y1] + x + frameplace);
                            } else
                                rmhlineasm4(x - lastx[y1],
                                            (bx >> 16) * tileHeight + (by >> 16)
                                            + bufplc, 0L, bx << 16, by << 16,
                                            ylookup[y1] + x + frameplace);
                        }
                        y1 = ny1;
                    } else {
                        while (y1 < ny1) {
                            y1++;
                            if ((y1 & 31) == 0)
                                faketimerhandler();

                            /* x,y1 */
                            bx += xv * (y1 - oy);
                            by += yv * (y1 - oy);
                            oy = y1;
                            if (dastat & 64) {
                                if (qlinemode)
                                    rhlineasm4(x - lastx[y1],
                                               (bx >> 16) * tileHeight + (
                                                   by >> 16) + bufplc, 0L, 0L,
                                               by << 16,
                                               ylookup[y1] + x + frameplace);
                                else
                                    rhlineasm4(x - lastx[y1],
                                               (bx >> 16) * tileHeight + (
                                                   by >> 16) + bufplc, 0L,
                                               bx << 16, by << 16,
                                               ylookup[y1] + x + frameplace);
                            } else
                                rmhlineasm4(x - lastx[y1],
                                            (bx >> 16) * tileHeight + (by >> 16)
                                            + bufplc, 0L, bx << 16, by << 16,
                                            ylookup[y1] + x + frameplace);
                        }
                        while (y1 > ny1)
                            lastx[y1--] = x;
                    }
                    while (y2 > ny2) {
                        y2--;
                        if ((y2 & 31) == 0)
                            faketimerhandler();

                        /* x,y2 */
                        bx += xv * (y2 - oy);
                        by += yv * (y2 - oy);
                        oy = y2;
                        if (dastat & 64) {
                            if (qlinemode)
                                rhlineasm4(x - lastx[y2],
                                           (bx >> 16) * tileHeight + (by >> 16)
                                           + bufplc, 0L, 0L, by << 16,
                                           ylookup[y2] + x + frameplace);
                            else
                                rhlineasm4(x - lastx[y2],
                                           (bx >> 16) * tileHeight + (by >> 16)
                                           + bufplc, 0L, bx << 16, by << 16,
                                           ylookup[y2] + x + frameplace);
                        } else
                            rmhlineasm4(x - lastx[y2],
                                        (bx >> 16) * tileHeight + (by >> 16) +
                                        bufplc, 0L, bx << 16, by << 16,
                                        ylookup[y2] + x + frameplace);
                    }
                    while (y2 < ny2)
                        lastx[y2++] = x;
                } else {
                    while (y1 < y2 - 1) {
                        y1++;
                        if ((y1 & 31) == 0)
                            faketimerhandler();

                        /* x,y1 */
                        bx += xv * (y1 - oy);
                        by += yv * (y1 - oy);
                        oy = y1;
                        if (dastat & 64) {
                            if (qlinemode)
                                rhlineasm4(x - lastx[y1],
                                           (bx >> 16) * tileHeight + (by >> 16)
                                           + bufplc, 0L, 0L, by << 16,
                                           ylookup[y1] + x + frameplace);
                            else
                                rhlineasm4(x - lastx[y1],
                                           (bx >> 16) * tileHeight + (by >> 16)
                                           + bufplc, 0L, bx << 16, by << 16,
                                           ylookup[y1] + x + frameplace);
                        } else
                            rmhlineasm4(x - lastx[y1],
                                        (bx >> 16) * tileHeight + (by >> 16) +
                                        bufplc, 0L, bx << 16, by << 16,
                                        ylookup[y1] + x + frameplace);
                    }
                    if (x == x2 - 1) {
                        bx += xv2;
                        by += yv2;
                        break;
                    }

                    y1 = uplc[x + 1];

                    if (((dastat & 8) == 0) && (startumost[x + 1] > y1))
                        y1 = startumost[x + 1];

                    y2 = y1;
                }
                bx += xv2;
                by += yv2;
            }
            while (y1 < y2 - 1) {
                y1++;
                if ((y1 & 31) == 0)
                    faketimerhandler();

                /* x2,y1 */
                bx += xv * (y1 - oy);
                by += yv * (y1 - oy);
                oy = y1;
                if (dastat & 64) {
                    if (qlinemode)
                        rhlineasm4(x2 - lastx[y1],
                                   (bx >> 16) * tileHeight + (by >> 16) +
                                   bufplc, 0L, 0L, by << 16,
                                   ylookup[y1] + x2 + frameplace);
                    else
                        rhlineasm4(x2 - lastx[y1],
                                   (bx >> 16) * tileHeight + (by >> 16) +
                                   bufplc, 0L, bx << 16, by << 16,
                                   ylookup[y1] + x2 + frameplace);
                } else
                    rmhlineasm4(x2 - lastx[y1],
                                (bx >> 16) * tileHeight + (by >> 16) + bufplc,
                                0L, bx << 16, by << 16,
                                ylookup[y1] + x2 + frameplace);
            }
        }
    } else {
        tsetupspritevline(palookupoffs, (xv >> 16) * tileHeight, xv << 16,
                          tileHeight, yv);

        if (dastat & 32)
            settrans(TRANS_REVERSE);
        else
            settrans(TRANS_NORMAL);

        for (x = x1; x < x2; x++) {
            bx += xv2;
            by += yv2;

            y1 = uplc[x];
            y2 = dplc[x];
            if ((dastat & 8) == 0) {
                if (startumost[x] > y1)
                    y1 = startumost[x];
                if (startdmost[x] < y2)
                    y2 = startdmost[x];
            }
            if (y2 <= y1)
                continue;

            switch (y1 - oy) {
                case -1:
                    bx -= xv;
                    by -= yv;
                    oy = y1;
                    break;
                case 0:
                    break;
                case 1:
                    bx += xv;
                    by += yv;
                    oy = y1;
                    break;
                default:
                    bx += xv * (y1 - oy);
                    by += yv * (y1 - oy);
                    oy = y1;
                    break;
            }

            p = ylookup[y1] + x + frameplace;

            DrawSpriteVerticalLine(by << 16, y2 - y1 + 1, bx << 16,
                                   (bx >> 16) * tileHeight + (by >> 16) +
                                   bufplc, p);
            transarea += (y2 - y1);

            faketimerhandler();
        }
    }

    if ((vidoption == 1) && (dastat & 128) && (origbuffermode == 0)) {
        buffermode = obuffermode;

    }
}


static int R_SpriteWallFront(spritetype* s, int32_t w) {
    walltype* wal;
    int32_t x1, y1;

    wal = &wall[w];
    x1 = wal->x;
    y1 = wal->y;
    wal = &wall[wal->point2];
    return (dmulscale32(wal->x - x1, s->y - y1, -(s->x - x1), wal->y - y1) >= 0);
}

static void R_CeilSpriteHline(int32_t x2, int32_t y) {
    int32_t x1, v, bx, by;

    /*
     * x = x1 + (x2-x1)t + (y1-y2)u  �  x = 160v
     * y = y1 + (y2-y1)t + (x2-x1)u  �  y = (scrx-160)v
     * z = z1 = z2                   �  z = posz + (scry-horiz)v
     */

    x1 = lastx[y];
    if (x2 < x1)
        return;

    v = mulscale20(globalzd, horizlookup[y - globalhoriz + horizycent]);
    bx = mulscale14(globalx2 * x1 + globalx1, v) + globalxpanning;
    by = mulscale14(globaly2 * x1 + globaly1, v) + globalypanning;
    asm1 = mulscale14(globalx2, v);
    asm2 = mulscale14(globaly2, v);
    asm3 = (intptr_t) palookup[globalpal] + (getpalookup(
                                                 (int32_t) mulscale28(
                                                     klabs(v), globvis),
                                                 globalshade) << 8);

    if ((globalorientation & 2) == 0)
        mhline(globalbufplc, bx, (x2 - x1) << 16, 0L, by,
               ylookup[y] + x1 + frameoffset);
    else {
        thline(globalbufplc, bx, (x2 - x1) << 16, 0L, by,
               ylookup[y] + x1 + frameoffset);
        transarea += (x2 - x1);
    }
}

static void R_CeilSpriteScan(int32_t x1, int32_t x2) {
    int32_t x, y1, y2, twall, bwall;

    y1 = uwall[x1];
    y2 = y1;
    for (x = x1; x <= x2; x++) {
        twall = uwall[x] - 1;
        bwall = dwall[x];
        if (twall < bwall - 1) {
            if (twall >= y2) {
                while (y1 < y2 - 1)
                    R_CeilSpriteHline(x - 1, ++y1);
                y1 = twall;
            } else {
                while (y1 < twall)
                    R_CeilSpriteHline(x - 1, ++y1);
                while (y1 > twall)
                    lastx[y1--] = x;
            }
            while (y2 > bwall)
                R_CeilSpriteHline(x - 1, --y2);
            while (y2 < bwall)
                lastx[y2++] = x;
        } else {
            while (y1 < y2 - 1)
                R_CeilSpriteHline(x - 1, ++y1);
            if (x == x2)
                break;
            y1 = uwall[x + 1];
            y2 = y1;
        }
    }
    while (y1 < y2 - 1)
        R_CeilSpriteHline(x2, ++y1);
    faketimerhandler();
}

//
// This renders masking sprites.
// See wallscan().
//
static void maskwallscan(int32_t x1, int32_t x2,
                         short* uwal, short* dwal,
                         int32_t* swal, int32_t* lwal) {
    int32_t x, startx, xnice, ynice;
    intptr_t i;
    uint8_t* fpalookup;
    int32_t y1ve[4], y2ve[4], u4, d4, dax, z, tileWidth, tileHeight;
    uint8_t* p;
    uint8_t bad;

    tileWidth = tiles[globalpicnum].dim.width;
    tileHeight = tiles[globalpicnum].dim.height;
    setgotpic(globalpicnum);

    if ((tileWidth <= 0) || (tileHeight <= 0))
        return;
    if ((uwal[x1] > ydimen) && (uwal[x2] > ydimen))
        return;
    if ((dwal[x1] < 0) && (dwal[x2] < 0))
        return;

    TILE_MakeAvailable(globalpicnum);

    startx = x1;

    xnice = (pow2long[picsiz[globalpicnum] & 15] == tileWidth);
    if (xnice)
        tileWidth = (tileWidth - 1);

    ynice = (pow2long[picsiz[globalpicnum] >> 4] == tileHeight);
    if (ynice)
        tileHeight = (picsiz[globalpicnum] >> 4);

    fpalookup = palookup[globalpal];

    setupmvlineasm(globalshiftval);

    x = startx;
    while ((startumost[x + windowx1] > startdmost[x + windowx1]) && (x <= x2))
        x++;

    p = x + frameoffset;

    for (; (x <= x2) && ((p - (uint8_t*) NULL) & 3); x++, p++) {
        y1ve[0] = max(uwal[x], startumost[x+windowx1]-windowy1);
        y2ve[0] = min(dwal[x], startdmost[x+windowx1]-windowy1);
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
            bufplce[0] *= tileHeight;
        else
            bufplce[0] <<= tileHeight;

        vince[0] = swal[x] * globalyscale;
        vplce[0] = globalzd + vince[0] * (y1ve[0] - globalhoriz + 1);

        mvlineasm1(vince[0], palookupoffse[0], y2ve[0] - y1ve[0] - 1, vplce[0],
                   bufplce[0] + tiles[globalpicnum].data, p + ylookup[y1ve[0]]);
    }
    for (; x <= x2 - 3; x += 4, p += 4) {
        bad = 0;
        for (z = 3, dax = x + 3; z >= 0; z--, dax--) {
            y1ve[z] = max(uwal[dax], startumost[dax+windowx1]-windowy1);
            y2ve[z] = min(dwal[dax], startdmost[dax+windowx1]-windowy1) - 1;
            if (y2ve[z] < y1ve[z]) {
                bad += pow2char[z];
                continue;
            }

            i = lwal[dax] + globalxpanning;
            if (i >= tileWidth) {
                if (xnice == 0)
                    i %= tileWidth;
                else
                    i &= tileWidth;
            }

            if (ynice == 0)
                i *= tileHeight;
            else
                i <<= tileHeight;

            bufplce[z] = (intptr_t) (tiles[globalpicnum].data + i);

            vince[z] = swal[dax] * globalyscale;
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

        if ((bad > 0) || (u4 >= d4)) {
            if (!(bad & 1))
                mvlineasm1(vince[0], palookupoffse[0], y2ve[0] - y1ve[0],
                           vplce[0], (const uint8_t*) bufplce[0],
                           ylookup[y1ve[0]] + p + 0);
            if (!(bad & 2))
                mvlineasm1(vince[1], palookupoffse[1], y2ve[1] - y1ve[1],
                           vplce[1], (const uint8_t*) bufplce[1],
                           ylookup[y1ve[1]] + p + 1);
            if (!(bad & 4))
                mvlineasm1(vince[2], palookupoffse[2], y2ve[2] - y1ve[2],
                           vplce[2], (const uint8_t*) bufplce[2],
                           ylookup[y1ve[2]] + p + 2);
            if (!(bad & 8))
                mvlineasm1(vince[3], palookupoffse[3], y2ve[3] - y1ve[3],
                           vplce[3], (const uint8_t*) bufplce[3],
                           ylookup[y1ve[3]] + p + 3);
            continue;
        }

        if (u4 > y1ve[0])
            vplce[0] = mvlineasm1(vince[0], palookupoffse[0], u4 - y1ve[0] - 1,
                                  vplce[0], (const uint8_t*) bufplce[0],
                                  ylookup[y1ve[0]] + p + 0);
        if (u4 > y1ve[1])
            vplce[1] = mvlineasm1(vince[1], palookupoffse[1], u4 - y1ve[1] - 1,
                                  vplce[1], (const uint8_t*) bufplce[1],
                                  ylookup[y1ve[1]] + p + 1);
        if (u4 > y1ve[2])
            vplce[2] = mvlineasm1(vince[2], palookupoffse[2], u4 - y1ve[2] - 1,
                                  vplce[2], (const uint8_t*) bufplce[2],
                                  ylookup[y1ve[2]] + p + 2);
        if (u4 > y1ve[3])
            vplce[3] = mvlineasm1(vince[3], palookupoffse[3], u4 - y1ve[3] - 1,
                                  vplce[3], (const uint8_t*) bufplce[3],
                                  ylookup[y1ve[3]] + p + 3);

        if (d4 >= u4)
            mvlineasm4(d4 - u4 + 1, ylookup[u4] + p);

        i = (intptr_t) (p + ylookup[d4 + 1]);
        if (y2ve[0] > d4)
            mvlineasm1(vince[0], palookupoffse[0], y2ve[0] - d4 - 1, vplce[0],
                       (const uint8_t*) bufplce[0], (uint8_t*) i + 0);
        if (y2ve[1] > d4)
            mvlineasm1(vince[1], palookupoffse[1], y2ve[1] - d4 - 1, vplce[1],
                       (const uint8_t*) bufplce[1], (uint8_t*) i + 1);
        if (y2ve[2] > d4)
            mvlineasm1(vince[2], palookupoffse[2], y2ve[2] - d4 - 1, vplce[2],
                       (const uint8_t*) bufplce[2], (uint8_t*) i + 2);
        if (y2ve[3] > d4)
            mvlineasm1(vince[3], palookupoffse[3], y2ve[3] - d4 - 1, vplce[3],
                       (const uint8_t*) bufplce[3], (uint8_t*) i + 3);
    }
    for (; x <= x2; x++, p++) {
        y1ve[0] = max(uwal[x], startumost[x+windowx1]-windowy1);
        y2ve[0] = min(dwal[x], startdmost[x+windowx1]-windowy1);
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
            bufplce[0] *= tileHeight;
        else
            bufplce[0] <<= tileHeight;

        vince[0] = swal[x] * globalyscale;
        vplce[0] = globalzd + vince[0] * (y1ve[0] - globalhoriz + 1);

        mvlineasm1(vince[0], palookupoffse[0], y2ve[0] - y1ve[0] - 1, vplce[0],
                   bufplce[0] + tiles[globalpicnum].data, p + ylookup[y1ve[0]]);
    }
    faketimerhandler();
}

static void R_TransMaskVline(int32_t x) {
    int32_t vplc, vinc, i;
    uint8_t* palookupoffs;
    uint8_t* bufplc;
    uint8_t* p;
    short y1v, y2v;

    if ((x < 0) || (x >= xdimen))
        return;

    y1v = max(uwall[x], startumost[x+windowx1]-windowy1);
    y2v = min(dwall[x], startdmost[x+windowx1]-windowy1);
    y2v--;
    if (y2v < y1v)
        return;

    palookupoffs = palookup[globalpal] + (getpalookup(
                                              (int32_t) mulscale16(
                                                  swall[x], globvis),
                                              globalshade) << 8);

    vinc = swall[x] * globalyscale;
    vplc = globalzd + vinc * (y1v - globalhoriz + 1);

    i = lwall[x] + globalxpanning;

    if (i >= tiles[globalpicnum].dim.width)
        i %= tiles[globalpicnum].dim.width;

    bufplc = tiles[globalpicnum].data + i * tiles[globalpicnum].dim.height;

    p = ylookup[y1v] + x + frameoffset;

    tvlineasm1(vinc, palookupoffs, y2v - y1v, vplc, bufplc, p);

    transarea += y2v - y1v;
}


static void R_TransMaskVline2(int32_t x) {
    int32_t y1, y2, x2;
    intptr_t i;
    short y1ve[2], y2ve[2];

    if ((x < 0) || (x >= xdimen))
        return;
    if (x == xdimen - 1) {
        R_TransMaskVline(x);
        return;
    }

    x2 = x + 1;

    y1ve[0] = max(uwall[x], startumost[x+windowx1]-windowy1);
    y2ve[0] = min(dwall[x], startdmost[x+windowx1]-windowy1) - 1;
    if (y2ve[0] < y1ve[0]) {
        R_TransMaskVline(x2);
        return;
    }
    y1ve[1] = max(uwall[x2], startumost[x2+windowx1]-windowy1);
    y2ve[1] = min(dwall[x2], startdmost[x2+windowx1]-windowy1) - 1;
    if (y2ve[1] < y1ve[1]) {
        R_TransMaskVline(x);
        return;
    }

    palookupoffse[0] = palookup[globalpal] + (getpalookup(
                                                  (int32_t) mulscale16(
                                                      swall[x], globvis),
                                                  globalshade) << 8);
    palookupoffse[1] = palookup[globalpal] + (getpalookup(
                                                  (int32_t) mulscale16(
                                                      swall[x2], globvis),
                                                  globalshade) << 8);

    setuptvlineasm2(globalshiftval, palookupoffse[0], palookupoffse[1]);

    vince[0] = swall[x] * globalyscale;
    vince[1] = swall[x2] * globalyscale;
    vplce[0] = globalzd + vince[0] * (y1ve[0] - globalhoriz + 1);
    vplce[1] = globalzd + vince[1] * (y1ve[1] - globalhoriz + 1);

    i = lwall[x] + globalxpanning;
    if (i >= tiles[globalpicnum].dim.width)
        i %= tiles[globalpicnum].dim.width;
    bufplce[0] = (intptr_t) (tiles[globalpicnum].data + i * tiles[globalpicnum].
                             dim.height);

    i = lwall[x2] + globalxpanning;
    if (i >= tiles[globalpicnum].dim.width)
        i %= tiles[globalpicnum].dim.width;
    bufplce[1] = (intptr_t) (tiles[globalpicnum].data + i * tiles[globalpicnum].
                             dim.height);


    y1 = max(y1ve[0], y1ve[1]);
    y2 = min(y2ve[0], y2ve[1]);

    i = (intptr_t) (x + frameoffset);

    if (y1ve[0] != y1ve[1]) {
        if (y1ve[0] < y1)
            vplce[0] = tvlineasm1(vince[0], palookupoffse[0], y1 - y1ve[0] - 1,
                                  vplce[0], (const uint8_t*) bufplce[0],
                                  (uint8_t*) ylookup[y1ve[0]] + i);
        else
            vplce[1] = tvlineasm1(vince[1], palookupoffse[1], y1 - y1ve[1] - 1,
                                  vplce[1], (const uint8_t*) bufplce[1],
                                  (uint8_t*) ylookup[y1ve[1]] + i + 1);
    }

    if (y2 > y1) {
        asm1 = vince[1];
        asm2 = ylookup[y2] + i + 1;
        tvlineasm2(vplce[1], vince[0], bufplce[0], bufplce[1], vplce[0],
                   ylookup[y1] + i);
        transarea += ((y2 - y1) << 1);
    } else {
        asm1 = vplce[0];
        asm2 = vplce[1];
    }

    if (y2ve[0] > y2ve[1])
        tvlineasm1(vince[0], palookupoffse[0], y2ve[0] - y2 - 1, asm1,
                   (const uint8_t*) bufplce[0], (uint8_t*) ylookup[y2 + 1] + i);
    else if (y2ve[0] < y2ve[1])
        tvlineasm1(vince[1], palookupoffse[1], y2ve[1] - y2 - 1, asm2,
                   (const uint8_t*) bufplce[1],
                   (uint8_t*) ylookup[y2 + 1] + i + 1);

    faketimerhandler();
}

static void R_TransMaskWallScan(int32_t x1, int32_t x2) {
    int32_t x;

    setgotpic(globalpicnum);

    //Tile dimensions are invalid
    if ((tiles[globalpicnum].dim.width <= 0) ||
        (tiles[globalpicnum].dim.height <= 0))
        return;

    TILE_MakeAvailable(globalpicnum);

    x = x1;
    while ((startumost[x + windowx1] > startdmost[x + windowx1]) && (x <= x2))
        x++;
    if ((x <= x2) && (x & 1))
        R_TransMaskVline(x), x++;
    while (x < x2)
        R_TransMaskVline2(x), x += 2;
    while (x <= x2)
        R_TransMaskVline(x), x++;
    faketimerhandler();
}

static void R_DrawSprite(int32_t snum) {
    spritetype* tspr;
    sectortype* sec;
    int32_t startum, startdm, sectnum, xb, yp, cstat;
    int32_t siz, xsiz, ysiz, xoff, yoff;
    dimensions_t spriteDim;
    int32_t x1, y1, x2, y2, lx, rx, dalx2, darx2, i, j, k, x, linum, linuminc;
    int32_t yinc, z, z1, z2, xp1, yp1, xp2, yp2;
    int32_t xv, yv, top, topinc, bot, botinc, hplc, hinc;
    int32_t cosang, sinang, dax, day, lpoint, lmax, rpoint, rmax, dax1, dax2, y;
    int32_t npoints, npoints2, zz, t, zsgn, zzsgn;
    short tilenum, spritenum;
    uint8_t swapped, daclip;

    tspr = tspriteptr[snum];

    xb = spritesx[snum];
    yp = spritesy[snum];
    tilenum = tspr->picnum;
    spritenum = tspr->owner;
    cstat = tspr->cstat;

    if ((cstat & 48) != 48) {
        if (tiles[tilenum].anim_flags & 192)
            tilenum += animateoffs(tilenum);

        if ((tiles[tilenum].dim.width <= 0) || (tiles[tilenum].dim.height <= 0)
            || (spritenum < 0))
            return;
    }
    if ((tspr->xrepeat <= 0) || (tspr->yrepeat <= 0))
        return;

    sectnum = tspr->sectnum;
    sec = &sector[sectnum];
    globalpal = tspr->pal;
    // FIX_00088: crash on maps using a bad palette index (like the end of roch3.map)
    if (!palookup[globalpal])
        globalpal = 0; // seem to crash when globalpal > 25
    globalshade = tspr->shade;
    if (cstat & 2) {

        if (cstat & 512)
            settrans(TRANS_REVERSE);
        else
            settrans(TRANS_NORMAL);
    }

    xoff = (int32_t) ((int8_t) ((tiles[tilenum].anim_flags >> 8) & 255)) + ((
               int32_t) tspr->xoffset);
    yoff = (int32_t) ((int8_t) ((tiles[tilenum].anim_flags >> 16) & 255)) + ((
               int32_t) tspr->yoffset);

    if ((cstat & 48) == 0) {
        if (yp <= (4 << 8))
            return;

        siz = divscale19(xdimenscale, yp);

        xv = mulscale16(((int32_t) tspr->xrepeat) << 16, xyaspect);

        spriteDim.width = tiles[tilenum].dim.width;
        spriteDim.height = tiles[tilenum].dim.height;

        xsiz = mulscale30(siz, xv * spriteDim.width);
        ysiz = mulscale14(siz, tspr->yrepeat * spriteDim.height);

        if (((tiles[tilenum].dim.width >> 11) >= xsiz) || (
                spriteDim.height >= (ysiz >> 1)))
            return; /* Watch out for divscale overflow */

        x1 = xb - (xsiz >> 1);
        if (spriteDim.width & 1)
            x1 += mulscale31(siz, xv); /* Odd xspans */
        i = mulscale30(siz, xv * xoff);
        if ((cstat & 4) == 0)
            x1 -= i;
        else
            x1 += i;

        y1 = mulscale16(tspr->z - globalposz, siz);
        y1 -= mulscale14(siz, tspr->yrepeat * yoff);
        y1 += (globalhoriz << 8) - ysiz;
        if (cstat & 128) {
            y1 += (ysiz >> 1);
            if (spriteDim.height & 1)
                y1 += mulscale15(siz, tspr->yrepeat); /* Odd yspans */
        }

        x2 = x1 + xsiz - 1;
        y2 = y1 + ysiz - 1;
        if ((y1 | 255) >= (y2 | 255))
            return;

        lx = (x1 >> 8) + 1;
        if (lx < 0)
            lx = 0;
        rx = (x2 >> 8);
        if (rx >= xdimen)
            rx = xdimen - 1;
        if (lx > rx)
            return;

        yinc = divscale32(spriteDim.height, ysiz);

        if ((sec->ceilingstat & 3) == 0)
            startum = globalhoriz + mulscale24(siz, sec->ceilingz - globalposz)
                      - 1;
        else
            startum = 0;

        if ((sec->floorstat & 3) == 0)
            startdm = globalhoriz + mulscale24(siz, sec->floorz - globalposz) +
                      1;
        else
            startdm = 0x7fffffff;

        if ((y1 >> 8) > startum)
            startum = (y1 >> 8);
        if ((y2 >> 8) < startdm)
            startdm = (y2 >> 8);

        if (startum < -32768)
            startum = -32768;
        if (startdm > 32767)
            startdm = 32767;
        if (startum >= startdm)
            return;

        if ((cstat & 4) == 0) {
            linuminc = divscale24(spriteDim.width, xsiz);
            linum = mulscale8((lx << 8) - x1, linuminc);
        } else {
            linuminc = -divscale24(spriteDim.width, xsiz);
            linum = mulscale8((lx << 8) - x2, linuminc);
        }
        if ((cstat & 8) > 0) {
            yinc = -yinc;
            i = y1;
            y1 = y2;
            y2 = i;
        }

        for (x = lx; x <= rx; x++) {
            uwall[x] = max(startumost[x+windowx1]-windowy1, (short)startum);
            dwall[x] = min(startdmost[x+windowx1]-windowy1, (short)startdm);
        }
        daclip = 0;
        for (i = smostwallcnt - 1; i >= 0; i--) {
            if (smostwalltype[i] & daclip)
                continue;

            j = smostwall[i];
            if ((pv_walls[j].screenSpaceCoo[0][VEC_COL] > rx) || (
                    pv_walls[j].screenSpaceCoo[1][VEC_COL] < lx))
                continue;

            if ((yp <= pv_walls[j].screenSpaceCoo[0][VEC_DIST]) && (
                    yp <= pv_walls[j].screenSpaceCoo[1][VEC_DIST]))
                continue;

            if (R_SpriteWallFront(tspr, pv_walls[j].worldWallId) && (
                    (yp <= pv_walls[j].screenSpaceCoo[0][VEC_DIST]) || (
                        yp <= pv_walls[j].screenSpaceCoo[1][VEC_DIST])))
                continue;

            dalx2 = max(pv_walls[j].screenSpaceCoo[0][VEC_COL], lx);
            darx2 = min(pv_walls[j].screenSpaceCoo[1][VEC_COL], rx);

            switch (smostwalltype[i]) {
                case 0:
                    if (dalx2 <= darx2) {
                        if ((dalx2 == lx) && (darx2 == rx))
                            return;
                        clearbufbyte(&dwall[dalx2],
                                     (darx2 - dalx2 + 1) * sizeof(dwall[0]),
                                     0L);
                    }
                    break;
                case 1:
                    k = smoststart[i] - pv_walls[j].screenSpaceCoo[0][VEC_COL];
                    for (x = dalx2; x <= darx2; x++)
                        if (smost[k + x] > uwall[x])
                            uwall[x] = smost[k + x];
                    if ((dalx2 == lx) && (darx2 == rx))
                        daclip |= 1;
                    break;
                case 2:
                    k = smoststart[i] - pv_walls[j].screenSpaceCoo[0][VEC_COL];
                    for (x = dalx2; x <= darx2; x++)
                        if (smost[k + x] < dwall[x])
                            dwall[x] = smost[k + x];
                    if ((dalx2 == lx) && (darx2 == rx))
                        daclip |= 2;
                    break;
            }
        }

        if (uwall[rx] >= dwall[rx]) {
            for (x = lx; x < rx; x++)
                if (uwall[x] < dwall[x])
                    break;
            if (x == rx)
                return;
        }

        /* sprite */
        if ((searchit >= 1) && (searchx >= lx) && (searchx <= rx))
            if ((searchy >= uwall[searchx]) && (searchy < dwall[searchx])) {
                searchsector = sectnum;
                searchwall = spritenum;
                searchstat = 3;
                searchit = 1;
            }

        z2 = tspr->z - ((yoff * tspr->yrepeat) << 2);
        if (cstat & 128) {
            z2 += ((spriteDim.height * tspr->yrepeat) << 1);
            if (spriteDim.height & 1)
                z2 += (tspr->yrepeat << 1); /* Odd yspans */
        }
        z1 = z2 - ((spriteDim.height * tspr->yrepeat) << 2);

        globalorientation = 0;
        globalpicnum = tilenum;
        if ((uint32_t) globalpicnum >= (uint32_t) MAXTILES)
            globalpicnum = 0;
        globalxpanning = 0L;
        globalypanning = 0L;
        globvis = globalvisibility;
        if (sec->visibility != 0)
            globvis = mulscale4(globvis,
                                (int32_t) ((uint8_t) (sec->visibility + 16)));
        globalshiftval = (picsiz[globalpicnum] >> 4);
        if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height)
            globalshiftval++;

        globalshiftval = 32 - globalshiftval;
        globalyscale = divscale(512, tspr->yrepeat, globalshiftval - 19);
        globalzd = (((globalposz - z1) * globalyscale) << 8);
        if ((cstat & 8) > 0) {
            globalyscale = -globalyscale;
            globalzd = (((globalposz - z2) * globalyscale) << 8);
        }

        qinterpolatedown16((int32_t*) &lwall[lx], rx - lx + 1, linum, linuminc);
        clearbuf(&swall[lx], rx - lx + 1, mulscale19(yp, xdimscale));

        if ((cstat & 2) == 0)
            maskwallscan(lx, rx, uwall, dwall, swall, lwall);
        else
            R_TransMaskWallScan(lx, rx);
    } else if ((cstat & 48) == 16) {
        if ((cstat & 4) > 0)
            xoff = -xoff;
        if ((cstat & 8) > 0)
            yoff = -yoff;

        spriteDim.width = tiles[tilenum].dim.width;
        spriteDim.height = tiles[tilenum].dim.height;

        xv = tspr->xrepeat * sintable[(tspr->ang + 2560 + 1536) & 2047];
        yv = tspr->xrepeat * sintable[(tspr->ang + 2048 + 1536) & 2047];
        i = (spriteDim.width >> 1) + xoff;
        x1 = tspr->x - globalposx - mulscale16(xv, i);
        x2 = x1 + mulscale16(xv, spriteDim.width);
        y1 = tspr->y - globalposy - mulscale16(yv, i);
        y2 = y1 + mulscale16(yv, spriteDim.width);

        yp1 = dmulscale6(x1, cosviewingrangeglobalang, y1,
                         sinviewingrangeglobalang);
        yp2 = dmulscale6(x2, cosviewingrangeglobalang, y2,
                         sinviewingrangeglobalang);
        if ((yp1 <= 0) && (yp2 <= 0))
            return;
        xp1 = dmulscale6(y1, cosglobalang, -x1, singlobalang);
        xp2 = dmulscale6(y2, cosglobalang, -x2, singlobalang);

        x1 += globalposx;
        y1 += globalposy;
        x2 += globalposx;
        y2 += globalposy;

        swapped = 0;
        if (dmulscale32(xp1, yp2, -xp2, yp1) >= 0)
        /* If wall's NOT facing you */
        {
            if ((cstat & 64) != 0)
                return;
            i = xp1, xp1 = xp2, xp2 = i;
            i = yp1, yp1 = yp2, yp2 = i;
            i = x1, x1 = x2, x2 = i;
            i = y1, y1 = y2, y2 = i;
            swapped = 1;
        }

        if (xp1 >= -yp1) {
            if (xp1 > yp1)
                return;

            if (yp1 == 0)
                return;
            pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL] =
                halfxdimen + scale(xp1, halfxdimen, yp1);
            if (xp1 >= 0)
                pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL]++;
            /* Fix for SIGNED divide */
            if (pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL] >= xdimen)
                pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL] = xdimen - 1;
            pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_DIST] = yp1;
        } else {
            if (xp2 < -yp2)
                return;
            pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL] = 0;
            i = yp1 - yp2 + xp1 - xp2;
            if (i == 0)
                return;
            pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_DIST] =
                yp1 + scale(yp2 - yp1, xp1 + yp1, i);
        }
        if (xp2 <= yp2) {
            if (xp2 < -yp2)
                return;

            if (yp2 == 0)
                return;
            pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL] =
                halfxdimen + scale(xp2, halfxdimen, yp2) - 1;
            if (xp2 >= 0)
                pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL]++;
            /* Fix for SIGNED divide */
            if (pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL] >= xdimen)
                pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL] = xdimen - 1;
            pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_DIST] = yp2;
        } else {
            if (xp1 > yp1)
                return;

            pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL] = xdimen - 1;
            i = xp2 - xp1 + yp1 - yp2;
            if (i == 0)
                return;
            pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_DIST] =
                yp1 + scale(yp2 - yp1, yp1 - xp1, i);
        }

        if ((pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_DIST] < 256) || (
                pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_DIST] < 256) || (
                pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL] > pv_walls[
                    MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL]))
            return;

        topinc = -mulscale10(yp1, spriteDim.width);
        top = (((mulscale10(xp1, xdimen) - mulscale9(
                     pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL] -
                     halfxdimen, yp1)) * spriteDim.width) >> 3);
        botinc = ((yp2 - yp1) >> 8);
        bot = mulscale11(xp1 - xp2, xdimen) + mulscale2(
                  pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL] -
                  halfxdimen, botinc);

        j = pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL] + 3;
        z = mulscale20(top, krecipasm(bot));
        lwall[pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL]] = (z >> 8);
        for (x = pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL] + 4; x <= j;
             x += 4) {
            top += topinc;
            bot += botinc;
            zz = z;
            z = mulscale20(top, krecipasm(bot));
            lwall[x] = (z >> 8);
            i = ((z + zz) >> 1);
            lwall[x - 2] = (i >> 8);
            lwall[x - 3] = ((i + zz) >> 9);
            lwall[x - 1] = ((i + z) >> 9);
        }

        if (lwall[pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL]] < 0)
            lwall[pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL]] = 0;
        if (lwall[pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL]] >=
            spriteDim.width)
            lwall[pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL]] =
                spriteDim.width - 1;

        if ((swapped ^ ((cstat & 4) > 0)) > 0) {
            j = spriteDim.width - 1;
            for (x = pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL];
                 x <= pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL]; x++)
                lwall[x] = j - lwall[x];
        }

        pv_walls[MAXWALLSB - 1].cameraSpaceCoo[0][VEC_X] = xp1;
        pv_walls[MAXWALLSB - 1].cameraSpaceCoo[0][VEC_Y] = yp1;
        pv_walls[MAXWALLSB - 1].cameraSpaceCoo[1][VEC_X] = xp2;
        pv_walls[MAXWALLSB - 1].cameraSpaceCoo[1][VEC_Y] = yp2;


        hplc = divscale19(xdimenscale,
                          pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_DIST]);
        hinc = divscale19(xdimenscale,
                          pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_DIST]);
        hinc = (hinc - hplc) / (pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL] - pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL] + 1);

        z2 = tspr->z - ((yoff * tspr->yrepeat) << 2);
        if (cstat & 128) {
            z2 += ((spriteDim.height * tspr->yrepeat) << 1);
            if (spriteDim.height & 1)
                z2 += (tspr->yrepeat << 1); /* Odd yspans */
        }
        z1 = z2 - ((spriteDim.height * tspr->yrepeat) << 2);

        globalorientation = 0;
        globalpicnum = tilenum;
        if ((uint32_t) globalpicnum >= (uint32_t) MAXTILES)
            globalpicnum = 0;
        globalxpanning = 0L;
        globalypanning = 0L;
        globvis = globalvisibility;
        if (sec->visibility != 0)
            globvis = mulscale4(globvis,
                                (int32_t) ((uint8_t) (sec->visibility + 16)));
        globalshiftval = (picsiz[globalpicnum] >> 4);
        if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height)
            globalshiftval++;
        globalshiftval = 32 - globalshiftval;
        globalyscale = divscale(512, tspr->yrepeat, globalshiftval - 19);
        globalzd = (((globalposz - z1) * globalyscale) << 8);
        if ((cstat & 8) > 0) {
            globalyscale = -globalyscale;
            globalzd = (((globalposz - z2) * globalyscale) << 8);
        }

        if (((sec->ceilingstat & 1) == 0) && (z1 < sec->ceilingz))
            z1 = sec->ceilingz;
        if (((sec->floorstat & 1) == 0) && (z2 > sec->floorz))
            z2 = sec->floorz;

        owallmost(uwall, (int32_t) (MAXWALLSB - 1), z1 - globalposz);
        owallmost(dwall, (int32_t) (MAXWALLSB - 1), z2 - globalposz);
        for (i = pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL];
             i <= pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL]; i++) {
            swall[i] = (krecipasm(hplc) << 2);
            hplc += hinc;
        }

        for (i = smostwallcnt - 1; i >= 0; i--) {
            j = smostwall[i];

            if ((pv_walls[j].screenSpaceCoo[0][VEC_COL] > pv_walls[MAXWALLSB - 1].
                 screenSpaceCoo[1][VEC_COL]) || (
                    pv_walls[j].screenSpaceCoo[1][VEC_COL] < pv_walls[
                        MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL]))
                continue;

            dalx2 = pv_walls[j].screenSpaceCoo[0][VEC_COL];
            darx2 = pv_walls[j].screenSpaceCoo[1][VEC_COL];
            if (max(pv_walls[MAXWALLSB-1].screenSpaceCoo[0][VEC_DIST],
                    pv_walls[MAXWALLSB-1].screenSpaceCoo[1][VEC_DIST]) > min(
                    pv_walls[j].screenSpaceCoo[0][VEC_DIST],
                    pv_walls[j].screenSpaceCoo[1][VEC_DIST])) {
                if (min(pv_walls[MAXWALLSB-1].screenSpaceCoo[0][VEC_DIST],
                        pv_walls[MAXWALLSB-1].screenSpaceCoo[1][VEC_DIST]) > max(
                        pv_walls[j].screenSpaceCoo[0][VEC_DIST],
                        pv_walls[j].screenSpaceCoo[1][VEC_DIST])) {
                    x = 0x80000000;
                } else {
                    x = pv_walls[j].worldWallId;
                    xp1 = wall[x].x;
                    yp1 = wall[x].y;
                    x = wall[x].point2;
                    xp2 = wall[x].x;
                    yp2 = wall[x].y;

                    z1 = (xp2 - xp1) * (y1 - yp1) - (yp2 - yp1) * (x1 - xp1);
                    z2 = (xp2 - xp1) * (y2 - yp1) - (yp2 - yp1) * (x2 - xp1);
                    if ((z1 ^ z2) >= 0)
                        x = (z1 + z2);
                    else {
                        z1 = (x2 - x1) * (yp1 - y1) - (y2 - y1) * (xp1 - x1);
                        z2 = (x2 - x1) * (yp2 - y1) - (y2 - y1) * (xp2 - x1);

                        if ((z1 ^ z2) >= 0)
                            x = -(z1 + z2);
                        else {
                            if ((xp2 - xp1) * (tspr->y - yp1) == (tspr->x - xp1)
                                * (yp2 - yp1)) {
                                if (wall[pv_walls[j].worldWallId].nextsector ==
                                    tspr->sectnum)
                                    x = 0x80000000;
                                else
                                    x = 0x7fffffff;
                            } else {
                                /* INTERSECTION! */
                                x = (xp1 - globalposx) + scale(
                                        xp2 - xp1, z1, z1 - z2);
                                y = (yp1 - globalposy) + scale(
                                        yp2 - yp1, z1, z1 - z2);

                                yp1 = dmulscale14(
                                    x, cosglobalang, y, singlobalang);
                                if (yp1 > 0) {
                                    xp1 = dmulscale14(
                                        y, cosglobalang, -x, singlobalang);

                                    x = halfxdimen +
                                        scale(xp1, halfxdimen, yp1);
                                    if (xp1 >= 0)
                                        x++; /* Fix for SIGNED divide */

                                    if (z1 < 0) {
                                        if (dalx2 < x)
                                            dalx2 = x;
                                    } else {
                                        if (darx2 > x)
                                            darx2 = x;
                                    }
                                    x = 0x80000001;
                                } else
                                    x = 0x7fffffff;
                            }
                        }
                    }
                }
                if (x < 0) {
                    if (dalx2 < pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][
                            VEC_COL])
                        dalx2 = pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][
                            VEC_COL];
                    if (darx2 > pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][
                            VEC_COL])
                        darx2 = pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][
                            VEC_COL];
                    switch (smostwalltype[i]) {
                        case 0:
                            if (dalx2 <= darx2) {
                                if ((dalx2 == pv_walls[MAXWALLSB - 1].
                                     screenSpaceCoo[0][VEC_COL]) && (
                                        darx2 == pv_walls[MAXWALLSB - 1].
                                        screenSpaceCoo[1][VEC_COL]))
                                    return;
                                clearbufbyte(&dwall[dalx2],
                                             (darx2 - dalx2 + 1) * sizeof(dwall[
                                                 0]), 0L);
                            }
                            break;
                        case 1:
                            k = smoststart[i] - pv_walls[j].screenSpaceCoo[0][
                                    VEC_COL];
                            for (x = dalx2; x <= darx2; x++)
                                if (smost[k + x] > uwall[x])
                                    uwall[x] = smost[k + x];
                            break;
                        case 2:
                            k = smoststart[i] - pv_walls[j].screenSpaceCoo[0][
                                    VEC_COL];
                            for (x = dalx2; x <= darx2; x++)
                                if (smost[k + x] < dwall[x])
                                    dwall[x] = smost[k + x];
                            break;
                    }
                }
            }
        }

        /* sprite */
        if ((searchit >= 1) && (searchx >= pv_walls[MAXWALLSB - 1].screenSpaceCoo
                                [0][VEC_COL]) && (
                searchx <= pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL]))
            if ((searchy >= uwall[searchx]) && (searchy <= dwall[searchx])) {
                searchsector = sectnum;
                searchwall = spritenum;
                searchstat = 3;
                searchit = 1;
            }

        if ((cstat & 2) == 0)
            maskwallscan(pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL],
                         pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][VEC_COL],
                         uwall, dwall, swall, lwall);
        else
            R_TransMaskWallScan(pv_walls[MAXWALLSB - 1].screenSpaceCoo[0][VEC_COL],
                              pv_walls[MAXWALLSB - 1].screenSpaceCoo[1][
                                  VEC_COL]);
    } else if ((cstat & 48) == 32) {
        if ((cstat & 64) != 0)
            if ((globalposz > tspr->z) == ((cstat & 8) == 0))
                return;

        if ((cstat & 4) > 0)
            xoff = -xoff;
        if ((cstat & 8) > 0)
            yoff = -yoff;
        spriteDim.width = tiles[tilenum].dim.width;
        spriteDim.height = tiles[tilenum].dim.height;

        /* Rotate center point */
        dax = tspr->x - globalposx;
        day = tspr->y - globalposy;
        rzi[0] = dmulscale10(cosglobalang, dax, singlobalang, day);
        rxi[0] = dmulscale10(cosglobalang, day, -singlobalang, dax);

        /* Get top-left corner */
        i = ((tspr->ang + 2048 - globalang) & 2047);
        cosang = sintable[(i + 512) & 2047];
        sinang = sintable[i];
        dax = ((spriteDim.width >> 1) + xoff) * tspr->xrepeat;
        day = ((spriteDim.height >> 1) + yoff) * tspr->yrepeat;
        rzi[0] += dmulscale12(sinang, dax, cosang, day);
        rxi[0] += dmulscale12(sinang, day, -cosang, dax);

        /* Get other 3 corners */
        dax = spriteDim.width * tspr->xrepeat;
        day = spriteDim.height * tspr->yrepeat;
        rzi[1] = rzi[0] - mulscale12(sinang, dax);
        rxi[1] = rxi[0] + mulscale12(cosang, dax);
        dax = -mulscale12(cosang, day);
        day = -mulscale12(sinang, day);
        rzi[2] = rzi[1] + dax;
        rxi[2] = rxi[1] + day;
        rzi[3] = rzi[0] + dax;
        rxi[3] = rxi[0] + day;

        /* Put all points on same z */
        ryi[0] = scale((tspr->z - globalposz), yxaspect, 320 << 8);
        if (ryi[0] == 0)
            return;
        ryi[1] = ryi[2] = ryi[3] = ryi[0];

        if ((cstat & 4) == 0) {
            z = 0;
            z1 = 1;
            z2 = 3;
        } else {
            z = 1;
            z1 = 0;
            z2 = 2;
        }

        dax = rzi[z1] - rzi[z];
        day = rxi[z1] - rxi[z];
        bot = dmulscale8(dax, dax, day, day);
        if (((klabs(dax) >> 13) >= bot) || ((klabs(day) >> 13) >= bot))
            return;
        globalx1 = divscale18(dax, bot);
        globalx2 = divscale18(day, bot);

        dax = rzi[z2] - rzi[z];
        day = rxi[z2] - rxi[z];
        bot = dmulscale8(dax, dax, day, day);
        if (((klabs(dax) >> 13) >= bot) || ((klabs(day) >> 13) >= bot))
            return;
        globaly1 = divscale18(dax, bot);
        globaly2 = divscale18(day, bot);

        /* Calculate globals for hline texture mapping function */
        globalxpanning = (rxi[z] << 12);
        globalypanning = (rzi[z] << 12);
        globalzd = (ryi[z] << 12);

        rzi[0] = mulscale16(rzi[0], viewingrange);
        rzi[1] = mulscale16(rzi[1], viewingrange);
        rzi[2] = mulscale16(rzi[2], viewingrange);
        rzi[3] = mulscale16(rzi[3], viewingrange);

        if (ryi[0] < 0)
        /* If ceilsprite is above you, reverse order of points */
        {
            i = rxi[1];
            rxi[1] = rxi[3];
            rxi[3] = i;
            i = rzi[1];
            rzi[1] = rzi[3];
            rzi[3] = i;
        }


        /* Clip polygon in 3-space */
        npoints = 4;

        /* Clip edge 1 */
        npoints2 = 0;
        zzsgn = rxi[0] + rzi[0];
        for (z = 0; z < npoints; z++) {
            zz = z + 1;
            if (zz == npoints)
                zz = 0;
            zsgn = zzsgn;
            zzsgn = rxi[zz] + rzi[zz];
            if (zsgn >= 0) {
                rxi2[npoints2] = rxi[z];
                ryi2[npoints2] = ryi[z];
                rzi2[npoints2] = rzi[z];
                npoints2++;
            }
            if ((zsgn ^ zzsgn) < 0) {
                t = divscale30(zsgn, zsgn - zzsgn);
                rxi2[npoints2] = rxi[z] + mulscale30(t, rxi[zz] - rxi[z]);
                ryi2[npoints2] = ryi[z] + mulscale30(t, ryi[zz] - ryi[z]);
                rzi2[npoints2] = rzi[z] + mulscale30(t, rzi[zz] - rzi[z]);
                npoints2++;
            }
        }
        if (npoints2 <= 2)
            return;

        /* Clip edge 2 */
        npoints = 0;
        zzsgn = rxi2[0] - rzi2[0];
        for (z = 0; z < npoints2; z++) {
            zz = z + 1;
            if (zz == npoints2)
                zz = 0;
            zsgn = zzsgn;
            zzsgn = rxi2[zz] - rzi2[zz];
            if (zsgn <= 0) {
                rxi[npoints] = rxi2[z];
                ryi[npoints] = ryi2[z];
                rzi[npoints] = rzi2[z];
                npoints++;
            }
            if ((zsgn ^ zzsgn) < 0) {
                t = divscale30(zsgn, zsgn - zzsgn);
                rxi[npoints] = rxi2[z] + mulscale30(t, rxi2[zz] - rxi2[z]);
                ryi[npoints] = ryi2[z] + mulscale30(t, ryi2[zz] - ryi2[z]);
                rzi[npoints] = rzi2[z] + mulscale30(t, rzi2[zz] - rzi2[z]);
                npoints++;
            }
        }
        if (npoints <= 2)
            return;

        /* Clip edge 3 */
        npoints2 = 0;
        zzsgn = ryi[0] * halfxdimen + (rzi[0] * (globalhoriz - 0));
        for (z = 0; z < npoints; z++) {
            zz = z + 1;
            if (zz == npoints)
                zz = 0;
            zsgn = zzsgn;
            zzsgn = ryi[zz] * halfxdimen + (rzi[zz] * (globalhoriz - 0));
            if (zsgn >= 0) {
                rxi2[npoints2] = rxi[z];
                ryi2[npoints2] = ryi[z];
                rzi2[npoints2] = rzi[z];
                npoints2++;
            }
            if ((zsgn ^ zzsgn) < 0) {
                t = divscale30(zsgn, zsgn - zzsgn);
                rxi2[npoints2] = rxi[z] + mulscale30(t, rxi[zz] - rxi[z]);
                ryi2[npoints2] = ryi[z] + mulscale30(t, ryi[zz] - ryi[z]);
                rzi2[npoints2] = rzi[z] + mulscale30(t, rzi[zz] - rzi[z]);
                npoints2++;
            }
        }
        if (npoints2 <= 2)
            return;

        /* Clip edge 4 */
        npoints = 0;
        zzsgn = ryi2[0] * halfxdimen + (rzi2[0] * (globalhoriz - ydimen));
        for (z = 0; z < npoints2; z++) {
            zz = z + 1;
            if (zz == npoints2)
                zz = 0;
            zsgn = zzsgn;
            zzsgn = ryi2[zz] * halfxdimen + (rzi2[zz] * (globalhoriz - ydimen));
            if (zsgn <= 0) {
                rxi[npoints] = rxi2[z];
                ryi[npoints] = ryi2[z];
                rzi[npoints] = rzi2[z];
                npoints++;
            }
            if ((zsgn ^ zzsgn) < 0) {
                t = divscale30(zsgn, zsgn - zzsgn);
                rxi[npoints] = rxi2[z] + mulscale30(t, rxi2[zz] - rxi2[z]);
                ryi[npoints] = ryi2[z] + mulscale30(t, ryi2[zz] - ryi2[z]);
                rzi[npoints] = rzi2[z] + mulscale30(t, rzi2[zz] - rzi2[z]);
                npoints++;
            }
        }
        if (npoints <= 2)
            return;

        /* Project onto screen */
        lpoint = -1;
        lmax = 0x7fffffff;
        rpoint = -1;
        rmax = 0x80000000;
        for (z = 0; z < npoints; z++) {
            xsi[z] = scale(rxi[z], xdimen << 15, rzi[z]) + (xdimen << 15);
            ysi[z] = scale(ryi[z], xdimen << 15, rzi[z]) + (globalhoriz << 16);
            if (xsi[z] < 0)
                xsi[z] = 0;
            if (xsi[z] > (xdimen << 16))
                xsi[z] = (xdimen << 16);
            if (ysi[z] < ((int32_t) 0 << 16))
                ysi[z] = ((int32_t) 0 << 16);
            if (ysi[z] > ((int32_t) ydimen << 16))
                ysi[z] = ((int32_t) ydimen << 16);
            if (xsi[z] < lmax)
                lmax = xsi[z], lpoint = z;
            if (xsi[z] > rmax)
                rmax = xsi[z], rpoint = z;
        }

        /* Get uwall arrays */
        for (z = lpoint; z != rpoint; z = zz) {
            zz = z + 1;
            if (zz == npoints)
                zz = 0;

            dax1 = ((xsi[z] + 65535) >> 16);
            dax2 = ((xsi[zz] + 65535) >> 16);
            if (dax2 > dax1) {
                yinc = divscale16(ysi[zz] - ysi[z], xsi[zz] - xsi[z]);
                y = ysi[z] + mulscale16((dax1 << 16) - xsi[z], yinc);
                qinterpolatedown16short((int32_t*) (&uwall[dax1]), dax2 - dax1,
                                        y, yinc);
            }
        }

        /* Get dwall arrays */
        for (; z != lpoint; z = zz) {
            zz = z + 1;
            if (zz == npoints)
                zz = 0;

            dax1 = ((xsi[zz] + 65535) >> 16);
            dax2 = ((xsi[z] + 65535) >> 16);
            if (dax2 > dax1) {
                yinc = divscale16(ysi[zz] - ysi[z], xsi[zz] - xsi[z]);
                y = ysi[zz] + mulscale16((dax1 << 16) - xsi[zz], yinc);
                qinterpolatedown16short((int32_t*) (&dwall[dax1]), dax2 - dax1,
                                        y, yinc);
            }
        }


        lx = ((lmax + 65535) >> 16);
        rx = ((rmax + 65535) >> 16);
        for (x = lx; x <= rx; x++) {
            uwall[x] = max(uwall[x], startumost[x+windowx1]-windowy1);
            dwall[x] = min(dwall[x], startdmost[x+windowx1]-windowy1);
        }

        /* Additional uwall/dwall clipping goes here */
        for (i = smostwallcnt - 1; i >= 0; i--) {
            j = smostwall[i];
            if ((pv_walls[j].screenSpaceCoo[0][VEC_COL] > rx) || (
                    pv_walls[j].screenSpaceCoo[1][VEC_COL] < lx))
                continue;
            if ((yp <= pv_walls[j].screenSpaceCoo[0][VEC_DIST]) && (
                    yp <= pv_walls[j].screenSpaceCoo[1][VEC_DIST]))
                continue;

            /* if (spritewallfront(tspr,thewall[j]) == 0) */
            x = pv_walls[j].worldWallId;
            xp1 = wall[x].x;
            yp1 = wall[x].y;
            x = wall[x].point2;
            xp2 = wall[x].x;
            yp2 = wall[x].y;
            x = (xp2 - xp1) * (tspr->y - yp1) - (tspr->x - xp1) * (yp2 - yp1);
            if ((yp > pv_walls[j].screenSpaceCoo[0][VEC_DIST]) && (
                    yp > pv_walls[j].screenSpaceCoo[1][VEC_DIST]))
                x = -1;
            if ((x >= 0) && ((x != 0) || (
                                 wall[pv_walls[j].worldWallId].nextsector != tspr
                                 ->sectnum)))
                continue;

            dalx2 = max(pv_walls[j].screenSpaceCoo[0][VEC_COL], lx);
            darx2 = min(pv_walls[j].screenSpaceCoo[1][VEC_COL], rx);

            switch (smostwalltype[i]) {
                case 0:
                    if (dalx2 <= darx2) {
                        if ((dalx2 == lx) && (darx2 == rx))
                            return;
                        clearbufbyte(&dwall[dalx2],
                                     (darx2 - dalx2 + 1) * sizeof(dwall[0]),
                                     0L);
                    }
                    break;
                case 1:
                    k = smoststart[i] - pv_walls[j].screenSpaceCoo[0][VEC_COL];
                    for (x = dalx2; x <= darx2; x++)
                        if (smost[k + x] > uwall[x])
                            uwall[x] = smost[k + x];
                    break;
                case 2:
                    k = smoststart[i] - pv_walls[j].screenSpaceCoo[0][VEC_COL];
                    for (x = dalx2; x <= darx2; x++)
                        if (smost[k + x] < dwall[x])
                            dwall[x] = smost[k + x];
                    break;
            }
        }

        /* sprite */
        if ((searchit >= 1) && (searchx >= lx) && (searchx <= rx))
            if ((searchy >= uwall[searchx]) && (searchy <= dwall[searchx])) {
                searchsector = sectnum;
                searchwall = spritenum;
                searchstat = 3;
                searchit = 1;
            }

        globalorientation = cstat;
        globalpicnum = tilenum;
        if ((uint32_t) globalpicnum >= (uint32_t) MAXTILES)
            globalpicnum = 0;

        TILE_MakeAvailable(globalpicnum);

        setgotpic(globalpicnum);
        globalbufplc = tiles[globalpicnum].data;

        globvis = mulscale16(globalhisibility, viewingrange);
        if (sec->visibility != 0)
            globvis = mulscale4(globvis,
                                (int32_t) ((uint8_t) (sec->visibility + 16)));

        x = picsiz[globalpicnum];
        y = ((x >> 4) & 15);
        x &= 15;
        if (pow2long[x] != spriteDim.width) {
            x++;
            globalx1 = mulscale(globalx1, spriteDim.width, x);
            globalx2 = mulscale(globalx2, spriteDim.width, x);
        }

        dax = globalxpanning;
        day = globalypanning;
        globalxpanning = -dmulscale6(globalx1, day, globalx2, dax);
        globalypanning = -dmulscale6(globaly1, day, globaly2, dax);

        globalx2 = mulscale16(globalx2, viewingrange);
        globaly2 = mulscale16(globaly2, viewingrange);
        globalzd = mulscale16(globalzd, viewingrangerecip);

        globalx1 = (globalx1 - globalx2) * halfxdimen;
        globaly1 = (globaly1 - globaly2) * halfxdimen;

        if ((cstat & 2) == 0)
            msethlineshift(x, y);
        else
            tsethlineshift(x, y);

        /* Draw it! */
        R_CeilSpriteScan(lx, rx - 1);
    }

    if (automapping == 1)
        show2dsprite[spritenum >> 3] |= pow2char[spritenum & 7];
}


static void R_DrawMaskWall(short damaskwallcnt) {
    int32_t i, j, k, x, z, sectnum, z1, z2, lx, rx;
    sectortype *sec, *nsec;
    walltype* wal;

    // Retrieve pvWall ID.
    z = maskwall[damaskwallcnt];

    //Retrive world wall ID.
    wal = &wall[pv_walls[z].worldWallId];

    //Retrive sector ID
    sectnum = pv_walls[z].sectorId;

    //Retrive sector.
    sec = &sector[sectnum];

    //Retrive next sector.
    nsec = &sector[wal->nextsector];

    z1 = max(nsec->ceilingz, sec->ceilingz);
    z2 = min(nsec->floorz, sec->floorz);

    wallmost(uwall, z, sectnum, (uint8_t) 0);
    wallmost(uplc, z, (int32_t) wal->nextsector, (uint8_t) 0);
    for (x = pv_walls[z].screenSpaceCoo[0][VEC_COL];
         x <= pv_walls[z].screenSpaceCoo[1][VEC_COL]; x++)
        if (uplc[x] > uwall[x])
            uwall[x] = uplc[x];

    wallmost(dwall, z, sectnum, (uint8_t) 1);
    wallmost(dplc, z, (int32_t) wal->nextsector, (uint8_t) 1);
    for (x = pv_walls[z].screenSpaceCoo[0][VEC_COL];
         x <= pv_walls[z].screenSpaceCoo[1][VEC_COL]; x++)
        if (dplc[x] < dwall[x])
            dwall[x] = dplc[x];


    prepwall(z, wal);

    globalorientation = (int32_t) wal->cstat;
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
                            (int32_t) ((uint8_t) (sec->visibility + 16)));
    globalpal = (int32_t) wal->pal;
    globalshiftval = (picsiz[globalpicnum] >> 4);
    if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height)
        globalshiftval++;

    globalshiftval = 32 - globalshiftval;
    globalyscale = (wal->yrepeat << (globalshiftval - 19));
    if ((globalorientation & 4) == 0)
        globalzd = (((globalposz - z1) * globalyscale) << 8);
    else
        globalzd = (((globalposz - z2) * globalyscale) << 8);
    globalzd += (globalypanning << 24);
    if (globalorientation & 256)
        globalyscale = -globalyscale, globalzd = -globalzd;

    for (i = smostwallcnt - 1; i >= 0; i--) {
        j = smostwall[i];
        if ((pv_walls[j].screenSpaceCoo[0][VEC_COL] > pv_walls[z].screenSpaceCoo[
                 1][VEC_COL]) || (
                pv_walls[j].screenSpaceCoo[1][VEC_COL] < pv_walls[z].
                screenSpaceCoo[0][VEC_COL]))
            continue;
        if (wallfront(j, z))
            continue;

        lx = max(pv_walls[j].screenSpaceCoo[0][VEC_COL],
                 pv_walls[z].screenSpaceCoo[0][VEC_COL]);
        rx = min(pv_walls[j].screenSpaceCoo[1][VEC_COL],
                 pv_walls[z].screenSpaceCoo[1][VEC_COL]);

        switch (smostwalltype[i]) {
            case 0:
                if (lx <= rx) {
                    if ((lx == pv_walls[z].screenSpaceCoo[0][VEC_COL]) && (
                            rx == pv_walls[z].screenSpaceCoo[1][VEC_COL]))
                        return;
                    clearbufbyte(&dwall[lx], (rx - lx + 1) * sizeof(dwall[0]),
                                 0L);
                }
                break;
            case 1:
                k = smoststart[i] - pv_walls[j].screenSpaceCoo[0][VEC_COL];
                for (x = lx; x <= rx; x++)
                    if (smost[k + x] > uwall[x])
                        uwall[x] = smost[k + x];
                break;
            case 2:
                k = smoststart[i] - pv_walls[j].screenSpaceCoo[0][VEC_COL];
                for (x = lx; x <= rx; x++)
                    if (smost[k + x] < dwall[x])
                        dwall[x] = smost[k + x];
                break;
        }
    }

    /* maskwall */
    if ((searchit >= 1) && (searchx >= pv_walls[z].screenSpaceCoo[0][VEC_COL]) &&
        (searchx <= pv_walls[z].screenSpaceCoo[1][VEC_COL]))
        if ((searchy >= uwall[searchx]) && (searchy <= dwall[searchx])) {
            searchsector = sectnum;
            searchwall = pv_walls[z].worldWallId;
            searchstat = 4;
            searchit = 1;
        }

    if ((globalorientation & 128) == 0)
        maskwallscan(pv_walls[z].screenSpaceCoo[0][VEC_COL],
                     pv_walls[z].screenSpaceCoo[1][VEC_COL], uwall, dwall, swall,
                     lwall);
    else {
        if (globalorientation & 128) {
            if (globalorientation & 512)
                settrans(TRANS_REVERSE);
            else
                settrans(TRANS_NORMAL);
        }
        R_TransMaskWallScan(pv_walls[z].screenSpaceCoo[0][VEC_COL],
                          pv_walls[z].screenSpaceCoo[1][VEC_COL]);
    }
}

//
// Draw every transparent sprites in Back To Front Order.
// Also draw decals on the walls...
//
void drawmasks(void) {
    int32_t i, j, k, l, gap, xs, ys, xp, yp, yoff, yspan;
    /* int32_t zs, zp; */

    //Copy sprite address in a sprite proxy structure (pointers are easier to re-arrange than structs).
    for (i = spritesortcnt - 1; i >= 0; i--)
        tspriteptr[i] = &tsprite[i];


    //Generate screenspace coordinate (X column and Y distance).
    for (i = spritesortcnt - 1; i >= 0; i--) {
        //Translate and rotate the sprite in Camera space coordinate.
        xs = tspriteptr[i]->x - globalposx;
        ys = tspriteptr[i]->y - globalposy;
        yp = dmulscale6(xs, cosviewingrangeglobalang, ys,
                        sinviewingrangeglobalang);

        if (yp > (4 << 8)) {
            xp = dmulscale6(ys, cosglobalang, -xs, singlobalang);
            spritesx[i] = scale(xp + yp, xdimen << 7, yp);
        } else if ((tspriteptr[i]->cstat & 48) == 0) {
            spritesortcnt--; /* Delete face sprite if on wrong side! */
            //Move the sprite at the end of the array and decrease array length.
            if (i != spritesortcnt) {
                tspriteptr[i] = tspriteptr[spritesortcnt];
                spritesx[i] = spritesx[spritesortcnt];
                spritesy[i] = spritesy[spritesortcnt];
            }
            continue;
        }
        spritesy[i] = yp;
    }

    //FCS: Bubble sort ?! REally ?!?!?
    gap = 1;
    while (gap < spritesortcnt)
        gap = (gap << 1) + 1;
    for (gap >>= 1; gap > 0; gap >>= 1) /* Sort sprite list */
        for (i = 0; i < spritesortcnt - gap; i++)
            for (l = i; l >= 0; l -= gap) {
                if (spritesy[l] <= spritesy[l + gap])
                    break;

                swaplong((int32_t*) &tspriteptr[l],
                         (int32_t*) &tspriteptr[l + gap]);
                swaplong(&spritesx[l], &spritesx[l + gap]);
                swaplong(&spritesy[l], &spritesy[l + gap]);
            }

    if (spritesortcnt > 0)
        spritesy[spritesortcnt] = (spritesy[spritesortcnt - 1] ^ 1);

    ys = spritesy[0];
    i = 0;
    for (j = 1; j <= spritesortcnt; j++) {
        if (spritesy[j] == ys)
            continue;

        ys = spritesy[j];
        if (j > i + 1) {
            for (k = i; k < j; k++) {
                spritesz[k] = tspriteptr[k]->z;
                if ((tspriteptr[k]->cstat & 48) != 32) {
                    yoff = (int32_t) ((int8_t) (
                               (tiles[tspriteptr[k]->picnum].anim_flags >> 16) &
                               255)) + ((int32_t) tspriteptr[k]->yoffset);
                    spritesz[k] -= ((yoff * tspriteptr[k]->yrepeat) << 2);
                    yspan = (tiles[tspriteptr[k]->picnum].dim.height *
                             tspriteptr[k]->yrepeat << 2);
                    if (!(tspriteptr[k]->cstat & 128))
                        spritesz[k] -= (yspan >> 1);
                    if (klabs(spritesz[k] - globalposz) < (yspan >> 1))
                        spritesz[k] = globalposz;
                }
            }
            for (k = i + 1; k < j; k++)
                for (l = i; l < k; l++)
                    if (klabs(spritesz[k] - globalposz) < klabs(
                            spritesz[l] - globalposz)) {
                        swaplong((int32_t*) &tspriteptr[k],
                                 (int32_t*) &tspriteptr[l]);
                        swaplong(&spritesx[k], &spritesx[l]);
                        swaplong(&spritesy[k], &spritesy[l]);
                        swaplong(&spritesz[k], &spritesz[l]);
                    }
            for (k = i + 1; k < j; k++)
                for (l = i; l < k; l++)
                    if (tspriteptr[k]->statnum < tspriteptr[l]->statnum) {
                        swaplong((int32_t*) &tspriteptr[k],
                                 (int32_t*) &tspriteptr[l]);
                        swaplong(&spritesx[k], &spritesx[l]);
                        swaplong(&spritesy[k], &spritesy[l]);
                    }
        }
        i = j;
    }

    while ((spritesortcnt > 0) && (maskwallcnt > 0)) /* While BOTH > 0 */
    {
        j = maskwall[maskwallcnt - 1];
        if (R_SpriteWallFront(tspriteptr[spritesortcnt - 1],
                            pv_walls[j].worldWallId) == 0)
            R_DrawSprite(--spritesortcnt);
        else {
            /* Check to see if any sprites behind the masked wall... */
            k = -1;
            gap = 0;
            for (i = spritesortcnt - 2; i >= 0; i--)
                if ((pv_walls[j].screenSpaceCoo[0][VEC_COL] <= (
                         spritesx[i] >> 8)) && (
                        (spritesx[i] >> 8) <= pv_walls[j].screenSpaceCoo[1][
                            VEC_COL]))
                    if (R_SpriteWallFront(tspriteptr[i], pv_walls[j].worldWallId)
                        == 0) {
                        R_DrawSprite(i);
                        tspriteptr[i]->owner = -1;
                        k = i;
                        gap++;
                    }
            if (k >= 0) /* remove holes in sprite list */
            {
                for (i = k; i < spritesortcnt; i++)
                    if (tspriteptr[i]->owner >= 0) {
                        if (i > k) {
                            tspriteptr[k] = tspriteptr[i];
                            spritesx[k] = spritesx[i];
                            spritesy[k] = spritesy[i];
                        }
                        k++;
                    }
                spritesortcnt -= gap;
            }

            /* finally safe to draw the masked wall */
            R_DrawMaskWall(--maskwallcnt);
        }
    }
    while (spritesortcnt > 0)
        R_DrawSprite(--spritesortcnt);
    while (maskwallcnt > 0)
        R_DrawMaskWall(--maskwallcnt);
}


//
// Render a sprite on screen.
// This is used by the Engine but also the Game module when
// drawing the HUD or the Weapon held by the player.
//
void rotatesprite(
    int32_t sx,
    int32_t sy,
    int32_t z,
    short a,
    short picnum,
    int8_t dashade,
    uint8_t dapalnum,
    uint8_t dastat,
    int32_t cx1,
    int32_t cy1,
    int32_t cx2,
    int32_t cy2
) {
    // If 2D target coordinate do not make sense (left > right)..
    if ((cx1 > cx2) || (cy1 > cy2)) {
        return;
    }
    if (z <= 16) {
        return;
    }

    if (tiles[picnum].anim_flags & 192) {
        picnum += animateoffs(picnum);
    }
    // Does the tile has negative dimensions ?
    if ((tiles[picnum].dim.width <= 0) || (tiles[picnum].dim.height <= 0)) {
        return;
    }

    if (((dastat & 128) == 0) || (numpages < 2) || (beforedrawrooms != 0)) {
        dorotatesprite(sx, sy, z, a, picnum, dashade, dapalnum, dastat, cx1, cy1, cx2, cy2);
    }
}

