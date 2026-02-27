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

#include "input/joystick.h"
#include <SDL.h>

#define BUILD_SDLJOYSTICK "BUILD_SDLJOYSTICK"


boolean CONTROL_JoystickEnabled;

byte CONTROL_JoystickPort;
uint32 CONTROL_JoyButtonState1;
uint32 CONTROL_JoyButtonState2;
uint32 CONTROL_JoyHatState1; //[MAXJOYHATS];
uint32 CONTROL_JoyHatState2; //[MAXJOYHATS];


static SDL_Joystick* stick = NULL;
static int stick_id = -1;
static char* stick_name = NULL;
static int num_sticks;

static short joyHats[MAXJOYHATS] = {0};
static short lastjoyHats[MAXJOYHATS] = {0};

static int32 JoyAxisMapping[MAXJOYAXES];
static int32 JoyHatMapping[MAXJOYHATS][8];
static int32 JoyButtonMapping[MAXJOYBUTTONS];
static float JoyAnalogScale[MAXJOYAXES];
static int32 JoyAnalogDeadzone[MAXJOYAXES];


static void IN_PrintJoystick(void) {
    const int num_axes = SDL_JoystickNumAxes(stick);
    const int num_buttons = SDL_JoystickNumButtons(stick);
    const int num_hats = SDL_JoystickNumHats(stick);
    const int num_balls = SDL_JoystickNumBalls(stick);
    printf("%d axes, %d buttons, %d hats, %d balls.\n", num_axes, num_buttons,
           num_hats, num_balls);
}

static int IN_SelectJoystick(void) {
    for (int i = 0; i < num_sticks; i++) {
        stick = SDL_JoystickOpen(i);
        if (!stick) {
            printf("Joystick #%d failed to init: %s\n", i, SDL_GetError());
            return 0;
        }
        const char* name = SDL_JoystickName(stick);
        if (stick_name && (strcmp(stick_name, name) == 0)) {
            stick_id = i;
        }
        printf("Stick #%d: [%s]\n", i, name);
        SDL_JoystickClose(stick);
    }
    return 1;
}

static int IN_OpenJoystick(void) {
    if (!IN_SelectJoystick()) {
        return 0;
    }
    printf("Using Stick #%d.", stick_id);
    if (!stick_name && num_sticks > 1) {
        printf("Set BUILD_SDLJOYSTICK to one of the above names to change.\n");
    }
    stick = SDL_JoystickOpen(stick_id);
    return stick != NULL;
}

static int IN_CanInit(void) {
    stick_name = getenv(BUILD_SDLJOYSTICK);
    if (stick_name && (strcmp(stick_name, "none") == 0)) {
        printf("Skipping joystick detection/initialization at user request\n");
        return 0;
    }
    return 1;
}

void _joystick_init(void) {
    if (stick) {
        printf("Joystick appears to be already initialized.\n");
        printf("...deinitializing for stick redetection...\n");
        _joystick_deinit();
    }
    if (!IN_CanInit()) {
        return;
    }

    printf("Initializing SDL joystick subsystem...");
    printf(" (export environment variable BUILD_SDLJOYSTICK=none to skip)\n");
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) != 0) {
        printf("SDL_Init(SDL_INIT_JOYSTICK) failed: [%s].\n", SDL_GetError());
        return;
    }

    num_sticks = SDL_NumJoysticks();
    printf("SDL sees %d joystick%s.\n", num_sticks, num_sticks == 1 ? "" : "s");
    if (num_sticks == 0) {
        return;
    }

    if (!IN_OpenJoystick()) {
        printf("Joystick #%d failed to init: %s\n", stick_id, SDL_GetError());
        return;
    }
    printf("Joystick initialized. ");
    IN_PrintJoystick();
    SDL_JoystickEventState(SDL_QUERY);
}


void _joystick_deinit(void) {
    if (!stick) {
        return;
    }
    printf("Closing joystick device...\n");
    SDL_JoystickClose(stick);
    printf("Joystick device closed. Deinitializing SDL subsystem...\n");
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    printf("SDL joystick subsystem deinitialized.\n");
    stick = NULL;
}


int _joystick_update(void) {
    if (!stick) {
        return 0;
    }
    SDL_JoystickUpdate();
    return 1;
}


int _joystick_axis(int axis) {
    if (!stick) {
        return 0;
    }
    return SDL_JoystickGetAxis(stick, axis);
}

int _joystick_hat(int hat) {
    if (!stick) {
        return -1;
    }
    return SDL_JoystickGetHat(stick, hat);
}

int _joystick_button(int button) {
    if (!stick) {
        return 0;
    }
    return SDL_JoystickGetButton(stick, button) != 0;
}



static void JOY_SetButton(int i) {
    int b;
    if (i < 32) {
        b = 1 << i;
        CONTROL_JoyButtonState1 |= b;
    } else {
        i -= 32;
        b = 1 << i;
        CONTROL_JoyButtonState2 |= b;
    }
}

static void JOY_ResetButton(int i) {
    int b;
    if (i < 32) {
        b = 1 << i;
        CONTROL_JoyButtonState1 &= ~b;
    } else {
        i -= 32;
        b = 1 << i;
        CONTROL_JoyButtonState2 &= ~b;
    }
}

