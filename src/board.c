
#include "board.h"
#include "../board.c"

#define MATRIX_NEIR_N 4
#define MATRIX_NEIR(x,y) \
	{ .x = x - 1, .y = y/**/ },\
	{ .x = x + 1, .y = y/**/ },\
	{ .x = x/**/, .y = y - 1 },\
	{ .x = x/**/, .y = y + 1 }

#define MATRIX_DIST_N 8
#define MATRIX_DIST(x,y) MATRIX_NEIR(x,y),\
	{ .x = x - 1, .y = y + 1 },\
	{ .x = x + 1, .y = y - 1 },\
	{ .x = x - 1, .y = y - 1 },\
	{ .x = x + 1, .y = y + 1 }

static inline pos_t new_rand_pos(desk_t *brd, int free_n)
{
	int x, y, i = rand() % free_n;

	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < BOARD_DESK_W; x++)
		{
			cell_t c = board_get_cell(brd, x, y);

			if (no_ball != (c & MSK_BALL))
				continue;
			if (i == 0)
				return (pos_t){ .x = x, .y = y };
			i--;
		}
	}
	return (pos_t){ .x = -1, .y = -1 };
}

static inline cell_t new_rand_cell(void)
{
	int rnd = rand(),
	    col = rnd % 100;
	if (col < BALL_BONUS_D) {
		return NEW_BONUS_BALL(rand());
	} else
		return NEW_COLOR_BALL(col); // (rnd % (ball_max - 1)) + 1;
}

static inline cell_t get_mpath_num(move_t *mov, int x, int y, int id)
{
	if (IS_OUT_DESK(x, y) || mov->cuids[x][y] != id)
		return 0;
	return mov->matrix[x][y];
}

static void normalize_move_matrix(move_t *mov, int x, int y, int from_x, int from_y, int id)
{
	int i, lw = -1, n = mov->matrix[x][y];

	mov->matrix[x][y] |= FL_PATH;

	do {
		const pos_t p[] = { MATRIX_NEIR(x,y) };

		int ways[MATRIX_NEIR_N], dx = from_x - x,
		    nums[MATRIX_NEIR_N], dy = from_y - y;

		for (i = 0; i < MATRIX_NEIR_N; i++)
			nums[i] = get_mpath_num(mov, p[i].x, p[i].y, id);

		if (abs(dx) > abs(dy)) {
			ways[0] = dx > 0 ? 0 : 2;
			ways[1] = dy > 0 ? 1 : 3;
			ways[2] = dx > 0 ? 2 : 0;
			ways[3] = dy > 0 ? 3 : 1;
		} else {
			ways[0] = dy > 0 ? 1 : 3;
			ways[1] = dx > 0 ? 0 : 2;
			ways[2] = dy > 0 ? 3 : 1;
			ways[3] = dx > 0 ? 2 : 0;
		}

		if (lw == -1 || nums[lw] != n - 1) {
			for (i = 0; i < MATRIX_NEIR_N; i++) {
				if (nums[ways[i]] == n - 1) {
					lw = ways[i];
					break;
				}
			}
		}
		n = nums[lw];
		x = p[lw].x;
		y = p[lw].y;

		mov->matrix[x][y] |= FL_PATH;

	} while (n != 1);

# ifdef DEBUG /* =======> */
	fprintf(stdout, "\n=>\n");
	for (y = 0; y < BOARD_DESK_H; y ++) {
		for (x = 0; x < BOARD_DESK_W; x ++) {
			if (mov->matrix[x][y] & FL_PATH)
				fprintf(stdout, " %02d :", mov->matrix[x][y] & MSK_NUM);
			else	
				fprintf(stdout, "    :");
		}
		fprintf(stdout, "\n---------------------------------------------\n");
	}
# endif /* <======= */
}

static inline int act_mark_cell(desk_t *brd, move_t *mov, int x, int y, cell_t pN, int id)
{
	if (IS_OUT_DESK(x,y))
		return 0;

	ball_t b = board_get_cell(brd, x, y) & MSK_BALL;
	cell_t m = mov->matrix[x][y];

	cell_t iN = pN + 1;

	if (b == no_ball && !(m & FL_PATH) && (!m || m > iN)) {
		mov->cuids [x][y] = id;
		mov->matrix[x][y] = iN;
	} else
		iN = 0;
	return iN;
}

