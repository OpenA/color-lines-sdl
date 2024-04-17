
#include "game.h"

bool game_load_session(desk_t *brd, cstr_t path)
{
	FILE *file = fopen(path, "rb");
	bool  l_ok = false;

	if (file != NULL) {
		l_ok  = 0 < fread(brd, sizeof(desk_t), 1, file);
		fclose(file);
	}
	return l_ok;
}

void game_save_session(desk_t *brd, cstr_t path)
{
	FILE *file = fopen(path, "wb");
	if ( file != NULL ) {
		fwrite(brd, sizeof(desk_t), 1, file);
		fclose(file);
	}
}

bool game_load_settings(prefs_t *pref, cstr_t path)
{
	bool  l_ok = false;
	FILE *file = fopen(path, "rb");

	if ( file != NULL ) {
		l_ok = 0 < fread(pref, sizeof(prefs_t), 1, file);
		fclose(file);
	}
	return l_ok;
}

void game_save_settings(prefs_t *pref, cstr_t path)
{
	FILE *file = fopen(path, "wb");
	if ( file != NULL ) {
		fwrite(pref, sizeof(prefs_t), 1, file);
		fclose(file);
	}
}

void game_load_records(record_t list[], cstr_t path)
{
	FILE *file = fopen(path, "r");
	if ( file != NULL ) {
		for (int i = 0; i < HISCORES_NR; i++) {
			if (1 != fread(&list[i], sizeof(record_t), 1, file))
				break;
		}
		fclose(file);
	}
}

void game_save_records(record_t list[], cstr_t path)
{
	FILE *file = fopen(path, "w");
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

	sys_set_dpath(game_dir, SOUND_DIR);

# ifdef DEBUG
	printf("- check sounds in: %s\n", game_dir.path);
# endif
	for (int i = 0; i < SOUND_EFFECTS_N; i++) {
		cstr_t f = sys_get_fpath(game_dir, SOUND_EFFECTS_GET(i));
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
	sys_set_dpath(game_dir, FONT_DIR);

# ifdef DEBUG
	printf("- check fonts in: %s\n", game_dir.path);
# endif
	for (int i = 0; i < FONT_NAME_N; i++) {
		cstr_t f = sys_get_fpath(game_dir, FONT_NAME_GET(i));
# ifdef DEBUG
		printf("|~ font loading %i: %s\n", i, f);
# endif
		if (!(bitmap[i] = ui_load_image(f)))
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