static void JOY_SetHatButton(int i) {
    int b;
    if (i < 32) {
        b = 1 << i;
        CONTROL_JoyHatState1 |= b;
    } else {
        i -= 32;
        b = 1 << i;
        CONTROL_JoyHatState2 |= b;
    }
}

static void JOY_ResetHatButton(int i) {
    int b;
    if (i < 32) {
        b = 1 << i;
        CONTROL_JoyHatState1 &= ~b;
    } else {
        i -= 32;
        b = 1 << i;
        CONTROL_JoyHatState2 &= ~b;
    }
}

static void JOYSTICK_UpdateHats(void) {
    for (int i = 0; i < MAXJOYHATS; i++) {
        joyHats[i] = (short) _joystick_hat(i);
    }
}

void JOY_Update(ControlInfo* info) {
    JOYSTICK_UpdateHats();

    // Make this NOT use the BUTTON() system for storing the hat input.
    // (requires much game code changing)
    for (int i = 0; i < MAXJOYHATS; i++) {

        for (int j = 0; j < 8; j++) {
            if (!(lastjoyHats[i] & (1 << j)) && (joyHats[i] & (1 << j))) {
                JOY_SetHatButton(JoyHatMapping[i][j]);
            } else if ((lastjoyHats[i] & (1 << j))
                       && !(joyHats[i] & (1 << j))) {
                JOY_ResetHatButton(JoyHatMapping[i][j]);
            }
        }

        lastjoyHats[i] = joyHats[i];
    }


    for (int i = 0; i < MAXJOYAXES; i++) {
        switch (JoyAxisMapping[i]) {
            case analog_turning: {
                info->dyaw +=
                    (int32)((float) CONTROL_FilterDeadzone(
                                _joystick_axis(i), JoyAnalogDeadzone[i])
                            * JoyAnalogScale[i]);
            }
            break;
            case analog_strafing: {
                info->dx +=
                    (int32)((float) CONTROL_FilterDeadzone(
                                _joystick_axis(i), JoyAnalogDeadzone[i])
                            * JoyAnalogScale[i]);
            }
            break;
            case analog_lookingupanddown:
                info->dpitch +=
                    (int32)((float) CONTROL_FilterDeadzone(
                                _joystick_axis(i), JoyAnalogDeadzone[i])
                            * JoyAnalogScale[i]);
                break;
            case analog_elevation: //STUB
                break;
            case analog_rolling: //STUB
                break;
            case analog_moving: {
                info->dz +=
                    (int32)((float) CONTROL_FilterDeadzone(
                                _joystick_axis(i), JoyAnalogDeadzone[i])
                            * JoyAnalogScale[i]);
            }
            break;
            default:
                break;
        }
    }

    // SETBUTTON based on _joystick_button().
    for (int i = 0; i < MAXJOYBUTTONS; ++i) {
        if (_joystick_button(i)) {
            JOY_SetButton(JoyButtonMapping[i]);
        } else {
            JOY_ResetButton(JoyButtonMapping[i]);
        }
    }
}

void JOY_Reset(int32 whichbutton) {
    JOY_ResetButton(whichbutton);
    JOY_ResetHatButton(whichbutton);
}

void CONTROL_MapJoyButton(
    int32 whichfunction,
    int32 whichbutton,
    boolean doubleclicked
) {
    if (whichbutton < 0 || whichbutton >= MAXJOYBUTTONS) {
        return;
    }
    if (doubleclicked) {
        return;
    }
    JoyButtonMapping[whichbutton] = whichfunction;
}

void CONTROL_MapJoyHat(
    int32 whichfunction,
    int32 whichhat,
    int32 whichvalue
) {
    if (whichhat < 0 || whichhat >= MAXJOYHATS) {
        return;
    }
    JoyHatMapping[whichhat][whichvalue] = whichfunction;
}

void CONTROL_MapAnalogAxis(int32 whichaxis, int32 whichanalog) {
    if (whichaxis < MAXJOYAXES) {
        JoyAxisMapping[whichaxis] = whichanalog;
    }
}

void CONTROL_SetAnalogAxisScale(int32 whichaxis, float axisscale) {
    if (whichaxis < MAXJOYAXES) {
        // Set it... make sure we don't let them set it to 0.. div by 0 is bad.
        JoyAnalogScale[whichaxis] = (axisscale == 0) ? 1.0f : axisscale;
    }
}

void CONTROL_SetAnalogAxisDeadzone(int32 whichaxis, int32 axisdeadzone) {
    if (whichaxis < MAXJOYAXES) {
        // Set it...
        JoyAnalogDeadzone[whichaxis] = axisdeadzone;
    }
}

int32 CONTROL_GetFilteredAxisValue(int32 axis) {
    return (int32) ((float) CONTROL_FilterDeadzone(
                        _joystick_axis(axis),
                        JoyAnalogDeadzone[axis]) * JoyAnalogScale[axis]);
}

int32 CONTROL_FilterDeadzone(int32 axisvalue, int32 axisdeadzone) {
    if ((axisvalue < axisdeadzone) && (axisvalue > -axisdeadzone)) {
        return 0;
    }
    return axisvalue;
}
