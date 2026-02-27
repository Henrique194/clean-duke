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
#include "console/console.h"
#include "input/control.h"
#include "console/cvars.h"
#include "console/cvar_defs.h"
#include "function.h"
#include "build/engine.h"
#include "names.h"
#include "funct.h"
#include <SDL_stdinc.h>
#include <stdio.h>
#include <string.h>

#define MAX_CON_STR_LENGTH 70
#define MAX_CVAR_ARGS      10
#define MAX_CON_VIEWABLE   10

#define BUILD_NUMBER "Build 19"

#define STARTUP_SCRIPT "startup.cfg"


typedef struct con_element_s {
    char text[MAX_CON_STR_LENGTH];
    struct con_element_s* prev;
    struct con_element_s* next;
} con_element_t;


// Console argument tracker.
static int argc;
static char argv[MAX_CVAR_ARGS][MAX_CON_STR_LENGTH];

static char input_buffer[MAX_CON_STR_LENGTH];

// Console entries, prepending linked list.
static con_element_t* console_buffer = NULL;

// Current viewed section of the console.
static con_element_t* view_section = NULL;

// Previously entered commands.
static con_element_t* used_cmd_list = NULL;
static con_element_t* selected_cmd = NULL;

// text buffer control vars
static int cursor_pos = 0; // without spaces
static int num_spaces = 0; // number of spaces

// Toggle for the console state
static int active = 0;


/*
================================================================================

CONSOLE STATE

================================================================================
*/

static void CON_Toggle(void) {
    active = !active;
    if (active) {
        if (numplayers == 1) {
            // In singleplayer, pause when console
            // is active and not playing a demo.
            ud.pause_on = (short) (ud.recstat == 0);
        }
        return;
    }
    if (ud.multimode > 1 && ud.coop != 1) {
        // Make sure the frag bar displays after console is hidden.
        displayfragbar();
    }
    if (numplayers < 2) {
        // Game is only paused in singleplayer,
        // otherwise we get out of sync error.
        ud.pause_on = 0;
    }
}

int CLS_IsActive(void) {
    return active;
}

void CLS_Hide(void) {
    active = 0;
}

//==============================================================================


/*
================================================================================

COMMAND PARSING

================================================================================
*/

int CLS_GetArgc(void) {
    return argc;
}

char* CLS_GetArgv(unsigned int var) {
    return argv[var];
}

//
// Split the command into cvar and args.
// Returns the cvar name of the command.
//
static const char* CON_TokenizeCommand(char* command) {
    argc = 0;
    if (strlen(command) < 1) {
        return NULL;
    }
    char* ctx;
    const char* cvar = SDL_strtokr(command, " \r\n", &ctx);
    for (int i = 0; i < MAX_CVAR_ARGS; i++) {
        char* token = SDL_strtokr(NULL, " ", &ctx);
        if (!token) {
            break;
        }
        sprintf(argv[i], "%s", token);
        argc++;
    }
    return cvar;
}

static void CON_ParseCommand(char* cmd) {
    const char* cmd_name = CON_TokenizeCommand(cmd);
    cvar_t* cvar = CVAR_FindVar(cmd_name);
    if (cvar) {
        cvar->function(cvar);
    }
}

//==============================================================================


/*
================================================================================

INPUT HANDLING

================================================================================
*/

static void CON_SelectCmd(con_element_t* cmd) {
    selected_cmd = cmd;
    if (!selected_cmd) {
        return;
    }
    sprintf(input_buffer, "%s", selected_cmd->text);
    const size_t len = SDL_strlen(input_buffer);
    cursor_pos = 0;
    num_spaces = 0;
    for (size_t i = 0; i < len; i++) {
        if (input_buffer[i] == ' ') {
            num_spaces++;
        } else {
            cursor_pos++;
        }
    }
}

static void CON_InsertUsedCommand(const char* cmd) {
    // Create a new element in the list.
    con_element_t* element = malloc(sizeof(*element));
    if (!element) {
        return;
    }
    memset(element->text, 0, MAX_CON_STR_LENGTH);
    strncpy(element->text, cmd, MAX_CON_STR_LENGTH - 2);

    // Add new element to the end of the list.
    if (used_cmd_list) {
        used_cmd_list->next = element;
    }
    element->prev = used_cmd_list;
    used_cmd_list = element;
    element->next = NULL;
}

static void CON_KeyEnter(void) {
    // Push the input_buffer unto the console_buffer.
    CLS_Printf("%s", input_buffer);
    cursor_pos = 0;
    num_spaces = 0;

    // Parse the text.
    CON_InsertUsedCommand(input_buffer);
    CON_ParseCommand(input_buffer);

    // Clear console.
    CON_SelectCmd(NULL);
    memset(input_buffer, 0, MAX_CON_STR_LENGTH);
}

static void CON_KeyEscape(void) {
    CLS_Hide();
    KB_ClearKeyDown(sc_Escape);
    // FIX_00057: Using ESC to get out of the console mode wouldn't take pause mode off
    if (numplayers < 2) {
        ud.pause_on = 0;
    }
    // FIX_00078: Out Of Synch error (multiplayer) when using console in pause mode
}

