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

#include "renderer/renderer.h"
#include "renderer/draw.h"
#include "build/platform.h"
#include "build/build.h"
#include "build/engine.h"
#include "video/display.h"
#if WIN32
#include "io.h"
#endif


extern i32 xyaspect;
extern i32 viewingrangerecip;

extern i32 beforedrawrooms;

extern short radarang[1280];
extern short radarang2[MAXXDIM + 1];

extern i32 stereowidth;
extern i32 stereopixelwidth;
extern i32 ostereopixelwidth;

extern i32 viewoffset;


i32 globalposx;
i32 globalposy;
i32 globalposz;
i32 globalhoriz;
i32 globalx1;
i32 globaly1;
i32 globalx2;
i32 globaly2;
i32 globalx3;
i32 globaly3;
i32 globaluclip;
i32 globaldclip;
i32 globvis = 0;
i32 globalshade;
i32 globalorientation;
i32 globalvisibility;
i32 globalhisibility;
i32 globalpisibility;
i32 globalcisibility;
i16 globalang;
static i16 globalcursectnum;
i16 globalpicnum;
u8 globalxshift;
u8 globalyshift;
u8 globparaceilclip;
u8 globparaflorclip;
u8* globalbufplc;

// (up-most pixel on column x that can still be drawn to)
short umost[MAXXDIM + 1];

// (down-most pixel +1 on column x that can still be drawn to)
short dmost[MAXXDIM + 1];

i32 cosglobalang;
i32 singlobalang;

i32 cosviewingrangeglobalang;
i32 sinviewingrangeglobalang;


// bunchWallsList contains the list of walls in a bunch.
short bunchWallsList[MAXWALLSB];

short bunchfirst[MAXWALLSB];
short bunchlast[MAXWALLSB];

char r_tempbuf[MAXWALLS];


//FCS: Number of colums to draw. ALWAYS set to the screen dimension width.
short numhits;

static short numbunches;

// Num walls to potentially render.
static short numscans;


i32 getpalookup(i32 davis, i32 dashade) {
    return (min(max(dashade+(davis>>8),0), numpalookups-1));
}


/*
 FCS:
 Scan through sectors using portals (a portal is wall with a nextsector attribute >= 0).
 Flood is prevented if a portal does not face the POV.
 */
