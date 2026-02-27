/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */

/* SUPERBUILD define is in engine.h ... */

#define ENGINE

#if WIN32
#include "io.h"
#endif

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "build/platform.h"
#include "build/build.h"
#include "build/engine.h"
#include "build/tiles.h"
#include "video/display.h"
#include "renderer/draw.h"
#include "memory/cache.h"

#define MAXPERMS 512


void initksqrt(void);


int32_t stereowidth = 23040, stereopixelwidth = 28, ostereopixelwidth = -1;
int32_t stereomode = 0, visualpage, activepage, whiteband, blackband;
uint8_t oa1, o3c2, ortca, ortcb, overtbits, laststereoint;

/* used to be static. --ryan. */
uint8_t moustat = 0;

int32_t transarea = 0, beforedrawrooms = 1;

/* used to be static. --ryan. */
int32_t oxdimen = -1, oviewingrange = -1, oxyaspect = -1;

/* used to be static. --ryan. */
int32_t curbrightness = 0;

/*
 * !!! used to be static. If we ever put the original setgamemode() back, this
 * !!! can be made static again.  --ryan.
 */
uint8_t permanentlock = 255;
int32_t mapversion;

uint8_t picsiz[MAXTILES], tilefilenum[MAXTILES];
int32_t lastageclock;
int32_t tilefileoffs[MAXTILES];

int32_t artsize = 0, cachesize = 0;

short radarang[1280], radarang2[MAXXDIM + 1];
uint8_t pow2char[8] = {1, 2, 4, 8, 16, 32, 64, -128};
int32_t pow2long[32] = {
    1L, 2L, 4L, 8L,
    16L, 32L, 64L, 128L,
    256L, 512L, 1024L, 2048L,
    4096L, 8192L, 16384L, 32768L,
    65536L, 131072L, 262144L, 524288L,
    1048576L, 2097152L, 4194304L, 8388608L,
    16777216L, 33554432L, 67108864L, 134217728L,
    268435456L, 536870912L, 1073741824L, 2147483647L,
};
int32_t reciptable[2048], fpuasm;

extern uint8_t britable[16][64];
uint8_t textfont[1024], smalltextfont[1024];


int16_t bakumost[MAXXDIM + 1];
int16_t bakdmost[MAXXDIM + 1];

short uplc[MAXXDIM + 1];
short dplc[MAXXDIM + 1];

int32_t xdimen = -1;
int32_t xdimenrecip;
int32_t halfxdimen;
int32_t xdimenscale;
int32_t xdimscale;

int32_t wx1, wy1, wx2, wy2, ydimen;
int32_t viewoffset;

int32_t rxi[8];
int32_t ryi[8];
int32_t rzi[8];
int32_t rxi2[8];
int32_t ryi2[8];
int32_t rzi2[8];
int32_t xsi[8];
int32_t ysi[8];

/* used to be static. --ryan. */
int32_t* horizlookup = 0;
int32_t* horizlookup2 = 0;
int32_t horizycent;

int32_t globalpal;
uint8_t* globalpalwritten;

int32_t xyaspect, viewingrangerecip;


int32_t vplce[4], vince[4];
intptr_t bufplce[4];

uint8_t* palookupoffse[4];

int16_t globalshiftval;

static uint8_t tablesloaded = 0;
int32_t pageoffset, ydim16, qsetmode = 0;
int32_t startposx, startposy, startposz;
int16_t startang, startsectnum;
int16_t pointhighlight, linehighlight, highlightcnt;
int32_t lastx[MAXYDIM];
uint8_t paletteloaded = 0;

int32_t numtilefiles, artfil = -1, artfilnum, artfilplc;

int32_t totalclocklock;

uint16_t mapCRC;


