#ifndef __BOARD_H
#define __BOARD_H

#define BOARD_W 9
#define BOARD_H 9
#include <stdbool.h>

#define POOL_SIZE  3
#define BALLS_ROW  5
#define COLORS_NR  7
#define BONUSES_NR 4
#define BONUS_PCNT 5
#define BALLS_NR  (COLORS_NR + BONUSES_NR)
#define FL_PATH  0x80

typedef enum {
	no_ball = 0, ball1, ball2, ball3, ball4, ball5, ball6, ball7,
	ball_joker,
	ball_flush,
	ball_brush,
	ball_bomb1,
	ball_bomb2,
	ball_max,
} ball_t;

#define  IS_BALL_COLOR(i) (i != no_ball    && i <  ball_joker)
#define  IS_BALL_JOKER(i) (i == ball_joker || i == ball_flush)

#define NEW_COLOR_BALL(i) ((i) %  COLORS_NR) + ball1
#define NEW_BONUS_BALL(i) ((i) % BONUSES_NR) + ball_joker

typedef enum {
	ST_END = 0,
	ST_IDLE,
	ST_MOVING,
	ST_FILL_POOL,
	ST_FILL_BOARD,
	ST_CHECK,
	ST_REMOVE
} step_t;

typedef unsigned char cell_t;

extern void board_init(void);
extern bool board_fill(int *x, int *y); /* calls gfx_fill_cell, gfx_clear_pool */
extern void board_display(void); /* calls gfx_display_board */
extern void board_fill_pool(void); /* calls gfx_fill_pool */
extern bool board_select(int x, int y); /* calls gfx_select_ball, gfx_move, gfx_clean_cell */
extern cell_t board_cell(int x, int y);
extern void board_time_update(int sec);
extern bool board_selected(int *x, int *y);
extern bool board_moved(int *x, int *y);
extern step_t board_next_step(void);
extern bool board_follow_path(int x, int y, int *ox, int *oy, int id);
extern bool board_path(int x, int y);
extern void board_clear_path(int x, int y);
extern cell_t pool_cell(int x);
extern int board_time(void);
extern int board_score(void);
extern int board_score_mul(void);

static inline void board_finally(void)
{
	while (board_next_step() > ST_IDLE);
}
#endif
