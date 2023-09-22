
#include "board.h"

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

#define SCAN_H_LINE(brd,mov, x,y) scanline_by_matrix(brd,mov, x,y, 1, 0)
#define SCAN_V_LINE(brd,mov, x,y) scanline_by_matrix(brd,mov, x,y, 0, 1)
#define SCAN_A_LINE(brd,mov, x,y) scanline_by_matrix(brd,mov, x,y, 1,-1)
#define SCAN_B_LINE(brd,mov, x,y) scanline_by_matrix(brd,mov, x,y, 1, 1)

#define IS_SCAN_CONTINUE(x,y) (x < BOARD_DESK_W && y >= 0 && y < BOARD_DESK_H)

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
		return NEW_COLOR_BALL(col);
}

static inline int get_mpath_num(move_t *mov, int x, int y, muid_t id)
{
	if (IS_OUT_DESK(x, y) || board_get_mpid(mov, x, y) != id)
		return 0;
	return board_get_mnum(mov, x, y);
}

/** scaning line by giving offset
 ** ` — `  (1,  0)
 ** ` | `  (0,  1)
 ** ` \ `  (1,  1)
 ** ` / `  (1, -1)
 **/
static inline int scanline_by_matrix(desk_t *brd, move_t *mov, int sx, int sy, const int x1, const int y1)
{
	ball_t c, p;

	int ex, sj = 0, fc = 0,
	    ey, ej = 0;
	do { 
		p = board_get_cell(brd, sx, sy) & MSK_BALL;
		if (IS_BALL_COLOR(p))
			break; // find the first colored ball
		if (IS_BALL_JOKER(p))
			fc += (p == ball_flush),
			sj++; // one by one jokers counter
		else
			sj = fc = 0;
		sx += x1,
		sy += y1;
	} while (IS_SCAN_CONTINUE(sx,sy));

	// set start (x,y) pos before jokers
	// set end (x,y) pos to next cell, after color
	ex = sx + x1, sx = sx - (sj * x1),
	ey = sy + y1, sy = sy - (sj * y1);

	while (IS_SCAN_CONTINUE(ex,ey)) { 
		c = board_get_cell(brd, ex, ey) & MSK_BALL;
		if (IS_BALL_JOKER(c)) {
			fc += (c == ball_flush),
			ej++, c = p;
		} else {
			if (c != p)
				break;
			ej = 0;
		}
		ex += x1, // joining next
		ey += y1;
	}
	// vertical scanning require "y" coordinate diffs
	// all others checks the "x" coordinate diffs
	int diff  = x1 ? ex - sx : ey - sy;
	if (diff >= BALL_COLOR_D) {
		int x = sx, i,
		    y = sy, n;
		for(i = 0; i < diff; x += x1, y += y1, i++) {
			n = board_get_muid(mov, x, y) & FL_mBALL ?
			    board_get_mnum(mov, x, y) : 0;
			    board_set_muid(mov, x, y, FL_mBALL|FL_mPATH, (n + 1));
		}
		if (fc != 0)
			board_add_flush(mov, p);
		mov->flush_n++;
	} else {
		ex -= (ej * x1),
		ey -= (ej * y1);
	}
#ifdef DEBUG
	if (ej || sj)
		printf("~=/ Joker (x:%d y:%d) begin: %d end: %d /=~\n", sx, sy, sj, ej);
#endif
	// vertical scan returns next "y", all others returns next "x"
	return x1 ? (BOARD_DESK_W - ex) < BALL_COLOR_D ? BOARD_DESK_W : ex :
	            (BOARD_DESK_H - ey) < BALL_COLOR_D ? BOARD_DESK_H : ey ;
}

static void normalize_move_matrix(move_t *mov, int x, int y, int from_x, int from_y, muid_t id)
{
	int i, n, lw = -1;

	n = board_get_mnum(mov, x, y);
	    board_add_mpath(mov,x, y);

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

		board_add_mpath(mov, x, y);

	} while (n != 1);

# ifdef DEBUG /* =======> */
	printf("\nFollow Path >>\n");
	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < BOARD_DESK_W; x++) {
			if (board_has_mpath(mov, x, y))
				printf(" %02d :", board_get_mnum(mov, x, y));
			else
				printf("    :");
		}
		printf("\n---------------------------------------------\n");
	}
# endif /* <======= */
}

static inline int act_mark_cell(desk_t *brd, move_t *mov, int x, int y, int pN, muid_t id)
{
	if (IS_OUT_DESK(x,y))
		return 0;

	ball_t b = board_get_cell(brd, x, y) & MSK_BALL;
	int    n = board_get_mnum(mov, x, y);
	bool  hp = board_has_mpath(mov, x, y);

	if (b == no_ball && !hp && (!n || (n > pN + 1))) {
		board_set_muid(mov, x, y, id, (n = pN + 1));
	} else
		n = 0;
	return n;
}

