#ifndef _SOUND_H_
# include <SDL2/SDL_mixer.h>
# include "config.h"
# define _SOUND_H_
/* user flags */
# define FL_SFX_MUTE 0x01
# define FL_BGM_MUTE 0x02
/* internal flags */
# define FL_SFX_VOID 0x10
# define FL_BGM_VOID 0x20
# define FL_BGM_PLYS 0x40
# define FL_MIX_OPEN 0x80

# define INT_VOL_STEP(v) (MIX_MAX_VOLUME * v / 100)

typedef struct {
	Mix_Music *bgm;
	Mix_Chunk *sfx [SOUND_EFFECTS_N];
	unsigned char flags, pid;
} sound_t;

# define sound_set_bgm_onEnd(_fn) Mix_HookMusicFinished(_fn)
# define sound_has_mix_ready(snd) ((snd)->flags & FL_MIX_OPEN)
# define sound_has_bgm_plyed(snd) ((snd)->flags & FL_BGM_PLYS)
# define sound_has_bgm_muted(snd) ((snd)->flags & ( FL_BGM_MUTE | FL_BGM_VOID ))
# define sound_has_sfx_muted(snd) ((snd)->flags & ( FL_SFX_MUTE | FL_SFX_VOID ))

/* ========== Sound BGM ============= */
static inline void sound_bgm_stop(sound_t *snd) {
	if (snd->bgm != NULL) {
		Mix_HookMusicFinished(NULL),
		Mix_HaltMusic();
		Mix_FreeMusic(snd->bgm),
		/* - - - - */ snd->bgm = NULL;
		conf_param_del(snd, FL_BGM_PLYS);
	}
}
static inline bool sound_bgm_pause(sound_t *snd) {
	bool has_plyed = sound_has_bgm_plyed(snd);
	if ( snd->bgm != NULL ) {
		if (has_plyed)
			Mix_PauseMusic(),  conf_param_del(snd, FL_BGM_PLYS);
		else
			Mix_ResumeMusic(), conf_param_add(snd, FL_BGM_PLYS);
		return has_plyed;
	}
	return false;
}
static inline bool sound_bgm_play(sound_t *snd, unsigned char pid, const char *track)
{
	sound_bgm_stop(snd);
// -1 does not set the volume, but does return the current volume setting.
//  Mix_VolumeMusic(Mix_VolumeMusic(-1));
	snd->bgm = Mix_LoadMUS(track);

	if (snd->bgm   != NULL && !Mix_PlayMusic(snd->bgm, 0)) {
		snd->flags |= FL_BGM_PLYS;
		snd->pid    = pid;
		return true;
	}
	return false;
}
static inline void sound_set_bgm_volume(sound_t *snd, unsigned char vol) {
	if (Mix_VolumeMusic(INT_VOL_STEP(vol))) {
		conf_param_del(snd, FL_BGM_VOID);
	} else {
		conf_param_add(snd, FL_BGM_VOID);
	}
}

/* ========== Sound SFX ============= */
static inline void sound_sfx_play(sound_t *snd, unsigned char n, int loops) {
	if (!sound_has_sfx_muted(snd) && n < SOUND_EFFECTS_N) {
		Mix_PlayChannel(-1, snd->sfx[n], loops - 1);
	}
}
static inline void sound_set_sfx_volume(sound_t *snd, unsigned char vol) {
	if (Mix_Volume(-1, INT_VOL_STEP(vol))) {
		conf_param_del(snd, FL_SFX_VOID);
	} else {
		conf_param_add(snd, FL_SFX_VOID);
	}
}

/* ========== Sound Init ============= */
static inline bool sound_init_open(sound_t *snd) {
	bool no_err = !Mix_OpenAudio(44100, AUDIO_S16, 2, 4096);

	for (int i = 0; i < SOUND_EFFECTS_N; i++)
		snd->sfx[i] = NULL;
	snd->flags = no_err ? FL_MIX_OPEN : FL_SFX_VOID | FL_BGM_VOID;
	snd->pid   = 0;
	snd->bgm   = NULL;
	return no_err;
}
static inline void sound_push_sample(sound_t *snd, int n, Uint8 *sample) {
	snd->sfx[n] = Mix_QuickLoad_WAV(sample);
}
static inline void sound_load_sample(sound_t *snd, int n, const char *file) {
	snd->sfx[n] = Mix_LoadWAV(file);
}
/* ========== Sound Destroy ============= */
static inline void sound_close_done(sound_t *snd) {
	Mix_HaltChannel(-1);
	sound_bgm_stop(snd);
	for (int i = 0; i < SOUND_EFFECTS_N; i++) {
		Mix_FreeChunk(snd->sfx[i]),
		/* - - - - */ snd->sfx[i] = NULL;
	}
	Mix_CloseAudio();
}

#endif
