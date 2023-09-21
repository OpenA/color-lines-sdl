#include "board.h"

cell_t	move_matrix_ids[BOARD_W][BOARD_H];
cell_t	move_matrix[BOARD_W][BOARD_H];

int	ball_x = -1, ball_to_x;
int	ball_y = -1, ball_to_y;

static struct __SESS__ {
	cell_t	desk[BOARD_W][BOARD_H];
	cell_t	ball_pool[POOL_SIZE];
	unsigned int score;
	unsigned int score_mul;
	unsigned int score_delta;
	unsigned int free_cells;
	unsigned int iball;
	int last_time;
} Session;

static unsigned int flush_nr;
static step_t board_stat;

typedef struct {
	int x;
	int y;
	int nr;
	int bomb;
	cell_t col;
	struct {
		int x;
		int y;
	} cells[_max(BOARD_W,BOARD_H)];
} flush_t;

flush_t	*flushes[BOARD_W * BOARD_H];

#ifndef _BOARD_H_
static cell_t cell_get(int x, int y)
{
	if (x < 0 || x >= BOARD_W)
		return 0;
	if (y < 0 || y >= BOARD_H)
		return 0;
	return Session.desk[x][y];
}

bool board_selected(int *x, int *y)
{
	bool selected = !!cell_get(ball_x, ball_y);
	//fprintf(stderr,"Board selected: %d %d\n", ball_x, ball_y);
	if ( selected ) {
		if (x)
			*x = ball_x;
		if (y)
			*y = ball_y;
	}
	return selected;
}

bool board_moved(int *x, int *y)
{
	bool moved = ball_to_x > -1 && ball_to_y > -1 && board_stat == ST_CHECK;
	//fprintf(stderr,"Ball moved: %d %d\n", ball_to_x, ball_to_y);
	if ( moved ) {
		if (x)
			*x = ball_to_x;
		if (y)
			*y = ball_to_y;
	}
	return moved;
}

void board_init(void)
{
	Session.score      = flush_nr = 0;
	Session.score_mul  = 1;
	Session.free_cells = BOARD_W * BOARD_H;

	memset(Session.desk,      0, sizeof(Session.desk));
	memset(Session.ball_pool, 0, sizeof(Session.ball_pool));
	memset(flushes,           0, sizeof(flushes));
	memset(move_matrix,       0, sizeof(move_matrix));
	memset(move_matrix_ids,   0, sizeof(move_matrix));

	ball_x = ball_to_x = -1;
	ball_y = ball_to_y = -1;
	
	board_fill_pool();
	board_fill(NULL, NULL);
	board_fill(NULL, NULL);
	board_fill(NULL, NULL);
	board_fill_pool();
	
	board_stat = ST_IDLE;
}

cell_t board_cell(int x, int y)
{
	cell_t cell;
	cell = cell_get(x, y);
	return cell;
}

cell_t pool_cell(int x)
{
	cell_t cell;
	cell = x < Session.iball || x >= POOL_SIZE ? 0 : Session.ball_pool[x];
	return cell;
}

static cell_t *cell_ref(int x, int y)
{
	if ((x < 0) || (x >= BOARD_W))
		return NULL;
	if ((y < 0) || (y >= BOARD_H))
		return NULL;
	return &Session.desk[x][y];
}

static int mark_cell(int x, int y, cell_t n, int id)
{
	cell_t new;
	
	if (!n)
		return 0;
	if (x < 0 || x >= BOARD_W)
		return 0;
		
	if (y < 0 || y >= BOARD_H)
		return 0;
		
	if (cell_get(x,y))
		return 0;
		
	if (move_matrix[x][y] & FL_PATH)
		return 0;
		
	new = n + 1;
	if (!move_matrix[x][y] || move_matrix[x][y] > new) {
		move_matrix_ids[x][y] = id;
		move_matrix[x][y] = new;
	}
	else
		return 0;
	return new;
}

static int move_cell(int x, int y, int n, int id)
{
	if (x < 0 || x >= BOARD_W)
		return 0;
	if (y < 0 || y >= BOARD_H)
		return 0;
	if (move_matrix_ids[x][y] != id)
		return 0;
	return move_matrix[x][y];
}

