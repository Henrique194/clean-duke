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

#include "hu_draw.h"
#include "build/engine.h"
#include "funct.h"
#include "automap/am_draw.h"

static void HUD_MoveClouds(void) {
    if (totalclock <= cloudtotalclock && totalclock >= (cloudtotalclock - 7)) {
        return;
    }
    cloudtotalclock = totalclock + 6;
    const short ang = ps[screenpeek].ang;
    for (short i = 0; i < numclouds; i++) {
        cloudx[i] += (COS(ang) >> 9);
        cloudy[i] += (SIN(ang) >> 9);

        sector[clouds[i]].ceilingxpanning = cloudx[i] >> 6;
        sector[clouds[i]].ceilingypanning = cloudy[i] >> 6;
    }
}

static void HUD_DrawPlayer(void) {
    const player_t* p = &ps[screenpeek];
    if (p->newowner >= 0) {
        cameratext(p->newowner);
        return;
    }
    P_DisplayWeapon(screenpeek);
    if (!p->over_shoulder_on) {
        P_DisplayMasks(screenpeek);
    }
}

void HUD_Draw(int32_t smooth_ratio) {
    if (ud.overhead_on != 2) {
        HUD_DrawPlayer();
        HUD_MoveClouds();
    }
    AM_Draw(smooth_ratio);
}