static int act_boom_ball(desk_t *brd, int x, int y)
{
	const pos_t p[] = { MATRIX_DIST(x,y) };

	int i, n = 0;

	for (i = 0; i < MATRIX_DIST_N; i++)
	{
		cell_t c = board_get_ball(brd, p[i].x, p[i].y);

		if (c == no_ball)
			continue;
		if (c == ball_bomb1) {
			n += act_boom_ball(brd, p[i].x, p[i].y);
		} else {
			board_set_cell(brd, p[i].x, p[i].y, no_ball);
			brd->delta += c != ball_brush;
			n++;
		}
	}
	board_set_cell(brd, x, y, no_ball);

	return n + 1;
}

static int act_paint_ball(desk_t *brd, int x, int y)
{
	const pos_t p[] = { MATRIX_DIST(x,y) };
	const cell_t nc = NEW_COLOR_BALL(rand());

	int i, n = 0;

	for (i = 0; i < MATRIX_DIST_N; i++)
	{
		ball_t c = board_get_ball(brd, p[i].x, p[i].y);

		if (IS_BALL_COLOR(c)) {
			board_set_cell(brd, p[i].x, p[i].y, nc);
			n++;
		}
	}
	board_set_cell(brd, x, y, no_ball);

	return 1;
}

static bool act_move_ball(desk_t *brd, move_t *mov)
{
	int fX = mov->from.x, tX = mov->to.x,
	    fY = mov->from.y, tY = mov->to.y;

	int x, y, i, mk, id = fX + fY * BOARD_DESK_W;

	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < BOARD_DESK_W; x++) {
			if (!(mov->matrix[x][y] & FL_PATH)) {
				mov->matrix[x][y] = 0;
				mov->cuids [x][y] = (cell_t)-1;
			}
		}
	}
	if (mov->matrix[fX][fY])
		return false;

	mov->matrix[fX][fY] =  1;
	mov->cuids [fX][fY] = id;

	do {
		for (mk = y = 0; y < BOARD_DESK_H; y++) {
			for (x = 0; x < BOARD_DESK_W; x++) {

				cell_t m  = mov->matrix[x][y];
				pos_t p[] = { MATRIX_NEIR(x,y) };

				if (!m || (m & FL_PATH))
					continue;
				for (i = 0; i < MATRIX_NEIR_N; i++)
					mk += act_mark_cell(brd, mov, p[i].x, p[i].y, m, id);
			}
		}
	} while (mk != 0);

	cell_t tm = mov->matrix[tX][tY];

	if (!(tm & FL_PATH) && tm) {
		cell_t b = board_get_cell(brd, fX, fY);
	//	cell_t a = board_get_cell(brd, tX, tY);

		board_set_cell(brd, fX, fY, no_ball);
		board_set_cell(brd, tX, tY, b);

		normalize_move_matrix(mov, tX, tY, fX, fY, id);
		return true;
	}
	return false;
}

static int check_bombs(desk_t *brd, int x, int y)
{
	ball_t b = board_get_ball(brd, x, y);
	int n = 0;

	/*  */ if (b == ball_bomb1) {
		n += act_boom_ball(brd, x, y);
	} else if (b == ball_brush) {
		n += act_paint_ball(brd, x, y);
	}
	return n;
}

static int check_flushes(desk_t *brd)
{
	int x, y, n = flush_nr;
	
	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < BOARD_DESK_W;)
		     x = scan_Hline(brd, x, y);
	}
	for (x = 0; x < BOARD_DESK_W; x++) {
		for (y = 0; y < BOARD_DESK_H;)
		     y = scan_Vline(brd, x, y);
	}
	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < y;)
		     x = scan_Aline(brd, x, y - x);
	}
	for (y = 1; y < BOARD_DESK_W; y++) {
		for (x = y; x < BOARD_DESK_W;)
		     x = scan_Aline(brd, x, BOARD_DESK_H - 1 - (x - y));
	}
	for (y = 0; y < BOARD_DESK_W; y++) {
		for (x = y; x < BOARD_DESK_W;)
		     x = scan_Bline(brd, x, x - y);
	}
	for (y = 1; y < BOARD_DESK_H; y++) {
		for (x = 0; x < (BOARD_DESK_W-y);)
		     x = scan_Bline(brd, x, y + x);
	}
	return flush_nr - n;
}

void board_init_desk(desk_t *brd)
{
	int x, y, i;

	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < BOARD_DESK_W; x++) {
			board_set_cell(brd, x, y, no_ball);
		}
	}
	for (i = 0; i < BOARD_POOL_N; i++)
		board_set_pool(brd, i, no_ball);
	board_set_score( brd, 0 );
	board_set_delta( brd, 0 );
	board_set_time ( brd, 0 );
	board_set_dmul ( brd, 1 );
}

