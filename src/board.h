#ifndef _BOARD_H_
# include <stdbool.h>
# include <stdlib.h>
# define _BOARD_H_

# include "cl_defines.h"

# define MSK_mPID 0x007FF000u // ~ (2047 << 12) MAX
# define MSK_mNUM 0x000007FFu // ~  2047 MAX
# define MSK_BALL 0x0F
# define MSK_ATTR 0xF0

# define FL_QUEUE 0x80
# define FL_mPATH 0x00000800u
# define FL_mBALL 0x00800000u

typedef enum {
	no_ball = 0, ball1, ball2, ball3, ball4, ball5, ball6, ball7,
	ball_joker,
	ball_flush,
	ball_brush,
	ball_bomb1,
	ball_bomb2,
	ball_bomb3,
} ball_t;

typedef enum {
	attr_normal = 0x00,
	attr_level1 = 0x10,
	attr_level2 = 0x20,
	attr_level3 = 0x30,
	attr_frozen = 0x70,
	attr_hidden = 0xF0
} attr_t;

# define  IS_OUT_DESK(x,y) (x < 0 || x >= BOARD_DESK_W || y < 0 || y >= BOARD_DESK_H)
# define  IS_BALL_COLOR(i) (i != no_ball    && i <  ball_joker)
# define  IS_BALL_JOKER(i) (i == ball_joker || i == ball_flush)

# define NEW_COLOR_BALL(i) ((i) % BALL_COLOR_N) + ball1
# define NEW_BONUS_BALL(i) ((i) % BALL_BONUS_N) + ball_joker
# define NEW_MPATH_ID(x,y) ((x + y * BOARD_DESK_W) << 12)

typedef unsigned int  muid_t;
typedef unsigned char cell_t;
typedef struct {
	int x, y;
} pos_t;

typedef struct {
	cell_t cells[BOARD_DESK_W][BOARD_DESK_H],
	        pool[BOARD_POOL_N];

	unsigned int score, playtime;
	unsigned char dmul, user_id;
} desk_t;

typedef enum {
	ST_End = 0,
	ST_Idle,
	ST_Moving,
	ST_FillPool,
	ST_FillDesk,
	ST_Check,
	ST_Remove
} state_t;

typedef struct {
	muid_t matrix[BOARD_DESK_W][BOARD_DESK_H];
//	cell_t  cuids[BOARD_DESK_W][BOARD_DESK_H];

	pos_t from, to;

	unsigned short free_n, pool_i, flush_n;
	unsigned char state, flags, flush_c;
} move_t;

/* BOARD GETTERS */
# define board_get_score(brd)         (brd)->score
# define board_get_delta(brd)         (brd)->dmul
# define board_get_cell(brd, x, y)    (brd)->cells[x][y]
# define board_get_pool(brd, i)       (brd)->pool [i]
# define board_get_time(brd)          (brd)->playtime
# define board_get_user(brd)          (brd)->user_id
/* BOARD SETTERS */
# define board_set_score(brd, s)      (brd)->score = s
# define board_set_delta(brd, d)      (brd)->dmul = d
# define board_set_cell(brd, x, y, c) (brd)->cells[x][y] = c
# define board_set_pool(brd, i, c)    (brd)->pool [i] = c
# define board_set_time(brd, t)       (brd)->playtime = t
# define board_set_user(brd, id)      (brd)->user_id = id
/* MOVE PATH GETTERS */
# define board_get_mnum(mov, x, y)   ((mov)->matrix[x][y] & MSK_mNUM)
# define board_get_mpid(mov, x, y)   ((mov)->matrix[x][y] & MSK_mPID)
# define board_get_muid(mov, x, y)    (mov)->matrix[x][y]
/* MOVE PATH SETTERS */
# define board_set_muid(mov, x,y,p,n) (mov)->matrix[x][y] = p | n
/* MOVE PATH OTHER */
# define board_del_select(mov)        (mov)->from.x = (mov)->from.y = -1
# define board_add_mpath(mov, x, y)   (mov)->matrix[x][y] |=  FL_mPATH
# define board_del_mpath(mov, x, y)   (mov)->matrix[x][y] &= ~FL_mPATH
# define board_has_mpath(mov, x, y)  ((mov)->matrix[x][y] &   FL_mPATH)
# define board_has_mball(mov, x, y)  ((mov)->matrix[x][y] &   FL_mBALL)
# define board_add_flush(mov, b)      (mov)->flush_c |= (1 << b)
# define board_del_flush(mov)         (mov)->flush_c  = (mov)->flush_n = 0
# define board_has_flush(mov, b)     ((mov)->flush_c &  (1 << b))
# define board_has_moves(mov)        ((mov)->state == ST_Check && (mov)->to.x != -1 && (mov)->to.y != -1)
# define board_has_over(mov)         ((mov)->state == ST_End)

# ifdef DEBUG
#  define print_dbg printf
extern void board_dbg_desk(desk_t *brd);
# else
#  define print_dbg (void)
# endif

extern void board_init_desk(desk_t *brd);
extern void board_init_move(move_t *mov, int nb);
extern void board_fill_desk(desk_t *brd, int pool_i, int free_n);
extern void board_fill_pool(desk_t *brd);
extern char board_next_move(desk_t *brd, move_t *mov);
extern void board_select_ball(desk_t *brd, move_t *mov, int x, int y);

static inline ball_t board_get_ball(desk_t *brd, int x, int y) {
	if (IS_OUT_DESK(x, y))
		return no_ball;
	return board_get_cell(brd, x, y) & MSK_BALL;
}
static inline ball_t board_get_selected(desk_t *brd, move_t *mov, int *x, int *y)
{
	ball_t b = board_get_ball(brd, mov->from.x, mov->from.y);
	if (b != no_ball)
		*x = mov->from.x, *y = mov->from.y;
	return b;
}
static inline void board_wait_finish(desk_t *brd, move_t *mov)
{
	while (board_next_move(brd, mov) > ST_Idle);
}

#endif //__BOARD_H