typedef struct {
	int x;
	int y;
	int n;
} way_t;

static void normalize_move_matrix(int x, int y, int tox, int toy, int id)
{
	int dx, dy, i;
	int last_move = -1;
	int num = move_matrix[x][y];
	move_matrix[x][y] |= FL_PATH;
	
	do {
		way_t w[4];
		
		int ways[4] = { 0, 1, 2, 3 };
		
		w[0].x = x + 1; w[1].x = x; w[0].y = y; w[1].y = y + 1; 
		w[2].x = x - 1; w[3].x = x; w[2].y = y; w[3].y = y - 1;
		
		w[0].n = move_cell(x + 1, y, num, id);
		w[1].n = move_cell(x, y + 1, num, id);
		w[2].n = move_cell(x - 1, y, num, id);
		w[3].n = move_cell(x, y - 1, num, id);
		
		dx = tox - x;
		dy = toy - y;
		
		if (abs(dx) > abs(dy)) {
			if (dy > 0) {
				ways[0] = 1;
				ways[1] = 3;
			} else {
				ways[0] = 3;
				ways[1] = 1;
			}
			if (dx > 0) {
				ways[2] = 0;
				ways[3] = 2;
			} else {
				ways[2] = 2;
				ways[3] = 0;
			}	
		} else {
			if (dx > 0) {
				ways[0] = 0;
				ways[1] = 2;
			} else {
				ways[0] = 2;
				ways[1] = 0;
			}
			if (dy > 0) {
				ways[2] = 1;
				ways[3] = 3;
			} else {
				ways[2] = 3;
				ways[3] = 1;
			}
		}
		
		if (last_move == -1 || w[last_move].n != num - 1) {
			for (i = 0; i < 4; i++) {
				if (w[ways[i]].n == num - 1) {
					last_move = ways[i];
					break;
				}
			}
		}
		
		num = w[last_move].n;
		x   = w[last_move].x;
		y   = w[last_move].y;
		
		move_matrix[x][y] |= FL_PATH;
		
	} while (num != 1);
/*
	fprintf(stdout,"XXX\n");
	for (y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x ++) {
			if (move_matrix[x][y] & FL_PATH)
				fprintf(stdout, "%02d", move_matrix[x][y] & ~FL_PATH);
			else	
				fprintf(stdout, "..");
		}
		fprintf(stdout,"\n");
	}
*/
}

static bool _board_path(int x, int y)
{
	if (x < 0 || x >= BOARD_W)
		return false;
	if (y < 0 || y >= BOARD_H)
		return false;
	if (move_matrix[x][y] & FL_PATH) {
//		fprintf(stderr,"Stop\n");
		return true;
	}
	return false;
}

bool board_path(int x, int y)
{
	bool rc;
	rc = _board_path(x, y);
	return rc;
}

void board_clear_path(int x, int y)
{
	move_matrix[x][y] = move_matrix[x][y] & ~FL_PATH;
}

bool board_follow_path(int x, int y, int *ox, int *oy, int id)
{
	int num;
	int a,v;
	num  = move_matrix[x][y];
	if (!(num & FL_PATH)) {
		return false;
	}
	num &= ~FL_PATH;
	
	a = move_cell(x + 1, y, num, id);
	v = a & ~FL_PATH;
	if ((a & FL_PATH) && (v == num + 1)) {
		*ox = x + 1; *oy = y;
		return true;
	}
	a = move_cell(x, y + 1, num, id);
	v = a & ~FL_PATH;
	if ((a & FL_PATH) && (v == num + 1)) {
		*ox = x; *oy = y + 1;
		return true;
	}
	a = move_cell(x -1 , y, num, id);
	v = a & ~FL_PATH;
	if ((a & FL_PATH) && (v == num + 1)) {
		*ox = x - 1; *oy = y;
		return true;
	}
	a = move_cell(x, y - 1, num, id);
	v = a & ~FL_PATH;
	if ((a & FL_PATH) && (v == num + 1)) {
		*ox = x; *oy = y - 1;
		return true;
	}
//	fprintf(stderr,"No move %d:%d %d\n", id, x , y);
	*ox = x;
	*oy = y;
	return false;
}

