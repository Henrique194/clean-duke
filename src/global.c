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

#if WIN32
#include <fcntl.h>
#endif

#include <SDL.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "duke3d.h"
#include "global.h"
#include "names.h"
#include "build/platform.h"

#include <errno.h>
#include <time.h>


crc32_t crc32lookup[] = {
    // good:
    {CRC_BASE_GRP_SHAREWARE_13, "SHAREWARE 1.3D", 11035779},
    {CRC_BASE_GRP_FULL_13, "FULL 1.3D     ", 26524524},
    {CRC_BASE_GRP_PLUTONIUM_14, "PLUTONIUM 1.4 ", 44348015},
    {CRC_BASE_GRP_ATOMIC_15, "ATOMIC 1.5    ", 44356548},
    // unknown:
    {0, "HACK/UNKNOWN  ", 0},
};

uint8_t conVersion = 13;
uint8_t grpVersion = 0;


short global_random;
short neartagsector;
short neartagwall;
short neartagsprite;

int32_t gc;
int32_t neartaghitdist;
int32_t lockclock;
int32_t max_player_health;
int32_t max_armour_amount;
int32_t max_ammo_amount[MAX_WEAPONS];

struct weaponhit hittype[MAXSPRITES];
short spriteq[1024];
short spriteqloc;
short spriteqamount = 64;

struct animwalltype animwall[MAXANIMWALLS];
short numanimwalls;
int32_t* animateptr[MAXANIMATES];
int32_t animategoal[MAXANIMATES];
int32_t animatevel[MAXANIMATES];
int32_t animatecnt;
short animatesect[MAXANIMATES];
int32_t msx[2048];
int32_t msy[2048];
short cyclers[MAXCYCLERS][6];
short numcyclers;

uint8_t tempbuf[2048];
uint8_t packbuf[576];

char buf[80];

short camsprite;
short mirrorwall[64];
short mirrorsector[64];
short mirrorcnt;

int current_menu;

char level_names[44][33];
char level_file_names[44][128];
int32_t partime[44];
int32_t designertime[44];

// Names are not in 1.3 con files. MUST be in code.
char volume_names[4][33] = {
    "L.A. MELTDOWN",
    "LUNAR APOCALYPSE",
    "SHRAPNEL CITY",
    "",
};
char skill_names[5][33] = {
    "PIECE OF CAKE",
    "LET'S ROCK",
    "COME GET SOME",
    "DAMN I'M GOOD",
    "",
};

volatile int32_t checksume;
int32_t soundsiz[NUM_SOUNDS];

short soundps[NUM_SOUNDS];
short soundpe[NUM_SOUNDS];
short soundvo[NUM_SOUNDS];
uint8_t soundm[NUM_SOUNDS];
uint8_t soundpr[NUM_SOUNDS];
char sounds[NUM_SOUNDS][14];

SAMPLE Sound[NUM_SOUNDS];
SOUNDOWNER SoundOwner[NUM_SOUNDS][4];

uint8_t numplayersprites;
uint8_t earthquaketime;

int32_t fricxv;
int32_t fricyv;
struct player_orig po[MAXPLAYERS];
struct player_struct ps[MAXPLAYERS];
struct user_defs ud;

uint8_t pus;
uint8_t pub;
uint8_t syncstat;
uint8_t syncval[MAXPLAYERS][MOVEFIFOSIZ];
int32_t syncvalhead[MAXPLAYERS];
int32_t syncvaltail;
int32_t syncvaltottail;

input sync[MAXPLAYERS];
input loc;
input recsync[RECSYNCBUFSIZ];
int32_t avgfvel;
int32_t avgsvel;
int32_t avgavel;
int32_t avghorz;
int32_t avgbits;


input inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
input recsync[RECSYNCBUFSIZ];

int32_t movefifosendplc;

//Multiplayer syncing variables
short screenpeek;
int32_t movefifoend[MAXPLAYERS];


//Game recording variables

uint8_t playerreadyflag[MAXPLAYERS];
uint8_t ready2send;
uint8_t playerquitflag[MAXPLAYERS];
int32_t vel;
int32_t svel;
int32_t angvel;
int32_t horiz;
int32_t ototalclock;
int32_t respawnactortime = 768;
int32_t respawnitemtime = 768;
int32_t groupfile;


uint8_t display_mirror;
uint8_t typebuflen;
char typebuf[41];

