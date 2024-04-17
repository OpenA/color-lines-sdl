#ifndef _GAME_H_
# include <time.h>
# include "sys.h"
# include "board.c"
# define _GAME_H_

enum SND_T {
	SND_Click = 0,
	SND_Fadeout,
	SND_Bonus,
	SND_Boom,
	SND_Paint,
	SND_Hiscore,
	SND_Gameover
};

# include "sound_sdl.h"

enum FNT_T {
	FNT_Fancy,
	FNT_Pixil,
	FNT_Limon,
};

enum IMG_T {
	UI_Bg = 0,
	UI_Cell,
	UI_BallsCollect,
	UI_Volume,
	UI_Music,
	UI_Blank,
};

# include "ui.c"
# define INFL_SHOWED 0x2
# define INFL_MOVE   0x1

/* Player Preferences */
# define FL_PREF_BGM_LOOP 0x01
# define FL_PREF_BGM_PLAY 0x02
# define FL_PREF_SFX_MUTE 0x04

typedef struct {
	unsigned char sfx_vol, bgm_vol, track_num, flags;
	int _reservs;
} prefs_t;

# define DEFAULT_PREFS() {\
	.sfx_vol = 100, .track_num = 0,\
	.bgm_vol = 100, .flags = 0, ._reservs = 0\
}

# define game_prefs_add(pref,FL) ((pref)->flags |=  FL)
# define game_prefs_has(pref,FL) ((pref)->flags &   FL)
# define game_prefs_del(pref,FL) ((pref)->flags &= ~FL)

/* Player Records */
typedef struct {
	unsigned int hiscore, time;
	unsigned char who;
} record_t;

# define DEFAULT_RECORD(i) {\
	.hiscore = i, .time = 0, .who = 0,\
}
/* Load/Save last board session */
extern bool game_load_session(desk_t *brd, cstr_t path);
extern void game_save_session(desk_t *brd, cstr_t path);
/* Read/Write user settings */
extern bool game_load_settings(prefs_t *pref, cstr_t path);
extern void game_save_settings(prefs_t *pref, cstr_t path);
/* Load/Save records tab */
extern void game_load_records(record_t list[], cstr_t path);
extern void game_save_records(record_t list[], cstr_t path);

/* Init game subsystem */
extern SUCESS game_init_sound(sound_t *snd, prefs_t *pref, path_t game_dir);
extern SUCESS game_load_fonts(zfont_t fnt_list[], path_t game_dir);
#endif //_GAME_H_
