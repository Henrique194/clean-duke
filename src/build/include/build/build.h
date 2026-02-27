/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */

#ifndef _INCLUDE_BUILD_H_
#define _INCLUDE_BUILD_H_

#include <inttypes.h>
#include "types.h"

#define MAXSECTORS 1024
#define MAXWALLS   8192
#define MAXSPRITES 4096
#define MAXTILES   9216

#define MAXSTATUS          1024
#define MAXPLAYERS         16
#define MAXXDIM            4000
#define MAXYDIM            3000
#define MAXPALOOKUPS       256
#define MAXPSKYTILES       256
#define MAXSPRITESONSCREEN 1024

#define CLIPMASK0 (((1L) << 16) + 1L)
#define CLIPMASK1 (((256L) << 16) + 64L)


/*
 * Make all variables in BUILD.H defined in the ENGINE,
 * and externed in GAME
 * (dear lord.  --ryan.)
 */

#ifdef ENGINE
#define EXTERN
#else
#define EXTERN extern
#endif

#define CSTAT_HAS(obj, flags) (((uint16_t) obj->cstat & (flags)) == flags)

#pragma pack(1)

/*
 * ceilingstat/floorstat:
 *   bit 0: 1 = parallaxing, 0 = not                                 "P"
 *   bit 1: 1 = groudraw, 0 = not
 *   bit 2: 1 = swap x&y, 0 = not                                    "F"
 *   bit 3: 1 = double smooshiness                                   "E"
 *   bit 4: 1 = x-flip                                               "F"
 *   bit 5: 1 = y-flip                                               "F"
 *   bit 6: 1 = Align texture to first wall of sector                "R"
 *   bits 7-8:                                                       "T"
 *          00 = normal floors
 *          01 = masked floors
 *          10 = translucent masked floors
 *          11 = reverse translucent masked floors
 *   bits 9-15: reserved
 */

/* 40 bytes */
typedef struct {
    short wallptr, wallnum;
    int32_t ceilingz, floorz;
    short ceilingstat, floorstat;
    short ceilingpicnum, ceilingheinum;
    int8_t ceilingshade;
    uint8_t ceilingpal, ceilingxpanning, ceilingypanning;
    short floorpicnum, floorheinum;
    int8_t floorshade;
    uint8_t floorpal, floorxpanning, floorypanning;
    uint8_t visibility, filler;
    short lotag, hitag, extra;
} sectortype;

/*
 * cstat:
 *   bit 0: 1 = Blocking wall (use with clipmove, getzrange)         "B"
 *   bit 1: 1 = bottoms of invisible walls swapped, 0 = not          "2"
 *   bit 2: 1 = align picture on bottom (for doors), 0 = top         "O"
 *   bit 3: 1 = x-flipped, 0 = normal                                "F"
 *   bit 4: 1 = masking wall, 0 = not                                "M"
 *   bit 5: 1 = 1-way wall, 0 = not                                  "1"
 *   bit 6: 1 = Blocking wall (use with hitscan / cliptype 1)        "H"
 *   bit 7: 1 = Transluscence, 0 = not                               "T"
 *   bit 8: 1 = y-flipped, 0 = normal                                "F"
 *   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
 *   bits 10-15: reserved
 */

/* 32 bytes */
typedef struct {
    int32_t x;
    int32_t y;
    short point2;
    short nextwall;
    short nextsector;
    short cstat;
    short picnum;
    short overpicnum;
    int8_t shade;
    uint8_t pal;
    uint8_t xrepeat;
    uint8_t yrepeat;
    uint8_t xpanning;
    uint8_t ypanning;
    short lotag;
    short hitag;
    short extra;
} walltype;


//
// The sprite "cstat" is a bitfield for how to draw an actor,
// as well as a few flags for interaction.
//
typedef enum {
    // Make sprite blockable.
    SC_BLOCK = 1,
    // Make sprite transparent.
    SC_TRANSLUCENT = 2,
    // Flip sprite around x-axis.
    SC_XFLIP = 4,
    // Flip sprite around y-axis.
    SC_YFLIP = 8,
    // Draw sprite as vertically flat (wall aligned).
    SC_WALL = 16,
    // Draw sprite as horizontally flat (floor aligned).
    SC_FLOOR = 32,
    // Make sprite one-sided.
    SC_ONE_SIDE = 64,
    // Half submerged.
    SC_YCENTER = 128,
    // Make sprite able to be hit by weapons.
    SC_HITSCAN = 256,
    // Second transparency level (combine with cstat 2).
    SC_TRANS_FLIP = 512,
    // Invisible (do not render).
    SC_INVISIBLE = 32768,
} sprite_cstat_t;