int loadboard(char* filename, int32_t* daposx, int32_t* daposy,
              int32_t* daposz, short* daang, short* dacursectnum) {
    int x;
    short fil, i, numsprites;
    sectortype* sect;
    spritetype* s;
    walltype* w;

    x = 0;

    // FIX_00058: Save/load game crash in both single and multiplayer
    // We have to reset those arrays since the same
    // arrays are used as temporary space in the
    // compilecons() function like "label = (uint8_t  *)&sprite[0];"
    // to save memory space I guess.
    // Not reseting the array will leave dumps fooling
    // the function saveplayer(), eg at if(actorscrptr[PN] == 0)
    // where PN is sprite[i].picnum was beyong actorscrptr[] size)
    memset(sprite, 0, sizeof(sprite));
    memset(sector, 0, sizeof(sector));
    memset(wall, 0, sizeof(wall));

    if ((fil = kopen4load(filename, 0)) == -1) {
        mapversion = 7L;
        return (-1);
    }

    kread32(fil, &mapversion);
    if (mapversion != 7L)
        return (-1);

    B_InitSpriteLists();

    clearbuf(&show2dsector[0], (int32_t) ((MAXSECTORS + 3) >> 5), 0L);
    clearbuf(&show2dsprite[0], (int32_t) ((MAXSPRITES + 3) >> 5), 0L);
    clearbuf(&show2dwall[0], (int32_t) ((MAXWALLS + 3) >> 5), 0L);

    kread32(fil, daposx);
    kread32(fil, daposy);
    kread32(fil, daposz);
    kread16(fil, daang);
    kread16(fil, dacursectnum);
    kread16(fil, &numsectors);

    for (x = 0, sect = &sector[0]; x < numsectors; x++, sect++) {
        kread16(fil, &sect->wallptr);
        kread16(fil, &sect->wallnum);
        kread32(fil, &sect->ceilingz);
        kread32(fil, &sect->floorz);
        kread16(fil, &sect->ceilingstat);
        kread16(fil, &sect->floorstat);
        kread16(fil, &sect->ceilingpicnum);
        kread16(fil, &sect->ceilingheinum);
        kread8(fil, (uint8_t*) &sect->ceilingshade);
        kread8(fil, (uint8_t*) &sect->ceilingpal);
        kread8(fil, (uint8_t*) &sect->ceilingxpanning);
        kread8(fil, (uint8_t*) &sect->ceilingypanning);
        kread16(fil, &sect->floorpicnum);
        kread16(fil, &sect->floorheinum);
        kread8(fil, (uint8_t*) &sect->floorshade);
        kread8(fil, (uint8_t*) &sect->floorpal);
        kread8(fil, (uint8_t*) &sect->floorxpanning);
        kread8(fil, (uint8_t*) &sect->floorypanning);
        kread8(fil, (uint8_t*) &sect->visibility);
        kread8(fil, (uint8_t*) &sect->filler);
        kread16(fil, &sect->lotag);
        kread16(fil, &sect->hitag);
        kread16(fil, &sect->extra);
    }

    kread16(fil, &numwalls);
    for (x = 0, w = &wall[0]; x < numwalls; x++, w++) {
        kread32(fil, &w->x);
        kread32(fil, &w->y);
        kread16(fil, &w->point2);
        kread16(fil, &w->nextwall);
        kread16(fil, &w->nextsector);
        kread16(fil, &w->cstat);
        kread16(fil, &w->picnum);
        kread16(fil, &w->overpicnum);
        kread8(fil, (uint8_t*) &w->shade);
        kread8(fil, &w->pal);
        kread8(fil, &w->xrepeat);
        kread8(fil, &w->yrepeat);
        kread8(fil, &w->xpanning);
        kread8(fil, &w->ypanning);
        kread16(fil, &w->lotag);
        kread16(fil, &w->hitag);
        kread16(fil, &w->extra);
    }

    kread16(fil, &numsprites);
    for (x = 0, s = &sprite[0]; x < numsprites; x++, s++) {
        kread32(fil, &s->x);
        kread32(fil, &s->y);
        kread32(fil, &s->z);
        kread16(fil, &s->cstat);
        kread16(fil, &s->picnum);
        kread8(fil, (uint8_t*) &s->shade);
        kread8(fil, (uint8_t*) &s->pal);
        kread8(fil, (uint8_t*) &s->clipdist);
        kread8(fil, (uint8_t*) &s->filler);
        kread8(fil, (uint8_t*) &s->xrepeat);
        kread8(fil, (uint8_t*) &s->yrepeat);
        kread8(fil, (uint8_t*) &s->xoffset);
        kread8(fil, (uint8_t*) &s->yoffset);
        kread16(fil, &s->sectnum);
        kread16(fil, &s->statnum);
        kread16(fil, &s->ang);
        kread16(fil, &s->owner);
        kread16(fil, &s->xvel);
        kread16(fil, &s->yvel);
        kread16(fil, &s->zvel);
        kread16(fil, &s->lotag);
        kread16(fil, &s->hitag);
        kread16(fil, &s->extra);
    }


    for (i = 0; i < numsprites; i++)
        B_InsertSprite(sprite[i].sectnum, sprite[i].statnum);

    /* Must be after loading sectors, etc! */
    updatesector(*daposx, *daposy, dacursectnum);

    kclose(fil);

    // FIX_00009: Show map CRC and GRP file version of each player in case of Out Of Synch

    mapCRC = crc16((uint8_t*) sector, numsectors * sizeof(sectortype));
    mapCRC += crc16((uint8_t*) wall, numwalls * sizeof(walltype));
    mapCRC += crc16((uint8_t*) sprite, numsprites * sizeof(spritetype));

    return (0);
}


