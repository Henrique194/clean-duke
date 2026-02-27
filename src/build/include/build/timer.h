/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */

#ifndef TIMER_H
#define TIMER_H

#include "build/platform.h"

// This is implemented in the game, and your driver calls it.
void timerhandler(void);

int inittimer(int);

void uninittimer(void);

void sampletimer(void);

uint32_t getticks(void);

#endif
