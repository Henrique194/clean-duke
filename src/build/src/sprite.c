/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */

#include "build/sprite.h"
#include <sys/types.h>
#include "build/build.h"
#include "build/engine.h"
#include "types.h"
#if WIN32
#include "io.h"
#endif

short headspritesect[MAXSECTORS + 1];
short headspritestat[MAXSTATUS + 1];
short prevspritesect[MAXSPRITES];
short prevspritestat[MAXSPRITES];
short nextspritesect[MAXSPRITES];
short nextspritestat[MAXSPRITES];


static int B_InsertSpriteSect(short sectnum) {
    if (sectnum >= MAXSECTORS || headspritesect[MAXSECTORS] == -1) {
        // List full.
        return -1;
    }

    short blanktouse = headspritesect[MAXSECTORS];

    headspritesect[MAXSECTORS] = nextspritesect[blanktouse];
    if (headspritesect[MAXSECTORS] >= 0) {
        prevspritesect[headspritesect[MAXSECTORS]] = -1;
    }

    prevspritesect[blanktouse] = -1;
    nextspritesect[blanktouse] = headspritesect[sectnum];
    if (headspritesect[sectnum] >= 0) {
        prevspritesect[headspritesect[sectnum]] = blanktouse;
    }
    headspritesect[sectnum] = blanktouse;

    sprite[blanktouse].sectnum = sectnum;

    return blanktouse;
}


static int B_InsertSpriteStat(short statnum) {
    if (statnum >= MAXSTATUS || headspritestat[MAXSTATUS] == -1) {
        // List full.
        return -1;
    }

    short blanktouse = headspritestat[MAXSTATUS];

    headspritestat[MAXSTATUS] = nextspritestat[blanktouse];
    if (headspritestat[MAXSTATUS] >= 0) {
        prevspritestat[headspritestat[MAXSTATUS]] = -1;
    }

    prevspritestat[blanktouse] = -1;
    nextspritestat[blanktouse] = headspritestat[statnum];
    if (headspritestat[statnum] >= 0) {
        prevspritestat[headspritestat[statnum]] = blanktouse;
    }
    headspritestat[statnum] = blanktouse;

    sprite[blanktouse].statnum = statnum;

    return blanktouse;
}


int B_InsertSprite(short sectnum, short statnum) {
    B_InsertSpriteStat(statnum);
    return B_InsertSpriteSect(sectnum);
}


static int B_DeleteSpriteSect(short deleteme) {
    spritetype* s = &sprite[deleteme];
    if (s->sectnum == MAXSECTORS)
        return -1;

    if (headspritesect[s->sectnum] == deleteme) {
        headspritesect[s->sectnum] = nextspritesect[deleteme];
    }

    if (prevspritesect[deleteme] >= 0) {
        nextspritesect[prevspritesect[deleteme]] = nextspritesect[deleteme];
    }
    if (nextspritesect[deleteme] >= 0) {
        prevspritesect[nextspritesect[deleteme]] = prevspritesect[deleteme];
    }

    if (headspritesect[MAXSECTORS] >= 0) {
        prevspritesect[headspritesect[MAXSECTORS]] = deleteme;
    }
    prevspritesect[deleteme] = -1;
    nextspritesect[deleteme] = headspritesect[MAXSECTORS];
    headspritesect[MAXSECTORS] = deleteme;

    s->sectnum = MAXSECTORS;
    return 0;
}


static int B_DeleteSpriteStat(short deleteme) {
    spritetype* s = &sprite[deleteme];
    if (s->statnum == MAXSTATUS) {
        return -1;
    }

    if (headspritestat[s->statnum] == deleteme) {
        headspritestat[s->statnum] = nextspritestat[deleteme];
    }

    if (prevspritestat[deleteme] >= 0) {
        nextspritestat[prevspritestat[deleteme]] = nextspritestat[deleteme];
    }
    if (nextspritestat[deleteme] >= 0) {
        prevspritestat[nextspritestat[deleteme]] = prevspritestat[deleteme];
    }

    if (headspritestat[MAXSTATUS] >= 0) {
        prevspritestat[headspritestat[MAXSTATUS]] = deleteme;
    }
    prevspritestat[deleteme] = -1;
    nextspritestat[deleteme] = headspritestat[MAXSTATUS];
    headspritestat[MAXSTATUS] = deleteme;

    s->statnum = MAXSTATUS;
    return 0;
}


int deletesprite(short spritenum) {
    B_DeleteSpriteStat(spritenum);
    return B_DeleteSpriteSect(spritenum);
}


int B_SetSprite(short spritenum, int32_t newx, int32_t newy, int32_t newz) {
    short tempsectnum;

    sprite[spritenum].x = newx;
    sprite[spritenum].y = newy;
    sprite[spritenum].z = newz;

    tempsectnum = sprite[spritenum].sectnum;
    updatesector(newx, newy, &tempsectnum);
    if (tempsectnum < 0) {
        return -1;
    }
    if (tempsectnum != sprite[spritenum].sectnum) {
        B_ChangeSpriteSect(spritenum, tempsectnum);
    }

    return 0;
}


int B_ChangeSpriteSect(short spritenum, short newsectnum) {
    if (newsectnum < 0 || newsectnum > MAXSECTORS) {
        return -1;
    }
    if (sprite[spritenum].sectnum == newsectnum) {
        return 0;
    }
    if (sprite[spritenum].sectnum == MAXSECTORS) {
        return -1;
    }
    if (B_DeleteSpriteSect(spritenum) < 0) {
        return -1;
    }
    B_InsertSpriteSect(newsectnum);
    return 0;
}


int B_ChangeSpriteStat(short spritenum, short newstatnum) {
    if (newstatnum < 0 || newstatnum > MAXSTATUS) {
        return -1;
    }
    if (sprite[spritenum].statnum == newstatnum) {
        return 0;
    }
    if (sprite[spritenum].statnum == MAXSTATUS) {
        return -1;
    }
    if (B_DeleteSpriteStat(spritenum) < 0) {
        return -1;
    }
    B_InsertSpriteStat(newstatnum);
    return 0;
}


void B_InitSpriteLists(void) {
    for (i32 i = 0; i < MAXSECTORS; i++) {
        // Init doubly-linked sprite sector lists.
        headspritesect[i] = -1;
    }
    headspritesect[MAXSECTORS] = 0;

    for (i32 i = 0; i < MAXSPRITES; i++) {
        prevspritesect[i] = i - 1;
        nextspritesect[i] = i + 1;
        sprite[i].sectnum = MAXSECTORS;
    }
    prevspritesect[0] = -1;
    nextspritesect[MAXSPRITES - 1] = -1;


    for (i32 i = 0; i < MAXSTATUS; i++) {
        // Init doubly-linked sprite status lists.
        headspritestat[i] = -1;
    }
    headspritestat[MAXSTATUS] = 0;

    for (i32 i = 0; i < MAXSPRITES; i++) {
        prevspritestat[i] = i - 1;
        nextspritestat[i] = i + 1;
        sprite[i].statnum = MAXSTATUS;
    }
    prevspritestat[0] = -1;
    nextspritestat[MAXSPRITES - 1] = -1;
}
