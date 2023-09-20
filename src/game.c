
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

bool game_load_settings(setts_t *pref, cstr_t path)
{
	bool  l_ok = false;
	FILE *file = fopen(path, "rb");

	if ( file != NULL ) {
		l_ok = 0 < fread(pref, sizeof(setts_t), 1, file);
		fclose(file);
	}
	return l_ok;
}

void game_save_settings(setts_t *pref, cstr_t path)
{
	FILE *file = fopen(path, "wb");
	if ( file != NULL ) {
		fwrite(pref, sizeof(setts_t), 1, file);
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
