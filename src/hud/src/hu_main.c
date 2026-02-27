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

#include "hu_draw.h"
#include "audiolib/fx_man.h"
#include "console/console.h"
#include "build/engine.h"
#include "input/keyboard.h"
#include "soundefs.h"
#include "funct.h"
#include "video/display.h"


#define MINITEXT_BLUE	0
#define MINITEXT_RED	2
#define MINITEXT_YELLOW	23
#define MINITEXT_GRAY	17

#define COLOR_ON  MINITEXT_YELLOW

#define patchstatusbar(x1,y1,x2,y2)                                            \
    {                                                                          \
        rotatesprite(0, (200-34)<<16, 65536L, 0, BOTTOMSTATUSBAR, 4, 0, 10+16+64+128, \
        scale(x1,xdim,320),scale(y1,ydim,200),                                 \
        scaleup(x2,xdim,320)-1,scaleup(y2,ydim,200)-1);                        \
    }


void typemode(void);
void HU_DrawStatusBar(short snum);


static void HUD_Update2dSector(int32_t i) {
    show2dsector[i >> 3] |= (1 << (i & 7));
    const walltype* wal = &wall[sector[i].wallptr];

    for (int32_t j = sector[i].wallnum; j > 0; j--, wal++) {
        i = wal->nextsector;
        if (i < 0) {
            continue;
        }
        if (wal->cstat & 0x0071) {
            continue;
        }
        if (wall[wal->nextwall].cstat & 0x0071) {
            continue;
        }
        if (sector[i].lotag == 32767) {
            continue;
        }
        if (sector[i].ceilingz >= sector[i].floorz) {
            continue;
        }
        show2dsector[i >> 3] |= (1 << (i & 7));
    }
}

static void HUD_ShowHelp(void) {
    const int32_t sx = 0;
    const int32_t sy = 0;
    const int32_t z = 65536L;
    const short a = 0;
    short picnum;
    const int8_t shade = 0;
    const uint8_t pal = 0;
    const uint8_t stat = 10 + 16 + 64;
    const int32_t cx1 = 0;
    const int32_t cy1 = 0;
    const int32_t cx2 = xdim - 1;
    const int32_t cy2 = ydim - 1;

    if (ud.show_help == 1) {
        picnum = TEXTSTORY;
    } else {
        // show_help == 2
        picnum = F1HELP;
    }

    rotatesprite(sx, sy, z, a, picnum, shade, pal, stat, cx1, cy1, cx2, cy2);

    if (KB_KeyPressed(sc_Escape)) {
        KB_ClearKeyDown(sc_Escape);
        ud.show_help = 0;
        if (ud.multimode < 2 && ud.recstat != 2) {
            ready2send = 1;
            totalclock = ototalclock;
        }
        P_ViewChanged();
    }
}

//
// Draw 2D elements (HUD, hand with weapon).
//   smoothratio := Q16.16
//
void displayrest(int32_t smoothratio) {
    if (ud.show_help) {
        HUD_ShowHelp();
        return;
    }

    const player_t* pp = &ps[screenpeek];
    HUD_Update2dSector(pp->cursectnum);

    if (ud.camerasprite == -1) {
        HUD_Draw(smoothratio);
    }

    HU_DrawStatusBar(screenpeek);
    HU_DrawQuotes();

    if (KB_KeyPressed(sc_Escape) && ud.overhead_on == 0 && ud.show_help == 0 && ps[myconnectindex].newowner == -1) {
        if ((ps[myconnectindex].gm & MODE_MENU) != MODE_MENU &&
            ps[myconnectindex].newowner == -1 &&
            (ps[myconnectindex].gm & MODE_TYPE) != MODE_TYPE) {
            KB_ClearKeyDown(sc_Escape);
            FX_StopAllSounds();
            clearsoundlocks();

            intomenusounds();

            ps[myconnectindex].gm |= MODE_MENU;

            if (ud.multimode < 2 && ud.recstat != 2)
                ready2send = 0;

            if (ps[myconnectindex].gm & MODE_GAME)
                cmenu(50);
            else
                cmenu(0);
            screenpeek = myconnectindex;
        }
    }

    if (ps[myconnectindex].newowner == -1 && ud.overhead_on == 0 && ud.crosshair
        && ud.camerasprite == -1)
        rotatesprite((160L - (ps[myconnectindex].look_ang >> 1)) << 16,
                     100L << 16, 65536L, 0,CROSSHAIR, 0, 0, 2 + 1, windowx1,
                     windowy1, windowx2, windowy2);

    if (ps[myconnectindex].gm & MODE_TYPE)
        typemode();
    else {
        CLS_HandleInput();
        if (!CLS_IsActive()) {
            menus();
        }
        CLS_Render();
    }

    if (ud.pause_on == 1 && (ps[myconnectindex].gm & MODE_MENU) == 0) {
        if (!CLS_IsActive()) //Addfaz Console Pause Game line addition
        {
            menutext(160, 100, 0, 0, "GAME PAUSED");
        } else {
            menutext(160, 120, 0, 0, "GAME PAUSED");
        }
    }

    if (ud.coords)
        coords(screenpeek);

    // FIX_00085: Optimized Video driver. FPS increases by +20%.
    if (pp->pals_time > 0 && pp->loogcnt == 0) {
        palto(pp->pals[0],
              pp->pals[1],
              pp->pals[2],
              pp->pals_time | 128,
              false);

        restorepalette = 1;
    } else if (restorepalette) {
        VID_SetBrightness(ud.brightness >> 2, &pp->palette[0]);
        restorepalette = 0;
    } else if (pp->loogcnt > 0)
        palto(0, 64, 0, (pp->loogcnt >> 1) + 128, false);
}