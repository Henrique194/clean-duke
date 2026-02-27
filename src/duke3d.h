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

#ifndef _INCL_DUKE3D_H_
#define _INCL_DUKE3D_H_

#include "build/build.h"

#ifndef MAX_PATH
#ifdef MAXPATHLEN
#define MAX_PATH MAXPATHLEN
#elif PATH_MAX
#define MAX_PATH PATH_MAX
#else
#define MAX_PATH 256
#endif
#endif

#if UNIX
#include "dukeunix.h"
#endif
#if PLATFORM_MACOSX
#include "dukeunix.h"
#endif
#if WIN32
#include "dukewin.h"
#endif
#include "types.h"
#include "gamedefs.h"
#include "audiolib/sounds.h"


#define RANCID_ID 1
#define XDUKE_ID  2
#define JONOF_ID  3

// Clean DukeNukem3D is a fork of xDuke v17.9
#define DUKE_ID XDUKE_ID


// rev is: CLEAN_DUKE_REV_X.CLEAN_DUKE_REV_DOT_Y
#define CLEAN_DUKE_REV_X     1
#define CLEAN_DUKE_REV_DOT_Y 0

#define MAX_KNOWN_GRP 4

#define CRC_BASE_GRP_SHAREWARE_13 0x983AD923
#define CRC_BASE_GRP_FULL_13      0xBBC9CE44
#define CRC_BASE_GRP_PLUTONIUM_14 0xF514A6AC
#define CRC_BASE_GRP_ATOMIC_15    0xFD3DCFF1
#define CRC_VALID(crc)                                                         \
    (crc == CRC_BASE_GRP_SHAREWARE_13 || crc == CRC_BASE_GRP_FULL_13           \
     || crc == CRC_BASE_GRP_PLUTONIUM_14 || crc == CRC_BASE_GRP_ATOMIC_15)

// implies  conVersion == 14 or conVersion == 15
#define PLUTOPAK  (!VOLUMEONE && !VOLUMEALL)

#define VOLUMEONE (getGRPcrc32(0) == CRC_BASE_GRP_SHAREWARE_13)

// VOLUMEALL = 1.3d full
#define VOLUMEALL                                                              \
    (getGRPcrc32(0) == CRC_BASE_GRP_FULL_13                                    \
     || (conVersion == 13 && getGRPcrc32(0) != CRC_BASE_GRP_SHAREWARE_13       \
         && getGRPcrc32(0) != CRC_BASE_GRP_PLUTONIUM_14                        \
         && getGRPcrc32(0) != CRC_BASE_GRP_ATOMIC_15))

#define SCREENSHOTPATH "screenshots"

// #define BETA
// #define AUSTRALIA

#define MAXSLEEPDIST 16384
#define SLEEPTIME    (24 * 64)

#define NUMPAGES 1

#define AUTO_AIM_ANGLE 48
#define RECSYNCBUFSIZ  2520 //2520 is the (LCM of 1-8)*3
#define MOVEFIFOSIZ    256

#define FOURSLEIGHT (1 << 8)

#define TICRATE      g_iTickRate
#define TICSPERFRAME (TICRATE / g_iTicksPerFrame)

#define NUM_SOUNDS 450

#define ALT_IS_PRESSED (KB_KeyPressed(sc_RightAlt) || KB_KeyPressed(sc_LeftAlt))
#define SHIFTS_IS_PRESSED                                                      \
    (KB_KeyPressed(sc_RightShift) || KB_KeyPressed(sc_LeftShift))
#define RANDOMSCRAP                                                            \
    EGS(s->sectnum, s->x + (TRAND & 255) - 128, s->y + (TRAND & 255) - 128,    \
        s->z - (8 << 8) - (TRAND & 8191), SCRAP6 + (TRAND & 15), -8, 48, 48,   \
        TRAND & 2047, (TRAND & 63) + 64, -512 - (TRAND & 2047), i, 5)

#define BLACK      0
#define DARKBLUE   1
#define DARKGREEN  2
#define DARKCYAN   3
#define DARKRED    4
#define DARKPURPLE 5
#define BROWN      6
#define LIGHTGRAY  7

#define DARKGRAY 8
#define BLUE     9
#define GREEN    10
#define CYAN     11
#define RED      12
#define PURPLE   13
#define YELLOW   14
#define WHITE    15

#define PHEIGHT (38 << 8)

