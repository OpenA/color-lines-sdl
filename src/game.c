
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
	FILE *cfg_fs = fopen(sys_make_file_path(cfg_dir, CL_PREFS_NAME), "r");
	if ( !cfg_fs )
		return false;

	char buf[32];
	int  idx,val;
	while (fgets(buf, sizeof(buf), cfg_fs)) {
		if((idx = cstr_cmp_lit(CFG_P_SFX, buf, 0)) > 0) {
			val = atoi(&buf[idx]);
			pref->sfx_mute = val < 0;
			pref->sfx_vol  = val < 0 ? -val : val;
		} else
		if((idx = cstr_cmp_lit(CFG_P_BGM, buf, 0)) > 0) {
			val = atoi(&buf[idx]);
			pref->bgm_mute = val < 0;
			pref->bgm_vol  = val < 0 ? -val : val;
		} else
		if((idx = cstr_cmp_lit(CFG_P_MUS, buf, 0)) > 0) {
			val = atoi(&buf[idx]);
			pref->mus_loop = val < 0;
			pref->mus_num  = val < 0 ? -val : val;
		}
	}
	fclose(cfg_fs);
	return true;
}

void game_save_settings(prefs_t *pref, path_t *cfg_dir)
{
	FILE *cfg_fs = fopen(sys_make_file_path(cfg_dir, CL_PREFS_NAME), "w");
	if ( cfg_fs != NULL ) {
		fprintf(cfg_fs, CFG_GET_STR("Global"),
			(pref->sfx_mute ? -pref->sfx_vol : pref->sfx_vol),
			(pref->bgm_mute ? -pref->bgm_vol : pref->bgm_vol),
			(pref->mus_loop ? -pref->mus_num : pref->mus_num));
		fclose(cfg_fs);
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
	if (!sound_init_open(snd))
		return SUCESS_fail;

	if (pref->bgm_mute) conf_param_add(snd, FL_BGM_MUTE);
	if (pref->sfx_mute) conf_param_add(snd, FL_SFX_MUTE);

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
