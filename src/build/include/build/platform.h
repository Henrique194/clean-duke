/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */

#ifndef _INCLUDE_PLATFORM_H_
#define _INCLUDE_PLATFORM_H_

#include <SDL.h>
#include <stdint.h>

#if (defined WIN32)
    #include "win32_compat.h"
#elif (defined UNIX)
    #include "unix_compat.h"
#elif (defined __APPLE__)
    #include "macos_compat.h"
#else
#error Define your platform!
#endif

#define BUILDSWAP_INTEL16 SDL_SwapLE16
#define BUILDSWAP_INTEL32 SDL_SwapLE32

#endif