#define WAIT(X)                                                                \
    ototalclock = totalclock + (X);                                            \
    while (totalclock < ototalclock)


#define MODE_MENU       1
#define MODE_DEMO       2
#define MODE_GAME       4
#define MODE_EOL        8
#define MODE_TYPE       16
#define MODE_RESTART    32
#define MODE_SENDTOWHOM 64
#define MODE_END        128


#define MAXANIMWALLS         512
#define MAXINTERPOLATIONS    2048
#define NUMOFFIRSTTIMEACTIVE 192

#define MAXCYCLERS    256
#define MAXSCRIPTSIZE 20460
#define MAXANIMATES   64

#define SP   sprite[i].yvel
#define SX   sprite[i].x
#define SY   sprite[i].y
#define SZ   sprite[i].z
#define SS   sprite[i].shade
#define PN   sprite[i].picnum
#define SA   sprite[i].ang
#define SV   sprite[i].xvel
#define ZV   sprite[i].zvel
#define RX   sprite[i].xrepeat
#define RY   sprite[i].yrepeat
#define OW   sprite[i].owner
#define CS   sprite[i].cstat
#define SH   sprite[i].extra
#define CX   sprite[i].xoffset
#define CY   sprite[i].yoffset
#define CD   sprite[i].clipdist
#define PL   sprite[i].pal
#define SLT  sprite[i].lotag
#define SHT  sprite[i].hitag
#define SECT sprite[i].sectnum

#define face_player       1
#define geth              2
#define getv              4
#define random_angle      8
#define face_player_slow  16
#define spin              32
#define face_player_smart 64
#define fleeenemy         128
#define jumptoplayer      257
#define seekplayer        512
#define furthestdir       1024
#define dodgebullet       4096

#define TRAND krand()

#define MAX_WEAPONS 12

#define KNEE_WEAPON       0
#define PISTOL_WEAPON     1
#define SHOTGUN_WEAPON    2
#define CHAINGUN_WEAPON   3
#define RPG_WEAPON        4
#define HANDBOMB_WEAPON   5
#define SHRINKER_WEAPON   6
#define DEVISTATOR_WEAPON 7
#define TRIPBOMB_WEAPON   8
#define FREEZE_WEAPON     9
#define HANDREMOTE_WEAPON 10
#define GROW_WEAPON       11

#define T1 hittype[i].temp_data[0]
#define T2 hittype[i].temp_data[1]
#define T3 hittype[i].temp_data[2]
#define T4 hittype[i].temp_data[3]
#define T5 hittype[i].temp_data[4]
#define T6 hittype[i].temp_data[5]

#define rnd(X) ((TRAND >> 8) >= (255 - (X)))

/* !!! FIXME: "sync" is defined in unistd.h ... :(  --ryan. */
#define sync duke_sync

#define MOVFIFOSIZ 256

#define NUMKEYS 19


enum {
    UNKNOWN_GRP = 0,
    SHAREWARE_GRP13,
    REGULAR_GRP13D,
    ATOMIC_GRP14_15,
    DUKEITOUTINDC_GRP
};

typedef struct {
    short i;
    int voice;
} SOUNDOWNER;


#pragma pack(push, 1)
typedef struct {
    int8_t avel;
    int8_t horz;

    short fvel;
    short svel;

    uint32_t bits;
} input;
#pragma pack(pop)

typedef struct {
    uint8_t* ptr;
    // tanguyf: if not volatile, the game can go into an infinite loop in optimised mode
    volatile uint8_t lock;
    int length;
    int num;
} SAMPLE;

struct animwalltype {
    short wallnum;
    int32_t tag;
};

#pragma pack(push, 4)
struct user_defs {
    uint8_t god;
    uint8_t warp_on;
    uint8_t cashman;
    uint8_t eog;
    uint8_t showallmap;

    uint8_t show_help;
    uint8_t scrollmode;
    uint8_t clipping;

    char user_name[MAXPLAYERS][32];
    char ridecule[10][40];
    char savegame[10][22];
    char pwlockout[128];
    char rtsname[128];

    //
    // 0: No Map
    // 1: Map Wire
    // 2: Full Map
    //
    uint8_t overhead_on;
    uint8_t last_overhead;

    short pause_on;
    short from_bonus;
    short camerasprite;
    short last_camsprite;
    short last_level;
    short secretlevel;