static bool board_move(int x1, int y1, int x2, int y2)
{
	int x, y, mark = 1;
	int id = x1 + y1 * BOARD_W;
	for (y = 0; y < BOARD_H; y++) {
		for (x = 0; x < BOARD_W; x++) {
			if (!(move_matrix[x][y] & FL_PATH)) {
				move_matrix[x][y] = 0;
				move_matrix_ids[x][y] = -1;
			}
		}
	}
//	memset(move_matrix, 0, sizeof(move_matrix));
	if (move_matrix[x1][y1])
		return false;
//	fprintf(stderr,"%d %d	-> %d %d\n", x1, y1, x2, y2);
	move_matrix[x1][y1] = 1;
	move_matrix_ids[x1][y1] = id;
	while (mark) {
		mark = 0;
		for (y = 0; y < BOARD_H; y++) {
			for (x = 0; x < BOARD_W; x++) {
				if (!move_matrix[x][y] || _board_path(x, y))
					continue;
				mark |= mark_cell(x - 1, y, move_matrix[x][y], id);
				mark |= mark_cell(x + 1, y, move_matrix[x][y], id);
				mark |= mark_cell(x, y - 1, move_matrix[x][y], id);
				mark |= mark_cell(x, y + 1, move_matrix[x][y], id);
			}
		}
	}
	
//	fprintf(stdout,"XXX\n");
//	for (y = 0; y < BOARD_H; y ++) {
//		for (x = 0; x < BOARD_W; x ++) {
//			if (move_matrix[x][y] & FL_PATH)
//				fprintf(stdout, "XX");
//			else
//				fprintf(stdout, "%02d", move_matrix[x][y]);
//		}
//		fprintf(stdout,"\n");
//	}
	
	if (!_board_path(x2, y2) && move_matrix[x2][y2]) {
		cell_t b;
		cell_t *c;
		c = cell_ref(x1, y1);
		b = *c;
		*c = 0;
		c = cell_ref(x2, y2);
		*c = b;
		normalize_move_matrix(x2, y2, x1, y1, id);
		return true;
	}
	return false;
}

static cell_t *find_pos(int pos, int *ox, int *oy)
{
	int x, y;
	for (y = 0; y < BOARD_H; y++) {
		for (x = 0; x < BOARD_W; x++) {
			cell_t *cell = cell_ref(x, y);
			if (ox)
				*ox = x;
			if (oy)
				*oy = y;	
			if (*cell)
				continue;
			if (!pos)
				return cell;
			pos--;	
		}
	}
	return NULL;
}
#endif
static inline cell_t gen_rand_cell(void)
{
	int rnd = rand(),
	    col = rnd % 100;
	if (col < BONUS_PCNT) {
		return NEW_BONUS_BALL(rnd >> 18);
	} else
		return NEW_COLOR_BALL(col); // (rnd % (ball_max - 1)) + 1;
}

static cell_t joinable(cell_t c, cell_t *prev)
{
	if (!c || !(*prev))
		return 0;
	if (IS_BALL_JOKER(*prev)) /* set joker suggestion */
		*prev = c;
	if ( c == *prev )
		return c;
	if (IS_BALL_JOKER(c))
		return *prev;
	return 0;
//	return ((a == b) || (a == ball_joker) || (b == ball_joker));
}

static void flush_add(desk_t *brd, int x1, int y1, int x2, int y2, cell_t col)
{
	flush_t *f;
	f = malloc(sizeof(flush_t));
	if (!f)
		exit(1);
	if (x2 > x1)
		x2++;
	if (y2 > y1)
		y2++;
	if (y2 < y1)
		y2--;
	f->nr = 0;
//	f->score = 0;
	f->col = col;
	f->bomb = 0;
	do {
		if (board_get_ball(brd, x1, y1) == ball_flush)
			f->bomb++;
		f->cells[f->nr].x = x1;
		f->cells[f->nr].y = y1;
		f->nr++;
		if (x1 < x2)
			x1 ++;
		if (y1 < y2)
			y1 ++;
		else if (y1 > y2)
			y1 --;	
	//	f->score ++;
	} while ((x1 != x2) || (y1 != y2));
	flushes[flush_nr ++] = f;
}

