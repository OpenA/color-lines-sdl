#ifndef __BOARD_H
#define __BOARD_H

#define BOARD_W 9
#define BOARD_H 9
#define POOL_SIZE 3
#define BALLS_ROW 5

enum {
	ball1 = 1,
	ball2,
	ball3,
	ball4,
	ball5,
	ball6,
	ball7,
	ball_joker,
	ball_bomb,
	ball_brush,
	ball_boom,
	ball_max,
} ball_t;

#define COLORS_NR 7
#define BONUSES_NR 4
typedef unsigned char cell_t;

extern int board_init(void);
extern int board_fill(int *x, int *y); /* calls gfx_fill_cell, gfx_clear_pool */
extern int board_display(void); /* calls gfx_display_board */
extern int board_fill_pool(void); /* calls gfx_fill_pool */
extern int board_select(int x, int y); /* calls gfx_select_ball, gfx_move, gfx_clean_cell */
extern cell_t board_cell(int x, int y);
extern void board_lock(void);
extern void board_unlock(void);
extern int board_selected(int *x, int *y);
extern int board_moved(int *x, int *y);
extern int board_logic(void);
extern int board_follow_path(int x, int y, int *ox, int *oy, int id);
extern int board_path(int x, int y);
extern void board_clear_path(int x, int y);
extern cell_t pool_cell(int x);
extern int board_score(void);
extern int board_score_mul(void);
extern int board_running(void);
extern int board_load(const char *path);
extern int board_save(const char *path);
#endif