    int32_t const_visibility;
    int32_t uw_framerate;
    int32_t camera_time;
    int32_t folfvel;
    int32_t folavel;
    int32_t folx;
    int32_t foly;
    int32_t fola;
    int32_t reccnt;

    int32_t entered_name;
    int32_t screen_tilting;
    int32_t shadows;
    int32_t fta_on;
    int32_t executions;
    int32_t auto_run;
    int32_t coords;
    int32_t tickrate;
    int32_t m_coop;
    int32_t coop;
    int32_t screen_size;
    int32_t extended_screen_size;
    int32_t lockout;
    int32_t crosshair;
    int32_t showweapons;
    int32_t mywchoice[MAX_WEAPONS];
    int32_t wchoice[MAXPLAYERS][MAX_WEAPONS];
    int32_t playerai;

    int32_t respawn_monsters;
    int32_t respawn_items;
    int32_t respawn_inventory;
    int32_t recstat;
    int32_t monsters_off;
    int32_t brightness;
    int32_t m_respawn_items;
    int32_t m_respawn_monsters;
    int32_t m_respawn_inventory;
    int32_t m_recstat;
    int32_t m_monsters_off;
    int32_t detail;
    // FIX_00082: /q option taken off when playing a demo (multimode_bot)
    int32_t m_ffire;
    int32_t ffire;
    int32_t m_player_skill;
    int32_t m_level_number;
    int32_t m_volume_number;
    int32_t multimode;
    int32_t multimode_bot;
    int32_t player_skill;
    int32_t level_number;
    int32_t volume_number;
    int32_t m_marker;
    int32_t marker;
    int32_t mouseflip;

    int32_t showcinematics;
    int32_t hideweapon;
    int32_t auto_aim;
    int32_t gitdat_mdk; //AutoAim toggle variable.
    int32_t weaponautoswitch;

    // FIX_00015: Backward compliance with older demos (down to demos v27, 28, 116 and 117 only)
    uint8_t playing_demo_rev;

    uint32_t groupefil_crc32[MAXPLAYERS][4];
    uint16_t conSize[MAXPLAYERS];

#ifdef CHECK_XDUKE_REV
    uint8_t rev[MAXPLAYERS][10];
#endif
    uint32_t mapCRC[MAXPLAYERS];
    uint32_t exeCRC[MAXPLAYERS];
    uint32_t conCRC[MAXPLAYERS];
};
#pragma pack(pop)

struct player_orig {
    int32_t ox;
    int32_t oy;
    int32_t oz;

    short oa;
    short os;
};

typedef struct {
    unsigned int crc32;
    char* name;
    uint32_t size;
} crc32_t;

