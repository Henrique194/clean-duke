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

#include "hu_text.h"
#include "funct.h"
#include "names.h"
#include "build/tiles.h"
#include "build/engine.h"

#define MAXUSERQUOTES 4

// Quote bottom: tracks where the quote starts.
int32_t quotebot;
// Where the quote is meant to end.
int32_t quotebotgoal;

static short user_quote_time[MAXUSERQUOTES];
static char user_quote[MAXUSERQUOTES][128];

char fta_quotes[NUMOFFIRSTTIMEACTIVE][64];


static bool HU_CanShowQuote(short q, const player_t* p, int mode) {
    if (ud.fta_on != 1 && !mode) {
        return false;
    }
    if (p->fta <= 0 || q == 115 || q == 116) {
        return true;
    }
    return p->ftq != 115 && p->ftq != 116;
}

void HU_SetPlayerQuote(short q, player_t* p, int mode) {
    if (!HU_CanShowQuote(q, p, mode)) {
        return;
    }
    p->fta = 100;
    if (p->ftq != q || q == 26) {
        p->ftq = q;
        pub = NUMPAGES;
        pus = NUMPAGES;
    }
}

void HU_AddUserQuote(const char* quote) {
    for (i32 i = MAXUSERQUOTES - 1; i > 0; i--) {
        strcpy(user_quote[i], user_quote[i - 1]);
        user_quote_time[i] = user_quote_time[i - 1];
    }
    strcpy(user_quote[0], quote);
    user_quote_time[0] = 180;
    pub = NUMPAGES;
}

void HU_TickPlayerQuote(player_t* p) {
    if (p->fta <= 0) {
        return;
    }
    p->fta--;
    if (p->fta == 0) {
        pub = NUMPAGES;
        pus = NUMPAGES;
        p->ftq = 0;
    }
}

void HU_TickUserQuotes(void) {
    for (i32 i = 0; i < MAXUSERQUOTES; i++) {
        if (!user_quote_time[i]) {
            continue;
        }
        user_quote_time[i]--;
        if (!user_quote_time[i]) {
            pub = NUMPAGES;
        }
    }
}


static i32 HU_GetUserQuotesYPos(void) {
    i32 j;
    if (ud.screen_size > 0) {
        j = 200 - 45;
    } else {
        j = 200 - 8;
    }
    quotebot = min(quotebot, j);
    quotebotgoal = min(quotebotgoal, j);
    if (ps[myconnectindex].gm & MODE_TYPE) {
        j -= 8;
    }
    quotebotgoal = j;
    j = quotebot;
    return j;
}

static i16 HU_GetUserQuoteBits(i32 q) {
    i32 time = user_quote_time[q];
    if (time > 4) {
        return 2 + 8 + 16;
    }
    if (time > 2) {
        return 2 + 8 + 16 + 1;
    }
    return 2 + 8 + 16 + 1 + 32;
}

static void HU_DrawUserQuotes(void) {
    i32 x = (320 >> 1);
    i32 y = HU_GetUserQuotesYPos();
    u8 shade = 0;
    for (i32 i = 0; i < MAXUSERQUOTES; i++) {
        const char* quote = user_quote[i];
        i32 time = user_quote_time[i];
        if (time <= 0) {
            break;
        }
        i16 style = HU_GetUserQuoteBits(i);
        HU_DrawText(x, y, quote, shade, style);
        y -= 8;
    }
}

static i32 HU_GetPlayerQuoteYPos(void) {
    if (ps[screenpeek].ftq == 115 || ps[screenpeek].ftq == 116) {
        int32_t k = quotebot;
        for (int32_t i = 0; i < MAXUSERQUOTES; i++) {
            if (user_quote_time[i] <= 0) {
                break;
            }
            k -= 8;
        }
        k -= 4;
        return k;
    }
    if (ud.coop == 1 || ud.screen_size < 1 || ud.multimode < 2) {
        return 0;
    }
    i32 j = 0;
    for (i32 i = connecthead; i >= 0; i = connectpoint2[i]) {
        if (i > j) {
            j = i;
        }
    }
    if (j >= 4 && j <= 8) {
        return 8 + 8;
    }
    if (j > 8 && j <= 12) {
        return 8 + 16;
    }
    if (j > 12) {
        return 8 + 24;
    }
    return 8;
}

