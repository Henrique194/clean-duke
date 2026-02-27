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

#include <SDL_stdinc.h>
#include "con/con.h"
#include "com_main.h"
#include "com_parser.h"
#include "build/engine.h"
#include "funct.h"

i32 script[MAXSCRIPTSIZE];
i32* actorscrptr[MAXTILES];
u8 actortype[MAXTILES];

static con_compiler_t ctx;


static void CON_SetCrc(const u8* con, u32 length) {
    ud.conCRC[0] = 0;
    ud.conCRC[0] = crc32_update(con, length, ud.conCRC[0]);
}

static void CON_ReadFile(i32 fp) {
    const i32 length = kfilelength(fp);
    ctx.src_size = length;
    ctx.src = (char*) hittype;
    ctx.cursor = ctx.src;
    kread(fp, ctx.cursor, length);
    CON_SetCrc((u8*) ctx.cursor, length);
#ifdef UNIX
    ctx.cursor[length - 1] = 0;
#else
    ctx.cursor[fs - 2] = 0;
#endif
}

static int CON_OpenFile(const char* file, int read_grp) {
    const i32 fp = TCkopen4load(file, read_grp);
    if (fp <= 0) {
        return 0;
    }
    CON_ReadFile(fp);
    kclose(fp);
    return 1;
}

static int CON_SetupCompiler(const char* file, int read_grp) {
    if (!CON_OpenFile(file, read_grp)) {
        Error(EXIT_SUCCESS, "ERROR: CON(%s) not found.\n", file);
        return 0;
    }
    memset(script, 0, sizeof(script));
    clearbuf(actorscrptr, MAXSPRITES, 0);
    clearbufbyte(actortype, MAXSPRITES, 0);
    ctx.script_cursor = script + 1;
    ctx.label = (char*) sprite;
    ctx.label_code = (i32*) sector;
    ctx.label_cnt = 0;
    ctx.line_number = 1;
    ctx.total_lines = 0;
    ctx.error = 0;
    ctx.warning = 0;
    ctx.read_grp = read_grp;
    return 1;
}

static void CON_SetSize(void) {
    const u16 con_size = (u16) (ctx.script_cursor - script) - 1;
    ud.conSize[0] = con_size;
    printf("Code Size:%d bytes(%d labels).\n", con_size * 4, ctx.label_cnt);
}

static void CON_DetectVersion(void) {
    if (ud.conSize[0] == 16208 && ctx.label_cnt == 1794 && conVersion == 15) {
        conVersion = 14;
    }
    printf("Con version: Looks like v%d\n", conVersion);
}

static void CON_SaveScript(void) {
    FILE* file = fopen("SCRIPT.TXT", "wb");
    for (int i = 0; i < MAXSCRIPTSIZE; i++) {
        fprintf(file, "%d\n", script[i]);
    }
    fclose(file);
}

bool COM_CompileFile(const char* file, int read_grp) {
    if (!CON_SetupCompiler(file, read_grp)) {
        return false;
    }
    printf("Compiling: '%s'.\n", file);
    COM_ParseFile(&ctx);
    if (ctx.error) {
        Error(EXIT_SUCCESS, "ERROR in CON(%s)\n", file);
        return false;
    }
    CON_SetSize();
    CON_DetectVersion();
    // CON_SaveScript();
    return true;
}


//
// Encode a scriptptr into a form suitable for portably
// inserting into bytecode. We store the pointer as minus
// the offset from the start of the script buffer, just
// to make it perhaps a little more obvious what is happening.
//
i32 CON_EncodeScript(const i32* scptr) {
    i32 offs = (i32) (scptr - script);
    assert(offs >= 0 && offs < MAXSCRIPTSIZE);
    return -offs;
}

//
// Decode an encoded representation of a scriptptr
//
i32* CON_DecodeScript(i32 scptr) {
    assert(scptr <= 0);
    i32 offs = -scptr;
    assert(offs >= 0 && offs < MAXSCRIPTSIZE);
    return script + offs;
}


void COM_Error(const char* fmt, ...) {
    ctx.error++;

    printf("  * ERROR!(L%hd) ", ctx.line_number);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void COM_Warn(const char* fmt, ...) {
    ctx.warning++;

    printf("  * WARNING.(L%hd) ", ctx.line_number);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