#pragma pack(push, 4)
typedef struct player_struct {
    int32_t zoom;
    int32_t exitx;
    int32_t exity;
    int32_t loogiex[64];
    int32_t loogiey[64];
    int32_t numloogs;
    int32_t loogcnt;
    int32_t posx;
    int32_t posy;
    int32_t posz;
    int32_t horiz;
    int32_t ohoriz;
    int32_t ohorizoff;
    int32_t invdisptime;
    int32_t bobposx;
    int32_t bobposy;
    int32_t oposx;
    int32_t oposy;
    int32_t oposz;
    int32_t pyoff;
    int32_t opyoff;
    int32_t posxv;
    int32_t posyv;
    int32_t poszv;
    int32_t last_pissed_time;
    int32_t truefz;
    int32_t truecz;
    int32_t player_par;
    int32_t visibility;
    int32_t bobcounter;
    int32_t weapon_sway;
    int32_t pals_time;
    int32_t randomflamex;
    int32_t crack_time;

    int32_t aim_mode;

    short ang;
    short oang;
    short angvel;
    short cursectnum;
    short look_ang;
    // Previous player health.
    short last_extra;
    short subweapon;
    short ammo_amount[MAX_WEAPONS];
    short wackedbyactor;
    short frag;
    short fraggedself;

    short curr_weapon;
    short last_weapon;
    short tipincs;
    short horizoff;
    short wantweaponfire;
    short holoduke_amount;
    short newowner;
    short hurt_delay;
    short hbomb_hold_delay;
    short jumping_counter;
    short airleft;
    short knee_incs;
    short access_incs;

    // count-down timer for the time to display a quote.
    short fta;
    // The quote number to display.
    short ftq;

    short access_wallnum;
    short access_spritenum;
    short kickback_pic;
    short got_access;
    short weapon_ang;
    short firstaid_amount;
    short somethingonplayer;
    short on_crane;
    short i;
    short one_parallax_sectnum;
    short over_shoulder_on;
    short random_club_frame;
    short fist_incs;
    short one_eighty_count;
    short cheat_phase;
    short dummyplayersprite;
    short extra_extra8;
    short quick_kick;
    short heat_amount;
    short actorsqu;
    short timebeforeexit;
    short customexitsound;

    short weaprecs[16];
    short weapreccnt;
    uint32_t interface_toggle_flag;

    short rotscrnang;
    short dead_flag;
    short show_empty_weapon;
    short scuba_amount;
    short jetpack_amount;
    short steroids_amount;
    short shield_amount;
    short holoduke_on;
    short pycount;
    short weapon_pos;
    short frag_ps;
    short transporter_hold;
    short last_full_weapon;
    short footprintshade;
    short boot_amount;

    int scream_voice;

    uint8_t gm;
    uint8_t on_warping_sector;
    uint8_t footprintcount;
    uint8_t hbomb_on;
    uint8_t jumping_toggle;
    uint8_t rapid_fire_hold;
    uint8_t on_ground;
    uint8_t name[32];
    uint8_t inven_icon;
    uint8_t buttonpalette;

    uint8_t jetpack_on;
    uint8_t spritebridge;
    uint8_t lastrandomspot;
    uint8_t scuba_on;
    uint8_t footprintpal;
    uint8_t heat_on;

    uint8_t holster_weapon;
    uint8_t falling_counter;
    uint8_t gotweapon[MAX_WEAPONS];
    uint8_t refresh_inventory;

    // tanguyf: Fix the palette pointer size being 32/64-bit dependent.
    // For some reason this is serialised into savegames.
    union
    {
        uint8_t* palette;
        uint64_t _palette_padding;
    };

    uint8_t toggle_key_flag;
    uint8_t knuckle_incs;
    uint8_t walking_snd_toggle;
    uint8_t palookup;
    uint8_t hard_landing;
    uint8_t max_secret_rooms;
    uint8_t secret_rooms;
    uint8_t pals[3];
    uint8_t max_actors_killed;
    uint8_t actors_killed;
    uint8_t return_to_center;

    // local but synch variables (ud is local but not synch):

    // FIX_00023: Moved Addfaz's autoaim handler to synch variables (to avoid out of synch)
    int32_t auto_aim; //AutoAim toggle variable.

    // FIX_00012:
    // Added "weapon autoswitch" toggle allowing to
    // turn the autoswitch off when picking up new weapons.
    // The weapon sound on pickup will remain on, to not
    // affect the opponent's gameplay (so he can still hear
    // you picking up new weapons)
    int32_t weaponautoswitch;

    uint8_t fakeplayer;
} player_t;
#pragma pack(pop)

#pragma pack(push, 4)
typedef struct weaponhit {
    uint8_t cgg;

    short picnum;
    short ang;
    short extra;
    short owner;
    short movflag;

    short tempang;
    short actorstayput;
    short dispicnum;

    short timetosleep;

    int32_t floorz;
    int32_t ceilingz;
    int32_t lastvx;
    int32_t lastvy;
    int32_t bposx;
    int32_t bposy;
    int32_t bposz;

    int32_t temp_data[6];
} weapon_hit_t;
#pragma pack(pop)

typedef struct {
    short frag[MAXPLAYERS];
    short got_access;
    short last_extra;
    short shield_amount;
    short curr_weapon;

    short ammo_amount[MAX_WEAPONS];
    short holoduke_on;

    uint8_t gotweapon[MAX_WEAPONS];
    uint8_t inven_icon;
    uint8_t jetpack_on;
    uint8_t heat_on;

    short firstaid_amount;
    short steroids_amount;
    short holoduke_amount;
    short jetpack_amount;

    short heat_amount;
    short scuba_amount;
    short boot_amount;

    short last_weapon;
    short weapon_pos;
    short kickback_pic;
} status_bar_t;


//  FIX_00022:
//  Automatically recognize the shareware grp (v1.3) +
//  full version (1.3d) + atomic (1.4/1.5 grp) and
//  the con files version (either 1.3 or 1.4) (JonoF's idea)
extern uint8_t conVersion;
extern uint8_t grpVersion;