void board_init_move(move_t *mov, int nb)
{
	int x, y;

	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < BOARD_DESK_W; x++) {
			mov->matrix[x][y] = mov->cuids[x][y] = 0;
		}
	}
	mov->from.x  = mov->to.x = -1;
	mov->from.y  = mov->to.y = -1;
	mov->flush_n = mov->pool_i = mov->flags = 0;

	if (BOARD_DESK_N <= nb) {
		mov->free_n = 0;
		mov->state  = ST_End;
	} else {
		mov->free_n = BOARD_DESK_N - nb;
		mov->state  = ST_Idle;
	}
}

void board_fill_pool(desk_t *brd)
{
	for (int i = 0; i < BOARD_POOL_N; i++) {
		board_set_pool(brd, i, new_rand_cell());
	}
}

void board_fill_desk(desk_t *brd, int pool_i, int free_n)
{
	cell_t c = board_get_pool(brd, pool_i);
	 pos_t p = new_rand_pos(brd, free_n);

	if (!IS_OUT_DESK(p.x, p.y)) {
		board_set_cell(brd, p.x, p.y, c);
	}
}

void board_select_ball(desk_t *brd, move_t *mov, int x, int y)
{
	if (IS_OUT_DESK(x, y))
		return;

	ball_t tB = board_get_cell(brd, x, y) & MSK_BALL,
	       fB = board_get_ball(brd, mov->from.x, mov->from.y);

	if (tB != no_ball) {
		mov->from.x = x; // mov->to.x = -1;
		mov->from.y = y; // mov->to.y = -1;
	} else
	if (fB != no_ball && mov->state == ST_Idle) {
		mov->to.x = x;
		mov->to.y = y;
		mov->state = ST_Moving;
	}
}

bool board_follow_path(move_t *mov, int x, int y, int *ox, int *oy, int id)
{
	cell_t gM = mov->matrix[x][y],
	       gN = gM & MSK_NUM;

	if (!(gM & FL_PATH))
		return false;

	const pos_t p[] = { MATRIX_NEIR(x,y) };

	for (int i = 0; i < MATRIX_NEIR_N; i++)
	{
		cell_t m = get_mpath_num(mov, p[i].x, p[i].y, id),
		       n = m & MSK_NUM;

		if ((m & FL_PATH) && (n == gN + 1)) {
			*ox = p[i].x;
			*oy = p[i].y;
			return true;
		}
	}
	//fprintf(stderr,"No move %d:%d %d\n", id, x , y);
	*ox = x;
	*oy = y;
	return false;
}

char board_next_move(desk_t *brd, move_t *mov)
{
	unsigned char  flags  = mov->flags , state  = mov->state;
	unsigned short free_n = mov->free_n, pool_i = mov->pool_i;

	int rN = 0;

	switch(state) {
	case ST_Idle:
		state = (free_n == BOARD_DESK_N ? ST_FillDesk :
		         free_n == 0 /*------*/ ? ST_End : ST_Idle);
		break;
	case ST_Moving:
		if (act_move_ball(brd, mov)) {
			state  = ST_Check;
			flags |= FL_DELAY;
			mov->from.x = -1;
			mov->from.y = -1;
		} else
			state = ST_Idle;
		break;
	case ST_FillPool:
		board_fill_pool(brd);
		state = (free_n == BOARD_DESK_N ? ST_FillDesk : ST_Idle);
		pool_i = 0;
		break;
	case ST_FillDesk:
		if (free_n) {
			board_fill_desk(brd, pool_i, free_n);
			flags &= ~FL_DELAY, free_n--;
			state  =  ST_Check, pool_i++;
		} else
			state = ST_End;
		break;
	case ST_Check:
		brd->delta = 0;
		if (flags & FL_DELAY) {
			rN = check_bombs(brd, mov->to.x, mov->to.y);
			free_n += rN;
		}
		if (rN || check_flushes(brd))
			state = ST_Remove;
		else if (flags & FL_DELAY)
			state = ST_FillDesk, brd->dmul = 1;
		else if (pool_i < BOARD_POOL_N)
			state = ST_FillDesk;
		else
			state = ST_FillPool;
		mov->to.x = -1;
		mov->to.y = -1;
		break;
	case ST_Remove:
		rN = flushes_remove(brd);
		brd->score += brd->delta * brd->dmul;
		if (rN)
			free_n += rN, brd->dmul++;
		if (flags & FL_DELAY)
			state = ST_Idle;
		else if (pool_i < BOARD_POOL_N)
			state = ST_FillDesk;
		else
			state = ST_FillPool;
		break;
	case ST_End:
	default: /* Game Over */
	}
	mov->state = state, mov->free_n = free_n;
	mov->flags = flags, mov->pool_i = pool_i;

	return state;
}