char music_fn[4][11][13];
uint8_t music_select;
char env_music_fn[4][13];
uint8_t rtsplaying;


int32_t impact_damage;

//GLOBAL.C - replace the end "my's" with this
int32_t myx;
int32_t omyx;
int32_t myxvel;
int32_t myy;
int32_t omyy;
int32_t myyvel;
int32_t myz;
int32_t omyz;
int32_t myzvel;
short myhoriz;
short omyhoriz;
short myhorizoff;
short omyhorizoff;
short myang;
short omyang;
short mycursectnum;
short myjumpingcounter;
short frags[MAXPLAYERS][MAXPLAYERS];

uint8_t myjumpingtoggle;
uint8_t myonground;
uint8_t myhardlanding;
uint8_t myreturntocenter;
int8_t multiwho;
int8_t multipos;
int8_t multiwhat;
int8_t multiflag;

int32_t fakemovefifoplc;
int32_t movefifoplc;
int32_t myxbak[MOVEFIFOSIZ];
int32_t myybak[MOVEFIFOSIZ];
int32_t myzbak[MOVEFIFOSIZ];
int32_t myhorizbak[MOVEFIFOSIZ];
int32_t dukefriction = 0xcc00;
int32_t show_shareware;

short myangbak[MOVEFIFOSIZ];
char myname[2048] = "XDUKE";
uint8_t camerashitable;
uint8_t freezerhurtowner = 0;
uint8_t lasermode;

uint8_t networkmode = 255;
uint8_t movesperpacket = 1;
uint8_t gamequit = 0;
uint8_t everyothertime;

int32_t numfreezebounces = 3;
int32_t rpgblastradius;
int32_t pipebombblastradius;
int32_t tripbombblastradius;
int32_t shrinkerblastradius;
int32_t morterblastradius;
int32_t bouncemineblastradius;
int32_t seenineblastradius;

status_bar_t sbar;

int32_t myminlag[MAXPLAYERS];
int32_t mymaxlag;
int32_t otherminlag;
int32_t bufferjitter = 1;
short numclouds;
short clouds[128];
short cloudx[128];
short cloudy[128];
int32_t cloudtotalclock = 0;
int32_t totalmemory = 0;


// portability stuff.  --ryan.
// A good portion of this was ripped from GPL'd Rise of the Triad.  --ryan.
void FixFilePath(char* filename) {
#if UNIX
    char* lastsep = filename;
    if ((!filename) || (*filename == '\0')) {
        return;
    }
    if (access(filename, F_OK) == 0) {
        // File exists, so we're good to go.
        return;
    }
    for (char* ptr = filename; 1; ptr++) {
        if (*ptr == '\\') {
            *ptr = PATH_SEP_CHAR;
        }
        if (*ptr != PATH_SEP_CHAR && *ptr != '\0') {
            continue;
        }

        char pch = *ptr;
        struct dirent* dent = NULL;
        DIR* dir;

        if ((pch == PATH_SEP_CHAR) && (*(ptr + 1) == '\0')) {
            // eos is pathsep; we're done.
            return;
        }
        if (lastsep == ptr) {
            // absolute path; skip to next one.
            continue;
        }

        *ptr = '\0';
        if (lastsep == filename) {
            dir = opendir((*lastsep == PATH_SEP_CHAR) ? ROOTDIR : CURDIR);
            if (*lastsep == PATH_SEP_CHAR) {
                lastsep++;
            }
        } else {
            *lastsep = '\0';
            dir = opendir(filename);
            *lastsep = PATH_SEP_CHAR;
            lastsep++;
        }

        if (dir == NULL) {
            // Maybe dir doesn't exist? Give up.
            *ptr = PATH_SEP_CHAR;
            return;
        }

        while ((dent = readdir(dir)) != NULL) {
            if (strcasecmp(dent->d_name, lastsep) == 0) {
                /* found match; replace it. */
                strcpy(lastsep, dent->d_name);
                break;
            }
        }

        closedir(dir);
        *ptr = pch;
        lastsep = ptr;

        if (dent == NULL) {
            // no match. oh well.
            return;
        }
        if (pch == '\0') {
            // eos?
            return;
        }
    }
#endif
}

#if WIN32