static void write32(int f, int32_t val) {
    val = BUILDSWAP_INTEL32(val);
    write(f, &val, 4);
}

static void write16(int f, short val) {
    val = BUILDSWAP_INTEL16(val);
    write(f, &val, 2);
}

static void write8(int f, uint8_t val) {
    write(f, &val, 1);
}


int saveboard(char* filename, int32_t* daposx, int32_t* daposy,
              int32_t* daposz, short* daang, short* dacursectnum) {
    int fil;
    int x;
    short i, j, numsprites;
    int permissions = 0;
    walltype* w;
    sectortype* sect;

#ifdef WIN32
    permissions = S_IWRITE;
#else
    permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
#endif

    if ((fil = open(filename,
                    O_BINARY | O_TRUNC | O_CREAT | O_WRONLY,
                    permissions)) == -1) {
        return (-1);
    }

    write32(fil, mapversion);

    write32(fil, *daposx);
    write32(fil, *daposy);
    write32(fil, *daposz);
    write16(fil, *daang);
    write16(fil, *dacursectnum);

    write16(fil, numsectors);
    for (x = 0, sect = &sector[0]; x < numsectors; x++, sect++) {
        write16(fil, sect->wallptr);
        write16(fil, sect->wallnum);
        write32(fil, sect->ceilingz);
        write32(fil, sect->floorz);
        write16(fil, sect->ceilingstat);
        write16(fil, sect->floorstat);
        write16(fil, sect->ceilingpicnum);
        write16(fil, sect->ceilingheinum);
        write8(fil, sect->ceilingshade);
        write8(fil, sect->ceilingpal);
        write8(fil, sect->ceilingxpanning);
        write8(fil, sect->ceilingypanning);
        write16(fil, sect->floorpicnum);
        write16(fil, sect->floorheinum);
        write8(fil, sect->floorshade);
        write8(fil, sect->floorpal);
        write8(fil, sect->floorxpanning);
        write8(fil, sect->floorypanning);
        write8(fil, sect->visibility);
        write8(fil, sect->filler);
        write16(fil, sect->lotag);
        write16(fil, sect->hitag);
        write16(fil, sect->extra);
    }

    write16(fil, numwalls);
    for (x = 0, w = &wall[0]; x < numwalls; x++, w++) {
        write32(fil, w->x);
        write32(fil, w->y);
        write16(fil, w->point2);
        write16(fil, w->nextwall);
        write16(fil, w->nextsector);
        write16(fil, w->cstat);
        write16(fil, w->picnum);
        write16(fil, w->overpicnum);
        write8(fil, w->shade);
        write8(fil, w->pal);
        write8(fil, w->xrepeat);
        write8(fil, w->yrepeat);
        write8(fil, w->xpanning);
        write8(fil, w->ypanning);
        write16(fil, w->lotag);
        write16(fil, w->hitag);
        write16(fil, w->extra);
    }

    numsprites = 0;
    for (j = 0; j < MAXSTATUS; j++) {
        i = headspritestat[j];
        while (i != -1) {
            numsprites++;
            i = nextspritestat[i];
        }
    }
    write16(fil, numsprites);

    for (j = 0; j < MAXSTATUS; j++) {
        i = headspritestat[j];
        while (i != -1) {
            spritetype* s = &sprite[i];
            write32(fil, s->x);
            write32(fil, s->y);
            write32(fil, s->z);
            write16(fil, s->cstat);
            write16(fil, s->picnum);
            write8(fil, s->shade);
            write8(fil, s->pal);
            write8(fil, s->clipdist);
            write8(fil, s->filler);
            write8(fil, s->xrepeat);
            write8(fil, s->yrepeat);
            write8(fil, s->xoffset);
            write8(fil, s->yoffset);
            write16(fil, s->sectnum);
            write16(fil, s->statnum);
            write16(fil, s->ang);
            write16(fil, s->owner);
            write16(fil, s->xvel);
            write16(fil, s->yvel);
            write16(fil, s->zvel);
            write16(fil, s->lotag);
            write16(fil, s->hitag);
            write16(fil, s->extra);

            i = nextspritestat[i];
        }
    }

    close(fil);
    return (0);
}