static bool remove_cell(cell_t *c)
{
	bool rc = false;
	if (!*c)
		return rc;
	
	if (IS_BALL_COLOR(*c) || IS_BALL_JOKER(*c)) {
		Session.score_delta++;
		rc = true;
	}
	if (*c == ball_joker)
		Session.score_mul *= 2;
	*c = 0;
	Session.free_cells++;
	return rc;
}

static int remove_color(desk_t *brd, cell_t col)
{
	int x, y, n = 0;
	for (y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x ++) {
			cell_t c = board_get_cell(brd, x, y) & MSK_BALL;
			if (c == col) {
				brd->delta += (IS_BALL_COLOR(c) || IS_BALL_JOKER(c));
				brd->dmul  += (c == ball_joker ? brd->dmul : 0);
				board_set_cell(brd, x, y, no_ball);
				n++;
			}
		}
	}
	return n;
}

static int flushes_remove(desk_t *brd)
{
	int i, k, n = 0;
	for (i = 0; i < flush_nr; i++) {
		//cell_t *c;
		flush_t *f = flushes[i];
		for (k = 0; k < f->nr; k++) {
			if (f->bomb)
				n += remove_color(brd, f->col);

			int x = f->cells[k].x,
			    y = f->cells[k].y;
			
			cell_t c = board_get_cell(brd, x, y) & MSK_BALL;

			if (c != no_ball) {
				brd->delta += (IS_BALL_COLOR(c) || IS_BALL_JOKER(c));
				brd->dmul  += (c == ball_joker ? brd->dmul : 0);
				board_set_cell(brd, x, y, no_ball);
				n++;
			}
		}
		free(f);
	}
	flush_nr = 0;
	return n;
}

void _board_fill_pool(void)
{
	for (int i = 0; i < POOL_SIZE; i++) {
		Session.ball_pool[i] = gen_rand_cell();
	}
	Session.iball = 0;
}

/* scaning  —  line */
static int scan_Hline(desk_t *brd, int sx, int sy)
{
	ball_t c, p;

	int ex, sj = 0,
	    ey, ej = 0;
	do { 
		p = board_get_cell(brd, sx, sy) & MSK_BALL;
		if (IS_BALL_COLOR(p))
			break; // find the first colored ball
		if (IS_BALL_JOKER(p))
			sj++; // one by one jokers counter
		else
			sj = 0;
		sx++;
	} while (sx < BOARD_DESK_W);

	ex = sx + 1, // set end-x pos to next cell, after colored
	sx = sx - sj, // set start-x pos before jokers
	ey = sy;

	while (ex < BOARD_DESK_W) { 
		c = board_get_cell(brd, ex, ey) & MSK_BALL;
		if (IS_BALL_JOKER(c)) {
			ej++, c = p;
		} else {
			if (c != p)
				break;
			ej = 0;
		}
		ex++; // joining next
	}
	if ((ex - sx) >= BALLS_ROW) {
		flush_add(brd, sx, sy, ex - 1, ey, p);
	} else
		ex -= ej;
#ifdef DEBUG
	if (ej || sj)
		fprintf(stderr, "~=/ Joker (x:%d y:%d) begin: %d end: %d /=~\n", sx, sy, sj, ej);
#endif
	if ((BOARD_DESK_W - ex) < BALLS_ROW)
		return BOARD_DESK_W;
	else
		return ex;
}

