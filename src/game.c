
#include "game.h"

bool game_load_session(desk_t *brd, path_t *cfg_dir)
{
	FILE *file = fopen(sys_make_file_path(cfg_dir, CL_SESSION_NAME), "rb");
	bool  l_ok = false;

	if (file != NULL) {
		l_ok  = 0 < fread(brd, sizeof(desk_t), 1, file);
		fclose(file);
	}
	return l_ok;
}

void game_save_session(desk_t *brd, path_t *cfg_dir)
{
	FILE *file = fopen(sys_make_file_path(cfg_dir, CL_SESSION_NAME), "wb");
	if ( file != NULL ) {
		fwrite(brd, sizeof(desk_t), 1, file);
		fclose(file);
	}
}

bool game_load_settings(prefs_t *pref, path_t *cfg_dir)
{
	bool  l_ok = false;
	FILE *file = fopen(sys_make_file_path(cfg_dir, CL_PREFS_NAME), "rb");

	if ( file != NULL ) {
		l_ok = 0 < fread(pref, sizeof(prefs_t), 1, file);
		fclose(file);
	}
	return l_ok;
}

void game_save_settings(prefs_t *pref, path_t *cfg_dir)
{
	FILE *file = fopen(sys_make_file_path(cfg_dir, CL_PREFS_NAME), "wb");
	if ( file != NULL ) {
		fwrite(pref, sizeof(prefs_t), 1, file);
		fclose(file);
	}
}

void game_load_records(record_t list[], path_t *cfg_dir)
{
	FILE *file = fopen(sys_make_file_path(cfg_dir, CL_RECORDS_NAME), "r");
	if ( file != NULL ) {
		for (int i = 0; i < HISCORES_NR; i++) {
			if (1 != fread(&list[i], sizeof(record_t), 1, file))
				break;
		}
		fclose(file);
	}
}

void game_save_records(record_t list[], path_t *cfg_dir)
{
	FILE *file = fopen(sys_make_file_path(cfg_dir, CL_RECORDS_NAME), "w");
	if ( file != NULL ) {
		for (int i = 0; i < HISCORES_NR; i++) {
			if (1 != fwrite(&list[i], sizeof(record_t), 1, file))
				break;
		}
		fclose(file);
	}
}

SUCESS game_init_sound(sound_t *snd, prefs_t *pref, path_t game_dir)
{
	// Initialize Sound
	if (!sound_init_open(snd)) {
		return SUCESS_fail;
	}
	sound_set_bgm_volume(snd, pref->bgm_vol);
	sound_set_sfx_volume(snd, pref->sfx_vol);

	sys_make_dir_path(&game_dir, SOUND_DIR);

# ifdef DEBUG
	printf("- check sounds in: %s\n", game_dir.path);
# endif
	for (int i = 0; i < SOUND_EFFECTS_N; i++) {
		cstr_t f = sys_make_file_path(&game_dir, SOUND_EFFECTS_GET(i));
		sound_load_sample(snd, i, f);
# ifdef DEBUG
		printf("|~ load sample %i: %s\n", i, f);
# endif
	}
	return SUCESS_ok;
}

SUCESS game_load_fonts(zfont_t fnt_list[], path_t game_dir)
{
	el_img bitmap[FONT_NAME_N];
	sys_make_dir_path(&game_dir, FONT_DIR);

# ifdef DEBUG
	printf("- check fonts in: %s\n", game_dir.path);
# endif
	for (int i = 0; i < FONT_NAME_N; i++) {
		cstr_t f = sys_make_file_path(&game_dir, FONT_NAME_GET(i));
# ifdef DEBUG
		printf("|~ font loading %i: %s\n", i, f);
# endif
		if (!(bitmap[i] = ui_load_image(f, SDL_TRUE)))
			return SUCESS_fail;
	}
	ui_init_font(&fnt_list[FNT_Fancy], bitmap[FNT_Fancy],
	ui_font_new_measures(
		FONT_FANCY_WIDTH,
		FONT_FANCY_HEIGHT,
		FONT_FANCY_SPACE,
		FONT_FANCY_PADDING));

	ui_init_font(&fnt_list[FNT_Pixil], bitmap[FNT_Pixil],
	ui_font_new_measures(
		FONT_PIXIL_WIDTH,
		FONT_PIXIL_HEIGHT,
		FONT_PIXIL_SPACE,
		FONT_PIXIL_PADDING));

	ui_init_font(&fnt_list[FNT_Limon], bitmap[FNT_Limon],
	ui_font_new_measures(
		FONT_LIMON_WIDTH,
		FONT_LIMON_HEIGHT,
		FONT_LIMON_SPACE,
		FONT_LIMON_PADDING));
	return SUCESS_ok;
}

SUCESS game_load_images(el_img img_list[], path_t game_dir)
{
	sys_make_dir_path(&game_dir, SVG_DIR);

# ifdef DEBUG
	printf("- check images in: %s\n", game_dir.path);
# endif
	for (int i = 0; i < SVG_IMAGES_N; i++) {
		cstr_t f = sys_make_file_path(&game_dir, SVG_IMAGE_GET(i));
# ifdef DEBUG
		printf("|~ load image %i: %s\n", i, f);
# endif
		if (!(img_list[i] = ui_load_image(f, (i != UI_Bg))))
			return SUCESS_fail;
	}
	img_list[UI_Blank] = GFX_CpySurfFormat(img_list[UI_Bg], GAME_SCREEN_W, GAME_SCREEN_H);
	                     GFX_DisableAlpha(img_list[UI_Blank]);
	return SUCESS_ok;
}