// FIX_00015: Backward compliance with older demos (down to demos v27, 28, 116 and 117 only)
// For BYTEVERSION diff, 27/116 vs 28/117 see extras\duke3d.h vs source\duke3d.h
// from the official source code release.

// 1.3 under 1.4 Plutonium. Not supported anymore
#define BYTEVERSION_27 27
// 1.4 Plutonium. Not supported anymore
#define BYTEVERSION_116 116
// 1.3 under 1.5 engine
#define BYTEVERSION_28 28
// 1.5 Atomic
#define BYTEVERSION_117 117
// 1.3 under xDuke v19.6.
#define BYTEVERSION_29 29
// 1.5 Atomic under xDuke v19.6.
#define BYTEVERSION_118 118
// for 1.3 demos (Not compatible)
#define BYTEVERSION_1_3 1;
// xDuke v19.7
#define BYTEVERSION 119

extern int g_iTickRate;
extern int g_iTicksPerFrame;

extern input inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
extern input sync[MAXPLAYERS];
extern input recsync[RECSYNCBUFSIZ];

extern int32_t movefifosendplc;

extern struct animwalltype animwall[MAXANIMWALLS];
extern short numanimwalls;
extern short probey;
extern short lastprobey;

extern uint8_t typebuflen;
char typebuf[41];
extern int32_t msx[2048];
extern int32_t msy[2048];
extern short cyclers[MAXCYCLERS][6];
extern short numcyclers;
extern char myname[2048];

extern uint8_t numplayersprites;

extern crc32_t crc32lookup[];

extern int32_t fricxv;
extern int32_t fricyv;

extern uint8_t tempbuf[2048];
extern uint8_t packbuf[576];

// Gravitational Constant.
extern int32_t gc;
extern int32_t max_player_health;
extern int32_t max_armour_amount;
extern int32_t max_ammo_amount[MAX_WEAPONS];

extern int32_t impact_damage;
extern int32_t respawnactortime;
extern int32_t respawnitemtime;

extern short spriteq[1024];
extern short spriteqloc;
extern short spriteqamount;
extern struct player_struct ps[MAXPLAYERS];
extern struct player_orig po[MAXPLAYERS];
extern struct user_defs ud;

extern short int global_random;
extern int32_t scaredfallz;
extern char buf[80]; //My own generic input buffer

extern uint8_t scantoasc[128];
extern uint8_t ready2send;
extern uint8_t scantoascwithshift[128];

extern SAMPLE Sound[NUM_SOUNDS];
extern int32 VoiceToggle;
extern int32 AmbienceToggle;
extern int32 OpponentSoundToggle;
extern int32 mouseSensitivity_X;
extern int32 mouseSensitivity_Y;
extern SOUNDOWNER SoundOwner[NUM_SOUNDS][4];

extern uint8_t playerreadyflag[MAXPLAYERS];
extern uint8_t playerquitflag[MAXPLAYERS];
extern char sounds[NUM_SOUNDS][14];

extern i32 script[MAXSCRIPTSIZE];
extern i32* actorscrptr[MAXTILES];
extern u8 actortype[MAXTILES];

extern uint8_t ipath[80];
extern uint8_t opath[80];

extern char music_fn[4][11][13];
extern uint8_t music_select;
extern char env_music_fn[4][13];
extern short camsprite;

extern uint8_t inspace(short sectnum);

extern struct weaponhit hittype[MAXSPRITES];

extern input loc;
extern int32_t avgfvel;
extern int32_t avgsvel;
extern int32_t avgavel;
extern int32_t avghorz;
extern int32_t avgbits;

extern short numplayers;
extern short myconnectindex;
extern short connecthead;
// Player linked list variables (indeces, not connection numbers).
extern short connectpoint2[MAXPLAYERS];
extern short screenpeek;

extern int current_menu;
extern int32_t tempwallptr;
extern int32_t animatecnt;
extern int32_t lockclock;
extern uint8_t display_mirror;
extern uint8_t rtsplaying;

extern int32_t movefifoend[MAXPLAYERS];
extern int32_t ototalclock;

extern int32_t* animateptr[MAXANIMATES];
extern int32_t animategoal[MAXANIMATES];
extern int32_t animatevel[MAXANIMATES];
extern short neartagsector;
extern short neartagwall;
extern short neartagsprite;
extern int32_t neartaghitdist;
extern short animatesect[MAXANIMATES];
extern int32_t movefifoplc;
extern int32_t vel;
extern int32_t svel;
extern int32_t angvel;
extern int32_t horiz;