// returns the number of destroyed balls
static int act_boom_ball(desk_t *brd, move_t *mov, int x, int y)
{
	const pos_t p[] = { MATRIX_DIST(x,y) };

	int i, n = 0;

	for (i = 0; i < MATRIX_DIST_N; i++)
	{
		ball_t c = board_get_ball(brd, p[i].x, p[i].y);

		if (c == no_ball || board_has_mball(mov, p[i].x, p[i].y))
			continue;
		if (c == ball_bomb1) {
			n += act_boom_ball(brd, mov, p[i].x, p[i].y);
		} else {
			board_set_muid(mov, p[i].x, p[i].y, FL_mBALL|FL_mPATH, (c != ball_brush));
			n++;
		}
	}
	board_set_muid(mov, x, y, FL_mBALL|FL_mPATH, 0);

	return n + 1;
}

// returns the number of repaint balls
static int act_paint_ball(desk_t *brd, move_t *mov, int x, int y)
{
	const pos_t p[] = { MATRIX_DIST(x,y) };
	const cell_t nc = NEW_COLOR_BALL(rand());

	int i, pc = 0;

	for (i = 0; i < MATRIX_DIST_N; i++)
	{
		ball_t c = board_get_ball(brd, p[i].x, p[i].y);

		if (IS_BALL_COLOR(c)) {
			board_set_cell(brd, p[i].x, p[i].y, nc);
			pc++;
		}
	}
	board_set_cell(brd, x, y, no_ball);
	//board_set_muid(mov, x, y, FL_mBALL|FL_mPATH, 0);

	return pc;
}

static bool act_move_ball(desk_t *brd, move_t *mov)
{
	int fX = mov->from.x, tX = mov->to.x,
	    fY = mov->from.y, tY = mov->to.y;

	muid_t id;
	int x, y, i, mk;

	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < BOARD_DESK_W; x++) {
			if(!board_has_mpath(mov, x, y)) {
				board_set_muid(mov, x, y, MSK_mPID, 0);
			}
		}
	}
	if (board_get_mnum(mov, fX, fY))
		return false;

	id = NEW_MPATH_ID(fX, fY),
	board_set_muid(mov, fX, fY, id, 1);

	do {
		for (mk = y = 0; y < BOARD_DESK_H; y++) {
			for (x = 0; x < BOARD_DESK_W; x++) {

				int n = board_get_mnum(mov, x, y);
				pos_t p[] = { MATRIX_NEIR(x,y) };

				if (n == 0 || board_has_mpath(mov, x, y))
					continue;
				for (i = 0; i < MATRIX_NEIR_N; i++)
					mk += act_mark_cell(brd, mov, p[i].x, p[i].y, n, id);
			}
		}
	} while (mk != 0);

	if (!board_has_mpath(mov, tX, tY) && board_get_mnum(mov, tX, tY)) {
		cell_t b = board_get_cell(brd, fX, fY);
	//	cell_t a = board_get_cell(brd, tX, tY);

		board_set_cell(brd, fX, fY, no_ball);
		board_set_cell(brd, tX, tY, b);

		normalize_move_matrix(mov, tX, tY, fX, fY, id);
		return true;
	}
	return false;
}

static int act_scan_flushes(desk_t *brd, move_t *mov)
{
	int x, y, n = mov->flush_n;
	
	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < BOARD_DESK_W;)
		     x = SCAN_H_LINE(brd, mov, x, y);
	}
	for (x = 0; x < BOARD_DESK_W; x++) {
		for (y = 0; y < BOARD_DESK_H;)
		     y = SCAN_V_LINE(brd, mov, x, y);
	}
	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < y;)
		     x = SCAN_A_LINE(brd, mov, x, y - x);
	}
	for (y = 1; y < BOARD_DESK_W; y++) {
		for (x = y; x < BOARD_DESK_W;)
		     x = SCAN_A_LINE(brd, mov, x, BOARD_DESK_H - 1 - (x - y));
	}
	for (y = 0; y < BOARD_DESK_W; y++) {
		for (x = y; x < BOARD_DESK_W;)
		     x = SCAN_B_LINE(brd, mov, x, x - y);
	}
	for (y = 1; y < BOARD_DESK_H; y++) {
		for (x = 0; x < (BOARD_DESK_W-y);)
		     x = SCAN_B_LINE(brd, mov, x, y + x);
	}
	return mov->flush_n - n;
}

