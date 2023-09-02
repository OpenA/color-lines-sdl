#ifndef __GAME_H
# define __GAME_H
# include <stdio.h>
# include "board.c"

extern bool game_load_session(desk_t *brd, const char *path);
extern void game_save_session(desk_t *brd, const char *path);

#endif //__GAME_H