int _dos_findfirst(uint8_t  *filename, int x, struct find_t *f)
{
    int32_t rc = _findfirst(filename, &f->data);
    f->handle = rc;
    if (rc != -1)
    {
        strncpy(f->name, f->data.name, sizeof (f->name) - 1);
        f->name[sizeof (f->name) - 1] = '\0';
        return(0);
    }
    return(1);
}

int _dos_findnext(struct find_t *f)
{
    int rc = 0;
    if (f->handle == -1)
        return(1);   /* invalid handle. */

    rc = _findnext(f->handle, &f->data);
    if (rc == -1)
    {
        _findclose(f->handle);
        f->handle = -1;
        return(1);
    }

    strncpy(f->name, f->data.name, sizeof (f->name) - 1);
    f->name[sizeof (f->name) - 1] = '\0';
    return(0);
}

#elif defined(UNIX) || defined(PLATFORM_MACOSX)

int _dos_findfirst(char  *filename, int x, struct find_t *f)
{
    char  *ptr;

    if (strlen(filename) >= sizeof (f->pattern))
        return(1);

    strcpy(f->pattern, filename);
    FixFilePath(f->pattern);
    ptr = strrchr(f->pattern, PATH_SEP_CHAR);

    if (ptr == NULL)
    {
        ptr = filename;
        f->dir = opendir(CURDIR);
    }
    else
    {
        *ptr = '\0';
        f->dir = opendir(f->pattern);
        memmove(f->pattern, ptr + 1, strlen(ptr + 1) + 1);
    }

    return(_dos_findnext(f));
}

static int check_pattern_nocase(const char  *x, const char  *y)
{
    if ((x == NULL) || (y == NULL))
        return(0);  /* not a match. */

    while ((*x) && (*y))
    {
        if (*x == '*')
        {
            x++;
            while (*y != '\0')
            {
                if (toupper((int) *x) == toupper((int) *y))
                    break;
                y++;
            }
        }

        else if (*x == '?')
        {
            if (*y == '\0')
                return(0);  /* anything but EOS is okay. */
        }

        else
        {
            if (toupper((int) *x) != toupper((int) *y))
                return(0);  /* not a match. */
        }

        x++;
        y++;
    }

    return(*x == *y);  /* it's a match (both should be EOS). */
}

int _dos_findnext(struct find_t *f)
{
    struct dirent *dent;

    if (f->dir == NULL)
        return(1);  /* no such dir or we're just done searching. */

    while ((dent = readdir(f->dir)) != NULL)
    {
        if (check_pattern_nocase(f->pattern, dent->d_name))
        {
            if (strlen(dent->d_name) < sizeof (f->name))
            {
                strcpy(f->name, dent->d_name);
                return(0);  /* match. */
            }
        }
    }

    closedir(f->dir);
    f->dir = NULL;
    return(1);  /* no match in whole directory. */
}

#else
#error please define for your platform.
#endif

int FindDistance2D(int ix, int iy)
{
  int   t;

  ix= abs(ix);        /* absolute values */
  iy= abs(iy);

  if (ix<iy)
  {
     int tmp = ix;
     ix = iy;
     iy = tmp;
  }

  t = iy + (iy>>1);

  return (ix - (ix>>5) - (ix>>7)  + (t>>2) + (t>>6));
}

int FindDistance3D(int ix, int iy, int iz)
{
   int   t;

   ix= abs(ix);           /* absolute values */
   iy= abs(iy);
   iz= abs(iz);

   if (ix<iy)
   {
     int tmp = ix;
     ix = iy;
     iy = tmp;
   }

   if (ix<iz)
   {
     int tmp = ix;
     ix = iz;
     iz = tmp;
   }

   t = iy + iz;

   return (ix - (ix>>4) + (t>>2) + (t>>3));
}

void Error (int errorType, const char *error, ...)
{
   va_list argptr;

   SDL_Quit();

   //FCS: http://duke3d.m-klein.com is obscolete :/ !
   /*
   if(errorType==EXIT_FAILURE)
	   printf("ERROR: Please copy that screen and visit http://duke3d.m-klein.com for report:\n");
	else
		printf("http://duke3d.m-klein.com\n");
	*/


   va_start (argptr, error);
   vprintf(error, argptr);
   va_end (argptr);

   //printf("Press any key to continue...\n");

	// FIX_00043: Nicer exit on error. Ask the user to hit a key on exits and error exits.
   //getch();

   exit (errorType);
}