extern short mirrorwall[64];
extern short mirrorsector[64];
extern short mirrorcnt;


extern int32_t chainplace;
extern int32_t chainnumpages;
extern volatile int32_t checksume;


extern uint8_t screencapt;
extern short soundps[NUM_SOUNDS];
extern short soundpe[NUM_SOUNDS];
extern short soundvo[NUM_SOUNDS];
extern uint8_t soundpr[NUM_SOUNDS];
extern uint8_t soundm[NUM_SOUNDS];
extern int32_t soundsiz[NUM_SOUNDS];
extern char level_names[44][33];
extern int32_t partime[44];
extern int32_t designertime[44];
extern char volume_names[4][33];
extern char skill_names[5][33];
extern char level_file_names[44][128];

extern int32 SoundToggle;
extern int32 MusicToggle;
extern short last_threehundred;
extern short lastsavedpos;
extern uint8_t restorepalette;

extern short buttonstat;
extern uint8_t waterpal[768];
extern uint8_t slimepal[768];
extern uint8_t titlepal[768];
extern uint8_t drealms[768];
extern uint8_t endingpal[768];
extern char boardfilename[128];
extern uint8_t earthquaketime;
extern uint8_t lumplockbyte[11];

//DUKE3D.H - replace the end "my's" with this
extern int32_t myx;
extern int32_t omyx;
extern int32_t myxvel;
extern int32_t myy;
extern int32_t omyy;
extern int32_t myyvel;
extern int32_t myz;
extern int32_t omyz;
extern int32_t myzvel;
extern short myhoriz;
extern short omyhoriz;
extern short myhorizoff;
extern short omyhorizoff;
extern short globalskillsound;
extern short myang;
extern short omyang;
extern short mycursectnum;
extern short myjumpingcounter;
extern uint8_t myjumpingtoggle;
extern uint8_t myonground;
extern uint8_t myhardlanding;
extern uint8_t myreturntocenter;
extern int32_t fakemovefifoplc;
extern int32_t myxbak[MOVEFIFOSIZ];
extern int32_t myybak[MOVEFIFOSIZ];
extern int32_t myzbak[MOVEFIFOSIZ];
extern int32_t myhorizbak[MOVEFIFOSIZ];
extern short myangbak[MOVEFIFOSIZ];


//DUKE3D.H:
extern status_bar_t sbar;

extern short frags[MAXPLAYERS][MAXPLAYERS];
extern int32_t cameradist;
extern int32_t cameraclock;
extern int32_t dukefriction;
extern int32_t show_shareware;
extern uint8_t networkmode;
extern uint8_t movesperpacket;
extern uint8_t gamequit;

extern uint8_t pus;
extern uint8_t pub;
extern uint8_t camerashitable;
extern uint8_t freezerhurtowner;
extern uint8_t lasermode;
extern uint8_t syncstat;
extern uint8_t syncval[MAXPLAYERS][MOVEFIFOSIZ];
extern int8_t multiwho;
extern int8_t multipos;
extern int8_t multiwhat;
extern int8_t multiflag;
extern int32_t syncvalhead[MAXPLAYERS];
extern int32_t syncvaltail;
extern int32_t syncvaltottail;
extern int32_t numfreezebounces;
extern int32_t rpgblastradius;
extern int32_t pipebombblastradius;
extern int32_t tripbombblastradius;
extern int32_t shrinkerblastradius;
extern int32_t morterblastradius;
extern int32_t bouncemineblastradius;
extern int32_t seenineblastradius;
extern uint8_t stereo;
extern uint8_t playerswhenstarted;
extern uint8_t everyothertime;
extern int32_t myminlag[MAXPLAYERS];
extern int32_t mymaxlag;
extern int32_t otherminlag;
extern int32_t bufferjitter;

extern int32_t numinterpolations;
extern int32_t bakipos[MAXINTERPOLATIONS];
extern int32_t* curipos[MAXINTERPOLATIONS];

extern short numclouds;
extern short clouds[128];
extern short cloudx[128];
extern short cloudy[128];
extern int32_t cloudtotalclock;
extern int32_t totalmemory;


extern int32_t myaimmode;
extern int32_t myaimstat;
extern int32_t omyaimstat;

extern uint8_t nHostForceDisableAutoaim;

#endif // include-once header.