void scansector(short sectnum) {
    walltype *wal, *wal2;
    spritetype* spr;
    i32 xs, ys, x1, y1, x2, y2, xp1, yp1, xp2 = 0, yp2 = 0, tempint;
    short z, zz, startwall, endwall, numscansbefore, scanfirst, bunchfrst;
    short nextsectnum;

    //The stack storing sectors to visit.
    short sectorsToVisit[256], numSectorsToVisit;


    if (sectnum < 0)
        return;

    if (automapping)
        show2dsector[sectnum >> 3] |= pow2char[sectnum & 7];

    sectorsToVisit[0] = sectnum;
    numSectorsToVisit = 1;
    do {
        sectnum = sectorsToVisit[--numSectorsToVisit];

        //Add every script in the current sector as potentially visible.
        for (z = headspritesect[sectnum]; z >= 0; z = nextspritesect[z]) {
            spr = &sprite[z];
            if ((((spr->cstat & 0x8000) == 0) || (showinvisibility)) &&
                (spr->xrepeat > 0) && (spr->yrepeat > 0) &&
                (spritesortcnt < MAXSPRITESONSCREEN)) {
                xs = spr->x - globalposx;
                ys = spr->y - globalposy;
                if ((spr->cstat & 48) || (
                        xs * cosglobalang + ys * singlobalang > 0)) {
                    copybufbyte(spr, &tsprite[spritesortcnt],
                                sizeof(spritetype));
                    tsprite[spritesortcnt++].owner = z;
                }
            }
        }

        //Mark the current sector bit as "visited" in the bitvector
        visitedSectors[sectnum >> 3] |= pow2char[sectnum & 7];

        bunchfrst = numbunches;
        numscansbefore = numscans;

        startwall = sector[sectnum].wallptr;
        endwall = startwall + sector[sectnum].wallnum;
        scanfirst = numscans;
        for (z = startwall, wal = &wall[z]; z < endwall; z++, wal++) {
            nextsectnum = wal->nextsector;

            wal2 = &wall[wal->point2];

            // In camera space the center is the player.
            // Tranform the 2 Wall endpoints (x,y) from worldspace to camera space.
            // After that we have two vectors starting from the camera and going to the endpoints (x1,y1) and (x2,y2).
            x1 = wal->x - globalposx;
            y1 = wal->y - globalposy;

            x2 = wal2->x - globalposx;
            y2 = wal2->y - globalposy;

            // If this is a portal...
            if ((nextsectnum >= 0) && ((wal->cstat & 32) == 0))
                //If this portal has not been visited yet.
                if ((visitedSectors[nextsectnum >> 3] & pow2char[
                         nextsectnum & 7]) == 0) {
                    //Cross product -> Z component
                    tempint = x1 * y2 - x2 * y1;

                    // Using cross product, determine if the portal is facing us or not.
                    // If it is: Add it to the stack and bump the stack counter.
                    // This line is equivalent to tempint < 0x40000
                    if (((u32) tempint + 262144) < 524288)
                    // ??? What is this test ?? How acute the angle is ?
                    {
                        //(x2-x1)*(x2-x1)+(y2-y1)*(y2-y1) is the squared length of the wall
                        // ??? What is this test ?? How acute the angle is ?
                        if (mulscale5(tempint, tempint) <= (x2 - x1) * (x2 - x1)
                            + (y2 - y1) * (y2 - y1))
                            sectorsToVisit[numSectorsToVisit++] = nextsectnum;
                    }
                }

            // Rotate the wall endpoints vectors according to the player orientation.
            // This is a regular rotation matrix using [29.3] fixed point.
            if ((z == startwall) || (wall[z - 1].point2 != z)) {
                //If this is the first endpoint of the bunch, rotate: This is a standard cos sin 2D rotation matrix projection
                xp1 = dmulscale6(y1, cosglobalang, -x1, singlobalang);
                yp1 = dmulscale6(x1, cosviewingrangeglobalang, y1,
                                 sinviewingrangeglobalang);
            } else {
                //If this is NOT the first endpoint, Save the coordinate for next loop.
                xp1 = xp2;
                yp1 = yp2;
            }

            // Rotate: This is a standard cos sin 2D rotation matrix projection
            xp2 = dmulscale6(y2, cosglobalang, -x2, singlobalang);
            yp2 = dmulscale6(x2, cosviewingrangeglobalang, y2,
                             sinviewingrangeglobalang);


            // Equivalent of a near plane clipping ?
            if ((yp1 < 256) && (yp2 < 256))
                goto skipitaddwall;

            /* If wall's NOT facing you */
            if (dmulscale32(xp1, yp2, -xp2, yp1) >= 0)
                goto skipitaddwall;

            // The wall is still not eligible for rendition: Let's do some more Frustrum culling !!
            if (xp1 >= -yp1) {

                if ((xp1 > yp1) || (yp1 == 0))
                    goto skipitaddwall;

                //Project the point onto screen and see in which column it belongs.
                pv_walls[numscans].screenSpaceCoo[0][VEC_COL] =
                    halfxdimen + scale(xp1, halfxdimen, yp1);
                if (xp1 >= 0)
                    pv_walls[numscans].screenSpaceCoo[0][VEC_COL]++;
                /* Fix for SIGNED divide */

                if (pv_walls[numscans].screenSpaceCoo[0][VEC_COL] >= xdimen)
                    pv_walls[numscans].screenSpaceCoo[0][VEC_COL] = xdimen - 1;

                pv_walls[numscans].screenSpaceCoo[0][VEC_DIST] = yp1;
            } else {

                if (xp2 < -yp2)
                    goto skipitaddwall;

                pv_walls[numscans].screenSpaceCoo[0][VEC_COL] = 0;
                tempint = yp1 - yp2 + xp1 - xp2;

                if (tempint == 0)
                    goto skipitaddwall;

                pv_walls[numscans].screenSpaceCoo[0][VEC_DIST] = yp1 + scale(
                    yp2 - yp1, xp1 + yp1, tempint);
            }

            if (pv_walls[numscans].screenSpaceCoo[0][VEC_DIST] < 256)
                goto skipitaddwall;

            if (xp2 <= yp2) {

                if ((xp2 < -yp2) || (yp2 == 0))
                    goto skipitaddwall;
                pv_walls[numscans].screenSpaceCoo[1][VEC_COL] =
                    halfxdimen + scale(xp2, halfxdimen, yp2) - 1;
                if (xp2 >= 0)
                    pv_walls[numscans].screenSpaceCoo[1][VEC_COL]++;
                /* Fix for SIGNED divide */
                if (pv_walls[numscans].screenSpaceCoo[1][VEC_COL] >= xdimen)
                    pv_walls[numscans].screenSpaceCoo[1][VEC_COL] = xdimen - 1;
                pv_walls[numscans].screenSpaceCoo[1][VEC_DIST] = yp2;
            } else {

                if (xp1 > yp1)
                    goto skipitaddwall;
                pv_walls[numscans].screenSpaceCoo[1][VEC_COL] = xdimen - 1;
                tempint = xp2 - xp1 + yp1 - yp2;
                if (tempint == 0)
                    goto skipitaddwall;
                pv_walls[numscans].screenSpaceCoo[1][VEC_DIST] = yp1 + scale(
                    yp2 - yp1, yp1 - xp1, tempint);
            }
            if ((pv_walls[numscans].screenSpaceCoo[1][VEC_DIST] < 256) || (
                    pv_walls[numscans].screenSpaceCoo[0][VEC_COL] > pv_walls[
                        numscans].screenSpaceCoo[1][VEC_COL]))
                goto skipitaddwall;

            // Made it all the way!
            // Time to add this wall information to the stack of wall potentially visible.
            pv_walls[numscans].sectorId = sectnum;
            pv_walls[numscans].worldWallId = z;

            //Save the camera space wall endpoints coordinate (camera origin at player location + rotated according to player orientation).
            pv_walls[numscans].cameraSpaceCoo[0][VEC_X] = xp1;
            pv_walls[numscans].cameraSpaceCoo[0][VEC_Y] = yp1;
            pv_walls[numscans].cameraSpaceCoo[1][VEC_X] = xp2;
            pv_walls[numscans].cameraSpaceCoo[1][VEC_Y] = yp2;


            bunchWallsList[numscans] = numscans + 1;
            numscans++;

        skipitaddwall:

            if ((wall[z].point2 < z) && (scanfirst < numscans)) {
                bunchWallsList[numscans - 1] = scanfirst;
                scanfirst = numscans;
            }
        }

        //FCS: TODO rename this p2[] to bunchList[] or something like that. This name is an abomination
        //     DONE, p2 is now called "bunchWallsList".

        //Break down the list of walls for this sector into bunchs. Since a bunch is a
        // continuously visible list of wall: A sector can generate many bunches.
        for (z = numscansbefore; z < numscans; z++) {
            if ((wall[pv_walls[z].worldWallId].point2 !=
                 pv_walls[bunchWallsList[z]].worldWallId) || (
                    pv_walls[z].screenSpaceCoo[1][VEC_COL] >= pv_walls[
                        bunchWallsList[z]].screenSpaceCoo[0][VEC_COL])) {
                // Create an entry in the bunch list
                bunchfirst[numbunches++] = bunchWallsList[z];

                //Mark the end of the bunch wall list.
                bunchWallsList[z] = -1;
            }
        }

        //For each bunch, find the last wall and cache it in bunchlast.
        for (z = bunchfrst; z < numbunches; z++) {
            for (zz = bunchfirst[z]; bunchWallsList[zz] >= 0;
                 zz = bunchWallsList[zz]);
            bunchlast[z] = zz;
        }

    } while (numSectorsToVisit > 0);
    // do this until the stack of sectors to visit if empty.
}