static void loadtables(void) {
    int32_t i, fil;

    if (tablesloaded == 0) {
        initksqrt();

        for (i = 0; i < 2048; i++)
            reciptable[i] = divscale30(2048L, i + 2048);

        if ((fil = TCkopen4load("tables.dat", 0)) != -1) {
            for (i = 0; i < 2048; i++)
                kread16(fil, &sintable[i]);

            for (i = 0; i < 640; i++)
                kread16(fil, &radarang[i]);

            for (i = 0; i < 640; i++)
                radarang[1279 - i] = -radarang[i];
            kread(fil, textfont, 1024);
            kread(fil, smalltextfont, 1024);
            kread(fil, britable, 1024);
            kclose(fil);
        }
        tablesloaded = 1;
    }
}


void initengine(void) {
    int32_t i;

    loadtables();

    xyaspect = -1;

    pskyoff[0] = 0;
    pskybits = 0;

    parallaxtype = 2;
    parallaxyoffs = 0L;
    parallaxyscale = 65536;
    showinvisibility = 0;

    paletteloaded = 0;

    for (i = 0; i < MAXPALOOKUPS; i++)
        palookup[i] = NULL;

    for (i = 0; i < MAXTILES; i++)
        tiles[i].data = NULL;

    clearbuf(&show2dsector[0], (int32_t) ((MAXSECTORS + 3) >> 5), 0L);
    clearbuf(&show2dsprite[0], (int32_t) ((MAXSPRITES + 3) >> 5), 0L);
    clearbuf(&show2dwall[0], (int32_t) ((MAXWALLS + 3) >> 5), 0L);
    automapping = 0;

    validmodecnt = 0;

    pointhighlight = -1;
    linehighlight = -1;
    highlightcnt = 0;

    totalclock = 0;
    visibility = 512;
    parallaxvisibility = 512;

    VID_LoadPalette();
}


void uninitengine(void) {
    if (transluc != NULL) {
        free(transluc);
        transluc = NULL;
    }
    if (pic != NULL) {
        free(pic);
        pic = NULL;
    }
    if (artfil != -1)
        kclose(artfil);
    _uninitengine(); /* video driver specific. */
}


/*
================================================================================

SCREEN STATUS FUNCTIONS

================================================================================
*/


int setgamemode(int32_t daxdim, int32_t daydim) {
    return (VID_SetGameMode(daxdim, daydim));
}


