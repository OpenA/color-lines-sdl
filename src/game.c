
#include "game.h"

bool game_load_session(desk_t *brd, const char *path)
{
	FILE *file = fopen(path, "rb");
	bool  l_ok = false;

	if (file != NULL) {
		l_ok  = 0 < fread(brd, sizeof(desk_t), 1, file);
		fclose(file);
	}
	return l_ok;
}

void game_save_session(desk_t *brd, const char *path)
{
	FILE *file = fopen(path, "wb");
	if ( file != NULL ) {
		fwrite(brd, sizeof(desk_t), 1, file);
		fclose(file);
	}
}
