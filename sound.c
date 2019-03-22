#include "main.h"
#include <SDL_mixer.h>
#include "sound.h"

#define EFFECTS_NR 7
#define MUS_HEAD 127

static struct {
	Mix_Music *soundtrack;
	Mix_Chunk *effects[ EFFECTS_NR ]; /* Mix_Chunk is like Mix_Music, only it's for ordinary sounds. */
	short current;
	bool disabled;
	char dir[ PATH_MAX ];
	char title[ MUS_HEAD ];
	int channels[ EFFECTS_NR ];
} SND = {
	.effects  = { NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	.channels = { -1, -1, -1, -1, -1, -1, -1}
};

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

bool snd_init(const char *game_data_dir)
{
	if (Mix_OpenAudio(44100, AUDIO_S16, 2, 4096)) {
		fprintf(stderr, "Unable to open audio!\n");
		return true;
	}
	strcpy(SND.dir, game_data_dir);
	strcat(SND.dir, "sounds/");
	
	char click   [ PATH_MAX ]; strcpy( click   , SND.dir ); strcat( click   , "click.wav"    );
	char bonus   [ PATH_MAX ]; strcpy( bonus   , SND.dir ); strcat( bonus   , "bonus.wav"    );
	char hiscore [ PATH_MAX ]; strcpy( hiscore , SND.dir ); strcat( hiscore , "hiscore.wav"  );
	char gameover[ PATH_MAX ]; strcpy( gameover, SND.dir ); strcat( gameover, "gameover.wav" );
	char boom    [ PATH_MAX ]; strcpy( boom    , SND.dir ); strcat( boom    , "boom.wav"     );
	char fadeout [ PATH_MAX ]; strcpy( fadeout , SND.dir ); strcat( fadeout , "fadeout.wav"  );
	char paint   [ PATH_MAX ]; strcpy( paint   , SND.dir ); strcat( paint   , "paint.wav"    );
	
	SND.effects[SND_CLICK]    = Mix_LoadWAV( click    );
	SND.effects[SND_BONUS]    = Mix_LoadWAV( bonus    );
	SND.effects[SND_HISCORE]  = Mix_LoadWAV( hiscore  );
	SND.effects[SND_GAMEOVER] = Mix_LoadWAV( gameover );
	SND.effects[SND_BOOM]     = Mix_LoadWAV( boom     );
	SND.effects[SND_FADEOUT]  = Mix_LoadWAV( fadeout  );
	SND.effects[SND_PAINT]    = Mix_LoadWAV( paint    );
	
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
	
	strcpy(track, SND.dir);
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
		Mix_FreeChunk(SND.effects[i]);
	Mix_HookMusicFinished(NULL);
	Mix_HaltMusic();
	Mix_FreeMusic(SND.soundtrack);
	SND.soundtrack = NULL;
	Mix_CloseAudio();
}

void snd_play(int num, int cnt)
{
	if (!SND.disabled && num < EFFECTS_NR)
		SND.channels[num] = Mix_PlayChannel(-1, SND.effects[num], cnt - 1);
}
