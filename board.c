#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <time.h>

#include "board.h"
// #include <pthread.h>
#define FL_PATH 0x80

#define max(a,b) (((a)>(b))?(a):(b))
#define BONUS_PCNT	5

enum {
	IDLE = 0,
	MOVING,
	FILL_POOL,
	FILL_BOARD,
	CHECK,
	REMOVE,
	END
} board_state_t;

cell_t	move_matrix_ids[BOARD_W][BOARD_H];
cell_t	move_matrix[BOARD_W][BOARD_H];
cell_t	board[BOARD_W][BOARD_H];
cell_t	ball_pool[POOL_SIZE];
int	ball_x, ball_to_x;
int	ball_y, ball_to_y;
unsigned int	pool_ball;

static unsigned int board_state;

typedef struct {
	int x;
	int y;
	int nr;
	int bomb;
	cell_t	col;
	struct {
		int x;
		int y;
	} cells[max(BOARD_W,BOARD_H)];
}flush_t;

flush_t	*flushes[BOARD_W * BOARD_H];
unsigned int	flush_nr;

unsigned int	free_cells;
unsigned int	score;
unsigned int	score_mul;
unsigned int	score_delta;


//pthread_mutex_t board_mutex = PTHREAD_MUTEX_INITIALIZER;
SDL_mutex 	*board_mutex;

void board_lock(void)
{
	SDL_mutexP(board_mutex);
//	pthread_mutex_lock(&board_mutex);
}

void board_unlock(void)
{
	SDL_mutexV(board_mutex);
//	pthread_mutex_unlock(&board_mutex);
}

static cell_t cell_get(int x, int y);

int board_selected(int *x, int *y)
{
	board_lock();

	if (!cell_get(ball_x, ball_y)) {
		board_unlock();
		return 0;
	}

	if (x)
		*x = ball_x;
	if (y)
		*y = ball_y;	
//	fprintf(stderr,"Board selected: %d %d\n", ball_x, ball_y);	
	board_unlock();	
	return 1;
}

int board_moved(int *x, int *y)
{
	board_lock();
	if (ball_to_x == -1 || ball_to_y == -1 || board_state != CHECK) {
		board_unlock();
		return 0;
	}
	if (x)
		*x = ball_to_x;
	if (y)
		*y = ball_to_y;
//	fprintf(stderr,"Ball moved: %d %d\n", ball_to_x, ball_to_y);	
	board_unlock();	
	return 1;
}

int board_init(void)
{
	if (board_mutex)
		SDL_DestroyMutex(board_mutex);
	board_mutex = SDL_CreateMutex();
	srand(time(NULL));
	score = 0;
	score_mul = 1;
	flush_nr = 0;
	free_cells = BOARD_W * BOARD_H;
	memset(board, 0, sizeof(board));
	memset(ball_pool, 0, sizeof(ball_pool));
	memset(flushes, 0, sizeof(flushes));
	memset(move_matrix, 0, sizeof(move_matrix));
	memset(move_matrix_ids, 0, sizeof(move_matrix));

	ball_x = -1;
	ball_y = -1;
	ball_to_x = -1;
	ball_to_y = -1;
	board_fill_pool();
	board_fill(NULL, NULL);
	board_fill(NULL, NULL);
	board_fill(NULL, NULL);
	board_fill_pool();	
	board_state = IDLE;
	return 0;
}
static cell_t cell_get(int x, int y)
{
	if ((x < 0) || (x >= BOARD_W))
		return 0;
	if ((y < 0) || (y >= BOARD_H))
		return 0;
	return board[x][y];
}

cell_t board_cell(int x, int y)
{
	cell_t cell;
	board_lock();
	cell =  cell_get(x, y);
	board_unlock();
	return cell;
}

cell_t pool_cell(int x)
{
	cell_t cell;
	if (x >= POOL_SIZE)
		return 0;
	board_lock();
	if (x < pool_ball)
		cell = 0;
	else
		cell =  ball_pool[x];
	board_unlock();
	return cell;
}

