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

#define MUSIC_MID_NATIVE
#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>

#include "audiolib/music.h"
#include "build/engine.h"

/*
 Because the music is stored in a GRP file that is never fully loaded in RAM
 (the full version of Duke Nukem 3D is a 43MB GRP) we need to extract the music
 from it and store it in RAM.
*/
#define KILOBYTE (1024*1024)
uint8_t musicDataBuffer[100 * KILOBYTE];

char  *MUSIC_ErrorString(int ErrorNumber)
{
	return "";
}

int MUSIC_Init(int SoundCard, int Address)
{
    if (Mix_Init(MIX_INIT_MID) != MIX_INIT_MID)
    {
        printf("Mix_Init: %s\n", Mix_GetError());
        //return MUSIC_Error; // Do not fatal on missing MIDI subsystem.
    }

	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)==-1) {
	    printf("Mix_OpenAudio: %s\n", Mix_GetError());
    }
    
    return MUSIC_Ok;
}

int MUSIC_Shutdown(void)
{
	MUSIC_StopSong();
    return(MUSIC_Ok);
}

void MUSIC_SetMaxFMMidiChannel(int channel)
{
}

void MUSIC_SetVolume(int volume)
{
    Mix_VolumeMusic((int)(volume / 2));
}

void MUSIC_SetMidiChannelVolume(int channel, int volume)
{
}

void MUSIC_ResetMidiChannelVolumes(void)
{
}

int MUSIC_GetVolume(void)
{
	return 0;
}

void MUSIC_SetLoopFlag(int loopflag)
{
}

int MUSIC_SongPlaying(void)
{
	return Mix_PlayingMusic();
}

void MUSIC_Continue(void)
{
    if (Mix_PausedMusic())
        Mix_ResumeMusic();
}

void MUSIC_Pause(void)
{
    Mix_PauseMusic();
}

int MUSIC_StopSong(void)
{
	if ( (Mix_PlayingMusic()) || (Mix_PausedMusic()) )
        Mix_HaltMusic();
    
    return(MUSIC_Ok);
}



int MUSIC_PlaySong(char  *songFilename, int loopflag)
{
    int32_t fd =  0;
    int fileSize;
    SDL_RWops *rw;
    Mix_Music* sdlMusic;
    
    fd = kopen4load(songFilename,0);
    
	if(fd == -1){ 
        printf("The music '%s' cannot be found in the GRP or the filesystem.\n",songFilename);
        return 0;
    }
    
    
    
    fileSize = kfilelength( fd );
    if(fileSize >= sizeof(musicDataBuffer))
    {
        printf("The music '%s' was found but is too big (%dKB)to fit in the buffer (%lluKB).\n",songFilename,fileSize/1024,sizeof(musicDataBuffer)/1024);
        kclose(fd);
        return 0;
    }
    
    kread( fd, musicDataBuffer, fileSize);
    kclose( fd );
    
    //Ok, the file is in memory
    if ((rw = SDL_RWFromMem((void *) musicDataBuffer, fileSize)) == NULL)
    {
        printf("SDL_RWFromMem: %s\n", SDL_GetError());
        return 0;
    }
    
    if ((sdlMusic = Mix_LoadMUS_RW(rw, SDL_TRUE)) == NULL)
    {
        printf("Mix_LoadMUS_RW: %s\n", Mix_GetError());
        return 0;
    }
    
    if (Mix_PlayMusic(sdlMusic, (loopflag == MUSIC_PlayOnce) ? 0 : -1) == -1)
    {
        printf("Mix_PlayMusic: %s\n", Mix_GetError());
        return 0;
    }
    
    return 1;
}


void MUSIC_SetContext(int context)
{
}

int MUSIC_GetContext(void)
{
	return 0;
}

void MUSIC_SetSongTick(uint32_t PositionInTicks)
{
}

void MUSIC_SetSongTime(uint32_t milliseconds)
{
}

void MUSIC_SetSongPosition(int measure, int beat, int tick)
{
}

void MUSIC_GetSongPosition(songposition *pos)
{
}

void MUSIC_GetSongLength(songposition *pos)
{
}

int MUSIC_FadeVolume(int tovolume, int milliseconds)
{
	return(MUSIC_Ok);
}

int MUSIC_FadeActive(void)
{
	return 0;
}

void MUSIC_StopFade(void)

{
}

void MUSIC_RerouteMidiChannel(int channel, int cdecl (*function)( int event, int c1, int c2 ))
{
}

void MUSIC_RegisterTimbreBank(uint8_t  *timbres)
{
}

// This is the method called from the Game Module.
void PlayMusic(char  *fileName){
    MUSIC_PlaySong(fileName,1);
}
