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

#include "automap/am_draw.h"
#include "am_overhead.h"
#include "am_view.h"
#include "build/engine.h"
#include "funct.h"

typedef struct {
    int32_t x;
    int32_t y;
    int32_t ang;
} map_pos_t;

uint8_t show2dsector[(MAXSECTORS + 7) >> 3];

static void AM_DrawLevelText(void) {
    const int32_t a = (ud.screen_size > 0 ? 147 : 182);
    const int x = 1;
    const uint8_t p = 0;
    const uint8_t sb = 2 + 8 + 16;

    // Draw episode name.
    int y = a + 6;
    char* str = volume_names[ud.volume_number];
    HU_DrawMiniText(x, a + 6, str, p, sb);

    // Draw level name.
    y += 6;
    str = level_names[ud.volume_number * 11 + ud.level_number];
    HU_DrawMiniText(x, y, str, p, sb);
}

static void AM_UpdateFollowMode(void) {
    ud.fola += ud.folavel >> 3;
    ud.folx += (ud.folfvel * COS(ud.fola)) >> 14;
    ud.foly += (ud.folfvel * SIN(ud.fola)) >> 14;
}

static map_pos_t AM_InterpolateMapPos(int32_t smooth_ratio) {
    int32_t x;
    int32_t dx;
    int32_t y;
    int32_t dy;
    short ang;
    short dang;
    if (screenpeek == myconnectindex && numplayers > 1) {
        x = omyx;
        dx = myx - omyx;
        y = omyy;
        dy = myy - omyy;
        ang = omyang;
        dang = myang - omyang;
    } else {
        const player_t* player = &ps[screenpeek];
        x = player->oposx;
        dx = player->posx - player->oposx;
        y = player->oposy;
        dy = player->posy - player->oposy;
        ang = player->oang;
        dang = player->ang - player->oang;
    }
    dang = ((dang + 1024) & 2047) - 1024;

    map_pos_t pos;
    pos.x = x + mulscale16(dx, smooth_ratio);
    pos.y = y + mulscale16(dy, smooth_ratio);
    pos.ang = ang + mulscale16(dang, smooth_ratio);

    return pos;
}

static map_pos_t AM_GetMapPos(int32_t smooth_ratio) {
    map_pos_t pos;
    if (ud.scrollmode) {
        AM_UpdateFollowMode();
        pos.x = ud.folx;
        pos.y = ud.foly;
        pos.ang = ud.fola;
        return pos;
    }
    const player_t* player = &ps[screenpeek];
    if (player->newowner != -1) {
        pos.x = player->oposx;
        pos.y = player->oposy;
        pos.ang = player->oang;
        return pos;
    }
    return AM_InterpolateMapPos(smooth_ratio);
}

void AM_Draw(int32_t smooth_ratio) {
    if (ud.overhead_on <= 0) {
        return;
    }
    smooth_ratio = SDL_clamp(smooth_ratio, 0, 65536);
    P_DoInterpolations(smooth_ratio);

    const player_t* plr = &ps[screenpeek];
    map_pos_t pos = AM_GetMapPos(smooth_ratio);
    int16_t ang = (int16_t) pos.ang;
    if (ud.overhead_on == 2) {
        R_ClearView(0L);
        AM_DrawMapView(pos.x, pos.y, plr->zoom, ang);
    }
    AM_DrawOverhead(pos.x, pos.y, plr->zoom, ang);

    P_RestoreInterpolations();

    if (ud.overhead_on == 2) {
        AM_DrawLevelText();
    }
}