static void CON_KeyEnd(void) {
    view_section = console_buffer;
}

static void CON_KeyPageUp(void) {
    if (!view_section) {
        return;
    }
    for (int i = 0; i < MAX_CON_VIEWABLE; i++) {
        if (!view_section->prev) {
            break;
        }
        view_section = view_section->prev;
    }
}

static void CON_KeyPageDown(void) {
    if (!view_section) {
        return;
    }
    for (int i = 0; i < MAX_CON_VIEWABLE; i++) {
        if (!view_section->next) {
            break;
        }
        view_section = view_section->next;
    }
}

static void CON_KeyUpArrow(void) {
    if (!selected_cmd) {
        if (used_cmd_list) {
            CON_SelectCmd(used_cmd_list);
        }
        return;
    }
    if (selected_cmd->prev) {
        CON_SelectCmd(selected_cmd->prev);
        return;
    }
    CON_SelectCmd(selected_cmd);
}

static void CON_KeyDownArrow(void) {
    if (!selected_cmd || !selected_cmd->next) {
        return;
    }
    CON_SelectCmd(selected_cmd->next);
}

static void CON_KeyDelete(void) {
    const size_t len = SDL_strlen(input_buffer);
    if (len <= 0) {
        return;
    }
    if (input_buffer[len - 1] == ' ') {
        input_buffer[len - 1] = '\0';
        num_spaces--;
    } else {
        input_buffer[len - 1] = '\0';
        cursor_pos--;
    }
}

static void CON_KeySpace(void) {
    if (strlen(input_buffer) < MAX_CON_STR_LENGTH - 2) {
        strncat(input_buffer, " ", 1);
        num_spaces++;
    }
}

static void CON_AddKey(kb_scancode key) {
    if (strlen(input_buffer) >= MAX_CON_STR_LENGTH - 2) {
        return;
    }
    const char* key_str = KB_ScanCodeToString(key);
    if (key_str) {
        strncat(input_buffer, key_str, 1);
        cursor_pos++;
    }
}

static void CON_KeyAction(kb_scancode key) {
    switch (key) {
        case sc_Space:
            CON_KeySpace();
            break;
        case sc_Delete:
        case sc_BackSpace:
            CON_KeyDelete();
            break;
        case sc_kpad_Enter:
        case sc_Enter:
            CON_KeyEnter();
            break;
        case sc_UpArrow:
            CON_KeyUpArrow();
            break;
        case sc_DownArrow:
            CON_KeyDownArrow();
            break;
        case sc_PgUp:
            CON_KeyPageUp();
            break;
        case sc_PgDn:
            CON_KeyPageDown();
            break;
        case sc_End:
            CON_KeyEnd();
            break;
        case sc_Escape:
            CON_KeyEscape();
            break;
        default:
            CON_AddKey(key);
            break;
    }
}

static int CON_IgnoreKey(kb_scancode key) {
    switch (key) {
        case sc_None:
        case sc_LeftShift:
        case sc_RightShift:
        case sc_PrintScreen:
        case sc_Tab:
        case sc_NumLock:
        case sc_LeftAlt:
        case sc_LeftControl:
        case sc_CapsLock:
        case sc_Bad:
        case sc_LeftArrow:
        case sc_RightArrow:
        case sc_Insert:
        case sc_Home: // This should take us to the top of the list.
        case sc_RightAlt:
        case sc_RightControl:
        case sc_Tilde:
            return true;
        default:
            return false;
    }
}

static void CON_ProcessInput(void) {
    kb_scancode key = KB_GetLastScanCode();
    if (!CON_IgnoreKey(key)) {
        CON_KeyAction(key);
    }
    KB_ClearLastScanCode();
}

void CLS_HandleInput(void) {
    if (g_CV_classic) {
        CLS_Hide();
        return;
    }
    if (ACTION(GF_CONSOLE)) {
        CON_Toggle();
        CONTROL_ClearAction(GF_CONSOLE);
        return;
    }
    if (active) {
        CON_ProcessInput();
    }
}

//==============================================================================


/*
================================================================================

CONSOLE RENDERING

================================================================================
*/

// Offset for the console text.
#define CON_TEXT_OFFSET 3

static void CON_DrawInputBox(void) {
    const int y = (8 * 10) + CON_TEXT_OFFSET;
    const uint8_t stat = 10 + 16;

    // Draw text buffer.
    int x = 5;
    uint8_t pal = 0;
    HU_DrawMiniText(x, y, input_buffer, pal, stat);

    // Draw the version number.
    x = 283;
    pal = 17;
    HU_DrawMiniText(x, y, BUILD_NUMBER, pal, stat);

    // Draw the cursor.
    // Change the color every second.
    x = (cursor_pos * 4) + (num_spaces * 5) + 5;
    pal = (time(NULL) % 2) + 1;
    HU_DrawMiniText(x, y, "_", pal, stat);
}