//
// Sets the viewing window to a given rectangle of the screen.
// Example: For full screen 320*200, call like this: setview(0L,0L,319L,199L);
//
void setview(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    int32_t i;

    windowx1 = x1;
    wx1 = (x1 << 12);
    windowy1 = y1;
    wy1 = (y1 << 12);
    windowx2 = x2;
    wx2 = ((x2 + 1) << 12);
    windowy2 = y2;
    wy2 = ((y2 + 1) << 12);

    xdimen = (x2 - x1) + 1;
    halfxdimen = (xdimen >> 1);
    xdimenrecip = divscale32(1L, xdimen);
    ydimen = (y2 - y1) + 1;

    setaspect(65536L, divscale16(ydim * 320L, xdim * 200L));

    for (i = 0; i < windowx1; i++) {
        startumost[i] = 1, startdmost[i] = 0;
    }
    for (i = windowx1; i <= windowx2; i++) {
        startumost[i] = windowy1, startdmost[i] = windowy2 + 1;
    }
    for (i = windowx2 + 1; i < xdim; i++) {
        startumost[i] = 1, startdmost[i] = 0;
    }

    viewoffset = windowy1 * bytesperline + windowx1;

    if (stereomode) {
        ostereopixelwidth = stereopixelwidth;
        xdimen = (windowx2 - windowx1 + 1) + (stereopixelwidth << 1);
        halfxdimen = (xdimen >> 1);
        xdimenrecip = divscale32(1L, xdimen);
        setaspect(divscale16(xdimen, windowx2 - windowx1 + 1), yxaspect);
    }
}


void nextpage(void) {
    _nextpage(); /* video driver specific. */

    faketimerhandler();

    if ((totalclock >= lastageclock + 8) || (totalclock < lastageclock)) {
        lastageclock = totalclock;
        agecache();
    }

    beforedrawrooms = 1;
    numframes++;
}

//==============================================================================


int initmouse(void) {
    return (moustat = setupmouse());
}


void getmousevalues(short* mousx, short* mousy, short* bstatus) {
    if (moustat == 0) {
        *mousx = 0;
        *mousy = 0;
        *bstatus = 0;
        return;
    }
    readmousexy(mousx, mousy);
    readmousebstatus(bstatus);
}


//
// Sets both the viewing range angle and Y/X aspect ratio.
//   daxrange := Q.16.16
//   daaspect := Q.16.16
//
void setaspect(int32_t daxrange, int32_t daaspect) {
    viewingrange = daxrange;
    viewingrangerecip = divscale32(1L, daxrange);

    yxaspect = daaspect;
    xyaspect = divscale32(1, yxaspect);
    xdimenscale = scale(xdimen, yxaspect, 320);
    xdimscale = scale(320, xyaspect, xdimen);
}


void plotpixel(int32_t x, int32_t y, uint8_t col) {
    drawpixel(ylookup[y] + x + frameplace, (int32_t) col);
}


uint8_t getpixel(int32_t x, int32_t y) {
    return readpixel(&frameplace[ylookup[y] + x]);
}

/* MUST USE RESTOREFORDRAWROOMS AFTER DRAWING */
int32_t setviewcnt = 0;
int32_t bakvidoption[4];
uint8_t* bakframeplace[4];
int32_t bakxsiz[4], bakysiz[4];
int32_t bakwindowx1[4], bakwindowy1[4];
int32_t bakwindowx2[4], bakwindowy2[4];

void setviewback(void) {
    int32_t i, j, k;

    if (setviewcnt <= 0)
        return;
    setviewcnt--;

    setview(bakwindowx1[setviewcnt], bakwindowy1[setviewcnt],
            bakwindowx2[setviewcnt], bakwindowy2[setviewcnt]);
    copybufbyte(&bakumost[windowx1], &startumost[windowx1],
                (windowx2 - windowx1 + 1) * sizeof(startumost[0]));
    copybufbyte(&bakdmost[windowx1], &startdmost[windowx1],
                (windowx2 - windowx1 + 1) * sizeof(startdmost[0]));
    vidoption = bakvidoption[setviewcnt];
    frameplace = bakframeplace[setviewcnt];
    if (setviewcnt == 0)
        k = bakxsiz[0];
    else
        k = max(bakxsiz[setviewcnt-1], bakxsiz[setviewcnt]);
    j = 0;
    for (i = 0; i <= k; i++)
        ylookup[i] = j, j += bytesperline;
    setBytesPerLine(bytesperline);
}