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

#include <stdio.h>
#include <string.h>

#include "duke3d.h"
#include "config/config.h"
#include "input/control.h"
#include "video/display.h"
#include "build/engine.h"
#include "function.h"
#include "input/joystick.h"
#include "input/mouse.h"


#define MOUSE_ACTION                                                           \
    (ControllerType == controltype_keyboardandmouse)                           \
        || (ControllerType == controltype_joystickandmouse)

#define JOY_ACTION                                                             \
    (ControllerType == controltype_keyboardandjoystick)                        \
        || (ControllerType == controltype_joystickandmouse)

key_mapping_t KeyMapping[MAXGAMEBUTTONS];
uint32 CONTROL_RudderEnabled;
boolean CONTROL_MousePresent;


int ACTION(int i) {
    //Keyboard input
    if ((KB_KeyDown[KeyMapping[i].key1]) || (KB_KeyDown[KeyMapping[i].key2])) {
        return 1;
    }

    // Check mouse
    if (MOUSE_ACTION) {
        //Mouse buttons
        if ((i) > 31) {
            if ((CONTROL_MouseButtonState2 >> ((i) -32)) & 1) {
                return 1;
            }
        } else {
            if ((CONTROL_MouseButtonState1 >> (i)) & 1) {
                return 1;
            }
        }

        // FIX_00019: DigitalAxis Handling now supported. (cool for medkit use)

        //Mouse Digital Axes
        if ((i) > 31) {
            if ((CONTROL_MouseDigitalAxisState2 >> ((i) -32)) & 1) {
                return 1;
            }
        } else {
            if ((CONTROL_MouseDigitalAxisState1 >> (i)) & 1) {
                return 1;
            }
        }
    }


    // Check joystick
    if (JOY_ACTION) {
        if ((i) > 31) {
            // Check the joystick
            if ((CONTROL_JoyButtonState2 >> (i - 32)) & 1) {
                return 1;
            }

            // Check the hats
            if ((CONTROL_JoyHatState2 >> (i - 32)) & 1) {
                return 1;
            }

        } else {
            if ((CONTROL_JoyButtonState1 >> i) & 1) {
                return 1;
            }

            // Check the hats
            if ((CONTROL_JoyHatState1 >> i) & 1) {
                return 1;
            }
        }
    }

    return 0;
}


void CONTROL_MapKey(int32 which, kb_scancode key1, kb_scancode key2) {
    // FIX_00020: Protect you from assigning a function to the ESC key through duke3d.cfg
    if (key1 == sc_Escape || key2 == sc_Escape) {
        if (key1 == sc_Escape)
            key1 = 0;
        else
            key2 = 0;

        printf("Discarding ESCAPE key for function : %s\n", gamefunctions[which]);
    }

    if (key1 || key2)
        KeyMapping[which].key_active = true;
    else
        KeyMapping[which].key_active = false;

    KeyMapping[which].key1 = key1;
    KeyMapping[which].key2 = key2;
}

void CONTROL_MapButton(
    int32 whichfunction,
    int32 whichbutton,
    boolean clicked_or_doubleclicked
) {
    if (clicked_or_doubleclicked) {
        return; // TODO
    }
    if (whichbutton < 0 || whichbutton >= MAXMOUSEBUTTONS) {
        return;
    }
    MouseMapping[whichbutton] = whichfunction;
}


void CONTROL_GetInput(ControlInfo* info) {
    memset(info, 0, sizeof(*info));
    // get the very last mouse position before reading it.
    _handle_events();
    MOUSE_GetInput(info);
    if (CONTROL_JoystickEnabled && _joystick_update()) {
        // update stick state.
        JOY_Update(info);
    }
}

void CONTROL_ClearAction(int32 whichbutton) {
    KB_KeyDown[KeyMapping[whichbutton].key1] = 0;
    KB_KeyDown[KeyMapping[whichbutton].key2] = 0;
    JOY_Reset(whichbutton);
    MOUSE_Reset(whichbutton);
}


void CONTROL_Startup(void) {
    _joystick_init();
    CONTROL_MouseButtonState1 = 0;
    CONTROL_MouseButtonState2 = 0;
    CONTROL_MouseDigitalAxisState1 = 0;
    CONTROL_MouseDigitalAxisState2 = 0;
    CONTROL_JoyButtonState1 = 0;
    CONTROL_JoyButtonState2 = 0;
    CONTROL_JoyHatState1 = 0;
    CONTROL_JoyHatState2 = 0;
}

void CONTROL_Shutdown(void) {
    _joystick_deinit();
}