static cell_t *cell_ref(int x, int y)
{
	if ((x < 0) || (x >= BOARD_W))
		return NULL;
	if ((y < 0) || (y >= BOARD_H))
		return NULL;
	return &board[x][y];
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


static int normalize_move_matrix(int x, int y, int tox, int toy, int id)
{
	int dx, dy, i;
	int last_move = -1;
	int num = move_matrix[x][y];
	move_matrix[x][y] |= FL_PATH;
	do {
		way_t w[4];
		int ways[4] = { 0, 1, 2, 3 };
		
		w[0].x = x + 1; w[1].x = x; w[2].x = x - 1; w[3].x = x;
		w[0].y = y; w[1].y = y + 1; w[2].y = y; w[3].y = y - 1;

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
			for (i = 0; i < 4; i ++) {
				if (w[ways[i]].n == num - 1) {
					last_move = ways[i];
					break;
				}
			}
		}
		num = w[last_move].n;
		x = w[last_move].x;
		y = w[last_move].y;
		move_matrix[x][y] |= FL_PATH;
	} while (num != 1);

#if 0
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
#endif
	return 0;	
}

static int _board_path(int x, int y)
{
	if (x < 0 || x >= BOARD_W)
		return 0;
	if (y < 0 || y >= BOARD_H)
		return 0;
	if (move_matrix[x][y] & FL_PATH) {
//		fprintf(stderr,"Stop\n");
		return 1;
	}
	return 0;	
}

int board_path(int x, int y)
{
	int rc;
	board_lock();
	rc = _board_path(x, y);
	board_unlock();
	return rc;	
}

void board_clear_path(int x, int y)
{
	board_lock();
	move_matrix[x][y] = move_matrix[x][y] & ~FL_PATH;
	board_unlock();

}
int board_follow_path(int x, int y, int *ox, int *oy, int id)
{
	int num;
	int a,v;
	board_lock();
	num  = move_matrix[x][y];
	if (!(num & FL_PATH)) {
		board_unlock();
		return 0;
	}
	num &= ~FL_PATH;	
	
	a = move_cell(x + 1, y, num, id); 
	v = a & ~FL_PATH;
	if ((a & FL_PATH) && (v == num + 1)) {
		*ox = x + 1; *oy = y;
		board_unlock();
		return 1;
	}
	a = move_cell(x, y + 1, num, id); 
	v = a & ~FL_PATH;
	if ((a & FL_PATH) && (v == num + 1)) {
		*ox = x; *oy = y + 1;
		board_unlock();
		return 1;
	}
	a = move_cell(x -1 , y, num, id); 
	v = a & ~FL_PATH;
	if ((a & FL_PATH) && (v == num + 1)) {
		*ox = x - 1; *oy = y;
		board_unlock();
		return 1;
	}
	a = move_cell(x, y - 1, num, id); 
	v = a & ~FL_PATH;
	if ((a & FL_PATH) && (v == num + 1)) {
		*ox = x; *oy = y - 1;
		board_unlock();
		return 1;
	}
	board_unlock();
//	fprintf(stderr,"No move %d:%d %d\n", id, x , y);
	*ox = x; 
	*oy = y;
	return 0;
}
static int board_move(int x1, int y1, int x2, int y2)
{
	int x, y, mark = 1;
	int id = x1 + y1 * BOARD_W;
	for (y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x ++) {
			if (!(move_matrix[x][y] & FL_PATH)) {
				move_matrix[x][y] = 0;
				move_matrix_ids[x][y] = -1;
			}
		}
	}
//	memset(move_matrix, 0, sizeof(move_matrix));
	if (move_matrix[x1][y1])
		return 0;
//	fprintf(stderr,"%d %d	-> %d %d\n", x1, y1, x2, y2);
	move_matrix[x1][y1] = 1;
	move_matrix_ids[x1][y1] = id;
	while (mark) {
		mark = 0;
		for (y = 0; y < BOARD_H; y ++) {
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

#if 0
	fprintf(stdout,"XXX\n");
	for (y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x ++) {
			if (move_matrix[x][y] & FL_PATH)
				fprintf(stdout, "XX");
			else
				fprintf(stdout, "%02d", move_matrix[x][y]);	
		}
		fprintf(stdout,"\n");
	}
#endif
	
	if (!_board_path(x2, y2) && move_matrix[x2][y2]) {
		cell_t b;
		cell_t *c;
		c = cell_ref(x1, y1);
		b = *c;
		*c = 0;
		c = cell_ref(x2, y2);
		*c = b;
		normalize_move_matrix(x2, y2, x1, y1, id);
		return 1;
	}
	return 0;	
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
			pos --;	
		}
	}
	return NULL;
}

