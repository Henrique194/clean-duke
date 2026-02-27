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

#ifndef VID_DISPLAY_H
#define VID_DISPLAY_H

#include "build/platform.h"
#include "mode.h"
#include "palette.h"

/* set these in your _platform_init() implementation. */
int _argc;
char** _argv;

/* !!! gads, look at all the namespace polution... */
extern int32_t xres, yres, bytesperline, imageSize, maxpages;
extern uint8_t *screen, vesachecked;
extern int32_t buffermode, origbuffermode, linearmode;
extern uint8_t permanentupdate, vgacompatible;
extern uint8_t moustat;
extern int32_t *horizlookup, *horizlookup2, horizycent;
extern int32_t oxdimen, oviewingrange, oxyaspect;
extern int32_t curbrightness;
extern int32_t qsetmode;
extern int32_t pageoffset, ydim16;
extern uint8_t* frameplace;
extern uint8_t* frameoffset;
extern uint8_t textfont[1024], smalltextfont[1024];
extern uint8_t pow2char[8];
extern int32_t stereomode, visualpage, activepage, whiteband, blackband;
extern int32_t wx1, wy1, wx2, wy2, ydimen;
extern int32_t xdimen, xdimenrecip, halfxdimen, xdimenscale, xdimscale;

/*
 * !!! used to be static. If we ever put the original setgamemode() back, this
 * !!! can be made static again.  --ryan.
 * !!! (Actually, most of these extern declarations used to be static...rcg.)
 */
extern uint8_t permanentlock;

// defined in the game project:
extern int32_t FullScreen; // defined in Game\src\config.c

/* these need to be implemented by YOUR driver. */
void _platform_init(int argc, char** argv, const char* title,
                    const char* iconName);
void _idle(void);
void _handle_events(void);
void initkeys(void);
void uninitkeys(void);
void _nextpage(void);
void _uninitengine(void);

void getvalidvesamodes(void);


uint8_t readpixel(uint8_t* location);
void drawpixel(uint8_t* location, uint8_t pixel);


void clear2dscreen(void);
void _updateScreenRect(int32_t x, int32_t y, int32_t w, int32_t h);

/* mouse/keystuff stuff. Also implemented in sdl_driver.c ... */
int setupmouse(void);
void readmousexy(short* x, short* y);
void readmousebstatus(short* bstatus);
void keyhandler(void);
uint8_t _readlastkeyhit(void);

#endif