static int act_remove_balls(desk_t *brd, move_t *mov) {

	int x, r = 0, d = board_get_delta(brd),
	    y, k = 0, s = board_get_score(brd);

	print_dbg("\nFlushing Balls >>\n");

	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < BOARD_DESK_W; x++)
		{
			ball_t b = board_get_cell(brd, x, y) & MSK_BALL;
			int    n = board_get_mnum(mov, x, y);

			if (board_has_mball(mov, x, y)) {
				if (IS_BALL_COLOR(b) || IS_BALL_JOKER(b))
					k += n, r++;
				if (b == ball_joker)
					d *= 2;
				board_set_cell(brd, x, y, no_ball);
				print_dbg(" (%d) :", n);
			} else if (IS_BALL_COLOR(b) && board_has_flush(mov, b)) {
				k++, r++;
				board_set_cell(brd, x, y, no_ball);
				print_dbg(" (1) :");
			} else {
				print_dbg("     :");
			}
			board_set_muid(mov, x, y, 0,0);
		}
		print_dbg("\n------------------------------------------------------\n");
	}
	board_set_score(brd, (k * d) + s);
	board_set_delta(brd, (r > 0) + d);
	board_del_flush(mov);
	return r;
}

static bool act_check_balls(desk_t *brd, move_t *mov)
{
	int x = mov->to.x, rc = 0,
	    y = mov->to.y;

	ball_t b = board_get_ball(brd, x, y);
	bool full_scan = true;

	if (b == ball_bomb1) {
		full_scan = false;
		rc += act_boom_ball(brd, mov, x, y);
	} else if (b == ball_brush) {
		full_scan = act_paint_ball(brd, mov, x, y);
		rc++;
	}
	if (full_scan)
		rc += act_scan_flushes(brd, mov);
	return rc != 0;
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
	board_set_delta( brd, 1 );
	board_set_time ( brd, 0 );
	board_set_user ( brd, 0 );
}

void board_init_move(move_t *mov, int nb)
{
	int x, y;

	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < BOARD_DESK_W; x++) {
			board_set_muid(mov, x, y, 0, 0);
		}
	}
	mov->from.x  = mov->to.x = -1;
	mov->from.y  = mov->to.y = -1;
	mov->flush_c = mov->flags = 0;
	mov->flush_n = mov->pool_i = 0;

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
	int i, n, gN = board_get_mnum(mov, x, y);

	if (!board_has_mpath(mov, x, y))
		return false;

	const pos_t p[] = { MATRIX_NEIR(x,y) };

	for (i = 0; i < MATRIX_NEIR_N; i++)
	{
		n = get_mpath_num(mov, p[i].x, p[i].y, id);

		if (board_has_mpath(mov, p[i].x, p[i].y) && (n == gN + 1)) {
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

	switch(state) {
	case ST_Idle:
		state = (free_n == BOARD_DESK_N ? ST_FillDesk :
		         free_n == 0 /*------*/ ? ST_End : ST_Idle);
		break;
	case ST_Moving:
		if (act_move_ball(brd, mov)) {
			state  = ST_Check;
			flags |= FL_QUEUE;
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
			flags &= ~FL_QUEUE, free_n--;
			state  =  ST_Check, pool_i++;
		} else
			state = ST_End;
		break;
	case ST_Check:
		if (act_check_balls(brd, mov))
			state = ST_Remove;
		else if (flags & FL_QUEUE)
			board_set_delta(brd, 1),
			state = ST_FillDesk;
		else if (pool_i < BOARD_POOL_N)
			state = ST_FillDesk;
		else
			state = ST_FillPool;
		mov->to.x = -1;
		mov->to.y = -1;
		break;
	case ST_Remove:
		/**/free_n += act_remove_balls(brd, mov);
		if (flags & FL_QUEUE)
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

#ifdef DEBUG
void board_dbg_desk(desk_t *brd)
{
	int x,y,b[] = {
		0,2,0,0,0,0,0,0,0,
		3,2,8,9,5,5,0,5,0,
		0,0,0,0,0,0,0,0,0,
		0,2,3,0,0,9,0,0,0,
		0,8,0,3,4,0,0,0,0,
		0,2,0,4,3,0,2,0,0,
		0,0,4,0,0,0,0,0,0,
		0,0,0,0,3,0,0,0,0,
		8,8,8,8,8,8,8,8,0
	};
	for (y = 0; y < BOARD_DESK_H; y++)
		for (x = 0; x < BOARD_DESK_W; x++)
			board_set_cell(brd, x, y, b[(x + y * BOARD_DESK_W)]);
}
#endif
