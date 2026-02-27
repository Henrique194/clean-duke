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

#include "com_cmds.h"
#include "com_misc.h"
#include "com_parser.h"
#include "build/filesystem.h"
#include "funct.h"
#include "global.h"

void COM_Include(con_compiler_t* ctx) {
    ctx->script_cursor--;

    COM_SkipSpace(ctx);

    char* file = COM_LexString(ctx);
    // Fix path for unix. (doesn't really matter...)
    FixFilePath(file);
    char con_file[512];
    sprintf(con_file, "%s", file);

    int fp = TCkopen4load(con_file, ctx->read_grp);
    if (fp <= 0) {
        COM_Error("Could not find '%s'.\n", con_file);
        gameexit("");
        return;
    }

    printf("Including: '%s'.\n", con_file);

    // Save current state.
    short temp_line_number = ctx->line_number;
    u8 temp_ifelse_check = ctx->if_depth;
    char* origtptr = ctx->cursor;

    // Lex included file.
    i32 j = kfilelength(fp);
    ctx->line_number = 1;
    ctx->if_depth = 0;
    ctx->cursor = &ctx->src[ctx->src_size];
    ctx->cursor[j] = 0;
    kread(fp, (u8*) ctx->cursor, j);
    kclose(fp);
    ud.conCRC[0] = crc32_update((u8*) ctx->cursor, j, ud.conCRC[0]);
    while (true) {
        con_keyword_t keyword = COM_ParseCmd(ctx);
        if (keyword == CK_NONE) {
            break;
        }
    }
    ctx->total_lines += ctx->line_number;

    // Restore current state.
    ctx->cursor = origtptr;
    ctx->line_number = temp_line_number;
    ctx->if_depth = temp_ifelse_check;
}
