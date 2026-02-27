/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */
// FIX_00007: game speed corrected. The game speed is now as the real
// DOS duke3d. Unloading a full 200 bullet pistol must take 45.1 sec.
// SDL timer was not fast/accurate enough and was slowing down the gameplay,
// so bad

#include "build/timer.h"
#include "build/sdl_util.h"


static int64_t timerfreq = 0;
static int32_t timerlastsample = 0;
static int timerticspersec = 0;
static void (*usertimercallback)(void) = NULL;


//
//  The tickspersecond parameter is a ratio value that helps
//  to replicate oldschool DOS tick per seconds.
//  The way the timer work is:
//  float newSystemTickPerSecond = [0,1]
//  tickPerSecond on a DOS system = tickspersecond * newSystemTickPerSecond;
//
int inittimer(int tickspersecond) {
    if (timerfreq) {
        return 0; // already installed
    }
    //printf("Initialising timer, with tickPerSecond=%d\n",tickspersecond);
    timerfreq = 1000;
    timerticspersec = tickspersecond;
    int64_t t = SDL_GetTicks();
    timerlastsample = (int32_t) (t * timerticspersec / timerfreq);
    return 0;
}

void uninittimer(void) {
    if (!timerfreq) {
        return;
    }
    timerfreq = 0;
    timerticspersec = 0;
}

//
// Update totalclock.
//
void sampletimer(void) {
    int64_t i;
    int32_t n;

    if (!timerfreq)
        return;

    i = SDL_GetTicks();

    n = (int32_t) (i * timerticspersec / timerfreq) - timerlastsample;
    if (n > 0) {
        totalclock += n;
        timerlastsample += n;
    }

    if (usertimercallback)
        for (; n > 0; n--)
            usertimercallback();
}

//
// Returns the windows ticks count
// FCS: This seems to be only used in the multiplayer code
//
uint32_t getticks(void) {
    int64_t i = SDL_GetTicks();
    return (uint32_t) (i * 1000 / timerfreq);
}