/*
 * 44 bytes.
 */
typedef struct {
    // x sprite position.
    int32_t x;

    // y sprite position.
    int32_t y;

    // z sprite position.
    int32_t z;

    // bit 0: 1 = Blocking sprite (use with clipmove, getzrange)       "B"
    // bit 1: 1 = translucence, 0 = normal                             "T"
    // bit 2: 1 = x-flipped, 0 = normal                                "F"
    // bit 3: 1 = y-flipped, 0 = normal                                "F"
    // bits 5-4: 00 = FACE sprite (default)                            "R"
    //           01 = WALL sprite (like masked walls)
    //           10 = FLOOR sprite (parallel to ceilings&floors)
    // bit 6: 1 = 1-sided sprite, 0 = normal                           "1"
    // bit 7: 1 = Real centered centering, 0 = foot center             "C"
    // bit 8: 1 = Blocking sprite (use with hitscan / cliptype 1)      "H"
    // bit 9: 1 = Translucence reversing, 0 = normal                   "T"
    // bits 10-14: reserved
    // bit 15: 1 = Invisible sprite, 0 = not invisible
    int16_t cstat;

    // texture index into art file
    int16_t picnum;

    // shade offset of sprite.
    int8_t shade;

    // palette lookup table number (0 - use standard colors).
    uint8_t pal;

    // the size of the movement clipping square (face sprites only).
    uint8_t clipdist;

    // useless byte to make structure aligned.
    uint8_t filler;

    // used to change the size of pixels (stretch textures).
    uint8_t xrepeat;

    // used to change the size of pixels (stretch textures).
    uint8_t yrepeat;

    // used to center the animation of sprites.
    int8_t xoffset;

    // used to center the animation of sprites.
    int8_t yoffset;

    // Current sector of sprite.
    int16_t sectnum;

    // Current status of sprite (inactive/monster/bullet, etc.).
    int16_t statnum;

    // Angle the sprite is facing.
    int16_t ang;

    // If the sprite was spawned by another sprite, then owner
    // holds the ID number of that sprite. This also determines
    // which player in multiplayer is awarded a frag if damage
    // from an actor kills another player.
    int16_t owner;

    // The forward/backward velocity of the sprite.
    // Positive values are forward, negative are backward.
    int16_t xvel;

    // Because the velocity is controlled solely by the xvel,
    // yvel is used to store other values. For an APLAYER sprite,
    // the yvel stores the ID of the player. For hitscan projectiles,
    // yvel represents the picnum of the projectile used with shoot
    // command. For normal projectiles, yvel equals the number of
    // bounces left.
    int16_t yvel;

    // In a non-projectile, zvel is the vertical velocity of a sprite.
    // Negative values are upward velocity, positive values are downward
    // velocity. In projectiles, zvel works somewhat differently, and
    // functions more like an angle.
    int16_t zvel;

    // Specify what skill the sprite should appear on:
    //   - 0 "No Monsters" and above.
    //   - 1 "Piece of Cake" and above
    //   - 2 "Let's Rock" and above
    //   - 3 "Come Get Some" and above
    //   - 4 "Damn, I'm Good" only
    int16_t lotag;

    // Actors use their hitag to store motion characteristics
    // temporarily, e.g., face_player, etc. Sector effectors
    // use their hitag to specify what their lotag effects
    int16_t hitag;

    // Typically the actor-in-question's health value.
    // It's also used to store the damage of hard-coded
    // projectiles.
    int16_t extra;
} spritetype;

#pragma pack()


typedef struct {
    i32 x;
    i32 y;
} vec2_t;

#define VEC2_FROM_ANG(ang) { .x = COS(ang), .y = SIN(ang) }