static cell_t get_rand_cell(void)
{
	cell_t c;
	int rnd = rand() % 100;
	if (rnd < BONUS_PCNT)
		return ball_joker + (rand() % BONUSES_NR);	
	c = (rnd % COLORS_NR) + 1;
	return c;	
}
static cell_t get_rand_color(void)
{
	return (rand() % 7) + 1;
}

static int is_color(cell_t c)
{
	if (!c)
		return 0;
	if (c <= 7)
		return 1;
	return 0;	
}
static int is_joker(cell_t c)
{
	if (c == ball_joker || c == ball_bomb)
		return 1;
	return 0;	
}

static cell_t joinable(cell_t c, cell_t *prev)
{
	if (!c || !(*prev))
		return 0;
	if (is_joker(*prev)) /* set joker suggestion */
		*prev = c;
	if ( c == *prev )
		return c;
	if (is_joker(c))
		return *prev;
	return 0;		
//	return ((a == b) || (a == ball_joker) || (b == ball_joker));
}

static void flush_add(int x1, int y1, int x2, int y2, cell_t col)
{
	flush_t *f;
	f = malloc(sizeof(flush_t));
	if (!f)
		exit(1);
	if (x2 > x1)
		x2 ++;
	if (y2 > y1)
		y2 ++;
	if (y2 < y1)
		y2 --;			
	f->nr = 0;
//	f->score = 0;	
	f->col = col;
	f->bomb = 0;
	do {
		if (cell_get(x1, y1) == ball_bomb)
			f->bomb ++;
		f->cells[f->nr].x = x1;
		f->cells[f->nr].y = y1;
		f->nr ++;
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

static int remove_cell(cell_t *c)
{
	int rc = 0;
	if (!*c)
		return 0;

	if (is_color(*c) || is_joker(*c)) {
		score_delta ++;
		rc ++;
	}
	if (*c == ball_joker)
		score_mul *= 2;
	*c = 0;
	free_cells ++;
	return rc;
}

static int remove_color(cell_t col)
{
	int x, y;
	for (y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x ++) {
			cell_t *c = cell_ref(x, y);
			if (*c == col) {
				remove_cell(c);
			}
		}
	} 
	return 0;
}


static int flushes_remove(void)
{
	int i, k;
	for (i = 0; i < flush_nr; i++) {
		cell_t *c;
		flush_t *f = flushes[i];
		for (k = 0; k < f->nr; k++) {
			if (f->bomb)
				remove_color(f->col);
			c = cell_ref(f->cells[k].x, f->cells[k].y);
			if (*c) {
	//			if (*c == ball_bomb) {
	//				remove_color(f->col);
	//			}
				remove_cell(c);
			}
		}
		free(f);
	}
	if (flush_nr) {
		flush_nr = 0;
		return 1;
	}
	return 0;
}

int board_fill_pool(void)
{
	int i;
//	board_lock();
	for (i = 0; i < POOL_SIZE; i++) {
		ball_pool[i] = get_rand_cell(); //(rand() % (ball_max - 1)) + 1;
	}
	pool_ball = 0;
//	board_unlock();
	return 0;
}


static int scan_hline(int x, int y)
{
	int xi, yi;
	cell_t b;
	for ( ; (x < BOARD_W) && !cell_get(x, y); x ++); // skip spaces
	if ((BOARD_W - x) < BALLS_ROW)
		return BOARD_W;
	b = cell_get(x, y);
//	if (is_joker(b))
//		fprintf(stderr, "Joker at %d %d\n", x, y);
	for ( xi = x, yi = y; (xi < BOARD_W) && (joinable(cell_get(xi, yi), &b)); xi ++);
//	if (is_joker(b))
//		fprintf(stderr, "Joker end at %d %d\n", xi, yi);
	
	if ( (xi - x) >= BALLS_ROW) {
		flush_add(x, y, xi - 1, y, b);
//		return xi;
	}
	if (is_joker(b)) { /* all jokers */
//		fprintf(stderr,"All jokers at %d\n", xi);
		return xi;
	}
	while (is_joker(cell_get(xi - 1, yi))) /*  prepeare jokers for next try */
		xi --;
	return xi;
}

static int scan_vline(int x, int y)
{
	int xi, yi;
	cell_t b;
	for ( ; (y < BOARD_H) && !cell_get(x, y); y ++); // skip spaces
	if ((BOARD_H - y) < BALLS_ROW)
		return BOARD_H;
	b = cell_get(x, y);
	for ( yi = y, xi = x; (yi < BOARD_H) && (joinable(cell_get(xi, yi), &b)); yi ++);
	if ( (yi - y) >= BALLS_ROW) {
		flush_add(x, y, xi, yi - 1, b);
//		return yi;
	}
	if (is_joker(b)) /* all jokers */
		return yi;
	while (is_joker(cell_get(xi, yi - 1))) /*  prepeare jokers for next try */
		yi --;
	return yi;
}

static int scan_aline(int x, int y) /* /  line */
{
	int xi, yi;
	cell_t b;
	for ( ; (x < BOARD_W) && (y >= 0) && !cell_get(x, y); y --, x ++); // skip spaces
//	if (y < BALLS_ROW)
//		return x + y;
//	if ((BOARD_W - x) < BALLS_ROW)	
//		return BOARD_W;
	b = cell_get(x, y);
	for ( xi = x, yi = y; (xi < BOARD_W) && (yi >= 0) && 
			(joinable(cell_get(xi, yi), &b)); yi --, xi ++);
	if ( (xi - x) >= BALLS_ROW) {
		flush_add(x, y, xi - 1, yi + 1, b);
//		return xi;
	}
	if (is_joker(b)) /* all jokers */
		return xi;
	while (is_joker(cell_get(xi - 1, yi + 1))) { /*  prepeare jokers for next try */
		yi ++;
		xi --;
	}
	return xi;
}

static int scan_bline(int x, int y) /* \  line */
{
	int xi, yi;
	cell_t b;
	for ( ; (x < BOARD_W) && (y < BOARD_H) && !cell_get(x, y); y ++, x ++); // skip spaces
	b = cell_get(x, y);
//	if ((BOARD_H - y) < BALLS_ROW)
//		return x + BOARD_H - y;
//	if ((BOARD_W - x) < BALLS_ROW)
//		return BOARD_W;
	for ( xi = x, yi = y; (xi < BOARD_W) && (yi < BOARD_H) && 
			(joinable(cell_get(xi, yi), &b)); yi ++, xi ++);
	if ( (xi - x) >= BALLS_ROW) {
		flush_add(x, y, xi - 1, yi - 1, b);
//		return xi;
	}
	if (is_joker(b)) /* all jokers */
		return xi;
	while (is_joker(cell_get(xi - 1, yi - 1))) { /*  prepeare jokers for next try */
		yi --;
		xi --;
	}
	return xi;
}

static int board_check_hlines(void)
{	
	int x, y;
	int of = flush_nr;
	
	for (y = 0; y < BOARD_H; y ++) {
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
	
	for (x = 0; x < BOARD_W;x ++) {
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
	for (y = 0; y < BOARD_H; y ++ ) {
		for (x = 0; x < y; ) {
			x = scan_aline(x, y - x);
		}
	}
	for (y = 1; y < BOARD_W; y ++ ) {
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
	for (y = 0; y < BOARD_W; y ++ ) {
		for (x = y; x < BOARD_W; ) {
			x = scan_bline(x, x - y);
		}
	}
	for (y = 1; y < BOARD_H; y ++ ) {
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
	if (*c == ball_boom) {
		rc += board_boom(x, y);
//		score_delta ++;
	} else if (*c) {
		rc += remove_cell(c);
	}
	return rc;
}

static int board_boom(int x, int y)
{
	int rc = 0;
	cell_t *c = cell_ref(x, y);
	if (c && *c == ball_boom) {
		*c = 0;
		free_cells ++;
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
	
	cell_t col = get_rand_color(); //TODO
	cell_t *c = cell_ref(x, y);
	if (c && *c == ball_brush) {
		*c = 0;
		free_cells ++;
		c = cell_ref(x - 1, y);
		if (c && is_color(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x + 1, y);
		if (c && is_color(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x, y - 1);
		if (c && is_color(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x, y + 1);
		if (c && is_color(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x + 1, y - 1);
		if (c && is_color(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x - 1, y - 1);
		if (c && is_color(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x + 1, y + 1);
		if (c && is_color(*c)) {
			*c = col;
			rc ++;
		}
		c = cell_ref(x - 1, y + 1);
		if (c && is_color(*c)) {
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

int board_fill(int *ox, int *oy)
{
	int x, y;
	int pos;
	cell_t *cell;

//	board_lock();
	if (free_cells < 1) {
//		board_unlock();
		return -1;
	}
//	for (i = 0; i < POOL_SIZE; i++) {
		pos = rand() % free_cells;
		cell = find_pos(pos, &x, &y);
		if (!cell) {
			fprintf(stderr,"Something really bad 1\n");
			exit(1);
		}
		*cell = ball_pool[pool_ball++];
		free_cells --;
		if (ox)
			*ox = x;
		if (oy)
			*oy = y;	
//		board_check_row(x, y);
//		flushes_remove();
//	}
//	board_unlock();
//	if (pool_ball == POOL_SIZE) {
//		pool_ball = 0;
//		return 0;
//	}
	return POOL_SIZE - pool_ball;
}

static void gfx_display_cell(int x, int y, cell_t cell)
{
	static char balls[]=".01234567";
	if (y == 0 && x == 0)
		fprintf(stdout,"  012345678\n");
	if (x == 0)
		fprintf(stdout,"%d|",y);	
	fprintf(stdout, "%c", balls[cell]);
	if ( x == (BOARD_W - 1)) {
		fprintf(stdout,"\n");
	}
	
}

int board_display(void)
{
	int x, y;
	for (y = 0; y < BOARD_H; y++) {
		for (x = 0; x < BOARD_W; x++) {
			gfx_display_cell(x, y, cell_get(x, y));		
		}
	}
	return 0;
}

int board_select(int x, int y)
{
	cell_t *c;
	if (x == -1 && y == -1) { /* special case */
		board_lock();
		ball_x = -1;
		ball_y = -1;	
		board_unlock();	
		return 0;
	}
	if (x < 0 || x >= BOARD_W)
		return 0;
		
	if (y < 0 || y >= BOARD_H)
		return 0;
		
	board_lock();
//
//	if (board_state != IDLE) {
//		board_unlock();
//		return 0;
//	}
			
	c = cell_ref(x, y);
	if (*c) {
		ball_x = x;
		ball_y = y;
//		ball_to_x = -1;
//		ball_to_y = -1;
		board_unlock();
		return 0;
	}
	
	if (!cell_get(ball_x, ball_y)) { /* nothing to move */
		board_unlock();
		return 0;
	}

	if (board_state != IDLE) {
		board_unlock();
		return 0;
	}

	ball_to_x = x;
	ball_to_y = y;
	board_state = MOVING;
/*	
	if (!board_move(ball_x, ball_y, x, y)) {
		board_unlock();
		return 0;
	}
	board_check_row(x, y);
	if (flushes_remove()) {
		board_unlock();
		return 2;
	} */
	board_unlock();
	return 1;
}

int board_logic(void)
{
	static int rc;
	static int x, y;
	int fl;
	board_lock();
//	fprintf(stderr,"state=%d\n", board_state);
	switch(board_state) {
	case IDLE:
		if (!free_cells)
			board_state = END;
		if (free_cells == (BOARD_W * BOARD_H))
			board_state = FILL_BOARD;
		break;
	case MOVING:
		if (!board_move(ball_x, ball_y, ball_to_x, ball_to_y)) {
			board_state = IDLE;
			break;
		}
		board_state = CHECK;
		rc = -1;
//		x = ball_to_x;
//		y = ball_to_y;
		ball_x = -1;
		ball_y = -1;
		break;
	case FILL_POOL:
		board_fill_pool();
		if (free_cells == BOARD_W * BOARD_H)
			board_state = FILL_BOARD;
		else
			board_state = IDLE;
//		score_mul = 1;
		break;
	case FILL_BOARD:
		rc = board_fill(&x, &y);
		if (rc < 0)
			board_state = END;
		else
			board_state = CHECK;
//		score_mul = 1; /* multiply == 1 */	
		break;
	case CHECK:
		score_delta = 0;
		if (rc == -1) {
			x = ball_to_x;// = -1;
			y = ball_to_y;// = -1;	
		} else {
			x = -1;
			y = -1;
		}

		ball_to_x = -1;
		ball_to_y = -1;	

		if (board_check(x, y)) {
			board_state = REMOVE;
			break;
		}
		if (rc != -1) {
			if ((POOL_SIZE - pool_ball) > 0)
				board_state = FILL_BOARD;
			else
				board_state = FILL_POOL;
		} else {
			board_state = FILL_BOARD;
			score_mul = 1;
		}
//		ball_to_x = -1;
//		ball_to_y = -1;	
		break;
	case REMOVE:
		fl = flushes_remove();
		score += score_delta * score_mul;
		if (fl)
			score_mul ++;
//		fprintf(stderr, "Score: %d (+%d * %d)\n", score, score_delta, score_mul);
//		show_score(score);
		if (rc != -1) {
			if ((POOL_SIZE - pool_ball) > 0)
				board_state = FILL_BOARD;
			else
				board_state = FILL_POOL;
		} else {
			board_state = IDLE;	
		}
		break;
	case END:
//		fprintf(stderr, "Game Over\n");
		break;				
	}
	board_unlock();
	return 0;
}

int board_score()
{
	int s;
	board_lock();
	s = score;
	board_unlock();
	return s;
}

int board_score_mul()
{
	int s;
	board_lock();
	s = score_mul - 1;
	board_unlock();
	return s;
}

int board_running(void)
{
	if (board_state == END)
		return 0;
	return 1;	
}

int board_save(const char *path)
{
	FILE *f = fopen(path, "w");
	if (!f)
		return -1;
	while ((board_state != IDLE) && board_running())
		board_logic();	
	if (!board_running())	
		return -1;
	board_lock();	
	if (fwrite(board, sizeof(cell_t), BOARD_W * BOARD_H, f) !=
		BOARD_W * BOARD_H)
		goto err;
	if (fwrite(ball_pool, sizeof(cell_t), POOL_SIZE, f) !=
		POOL_SIZE)
		goto err;

	if (fwrite(&score, sizeof(unsigned int), 1, f) != 1)
		goto err;

	if (fwrite(&score_mul, sizeof(unsigned int), 1, f) != 1)
		goto err;

	if (fwrite(&score_delta, sizeof(unsigned int), 1, f) != 1)
		goto err;

	if (fwrite(&free_cells, sizeof(unsigned int), 1, f) != 1)
		goto err;

	if (fwrite(&pool_ball, sizeof(unsigned int), 1, f) != 1)
		goto err;

	board_unlock();
	fclose(f);
	return 0;
err:		
	board_unlock();
	fclose(f);
	return -1;
}

int board_load(const char *path)
{
	FILE *f = fopen(path, "r");
	if (!f)
		return -1;
	board_lock();	
	if (fread(board, sizeof(cell_t), BOARD_W * BOARD_H, f) !=
		BOARD_W * BOARD_H)
		goto err;
	if (fread(ball_pool, sizeof(cell_t), POOL_SIZE, f) !=
		POOL_SIZE)
		goto err;

	if (fread(&score, sizeof(unsigned int), 1, f) != 1)
		goto err;

	if (fread(&score_mul, sizeof(unsigned int), 1, f) != 1)
		goto err;

	if (fread(&score_delta, sizeof(unsigned int), 1, f) != 1)
		goto err;

	if (fread(&free_cells, sizeof(unsigned int), 1, f) != 1)
		goto err;

	if (fread(&pool_ball, sizeof(unsigned int), 1, f) != 1)
		goto err;

	board_state = IDLE;
	board_unlock();
	fclose(f);
	return 0;
err:		
	board_unlock();
	fclose(f);
	return -1;
}