static void dosetaspect(void) {
    i32 i, j, k, x, xinc;

    if (xyaspect != oxyaspect) {
        oxyaspect = xyaspect;
        j = xyaspect * 320;
        horizlookup2[horizycent - 1] = divscale26(131072, j);
        for (i = ydim * 4 - 1; i >= 0; i--)
            if (i != (horizycent - 1)) {
                horizlookup[i] = divscale28(1, i - (horizycent - 1));
                horizlookup2[i] = divscale14(klabs(horizlookup[i]), j);
            }
    }


    if ((xdimen != oxdimen) || (viewingrange != oviewingrange)) {
        oxdimen = xdimen;
        oviewingrange = viewingrange;
        xinc = mulscale32(viewingrange * 320, xdimenrecip);
        x = (640 << 16) - mulscale1(xinc, xdimen);
        for (i = 0; i < xdimen; i++) {
            j = (x & 65535);
            k = (x >> 16);
            x += xinc;
            if (j != 0)
                j = mulscale16((i32) radarang[k + 1] - (i32) radarang[k], j);
            radarang2[i] = (short) (((i32) radarang[k] + j) >> 6);
        }
    }
}


/*
  Algorithm:

  1. Take wall 1 vector [point1,point2] and using two cross products
     determine if the two endpoints of wall 2 are on the same side of
     Wall 1 plan. If they are then we can determine according to globalposx
     and globalposy if wall2 is before or after wall1's plan.

  2. Do the same thing again but this time with wall2's plan.
     Try to find if wall1 is in front of behind wall2's plan.

  Key concept: If a cross-product is equal to 0 this mean they are parallel.

  Return: pvWallID1 in the potentially visible wall list is in front of
  pvWallID2 (in the same potentially visible list).
*/
int wallfront(i32 pvWallID1, i32 pvWallID2) {
    walltype* wal;
    i32 x11, y11, x21, y21, x12, y12, x22, y22, dx, dy, t1, t2;

    //It seems we are going to work in Worldspace coordinates.
    wal = &wall[pv_walls[pvWallID1].worldWallId];
    x11 = wal->x;
    y11 = wal->y;
    wal = &wall[wal->point2];
    x21 = wal->x;
    y21 = wal->y;
    wal = &wall[pv_walls[pvWallID2].worldWallId];
    x12 = wal->x;
    y12 = wal->y;
    wal = &wall[wal->point2];
    x22 = wal->x;
    y22 = wal->y;


    //This is part 1

    //Wall 1's vector
    dx = x21 - x11;
    dy = y21 - y11;

    //This is a cross-product between Wall 1 vector and the [Wall 1 Point 1-> Wall 2 Point 1] vector
    t1 = dmulscale2(x12 - x11, dy, -dx, y12 - y11); /* p1(l2) vs. l1 */
    //This is a cross-product between Wall 1 vector and the [Wall 1 Point 1-> Wall 2 Point 2] vector
    t2 = dmulscale2(x22 - x11, dy, -dx, y22 - y11); /* p2(l2) vs. l1 */

    //If the vectors a parallel, then the cross-product is zero.
    if (t1 == 0) {
        //wall2's point1 is on wall1's plan.
        t1 = t2;
        if (t1 == 0) // Those two walls are on the same plan.
        {
            //Wall 2's point 2 is on wall1's plan.
            return (-1);
        }
    }
    if (t2 == 0)
        t2 = t1;


    //This XOR just determine if the cross-product have the same sign and hence if both points are on the same side of wall 1 plan.
    //Test if both points of wall2 are on the same side of wall 1 (in front or behind).
    if ((t1 ^ t2) >= 0) {
        //cross-product have the same sign: Both points of wall2 are on the same side of wall1 : An answer is possible !!

        //Now is time to take into account the camera position and determine which of wall1 or wall2 is seen first.
        t2 = dmulscale2(globalposx - x11, dy, -dx, globalposy - y11);
        /* pos vs. l1 */

        //Test the cross product sign difference.
        //If (t2^t1) >= 0 then  both cross product had different sign so wall1 is in front of wall2
        //otherwise wall2 is in front of wall1
        return ((t2 ^ t1) >= 0);
    }


    //This is part 2
    //Do it again but this time will wall2's plan.

    //Wall 2's vector
    dx = x22 - x12;
    dy = y22 - y12;

    t1 = dmulscale2(x11 - x12, dy, -dx, y11 - y12); /* p1(l1) vs. l2 */
    t2 = dmulscale2(x21 - x12, dy, -dx, y21 - y12); /* p2(l1) vs. l2 */
    if (t1 == 0) {
        t1 = t2;
        if (t1 == 0)
            return (-1);
    }
    if (t2 == 0)
        t2 = t1;
    if ((t1 ^ t2) >= 0) {
        t2 = dmulscale2(globalposx - x12, dy, -dx, globalposy - y12);
        /* pos vs. l2 */
        return ((t2 ^ t1) < 0);
    }

    //FCS: No wall is in front of the other's plan: This means they are crossing.
    return (-2);
}