#define VEC2_ADD(v1, v2) { .x = (v1)->x + (v2)->x, .y = (v1)->y + (v2)->y }

#define VEC2_SUB(v1, v2) { .x = (v1)->x - (v2)->x, .y = (v1)->y - (v2)->y }

#define VEC2_SCALE(v, scale) { .x = scale * (v)->x, .y = scale * (v)->y }

#define VEC2_DOT(v1, v2) (((v1)->x * (v2)->x) + ((v1)->y * (v2)->y))

//
// The 2D vector cross product, also known as "perp dot product".
//
#define VEC2_CROSS(v1, v2) (((v1)->x * (v2)->y) - ((v1)->y * (v2)->x))

#define SPR_POS(spr) { .x = (spr)->x, .y = (spr)->y }

#define SPR_DIR(spr) VEC2_FROM_ANG((spr)->ang)


#define ANG22  128
#define ANG45  256
#define ANG90  512
#define ANG180 1024
#define ANG270 1536
#define ANG360 2048

#define SIN(angle) sintable[(angle) & 2047]
#define COS(angle) SIN((angle) + ANG90)


EXTERN sectortype sector[MAXSECTORS];
EXTERN walltype wall[MAXWALLS];
EXTERN spritetype sprite[MAXSPRITES];

EXTERN uint16_t mapCRC;

EXTERN int32_t spritesortcnt;
EXTERN spritetype tsprite[MAXSPRITESONSCREEN];

EXTERN uint8_t vidoption;
EXTERN int32_t xdim, ydim, numpages;

// Fast way to retrive the start of a column in the framebuffer, given a screenspace X coordinate.
EXTERN int32_t ylookup[MAXYDIM + 1];

EXTERN int32_t yxaspect, viewingrange;

EXTERN int32_t validmodecnt;
EXTERN short validmode[256];
EXTERN int32_t validmodexdim[256], validmodeydim[256];

EXTERN short numsectors, numwalls;
EXTERN volatile int32_t totalclock;
EXTERN int32_t numframes, randomseed;
EXTERN short sintable[2048];
EXTERN uint8_t palette[768];
EXTERN short numpalookups;
EXTERN uint8_t* palookup[MAXPALOOKUPS];
EXTERN uint8_t parallaxtype, showinvisibility;
EXTERN int32_t parallaxyoffs, parallaxyscale;
EXTERN int32_t visibility, parallaxvisibility;

EXTERN int32_t windowx1, windowy1, windowx2, windowy2;
EXTERN short startumost[MAXXDIM], startdmost[MAXXDIM];

EXTERN short pskyoff[MAXPSKYTILES], pskybits;


/*
 * These variables are for auto-mapping with the draw2dscreen function.
 * When you load a new board, these bits are all set to 0 - since
 * you haven't mapped out anything yet.  Note that these arrays are
 * bit-mapped.
 * If you want draw2dscreen() to show sprite #54 then you say:
 *     spritenum = 54;
 *     show2dsprite[spritenum>>3] |= (1<<(spritenum&7));
 * And if you want draw2dscreen() to not show sprite #54 then you say:
 *     spritenum = 54;
 *     show2dsprite[spritenum>>3] &= ~(1<<(spritenum&7));
 * Automapping defaults to 0 (do nothing).  If you set automapping to 1,
 * then in 3D mode, the walls and sprites that you see will show up the
 * next time you flip to 2D mode.
 */
extern uint8_t show2dsector[(MAXSECTORS + 7) >> 3];
EXTERN uint8_t show2dwall[(MAXWALLS + 7) >> 3];
EXTERN uint8_t show2dsprite[(MAXSPRITES + 7) >> 3];
EXTERN uint8_t automapping;

#define SEC_VIS(sec) ((show2dsector[(sec) >> 3] & (1 << ((sec) & 7))) != 0)


//This is the bit vector that marks visited sector during portal flooding. Size is hence (MAXSECTORS / 8)
EXTERN uint8_t visitedSectors[(MAXSECTORS + 7) >> 3];

