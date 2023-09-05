#ifndef _GAME_H_
# include <time.h>
# include "sys.h"
# include "board.c"
# define _GAME_H_

# define FL_PREF_LOOP 0x01

typedef struct {
	short volume;
	char track, flags;
	int _reservs;
} sets_t;

# define DEFAULT_SETS() {\
	.volume = 256, .track = 0,\
	._reservs = 0, .flags = 0,\
}

extern bool game_load_session(desk_t *brd, cstr_t path);
extern void game_save_session(desk_t *brd, cstr_t path);

extern bool game_load_settings(sets_t *pref, cstr_t path);
extern void game_save_settings(sets_t *pref, cstr_t path);

extern void game_load_hiscores(int *list, cstr_t path);
extern void game_save_hiscores(int *list, cstr_t path);
#endif //_GAME_H_