//Return 1 if bunch firstBunchID is in from of bunch secondBunchID.
static int bunchfront(i32 firstBunchID, i32 secondBunchID) {
    i32 x1b1, x2b1, x1b2, x2b2;


    x1b1 = pv_walls[bunchfirst[firstBunchID]].screenSpaceCoo[0][VEC_COL];
    x2b2 = pv_walls[bunchlast[secondBunchID]].screenSpaceCoo[1][VEC_COL] + 1;
    if (x1b1 >= x2b2) {
        //Bunch 1 left side is completely on the right of bunch2's right in screenspace: They do not overlap.
        return (-1);
    }


    x1b2 = pv_walls[bunchfirst[secondBunchID]].screenSpaceCoo[0][VEC_COL];
    x2b1 = pv_walls[bunchlast[firstBunchID]].screenSpaceCoo[1][VEC_COL] + 1;
    if (x1b2 >= x2b1) {
        //Bunch 2 left side is completely on the right of bunch 1 right side: They do not overlap.
        return (-1);
    }


    if (x1b1 >= x1b2) {
        //Get the last wall in the bunch2.
        int lastWallID;
        for (lastWallID = bunchfirst[secondBunchID];
             pv_walls[lastWallID].screenSpaceCoo[1][VEC_COL] < x1b1;
             lastWallID = bunchWallsList[lastWallID]);

        return (wallfront(bunchfirst[firstBunchID], lastWallID));
    } else {
        //Get the last wall in the bunch.
        int lastWallID;
        for (lastWallID = bunchfirst[firstBunchID];
             pv_walls[lastWallID].screenSpaceCoo[1][VEC_COL] < x1b2;
             lastWallID = bunchWallsList[lastWallID]);

        return (wallfront(lastWallID, bunchfirst[secondBunchID]));
    }
}