int32 SafeOpenAppend (const char  *_filename, int32 filetype)
{
	int	handle;
    char  filename[MAX_PATH];
    
    strncpy(filename, _filename, sizeof (filename));
    filename[sizeof (filename) - 1] = '\0';
    FixFilePath(filename);

#if (defined WIN32)
    handle = open(filename,O_RDWR | O_BINARY | O_CREAT | O_APPEND );
#else
	handle = open(filename,O_RDWR | O_BINARY | O_CREAT | O_APPEND , S_IREAD | S_IWRITE);
#endif

	if (handle == -1)
		Error (EXIT_FAILURE, "Error opening for append %s: %s",filename,strerror(errno));

	return handle;
}

boolean SafeFileExists ( const char  * _filename )
{
    char  filename[MAX_PATH];
    strncpy(filename, _filename, sizeof (filename));
    filename[sizeof (filename) - 1] = '\0';
    FixFilePath(filename);

#if( defined WIN32)
    return(access(filename, 6) == 0);
#else
    return(access(filename, F_OK) == 0);
#endif
}


int32 SafeOpenWrite (const char  *_filename, int32 filetype)
{
	int	handle;
    char  filename[MAX_PATH];
    strncpy(filename, _filename, sizeof (filename));
    filename[sizeof (filename) - 1] = '\0';
    FixFilePath(filename);

#if (defined WIN32)
    handle = open(filename,O_RDWR | O_BINARY | O_CREAT | O_TRUNC );
#else
	handle = open(filename,O_RDWR | O_BINARY | O_CREAT | O_TRUNC
	, S_IREAD | S_IWRITE);
#endif

	if (handle == -1)
		Error (EXIT_FAILURE, "Error opening %s: %s",filename,strerror(errno));

	return handle;
}




int32 SafeOpenRead (const char  *_filename, int32 filetype)
{
	int	handle;
    char  filename[MAX_PATH];
    strncpy(filename, _filename, sizeof (filename));
    filename[sizeof (filename) - 1] = '\0';
    FixFilePath(filename);

	handle = open(filename,O_RDONLY | O_BINARY);

	if (handle == -1)
		Error (EXIT_FAILURE, "Error opening %s: %s",filename,strerror(errno));

	return handle;
}


void SafeRead (int32 handle, void *buffer, int32 count)
{
	unsigned	iocount;

	while (count)
	{
		iocount = count > 0x8000 ? 0x8000 : count;
		if (read (handle,buffer,iocount) != (int)iocount)
			Error (EXIT_FAILURE, "File read failure reading %ld bytes",count);
		buffer = (void *)( (byte *)buffer + iocount );
		count -= iocount;
	}
}


void SafeWrite (int32 handle, void *buffer, int32 count)
{
	unsigned	iocount;

	while (count)
	{
		iocount = count > 0x8000 ? 0x8000 : count;
		if (write (handle,buffer,iocount) != (int)iocount)
			Error (EXIT_FAILURE, "File write failure writing %ld bytes",count);
		buffer = (void *)( (byte *)buffer + iocount );
		count -= iocount;
	}
}

void *SafeMalloc (int32_t size)
{
	void *ptr;

    ptr = malloc(size);

	if (!ptr)
      Error (EXIT_FAILURE, "SafeMalloc failure for %lu bytes",size);

	return ptr;
}

void SafeRealloc (void **x, int32 size)
{
	void *ptr;

    ptr = realloc(*x, size);

	if (!ptr)
      Error (EXIT_FAILURE, "SafeRealloc failure for %lu bytes",size);

    *x = ptr;
}

void SafeFree (void * ptr)
{
   if ( ptr == NULL )
      Error (EXIT_FAILURE, "SafeFree : Tried to free a freed pointer\n");

    free(ptr);

}


uint8_t CheckParm(const char* check) {
    for (int i = 1; i < _argc; i++) {
        const char* parm = _argv[i];
        if (*parm != '-') {
            continue;
        }
        if (strcmpi(parm + 1, check) == 0) {
            return (uint8_t) i;
        }
    }
    return 0;
}


static void (*shutdown_func)(void) = NULL;

void RegisterShutdownFunction( void (* shutdown) (void) )
{
    shutdown_func = shutdown;
}

void Shutdown(void)
{
    if (shutdown_func != NULL)
    {
        shutdown_func();
        shutdown_func = NULL;
    }
}