/*************************************************************************
POSITION VARIABLES:

		POSX is your x - position ranging from 0 to 65535
		POSY is your y - position ranging from 0 to 65535
			(the length of a side of the grid in EDITBORD would be 1024)
		POSZ is your z - position (height) ranging from 0 to 65535, 0 highest.
		ANG is your angle ranging from 0 to 2047.  Instead of 360 degrees, or
			 2 * PI radians, I use 2048 different angles, so 90 degrees would
			 be 512 in my system.

SPRITE VARIABLES:

	EXTERN short headspritesect[MAXSECTORS+1], headspritestat[MAXSTATUS+1];
	EXTERN short prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
	EXTERN short nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];

	Example: if the linked lists look like the following:
		 ��������������������������������������������������������������Ŀ
		 �      Sector lists:               Status lists:               �
		 ��������������������������������������������������������������Ĵ
		 �  Sector0:  4, 5, 8             Status0:  2, 0, 8             �
		 �  Sector1:  16, 2, 0, 7         Status1:  4, 5, 16, 7, 3, 9   �
		 �  Sector2:  3, 9                                              �
		 ����������������������������������������������������������������
	Notice that each number listed above is shown exactly once on both the
		left and right side.  This is because any sprite that exists must
		be in some sector, and must have some kind of status that you define.


Coding example #1:
	To go through all the sprites in sector 1, the code can look like this:

		sectnum = 1;
		i = headspritesect[sectnum];
		while (i != -1)
		{
			nexti = nextspritesect[i];

			//your code goes here
			//ex: printf("Sprite %d is in sector %d\n",i,sectnum);

			i = nexti;
		}

Coding example #2:
	To go through all sprites with status = 1, the code can look like this:

		statnum = 1;        //status 1
		i = headspritestat[statnum];
		while (i != -1)
		{
			nexti = nextspritestat[i];

			//your code goes here
			//ex: printf("Sprite %d has a status of 1 (active)\n",i,statnum);

			i = nexti;
		}

			 insertsprite(short sectnum, short statnum);
			 deletesprite(short spritenum);
			 changespritesect(short spritenum, short newsectnum);
			 changespritestat(short spritenum, short newstatnum);

TILE VARIABLES:
		NUMTILES - the number of tiles found TILES.DAT.
		TILESIZX[MAXTILES] - simply the x-dimension of the tile number.
		TILESIZY[MAXTILES] - simply the y-dimension of the tile number.
		WALOFF[MAXTILES] - the actual 32-bit offset pointing to the top-left
								 corner of the tile.
		PICANM[MAXTILES] - flags for animating the tile.

TIMING VARIABLES:
		TOTALCLOCK - When the engine is initialized, TOTALCLOCK is set to zero.
			From then on, it is incremented 120 times a second by 1.  That
			means that the number of seconds elapsed is totalclock / 120.
		NUMFRAMES - The number of times the draw3dscreen function was called
			since the engine was initialized.  This helps to determine frame
			rate.  (Frame rate = numframes * 120 / totalclock.)

OTHER VARIABLES:

		STARTUMOST[320] is an array of the highest y-coordinates on each column
				that my engine is allowed to write to.  You need to set it only
				once.
		STARTDMOST[320] is an array of the lowest y-coordinates on each column
				that my engine is allowed to write to.  You need to set it only
				once.
		SINTABLE[2048] is a sin table with 2048 angles rather than the
			normal 360 angles for higher precision.  Also since SINTABLE is in
			all integers, the range is multiplied by 16383, so instead of the
			normal -1<sin(x)<1, the range of sintable is -16383<sintable[]<16383
			If you use this sintable, you can possibly speed up your code as
			well as save space in memory.  If you plan to use sintable, 2
			identities you may want to keep in mind are:
				sintable[ang&2047]       = sin(ang * (3.141592/1024)) * 16383
				sintable[(ang+512)&2047] = cos(ang * (3.141592/1024)) * 16383
		NUMSECTORS - the total number of existing sectors.  Modified every time
			you call the loadboard function.
***************************************************************************/

#define PORTSIG                                                                \
    "Port by Ryan C. Gordon, Andrew Henderson, Dan Olson, Fabien Sanglard "    \
    "and a cast of thousands."

//Global.c
void Error(int errorType, const char* error, ...);
int FindDistance2D(int ix, int iy);


#include "build/sprite.h"


#endif /* defined _INCLUDE_BUILD_H_ */

/* end of build.h ... */