//
// Draw every wall in front to back order.
//
void drawrooms(
    i32 posx,
    i32 posy,
    i32 posz,
    short ang,
    i32 horiz,
    short cursectnum
) {
    i32 i, j, z, closest;
    //Ceiling and Floor height at the player position.
    i32 cz, fz;
    short *shortptr1, *shortptr2;

    // When visualizing the rendering process, part of the screen
    // are not updated: In order to avoid the "ghost effect", we
    // clear the framebuffer to black.
    if (CLEAR_FRAMEBUFFER) {
        clear2dscreen();
    }


    //CODE EXPLORATION
    /*
    if( KB_KeyDown[0x39]){ // 0x39 = SPACE
        //CODE EXPLORATION
        WriteLastPaletteToFile();
        WriteTranslucToFile();
    }
    */

    //pixelsAllowed = pixelRenderable;
#if RENDER_LIMIT_PIXELS
    pixelsAllowed = 100000000;
#endif
    //printf("%d\n",pixelsAllowed);

    beforedrawrooms = 0;

    // What was the point of having those values as
    // parameters of this function....
    // If it is to overwrite the values with the global variables ?!?!?
    globalposx = posx;
    globalposy = posy;
    globalposz = posz;
    // Mask and keep only 11 bits of angle value.
    globalang = (ang & 2047);

    globalhoriz = mulscale16(horiz - 100, xdimenscale) + (ydimen >> 1);
    globaluclip = (0 - globalhoriz) * xdimscale;
    globaldclip = (ydimen - globalhoriz) * xdimscale;

    i = mulscale16(xdimenscale, viewingrangerecip);
    globalpisibility = mulscale16(parallaxvisibility, i);
    globalvisibility = mulscale16(visibility, i);
    globalhisibility = mulscale16(globalvisibility, xyaspect);
    globalcisibility = mulscale8(globalhisibility, 320);

    globalcursectnum = cursectnum;
    totalclocklock = totalclock;

    cosglobalang = sintable[(globalang + 512) & 2047];
    singlobalang = sintable[globalang & 2047];
    cosviewingrangeglobalang = mulscale16(cosglobalang, viewingrange);
    sinviewingrangeglobalang = mulscale16(singlobalang, viewingrange);

    if (stereomode != 0) {
        if (stereopixelwidth != ostereopixelwidth) {
            ostereopixelwidth = stereopixelwidth;
            xdimen = (windowx2 - windowx1 + 1) + (stereopixelwidth << 1);
            halfxdimen = (xdimen >> 1);
            xdimenrecip = divscale32(1L, xdimen);
            setaspect(divscale16(xdimen, windowx2 - windowx1 + 1), yxaspect);
        }

        if ((!(activepage & 1)) ^ inpreparemirror) {
            for (i = windowx1; i < windowx1 + (stereopixelwidth << 1); i++) {
                startumost[i] = 1, startdmost[i] = 0;
            }
            for (; i < windowx2 + 1 + (stereopixelwidth << 1); i++) {
                startumost[i] = windowy1, startdmost[i] = windowy2 + 1;
            }
            viewoffset = windowy1 * bytesperline + windowx1 - (
                             stereopixelwidth << 1);
            i = stereowidth;
        } else {
            for (i = windowx1; i < windowx2 + 1; i++) {
                startumost[i] = windowy1, startdmost[i] = windowy2 + 1;
            }
            for (; i < windowx2 + 1 + (stereopixelwidth << 1); i++) {
                startumost[i] = 1, startdmost[i] = 0;
            }
            viewoffset = windowy1 * bytesperline + windowx1;
            i = -stereowidth;
        }
        globalposx += mulscale24(singlobalang, i);
        globalposy -= mulscale24(cosglobalang, i);

    }

    if ((xyaspect != oxyaspect) || (xdimen != oxdimen) || (viewingrange != oviewingrange))
        dosetaspect();

    frameoffset = frameplace + viewoffset;

    //Clear the bit vector that keep track of what sector has been flooded in.
    clearbufbyte(visitedSectors, ((numsectors + 7) >> 3), 0L);

    //Clear the occlusion array.
    shortptr1 = &startumost[windowx1];
    shortptr2 = &startdmost[windowx1];
    i = xdimen - 1;
    do {
        umost[i] = shortptr1[i] - windowy1;
        dmost[i] = shortptr2[i] - windowy1;
        i--;
    } while (i != 0);
    umost[0] = shortptr1[0] - windowy1;
    dmost[0] = shortptr2[0] - windowy1;

    //NumHits is the number of column to draw.
    numhits = xdimen;
    //Num walls to potentially render.
    numscans = 0;

    numbunches = 0;
    maskwallcnt = 0;
    smostwallcnt = 0;
    smostcnt = 0;
    spritesortcnt = 0;

    if (globalcursectnum >= MAXSECTORS)
        globalcursectnum -= MAXSECTORS;
    else {
        // Even if the player leaves the map, the engine will keep on rendering from the last visited sector.
        // Save it.
        i = globalcursectnum;
        updatesector(globalposx, globalposy, &globalcursectnum);
        //Seem the player has left the map since updatesector cannot locate him -> Restore to the last known sector.
        if (globalcursectnum < 0)
            globalcursectnum = i;
    }

    globparaceilclip = 1;
    globparaflorclip = 1;

    // Update the ceiling and floor Z coordinate for the player's 2D position.
    getzsofslope(globalcursectnum, globalposx, globalposy, &cz, &fz);

    if (globalposz < cz)
        globparaceilclip = 0;
    if (globalposz > fz)
        globparaflorclip = 0;

    //Build the list of potentially visible wall in to "bunches".
    scansector(globalcursectnum);

    if (inpreparemirror) {
        inpreparemirror = 0;
        mirrorsx1 = xdimen - 1;
        mirrorsx2 = 0;
        for (i = numscans - 1; i >= 0; i--) {
            if (wall[pv_walls[i].worldWallId].nextsector < 0)
                continue;
            if (pv_walls[i].screenSpaceCoo[0][VEC_COL] < mirrorsx1)
                mirrorsx1 = pv_walls[i].screenSpaceCoo[0][VEC_COL];
            if (pv_walls[i].screenSpaceCoo[1][VEC_COL] > mirrorsx2)
                mirrorsx2 = pv_walls[i].screenSpaceCoo[1][VEC_COL];
        }

        if (stereomode) {
            mirrorsx1 += (stereopixelwidth << 1);
            mirrorsx2 += (stereopixelwidth << 1);
        }

        for (i = 0; i < mirrorsx1; i++)
            if (umost[i] <= dmost[i]) {
                umost[i] = 1;
                dmost[i] = 0;
                numhits--;
            }
        for (i = mirrorsx2 + 1; i < xdimen; i++)
            if (umost[i] <= dmost[i]) {
                umost[i] = 1;
                dmost[i] = 0;
                numhits--;
            }

        drawalls(0L);
        numbunches--;
        bunchfirst[0] = bunchfirst[numbunches];
        bunchlast[0] = bunchlast[numbunches];

        mirrorsy1 = min(umost[mirrorsx1], umost[mirrorsx2]);
        mirrorsy2 = max(dmost[mirrorsx1], dmost[mirrorsx2]);
    }

    // scansector has generated the bunches, it is now time to see which ones to render.
    // numhits is the number of column of pixels to draw: (if the screen is 320x200 then numhits starts at 200).
    // Due to rounding error, not all columns may be drawn so an additional stop condition is here:
    // When every bunches have been tested for rendition.
    while ((numbunches > 0) && (numhits > 0)) {
        // r_tempbuf is used to mark which bunches have been elected as "closest".
        // if tempbug[x] == 1 then it should be skipped.
        clearbuf(&r_tempbuf[0], (i32) ((numbunches + 3) >> 2), 0L);

        /* Almost works, but not quite :( */
        closest = 0;
        r_tempbuf[closest] = 1;
        for (i = 1; i < numbunches; i++) {
            if ((j = bunchfront(i, closest)) < 0)
                continue;
            r_tempbuf[i] = 1;
            if (j == 0) {
                r_tempbuf[closest] = 1;
                closest = i;
            }
        }

        /* Double-check */
        for (i = 0; i < numbunches; i++) {
            if (r_tempbuf[i])
                continue;
            if ((j = bunchfront(i, closest)) < 0)
                continue;
            r_tempbuf[i] = 1;
            if (j == 0) {
                r_tempbuf[closest] = 1;
                closest = i, i = 0;
            }
        }

        //Draw every solid walls with ceiling/floor in the bunch "closest"
        drawalls(closest);

        if (automapping) {
            for (z = bunchfirst[closest]; z >= 0; z = bunchWallsList[z])
                show2dwall[pv_walls[z].worldWallId >> 3] |=
                    pow2char[pv_walls[z].worldWallId & 7];
        }

        //Since we just rendered a bunch, lower the current stack element so we can treat the next item
        numbunches--;
        //...and move the bunch at the top of the stack so we won't iterate on it again...
        bunchfirst[closest] = bunchfirst[numbunches];
        bunchlast[closest] = bunchlast[numbunches];
    }
}


