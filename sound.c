#include "sound.h"
#include <SDL.h>
#include <SDL_mixer.h>

#include <stdio.h>
#include <stdlib.h>

/* Mix_Chunk is like Mix_Music, only it's for ordinary sounds. */
#define EFFECTS_NR 7

Mix_Chunk *effects[EFFECTS_NR] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
int channels[EFFECTS_NR] = { -1, -1, -1, -1, -1, -1, -1};

Mix_Music *music = NULL;

int audio_rate = 44100;
Uint16 audio_format = AUDIO_S16; 
int audio_channels = 2;
int audio_buffers = 4096;
static int snd_disabled = 0;

int snd_init(void) 
{
	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
		fprintf(stderr, "Unable to open audio!\n");
		return -1;
	}
	effects[SND_CLICK] = Mix_LoadWAV(GAMEDATADIR"sounds/click.wav");
	effects[SND_BONUS] = Mix_LoadWAV(GAMEDATADIR"sounds/bonus.wav");
	effects[SND_HISCORE] = Mix_LoadWAV(GAMEDATADIR"sounds/hiscore.wav");
	effects[SND_GAMEOVER] = Mix_LoadWAV(GAMEDATADIR"sounds/gameover.wav");
	effects[SND_BOOM]  = Mix_LoadWAV(GAMEDATADIR"sounds/boom.wav");
	effects[SND_FADEOUT]  = Mix_LoadWAV(GAMEDATADIR"sounds/fadeout.wav");
	effects[SND_PAINT]  = Mix_LoadWAV(GAMEDATADIR"sounds/paint.wav");
//	Mix_Volume(-1, 127);
	return 0;
}

void snd_volume(int vol)
{
//	int i;
	int vsnd = vol / 2;
	int vmusic = (vol - vol / 8) / 2;
	Mix_Volume(-1, vsnd);
	Mix_VolumeMusic(vmusic);

//	for (i = 0 ; i < EFFECTS_NR && effects[i]; i ++)
//		Mix_VolumeChunk(effects[i], vsnd);
	
	if (!vsnd) {
		snd_disabled = 1;
	} else	
		snd_disabled = 0;
}

int snd_music_start(void)
{
	int rc;
	if (music) {
		if (Mix_PausedMusic())
			Mix_ResumeMusic();
		return 1;
	}
	music = Mix_LoadMUS(GAMEDATADIR"sounds/satellite.s3m");
	rc = (!Mix_PlayMusic(music, -1))? 1: 0;
	Mix_VolumeMusic(Mix_VolumeMusic(-1)); // hack?
	return rc;
}

void snd_music_stop(void)
{
	if (!music)
		return;
	if (!Mix_PausedMusic())	
		Mix_PauseMusic();
}

void snd_done(void)
{
	int i = 0;
	for (i = 0 ; i < EFFECTS_NR; i ++)
		Mix_FreeChunk(effects[i]);
	Mix_HaltMusic();
	Mix_FreeMusic(music);
	music = NULL;
	Mix_CloseAudio();
}

void snd_play(int num, int cnt)
{
	if (num >= EFFECTS_NR)
		return;
	if (snd_disabled)
		return;
	channels[num] = Mix_PlayChannel(-1, effects[num], cnt - 1);
}
