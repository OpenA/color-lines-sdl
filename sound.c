#include <SDL.h>
#include <SDL_mixer.h>
#include "sound.h"

/* Mix_Chunk is like Mix_Music, only it's for ordinary sounds. */
#define EFFECTS_NR 7

Mix_Chunk *effects[EFFECTS_NR] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
      int channels[EFFECTS_NR] = { -1, -1, -1, -1, -1, -1, -1};

Mix_Music *Music = NULL;
static bool SND_DISABLED = false;
static char SND_DIR[ 1024 ];

bool snd_init(char *g_datadir)
{
	if (Mix_OpenAudio(44100, AUDIO_S16, 2, 4096)) {
		fprintf(stderr, "Unable to open audio!\n");
		return true;
	}
	strcpy(SND_DIR, g_datadir);
	strcat(SND_DIR, "sounds/");
	
	char click   [ 1024 ]; strcpy( click   , SND_DIR ); strcat( click   , "click.wav"    );
	char bonus   [ 1024 ]; strcpy( bonus   , SND_DIR ); strcat( bonus   , "bonus.wav"    );
	char hiscore [ 1024 ]; strcpy( hiscore , SND_DIR ); strcat( hiscore , "hiscore.wav"  );
	char gameover[ 1024 ]; strcpy( gameover, SND_DIR ); strcat( gameover, "gameover.wav" );
	char boom    [ 1024 ]; strcpy( boom    , SND_DIR ); strcat( boom    , "boom.wav"     );
	char fadeout [ 1024 ]; strcpy( fadeout , SND_DIR ); strcat( fadeout , "fadeout.wav"  );
	char paint   [ 1024 ]; strcpy( paint   , SND_DIR ); strcat( paint   , "paint.wav"    );
	
	effects[SND_CLICK]    = Mix_LoadWAV( click    );
	effects[SND_BONUS]    = Mix_LoadWAV( bonus    );
	effects[SND_HISCORE]  = Mix_LoadWAV( hiscore  );
	effects[SND_GAMEOVER] = Mix_LoadWAV( gameover );
	effects[SND_BOOM]     = Mix_LoadWAV( boom     );
	effects[SND_FADEOUT]  = Mix_LoadWAV( fadeout  );
	effects[SND_PAINT]    = Mix_LoadWAV( paint    );
//	Mix_Volume(-1, 127);
	return false;
}

void snd_volume(int vol)
{
	int vsnd   =  vol / 2;
	int vmusic = (vol - vol / 8) / 2;
	
	Mix_Volume(-1, vsnd);
	Mix_VolumeMusic(vmusic);
	
//	for (int i = 0 ; i < EFFECTS_NR && effects[i]; i++)
//		Mix_VolumeChunk(effects[i], vsnd);
	
	SND_DISABLED = !vsnd;
}

bool snd_music_start(void)
{
	if (Music) {
		if (Mix_PausedMusic())
			Mix_ResumeMusic();
		return true;
	}
	char track[1024];
	strcpy(track, SND_DIR);
	strcat(track, "hipchip2.xm");
	Mix_VolumeMusic(Mix_VolumeMusic(-1)); // hack?
	return !Mix_PlayMusic((Music = Mix_LoadMUS(track)), -1);
}

void snd_music_stop(void)
{
	if (Music && !Mix_PausedMusic())
		Mix_PauseMusic();
}

void snd_done(void)
{
	for (int i = 0; i < EFFECTS_NR; i++)
		Mix_FreeChunk(effects[i]);
	Mix_HaltMusic();
	Mix_FreeMusic(Music);
	Music = NULL;
	Mix_CloseAudio();
}

void snd_play(int num, int cnt)
{
	if (!SND_DISABLED && num < EFFECTS_NR)
		channels[num] = Mix_PlayChannel(-1, effects[num], cnt - 1);
}