void R_ClearView(i32 col) {
    if (qsetmode != 200) {
        return;
    }

    i32 dx = windowx2 - windowx1 + 1;
    col += (col << 8);
    col += (col << 16);

    u8* p = &frameplace[ylookup[windowy1] + windowx1];
    for (i32 y = windowy1; y <= windowy2; y++) {
        clearbufbyte(p, dx, col);
        p += ylookup[1];
    }
    faketimerhandler();
}


void drawline256(i32 x1, i32 y1, i32 x2, i32 y2, u8 col) {
    i32 dx, dy, i, j, inc, plc, daend;
    u8* p;
    col = palookup[0][col];

    dx = x2 - x1;
    dy = y2 - y1;
    if (dx >= 0) {
        if ((x1 >= wx2) || (x2 < wx1))
            return;
        if (x1 < wx1)
            y1 += scale(wx1 - x1, dy, dx), x1 = wx1;
        if (x2 > wx2)
            y2 += scale(wx2 - x2, dy, dx), x2 = wx2;
    } else {
        if ((x2 >= wx2) || (x1 < wx1))
            return;
        if (x2 < wx1)
            y2 += scale(wx1 - x2, dy, dx), x2 = wx1;
        if (x1 > wx2)
            y1 += scale(wx2 - x1, dy, dx), x1 = wx2;
    }
    if (dy >= 0) {
        if ((y1 >= wy2) || (y2 < wy1))
            return;
        if (y1 < wy1)
            x1 += scale(wy1 - y1, dx, dy), y1 = wy1;
        if (y2 > wy2)
            x2 += scale(wy2 - y2, dx, dy), y2 = wy2;
    } else {
        if ((y2 >= wy2) || (y1 < wy1))
            return;
        if (y2 < wy1)
            x2 += scale(wy1 - y2, dx, dy), y2 = wy1;
        if (y1 > wy2)
            x1 += scale(wy2 - y1, dx, dy), y1 = wy2;
    }

    if (klabs(dx) >= klabs(dy)) {
        if (dx == 0)
            return;
        if (dx < 0) {
            i = x1;
            x1 = x2;
            x2 = i;
            i = y1;
            y1 = y2;
            y2 = i;
        }

        inc = divscale12(dy, dx);
        plc = y1 + mulscale12((2047 - x1) & 4095, inc);
        i = ((x1 + 2048) >> 12);
        daend = ((x2 + 2048) >> 12);
        for (; i < daend; i++) {
            j = (plc >> 12);
            if ((j >= startumost[i]) && (j < startdmost[i]))
                drawpixel(ylookup[j] + i + frameplace, col);
            plc += inc;
        }
    } else {
        if (dy < 0) {
            i = x1;
            x1 = x2;
            x2 = i;
            i = y1;
            y1 = y2;
            y2 = i;
        }

        inc = divscale12(dx, dy);
        plc = x1 + mulscale12((2047 - y1) & 4095, inc);
        i = ((y1 + 2048) >> 12);
        daend = ((y2 + 2048) >> 12);
        p = ylookup[i] + frameplace;
        for (; i < daend; i++) {
            j = (plc >> 12);
            if ((i >= startumost[j]) && (i < startdmost[j]))
                drawpixel(j + p, col);
            plc += inc;
            p += ylookup[1];
        }
    }
}