/* scaning  |  line */
static int scan_Vline(desk_t *brd, int sx, int sy)
{
	ball_t c, p;

	int ex, sj = 0,
	    ey, ej = 0;
	do { 
		p = board_get_cell(brd, sx, sy) & MSK_BALL;
		if (IS_BALL_COLOR(p))
			break; // find the first colored ball
		if (IS_BALL_JOKER(p))
			sj++; // one by one jokers counter
		else
			sj = 0;
		sy++;
	} while (sy < BOARD_DESK_H);

	ey = sy + 1, // set end-y pos to next cell, after colored
	sy = sy - sj, // set start-y pos before jokers
	ex = sx;

	while (ey < BOARD_DESK_H) { 
		c = board_get_cell(brd, ex, ey) & MSK_BALL;
		if (IS_BALL_JOKER(c)) {
			ej++, c = p;
		} else {
			if (c != p)
				break;
			ej = 0;
		}
		ey++; // joining next
	}
	if ((ey - sy) >= BALLS_ROW) {
		flush_add(brd, sx, sy, ex, ey - 1, p);
	} else
		ey -= ej;
#ifdef DEBUG
	if (ej || sj)
		fprintf(stderr, "~=/ Joker (x:%d y:%d) begin: %d end: %d /=~\n", sx, sy, sj, ej);
#endif
	if ((BOARD_DESK_H - ey) < BALLS_ROW)
		return BOARD_DESK_H;
	else
		return ey;
}

static int scan_Aline(desk_t *brd, int x, int y) /* /  line */
{
	while (x < BOARD_W && y >= 0 && !board_get_ball(brd, x, y)) { // skip spaces
		y--; x++;
	}
//	if (y < BALLS_ROW)
//		return x + y;
//	if ((BOARD_W - x) < BALLS_ROW)	
//		return BOARD_W;
	
	int xi = x;
	int yi = y;
	cell_t b = board_get_ball(brd, x, y);
	
	while (xi < BOARD_W && yi >= 0 && joinable(board_get_ball(brd, xi, yi), &b)) {
		yi--; xi++;
	}
	if ((xi - x) >= BALLS_ROW) {
		flush_add(brd, x, y, xi - 1, yi + 1, b);
	}
	
	bool joker = IS_BALL_JOKER(b); /* all jokers */
	
	while (!joker && IS_BALL_JOKER(board_get_ball(brd, xi - 1, yi + 1))) { /* prepeare jokers for next try */
		yi++; xi--;
	}
	return xi;
}

static int scan_Bline(desk_t *brd, int x, int y) /* \  line */
{
	while (x < BOARD_W && y < BOARD_H && !board_get_ball(brd, x, y)) { // skip spaces
		y++; x++;
	}
//	if ((BOARD_H - y) < BALLS_ROW)
//		return x + BOARD_H - y;
//	if ((BOARD_W - x) < BALLS_ROW)
//		return BOARD_W;
	
	int xi = x;
	int yi = y;
	cell_t b = board_get_ball(brd, x, y);
	
	while (xi < BOARD_W && yi < BOARD_H && joinable(board_get_ball(brd, xi, yi), &b)) {
		yi++; xi++;
	}
	if ((xi - x) >= BALLS_ROW) {
		flush_add(brd, x, y, xi - 1, yi - 1, b);
	}
	
	bool joker = IS_BALL_JOKER(b); /* all jokers */
	
	while (!joker && IS_BALL_JOKER(board_get_ball(brd, xi - 1, yi - 1))) { /* prepeare jokers for next try */
		yi--; xi--;
	}
	return xi;
}

#ifndef _BOARD_H_

static int board_check_hlines(void)
{	
	int x, y;
	int of = flush_nr;
	
	for (y = 0; y < BOARD_H; y++ ) {
		for (x = 0; x < BOARD_W; ) {
			x = scan_hline(x, y);
		}
	}
	return flush_nr - of;
}

static int board_check_vlines(void)
{	
	int x, y;
	int of = flush_nr;
	
	for (x = 0; x < BOARD_W; x++ ) {
		for (y = 0; y < BOARD_H; ) {
			y = scan_vline(x, y);
		}
	}
	return flush_nr - of;
}

static int board_check_alines(void)
{	
	int x, y;
	int of = flush_nr;
	for (y = 0; y < BOARD_H; y++ ) {
		for (x = 0; x < y; ) {
			x = scan_aline(x, y - x);
		}
	}
	for (y = 1; y < BOARD_W; y++ ) {
		for (x = y; x < BOARD_W; ) {
			x = scan_aline(x, BOARD_H -1 - (x - y));
		}
	}
	return flush_nr - of;
}

