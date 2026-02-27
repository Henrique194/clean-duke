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

#include "duke3d.h"
#include "input/mouse.h"
#include "config/config.h"
#include "build/engine.h"


uint32 CONTROL_MouseButtonState1;
uint32 CONTROL_MouseButtonState2;

uint32 CONTROL_MouseDigitalAxisState1;
uint32 CONTROL_MouseDigitalAxisState2;

boolean CONTROL_MouseEnabled;

int32 MouseMapping[MAXMOUSEBUTTONS];
// [axesX/Y][directionLeft/Right or directionUp/Down]
int32 MouseDigitalAxeMapping[MAXMOUSEAXES][2];

static short mouseButtons = 0;
static short lastmousebuttons = 0;

static int32 mouseRelativeX = 0;
static int32 mouseRelativeY = 0;

static void IN_UpdateMouse(void) {
    short x;
    short y;
    getmousevalues(&x, &y, &mouseButtons);

    mouseRelativeX += x;
    mouseRelativeY += y;
}

void MOUSE_GetDelta(int32* x, int32* y) {
    IN_UpdateMouse();
    if (x) {
        *x = mouseRelativeX;
    }
    if (y) {
        *y = mouseRelativeY;
    }
    mouseRelativeX = 0;
    mouseRelativeY = 0;
}

int32 MOUSE_GetButtons(void) {
    return mouseButtons;
}


static void SETMOUSEDIGITALAXIS(int i) {
    int b;
    if (i < 0) {
        return;
    }
    if (i < 32) {
        b = 1 << i;
        CONTROL_MouseDigitalAxisState1 |= b;
    } else {
        i -= 32;
        b = 1 << i;
        CONTROL_MouseDigitalAxisState2 |= b;
    }
}

static void RESMOUSEDIGITALAXIS(int i) {

    int b;

    if (i < 0)
        return;

    if (i < 32) {
        b = 1 << i;
        CONTROL_MouseDigitalAxisState1 &= ~b;
    } else {
        i -= 32;
        b = 1 << i;
        CONTROL_MouseDigitalAxisState2 &= ~b;
    }
}

static void SETMOUSEBUTTON(int i) {
    int b;
    if (i < 32) {
        b = 1 << i;
        CONTROL_MouseButtonState1 |= b;
    } else {
        i -= 32;
        b = 1 << i;
        CONTROL_MouseButtonState2 |= b;
    }
}

static void RESMOUSEBUTTON(int i) {
    int b;
    if (i < 32) {
        b = 1 << i;
        CONTROL_MouseButtonState1 &= ~b;
    } else {
        i -= 32;
        b = 1 << i;
        CONTROL_MouseButtonState2 &= ~b;
    }
}

void MOUSE_GetInput(ControlInfo* info) {
    int32 sens_X = CONTROL_GetMouseSensitivity_X();
    int32 sens_Y = CONTROL_GetMouseSensitivity_Y();
    int32 mx = 0, my = 0;

    MOUSE_GetDelta(&mx, &my);

    info->dyaw = (mx * sens_X);

    switch (ControllerType) {
        case controltype_keyboardandjoystick: {
        } break;

        case controltype_joystickandmouse:
            // Not sure what I was thinking here...
            // Commented this out because it totally breaks smooth mouse etc...
            /*
                        {
                                // Mouse should use pitch instead of forward movement.
                                info->dpitch = my * sens*2;
                        }
                        break;
                        */

        default: {
            // If mouse aim is active
            if (myaimmode) {
                // FIX_00052: Y axis for the mouse is now twice as sensitive as before
                info->dpitch = (my * sens_Y * 2);
            } else {
                info->dz = (my * sens_Y * 2);
            }
        } break;
    }

    // releasing the mouse button does not honor if a keyboard key with
    // the same function is still pressed. how should it?
    for (int i = 0; i < MAXMOUSEBUTTONS; ++i) {
        if (MouseMapping[i] != -1) {
            if (!(lastmousebuttons & (1 << i)) && mouseButtons & (1 << i)) {
                SETMOUSEBUTTON(MouseMapping[i]); //MouseMapping[i]
                //printf("mouse button: %d\n", i);
            } else if (lastmousebuttons & (1 << i)
                       && !(mouseButtons & (1 << i))) {
                RESMOUSEBUTTON(MouseMapping[i]); //MouseMapping[i]
                       }
        }
    }
    lastmousebuttons = mouseButtons;

    // FIX_00019: DigitalAxis Handling now supported. (cool for medkit use)
    // update digital axis
    RESMOUSEDIGITALAXIS(MouseDigitalAxeMapping[0][0]);
    RESMOUSEDIGITALAXIS(MouseDigitalAxeMapping[0][1]);
    RESMOUSEDIGITALAXIS(MouseDigitalAxeMapping[1][0]);
    RESMOUSEDIGITALAXIS(MouseDigitalAxeMapping[1][1]);

    if (mx < 0) {
        SETMOUSEDIGITALAXIS(MouseDigitalAxeMapping[0][0]);
    } else if (mx > 0) {
        SETMOUSEDIGITALAXIS(MouseDigitalAxeMapping[0][1]);
    }

    if (my < 0) {
        SETMOUSEDIGITALAXIS(MouseDigitalAxeMapping[1][0]);
    } else if (my > 0) {
        SETMOUSEDIGITALAXIS(MouseDigitalAxeMapping[1][1]);
    }
}

void MOUSE_Reset(int32 whichbutton) {
    RESMOUSEDIGITALAXIS(whichbutton);
}


int32 CONTROL_GetMouseSensitivity_X(void) {
    return mouseSensitivity_X;
}

void CONTROL_SetMouseSensitivity_X(int32 newsensitivity) {
    mouseSensitivity_X = newsensitivity;
}

// FIX_00014: Added Y cursor setup for mouse sensitivity in the menus
int32 CONTROL_GetMouseSensitivity_Y(void) {
    return mouseSensitivity_Y;
}

void CONTROL_SetMouseSensitivity_Y(int32 newsensitivity) {
    mouseSensitivity_Y = newsensitivity;
}

void CONTROL_MapDigitalAxis(
    int32 whichaxis,
    int32 whichfunction,
    int32 direction
) {
    if (whichaxis < 0 || whichaxis >= MAXMOUSEAXES || direction < 0 || direction >= 2) {
        return;
    }
    MouseDigitalAxeMapping[whichaxis][direction] = whichfunction;
}
