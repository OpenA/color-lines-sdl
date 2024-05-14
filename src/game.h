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
	UI_SetBox,
	UI_Gear,
	UI_BallsCollect,
	UI_ProgressBar,
	UI_Volume,
	UI_Sound,
	UI_Music,
/* system */
	UI_Snapshot,
	UI_Blank,
};

# include "ui.c"
# define UI_BITMAPS_L (UI_Blank+1)

/* Player Preferences */
typedef struct {
	unsigned char sfx_vol:7, sfx_mute:1, bgm_vol:7, bgm_mute:1;
	unsigned char mus_num:7, mus_loop:1, flags;
	unsigned int _r32;
} prefs_t;

# define DEFAULT_PREFS \
	.sfx_vol = 100, .sfx_mute = false, .flags = 0,\
	.bgm_vol = 100, .bgm_mute = false, ._r32 = 0,\
	.mus_num = 0x0, .mus_loop = false, 

/* Player Records */
typedef struct {
	unsigned int hiscore, time;
	unsigned char who;
} record_t;

# define DEFAULT_RECORD(i) {\
	.hiscore = i, .time = 0, .who = 0,\
}
/* Load/Save last board session */
extern bool game_load_session(desk_t *brd, path_t *cfg_dir);
extern void game_save_session(desk_t *brd, path_t *cfg_dir);
/* Read/Write user settings */
extern bool game_load_settings(prefs_t *pref, path_t *cfg_dir);
extern void game_save_settings(prefs_t *pref, path_t *cfg_dir);
/* Load/Save records tab */
extern void game_load_records(record_t list[], path_t *cfg_dir);
extern void game_save_records(record_t list[], path_t *cfg_dir);

/* Init game subsystem */
extern SUCESS game_init_sound(sound_t *snd, prefs_t *pref, path_t game_dir);
extern SUCESS game_load_fonts(zfont_t fnt_list[], path_t game_dir);
extern SUCESS game_load_images(el_img img_list[], path_t game_dir);
#endif //_GAME_H_
