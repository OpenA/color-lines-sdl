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
} setts_t;

# define DEFAULT_SETTS() {\
	.volume = 256, .track = 0,\
	._reservs = 0, .flags = 0,\
}

typedef struct {
	unsigned int hiscore, time;
	unsigned char who;
} record_t;

# define DEFAULT_RECORD(i) {\
	.hiscore = i, .time = 0, .who = 0,\
}

extern bool game_load_session(desk_t *brd, cstr_t path);
extern void game_save_session(desk_t *brd, cstr_t path);

extern bool game_load_settings(setts_t *pref, cstr_t path);
extern void game_save_settings(setts_t *pref, cstr_t path);

extern void game_load_records(record_t list[], cstr_t path);
extern void game_save_records(record_t list[], cstr_t path);
#endif //_GAME_H_