//
// Draw the contents of the console buffer.
//
static void CON_DrawConsoleBuffer(const int num_lines) {
    const int x = 5;
    int y = CON_TEXT_OFFSET + (8 * (num_lines - 1));
    const uint8_t pal = (uint8_t) g_CV_console_text_color;
    const uint8_t sb = 10 + 16;

    con_element_t* element = view_section;
    for (int i = 0; i < num_lines; i++) {
        if (!element) {
            break;
        }
        HU_DrawMiniText(x, y, element->text, pal, sb);
        y -= 8;
        element = element->prev;
    }
}

//
// Draw bottom edges of console.
//
static void CON_DrawEdges(void) {
    const int sy = 94 << 16;
    const int z = 65536L;
    const short angle = 512;
    const short picnum = WINDOWBORDER1;
    const int8_t shade = 24;
    const uint8_t pal = 0;
    const uint8_t stat = 2 + 8;
    const int32_t cx1 = 0;
    const int32_t cy1 = 0;
    const int32_t cx2 = xdim - 1;
    const int32_t cy2 = ydim - 1;

    rotatesprite(78 << 16, sy, z, angle, picnum,
        shade, pal, stat,
        cx1, cy1, cx2, cy2
    );
    rotatesprite(238 << 16, sy, z, angle, picnum,
        shade, pal, stat,
        cx1, cy1, cx2, cy2
    );
}

static void CON_DrawBackground(void) {
    const int y1 = 0;
    const int y2 = (ydim / 2) - ((ydim / 2) / 12);
    const int z = 65536L;
    const short angle = 0;
    const short picnum = BIGHOLE;
    const int8_t shade = 8;
    const uint8_t pal = 0;
    const uint8_t stat = (g_CV_TransConsole ? 1 : 0) + 8 + 16 + 64 + 128;
    const int32_t cx1 = 0;
    const int32_t cy1 = y1;
    const int32_t cx2 = xdim - 1;
    const int32_t cy2 = y2 - 1;

    for (int y = y1; y < y2; y += 128) {
        const int sy = y << 16;
        for (int x = 0; x < xdim; x += 128) {
            const int sx = x << 16;
            rotatesprite(sx, sy, z, angle,picnum,
                shade, pal, stat,
                cx1, cy1, cx2, cy2
            );
        }
    }
}

void CON_DrawActive(void) {
    CON_DrawBackground();
    CON_DrawEdges();
    CON_DrawConsoleBuffer(MAX_CON_VIEWABLE);
    CON_DrawInputBox();
}

void CLS_Render(void) {
    if (g_CV_classic) {
        return;
    }
    // Let the Cvars draw something if they need to.
    CVAR_Render();
    if (active) {
        CON_DrawActive();
        return;
    }
    if (g_CV_num_console_lines > 0) {
        CON_DrawConsoleBuffer(g_CV_num_console_lines);
    }
}

//==============================================================================


/*
================================================================================

CONSOLE PRINTING

================================================================================
*/

void CLS_Printf(const char* newmsg, ...) {
    va_list argptr;
    char msg[512];
    va_start(argptr, newmsg);
    vsprintf(msg, newmsg, argptr);
    va_end(argptr);

    // Create a new element in the list.
    con_element_t* element =  malloc(sizeof(*element));
    if (!element) {
        return;
    }
    memset(element->text, 0, MAX_CON_STR_LENGTH);
    strncpy(element->text, msg, MAX_CON_STR_LENGTH - 2);

    // Add new element to the end of the list.
    if (console_buffer) {
        console_buffer->next = element;
    }
    element->prev = console_buffer;
    console_buffer = element;
    element->next = NULL;

    if (view_section == element->prev) {
        // Scroll console if user is at bottom.
        view_section = element;
    }
}

//==============================================================================


/*
================================================================================

INITIALIZATION AND SHUTDOWN

================================================================================
*/

static void CON_ParseStartupScript(void) {
    // FIX_00017: heavy autoexec.cfg not needed anymore.
    FILE* fp = fopen(STARTUP_SCRIPT, "r");
    if (!fp) {
        return;
    }
    char line[128] = {0};
    while (fgets(line, 128 - 1, fp) != NULL) {
        CON_ParseCommand(line);
        memset(line, 0, 128);
    }
    fclose(fp);
}

// Initialize the console
void CLS_Init(void) {
    CLS_Reset();

    // Console Variable init
    CVAR_RegisterDefaultCvarBindings();

    // read in our startup.txt
    CON_ParseStartupScript();

    CLS_Printf("Type \'HELP\' for help with console Commands.");
}


static void CON_ClearUsedCommandList(void) {
    // Free the list if it exists
    con_element_t* element = used_cmd_list;
    while (element) {
        con_element_t* next = element->prev;
        free(element);
        element = next;
    }
    used_cmd_list = NULL;
    selected_cmd = NULL;
}

void CLS_Shutdown(void) {
    // Do any clean up releasing to the console
    CLS_Reset();
    CON_ClearUsedCommandList();
}


void CLS_Reset(void) {
    // Free the list if it exists.
    con_element_t* element = console_buffer;
    while (element) {
        con_element_t* next = element->prev;
        free(element);
        element = next;
    }
    console_buffer = NULL;
    view_section = NULL;
}

//==============================================================================
