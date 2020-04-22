#include "main.h"
#include <SDL_mixer.h>
#include "sound.h"

#define EFFECTS_NR 7
#define MUS_HEAD 127

static struct {
	Mix_Music *soundtrack;
	short current;
	bool disabled;
	char title[ MUS_HEAD ];
} SND;

/* Mix_Chunk is like Mix_Music, only it's for ordinary sounds. */
Mix_Chunk *effects[ EFFECTS_NR ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
      int channels[ EFFECTS_NR ] = { -1, -1, -1, -1, -1, -1, -1 };

static const unsigned char trackList[ TRACKS_COUNT ][32] = {
	"4stonewalls.it",
	"allnightalone.mod",
	"aloneinaworld.xm",
	"boondocks.xm",
	"crossfire.xm",
	"crystalhammer.mod",
	"flying.it",
	"hidninja.xm",
	"hipchip2.xm",
	"ihaveakey.xm",
	"insideoldpc.mod",
	"japanese.xm",
	"kilobyte.xm",
	"meeting.xm",
	"melancolie.s3m",
	"modernsurf.xm",
	"monastery.xm",
	"offencyt.xm",
	"ringsofmedusa.xm",
	"rusinahumppa.mod",
	"satellite.s3m",
	"skogensdjur.xm",
	"someone.mod",
	"tikob_ii.mod",
	"trackerode.xm",
	"xmas.xm"
};

bool snd_init(void)
{
	if (Mix_OpenAudio(44100, AUDIO_S16, 2, 4096)) {
		fprintf(stderr, "sound.c: Unable to open audio!\n");
		return true;
	}
	char click   [ PATH_MAX ]; strcpy( click   , GAME_DIR ); strcat( click   , "sounds/click.wav"    );
	char bonus   [ PATH_MAX ]; strcpy( bonus   , GAME_DIR ); strcat( bonus   , "sounds/bonus.wav"    );
	char hiscore [ PATH_MAX ]; strcpy( hiscore , GAME_DIR ); strcat( hiscore , "sounds/hiscore.wav"  );
	char gameover[ PATH_MAX ]; strcpy( gameover, GAME_DIR ); strcat( gameover, "sounds/gameover.wav" );
	char boom    [ PATH_MAX ]; strcpy( boom    , GAME_DIR ); strcat( boom    , "sounds/boom.wav"     );
	char fadeout [ PATH_MAX ]; strcpy( fadeout , GAME_DIR ); strcat( fadeout , "sounds/fadeout.wav"  );
	char paint   [ PATH_MAX ]; strcpy( paint   , GAME_DIR ); strcat( paint   , "sounds/paint.wav"    );

	effects[SND_CLICK]    = Mix_LoadWAV( click    );
	effects[SND_BONUS]    = Mix_LoadWAV( bonus    );
	effects[SND_HISCORE]  = Mix_LoadWAV( hiscore  );
	effects[SND_GAMEOVER] = Mix_LoadWAV( gameover );
	effects[SND_BOOM]     = Mix_LoadWAV( boom     );
	effects[SND_FADEOUT]  = Mix_LoadWAV( fadeout  );
	effects[SND_PAINT]    = Mix_LoadWAV( paint    );

	return false;
}

void snd_volume(short vol)
{
	int vsnd   =  vol / 2;
	int vmusic = (vol - vol / 5.5) / 2;
	//fprintf(stderr, " Volume { sounds: %d, music: %d }\n", vsnd, vmusic);
	
	Mix_Volume(-1, vsnd);
	Mix_VolumeMusic(vmusic);
	
	SND.disabled = !vsnd;
}

void snd_music_start(short num, char *name)
{
	if (SND.soundtrack) {
		if (SND.current == num) {
			Mix_PausedMusic() ? Mix_ResumeMusic() : Mix_PlayMusic(SND.soundtrack, 0);
			return;
		}
		Mix_HookMusicFinished(NULL);
		Mix_HaltMusic();
		Mix_FreeMusic(SND.soundtrack);
	}
	char track[ PATH_MAX ],
	      head[ MUS_HEAD ];
	
	strcpy(track, GAME_DIR);
	strcat(track, "sounds/");
	strcat(track, (char*)trackList[ (SND.current = num) ]);
	
	Mix_VolumeMusic(Mix_VolumeMusic(-1)); // -1 does not set the volume, but does return the current volume setting.
	SND.soundtrack = Mix_LoadMUS(track);
	FILE * file = fopen(track , "rb");
	
	if (file && !Mix_PlayMusic(SND.soundtrack, 0)) {
		fread(head, sizeof(head), 1, file);
		int i = 0, k = !strncmp(head, "Extended", 8) ? 17 : !strncmp(head, "IMPM", 4) ? 4 : 0;
		for (; k < strlen(head); i++, k++) {
			if (head[k] == 0x1A || (name[i] = SND.title[i] = head[k]) == '\0')
				break;
		}
		fclose(file);
		for (; i < strlen(name); i++)
			SND.title[i] = name[i] = '\0';
		for (; i < strlen(SND.title); i++)
			SND.title[i] = '\0';
		Mix_HookMusicFinished(track_switch);
	}
}

void snd_music_stop(void)
{
	if (SND.soundtrack && !Mix_PausedMusic()) {
		Mix_PauseMusic();
	}
}

void snd_done(void)
{
	for (int i = 0; i < EFFECTS_NR; i++)
		Mix_FreeChunk(effects[i]);
	Mix_HookMusicFinished(NULL);
	Mix_HaltMusic();
	Mix_FreeMusic(SND.soundtrack);
	SND.soundtrack = NULL;
	Mix_CloseAudio();
}

void snd_play(int num, int cnt)
{
	if (!SND.disabled && num < EFFECTS_NR)
		channels[num] = Mix_PlayChannel(-1, effects[num], cnt - 1);
}
