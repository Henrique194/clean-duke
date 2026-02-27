/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */

#ifndef Duke3D_tiles_h
#define Duke3D_tiles_h

#include "build/build.h"


typedef struct {
    // The x-dimension of the tile number.
    short width;

    // The y-dimension of the tile number.
    short height;
} dimensions_t;

typedef struct {
    // The dimension of the tile in texels unit.
    dimensions_t dim;

    // sed to check if a texture is in RAM or in the GRP.
    uint8_t lock;

    // Flag for animating the tile.
    int32_t anim_flags;

    // The wall texture data.
    uint8_t* data;
} tile_t;


extern tile_t tiles[MAXTILES];

void setviewtotile(short tilenume, int32_t tileWidth, int32_t tileHeight);

void squarerotatetile(short tilenume);

void loadtile(short tilenume);

uint8_t* allocatepermanenttile(short tilenume, int32_t width, int32_t height);

int loadpics(char* filename, char* gamedir);

void copytilepiece(
    int32_t tilenume1,
    int32_t sx1,
    int32_t sy1,
    int32_t xsiz,
    int32_t ysiz,
    int32_t tilenume2,
    int32_t sx2,
    int32_t sy2
);


// Bitvector marking picture used for rendition.
extern uint8_t gotpic[(MAXTILES + 7) >> 3];
void setgotpic(int32_t tilenume);


int animateoffs(int16_t tilenum);

extern uint8_t* pic;

void TILE_MakeAvailable(short picID);

#endif