static int board_check_blines(void)
{	
	int x, y;
	int of = flush_nr;
	for (y = 0; y < BOARD_W; y++ ) {
		for (x = y; x < BOARD_W; ) {
			x = scan_bline(x, x - y);
		}
	}
	for (y = 1; y < BOARD_H; y++ ) {
		for (x = 0; x < (BOARD_W - y); ) {
			x = scan_bline(x, y + x);
		}
	}
	return flush_nr - of;
}

static int board_boom(int x, int y);

static int boom_ball(int x, int y)
{
	int rc = 0;
	cell_t *c = cell_ref(x, y);
	if (!c)
		return 0;
	if (*c == ball_bomb1) {
		rc += board_boom(x, y);
//		Session.score_delta ++;
	} else if (*c) {
		rc += remove_cell(c);
	}
	return rc;
}

static int board_boom(int x, int y)
{
	int rc = 0;
	cell_t *c = cell_ref(x, y);
	if (c && *c == ball_bomb1) {
		*c = 0;
		Session.free_cells ++;
		c = cell_ref(x - 1, y);
		if (c && *c)
			rc += boom_ball(x - 1, y);
		c = cell_ref(x + 1, y);
		if (c && *c)
			rc += boom_ball(x + 1, y);
		c = cell_ref(x, y - 1);
		if (c && *c)
			rc += boom_ball(x, y - 1);
		c = cell_ref(x, y + 1);
		if (c && *c)
			rc += boom_ball(x, y + 1);
		c = cell_ref(x + 1, y - 1);
		if (c && *c)
			rc += boom_ball(x + 1, y - 1);
		c = cell_ref(x - 1, y - 1);
		if (c && *c)
			rc += boom_ball(x - 1, y - 1);
		c = cell_ref(x + 1, y + 1);
		if (c && *c)
			rc += boom_ball(x + 1, y + 1);
		c = cell_ref(x - 1, y + 1);
		if (c && *c)
			rc += boom_ball(x - 1, y + 1);
		return rc;	
	}
	return 0; 
}