static i16 HU_GetPlayerQuoteBits(void) {
    i32 time = ps[screenpeek].fta;
    if (time > 4) {
        return 2 + 8 + 16;
    }
    if (time > 2) {
        return 2 + 8 + 16 + 1;
    }
    return 2 + 8 + 16 + 1 + 32;
}

static void HU_DrawPlayerQuote(void) {
    if (ps[screenpeek].fta <= 1) {
        return;
    }
    i32 x = (320 >> 1);
    i32 y = HU_GetPlayerQuoteYPos();
    const char* quote = fta_quotes[ps[screenpeek].ftq];
    u8 shade = 0;
    i16 bits = HU_GetPlayerQuoteBits();
    HU_DrawText(x, y, quote, shade, bits);
}

void HU_DrawQuotes(void) {
    HU_DrawUserQuotes();
    HU_DrawPlayerQuote();
}


static void HU_DrawChar(int x, int y, short c, i8 shade, u8 pal, u8 stat) {
    i32 sx = x << 16;
    i32 sy = y << 16;
    i32 z = 65536L;
    i16 a = 0;
    i32 cx1 = 0;
    i32 cy1 = 0;
    i32 cx2 = xdim - 1;
    i32 cy2 = ydim - 1;
    rotatesprite(sx, sy, z, a, c, shade, pal, stat, cx1, cy1, cx2, cy2);
}

static int HU_CenterText(const char* text) {
    short newx = 0;
    while (*text) {
        char c = *text;
        if (c == ' ') {
            newx += 5;
            text++;
            continue;
        }
        short ac = c - '!' + STARTALPHANUM;
        if (ac < STARTALPHANUM || ac > ENDALPHANUM) {
            break;
        }
        if (c >= '0' && c <= '9') {
            newx += 8;
        } else {
            newx += tiles[ac].dim.width;
        }
        text++;
    }
    return (320 >> 1) - (newx >> 1);
}

static int HU_DrawTextFull(int x, int y, const char* text, i8 shade, u8 pal, u8 stat) {
    if (x == (320 >> 1)) {
        x = HU_CenterText(text);
    }
    while (*text) {
        char c = *text;
        if (c == ' ') {
            x += 5;
            text++;
            continue;
        }
        short ac = c - '!' + STARTALPHANUM;
        if (ac < STARTALPHANUM || ac > ENDALPHANUM) {
            break;
        }
        HU_DrawChar(x, y, ac, shade, pal, stat);
        if (c >= '0' && c <= '9') {
            x += 8;
        } else {
            x += tiles[ac].dim.width;
        }
        text++;
    }
    return x;
}

int HU_DrawText(int x, int y, const char* text, u8 shade, short stat) {
    u8 pal = 0;
    return HU_DrawTextFull(x, y, text, shade, pal, stat);
}

int HU_GameTextPal(int x, int y, const char* text, u8 shade, u8 pal) {
    u8 stat = 2 + 8 + 16;
    return HU_DrawTextFull(x, y, text, shade, pal, stat);
}

int HU_DrawMiniText(int x, int y, const char* text, u8 pal, u8 stat) {
    return HU_MiniTextShade(x, y, text, 0, pal, stat);
}

int HU_MiniTextShade(int x, int y, const char* text, u8 shade, u8 pal, u8 stat) {
    static char buf[128];
    strncpy(buf, text, 128);
    buf[127] = 0;
    const char* t = buf;
    while (*t) {
        char c = (char) toupper(*t);
        if (c == ' ') {
            x += 5;
            t++;
            continue;
        }
        short ac = c - '!' + MINIFONT;
        HU_DrawChar(x, y, ac, shade, pal, stat);
        x += 4; // tilesizx[ac]+1;
        t++;
    }
    return x;
}