/*
 *  This is ryan's change. SDL requires me to call SDL_UpdateRect() to force
 *  vid updates without a SDL_Flip() call, but there's no such thing in the
 *  DOS version of this program, so text won't show up sometimes without
 *  my update call in Linux.  However, to get a nice shadow effect on some
 *  text, Ken draws a string at an offset and darker, and then on top of it
 *  draws the actual string. Two SDL_UpdateRect() calls in over top of each
 *  other cause flicker, so I have this function here so the shadow can
 *  be drawn with _noupdate, and the actual string is draw with an update.
 */
void printext256(i32 xpos, i32 ypos, short col, short backcol,
                 char* name, u8 fontsize) {
    i32 stx, i, x, y, charxsiz;
    u8 *fontptr, *letptr, *ptr;


    stx = xpos;

    if (fontsize) {
        fontptr = smalltextfont;
        charxsiz = 4;
    } else {
        fontptr = textfont;
        charxsiz = 8;
    }

    //For each character in the string.
    for (i = 0; name[i]; i++) {
        letptr = &fontptr[name[i] << 3];
        ptr = ylookup[ypos + 7] + (stx - fontsize) + frameplace;
        for (y = 7; y >= 0; y--) {
            for (x = charxsiz - 1; x >= 0; x--) {
                if (letptr[y] & pow2char[7 - fontsize - x])
                    ptr[x] = (u8) col;
                else if (backcol >= 0)
                    ptr[x] = (u8) backcol;
            }
            ptr -= ylookup[1];
        }
        stx += charxsiz;
    }

    _updateScreenRect(xpos, ypos, charxsiz * i, 8);
}