static int board_paint(int x, int y)
{
	int rc = 0;
	
	cell_t col = NEW_COLOR_BALL(rand()); //TODO
	cell_t *c = cell_ref(x, y);
	if (c && *c == ball_brush) {
		*c = 0;
		Session.free_cells ++;
		c = cell_ref(x - 1, y);
		if (c && IS_BALL_COLOR(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x + 1, y);
		if (c && IS_BALL_COLOR(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x, y - 1);
		if (c && IS_BALL_COLOR(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x, y + 1);
		if (c && IS_BALL_COLOR(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x + 1, y - 1);
		if (c && IS_BALL_COLOR(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x - 1, y - 1);
		if (c && IS_BALL_COLOR(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x + 1, y + 1);
		if (c && IS_BALL_COLOR(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x - 1, y + 1);
		if (c && IS_BALL_COLOR(*c)) {
			*c = col;
			rc ++;
		}
		return rc;	
	}
	return 0; 
}
static int board_check(int x, int y)
{
	/* h line */
	int rc;
	rc = board_paint(x, y);
	rc += board_boom(x, y);
	return rc + board_check_hlines() + board_check_vlines() + board_check_alines() +
		board_check_blines();
}

bool board_fill(int *ox, int *oy)
{
	int x, y;
	int pos;
	cell_t *cell;

	if (Session.free_cells < 1) {
		return true;
	}
//	for (int i = 0; i < POOL_SIZE; i++) {
		pos = rand() % Session.free_cells;
		cell = find_pos(pos, &x, &y);
		if (!cell) {
			fprintf(stderr,"Something really bad 1\n");
			exit(1);
		}
		*cell = Session.ball_pool[Session.iball++];
		Session.free_cells--;
		if (ox)
			*ox = x;
		if (oy)
			*oy = y;	
//		board_check_row(x, y);
//		flushes_remove();
//	}
//	if (Session.iball == POOL_SIZE) {
//		Session.iball = 0;
//		return false;
//	}
	return false;
}

static void gfx_display_cell(int x, int y, cell_t cell)
{
	static char balls[] = ".01234567";
	if (y == 0 && x == 0)
		fprintf(stdout,"  012345678\n");
	if (x == 0)
		fprintf(stdout,"%d|", y);
	fprintf(stdout, "%c", balls[cell]);
	if (x == (BOARD_W - 1)) {
		fprintf(stdout,"\n");
	}
}

void board_display(void)
{
	int x, y;
	for (y = 0; y < BOARD_H; y++) {
		for (x = 0; x < BOARD_W; x++) {
			gfx_display_cell(x, y, cell_get(x, y));		
		}
	}
}

bool board_select(int x, int y)
{
	if (x == -1 && y == -1) { /* special case */
		ball_x = -1;
		ball_y = -1;
		return false;
	}
	if (x < 0 || x >= BOARD_W)
		return false;
		
	if (y < 0 || y >= BOARD_H)
		return false;
	
//	if (board_stat != ST_IDLE) {
//		return false;
//	}
	
	cell_t *c = cell_ref(x, y);
	
	if (*c) {
		ball_x = x;
		ball_y = y;
//		ball_to_x = -1;
//		ball_to_y = -1;
		return false;
	}
	
	if (!cell_get(ball_x, ball_y)) { /* nothing to move */
		return false;
	}

	if (board_stat != ST_IDLE) {
		return false;
	}

	ball_to_x = x;
	ball_to_y = y;
	board_stat = ST_MOVING;
	
//	if (!board_move(ball_x, ball_y, x, y)) {
//		return false;
//	}
//	board_check_row(x, y);
//	if (flushes_remove()) {
//		return true;
//	}
	return true;
}

step_t board_next_step()
{
	static bool rc;
	static int x, y;
	bool fl;
//	fprintf(stderr,"state=%d\n", board_stat);
	switch(board_stat) {
	case ST_IDLE:
		if (!Session.free_cells)
			board_stat = ST_END;
		else if (Session.free_cells == (BOARD_W * BOARD_H))
			board_stat = ST_FILL_BOARD;
		break;
	case ST_MOVING:
		if ((rc = board_move(ball_x, ball_y, ball_to_x, ball_to_y))) {
			board_stat = ST_CHECK;
//			x = ball_to_x;
//			y = ball_to_y;
			ball_x = -1;
			ball_y = -1;
		} else
			board_stat = ST_IDLE;
		break;
	case ST_FILL_POOL:
		_board_fill_pool();
		board_stat = (Session.free_cells == BOARD_W * BOARD_H) ? ST_FILL_BOARD : ST_IDLE;
		break;
	case ST_FILL_BOARD:
		board_stat = (rc = board_fill(&x, &y)) ? ST_END : ST_CHECK;
		break;
	case ST_CHECK:
		if (rc) {
			x = ball_to_x;// = -1;
			y = ball_to_y;// = -1;
		} else {
			x = -1;
			y = -1;
		}
		Session.score_delta = 0;
		ball_to_x  = -1;
		ball_to_y  = -1;
		if (board_check(x, y))
			board_stat = ST_REMOVE;
		else if (rc)
			board_stat = ST_FILL_BOARD,
			Session.score_mul = 1; /* multiply == 1 */
		else if (Session.iball < POOL_SIZE)
			board_stat = ST_FILL_BOARD;
		else
			board_stat = ST_FILL_POOL;
		break;
	case ST_REMOVE:
		fl = flushes_remove();
		Session.score += Session.score_delta * Session.score_mul;
		if (fl)
			Session.score_mul++;
//		fprintf(stderr, "Score: %d (+%d * %d)\n", Session.score, Session.score_delta, Session.score_mul);
//		show_score(Session.score);
		if (rc)
			board_stat = ST_IDLE;
		else if (Session.iball < POOL_SIZE)
			board_stat = ST_FILL_BOARD;
		else
			board_stat = ST_FILL_POOL;
		break;
	case ST_END:
//		fprintf(stderr, "Game Over\n");
		break;
	}
	return board_stat;
}

int board_score()
{
	int s;
	s = Session.score;
	return s;
}

int board_score_mul()
{
	int s;
	s = Session.score_mul - 1;
	return s;
}

bool board_running(void)
{
	return board_stat != ST_END;
}

#endif