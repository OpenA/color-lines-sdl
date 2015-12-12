#include "board.h"
#include "graphics.h"
#include "sound.h"
#include <stdio.h>
#include <time.h>
#include <SDL.h>
#include <SDL_thread.h>
// #include <pthread.h>

#ifndef WINDOWS
#include <unistd.h>
#include <pwd.h>
#else
#include <windows.h>
#include <shlobj.h>
#endif

#ifndef MAEMO
char *get_path(int n);
#endif

#ifdef MAEMO
#define SAVE_PATH "/home/user/.color-lines-save"
#else
#define SAVE_PATH get_path(0)
#endif

#ifdef MAEMO
#define SCORES_PATH "/home/user/.color-lines-scores"
#else
#define SCORES_PATH get_path(1)
#endif

#ifdef MAEMO
#define PREFS_PATH "/home/user/.color-lines"
#else
#define PREFS_PATH get_path(2)
#endif

#define _max(a,b) (((a)>(b))?(a):(b))	// developer: 'max' was a global define, so it was replaced to '_max'
#define _min(a,b) (((a)<(b))?(a):(b))     // developer: 'min' was a global define, so it was replaced to '_min'

#define SCREEN_W	800
#define SCREEN_H	480

#define POOL_SPACE	8
#define FONT_WIDTH	24
#define SCORES_X	60
#define SCORES_Y	225
#define SCORES_W	(BOARD_X - TILE_WIDTH - SCORES_X - 40)
#define SCORE_X		0
#define SCORE_Y		175
#define SCORE_W		(BOARD_X - 23)
#define ALPHA_STEPS	16 // 16
#define BALLS_NR	7
#ifdef MAEMO
#define SIZE_STEPS	8 // 16
#else
#define SIZE_STEPS	16
#endif
#define JUMP_STEPS	8
#define JUMP_MAX	0.8
#define BALL_STEP	25
#define TILE_WIDTH	50
#define TILE_HEIGHT	50

#define BOARD_X (800 - 450 - 15)
#define BOARD_Y (15)
#define BOARD_WIDTH (BOARD_W * TILE_WIDTH)
#define BOARD_HEIGHT (BOARD_H * TILE_HEIGHT)
#define SPECIAL_BALLS	4
#define BALL_JOKER	8
#define BALL_BOMB	9
#define BALL_BRUSH	10

static int g_mouse_down = 0;
static int update_needed = 0;
static int g_volume = 256;

static int restart_x;
static int restart_y;
static int restart_w;
static int restart_h;

static int music_w;
static int music_h;

static int music_x;
static int music_y;

static int info_w;
static int info_h;

static int info_x;
static int info_y;

static int vol_w;
static int vol_h;

static int g_snd_disabled = 0;
// pthread_mutex_t game_lock = PTHREAD_MUTEX_INITIALIZER;

SDL_mutex 	*game_mutex;

void game_lock(void)
{
	SDL_mutexP(game_mutex);
//	pthread_mutex_lock(&board_mutex);
}

void game_unlock(void)
{
	SDL_mutexV(game_mutex);
//	pthread_mutex_unlock(&board_mutex);
}

#ifndef MAEMO
img_t	pb_logo = NULL;
#endif
img_t	bg_saved = NULL;
static int g_info_window = 0;

int 	moving_nr = 0;
fnt_t	font;
img_t	balls[BALLS_NR + SPECIAL_BALLS][ALPHA_STEPS];
img_t	resized_balls[BALLS_NR + SPECIAL_BALLS][SIZE_STEPS];
img_t	jumping_balls[BALLS_NR + SPECIAL_BALLS][JUMP_STEPS];
img_t	cell;
img_t	bg;
img_t	music_on, music_off, info_on, info_off, vol_empty, vol_full;

#ifdef SHOW_CLOCK
static int show_time(int force)
{
	static int last_w = 0;
	static int last_minute = -1;
	int w;
	char buf[64];
	time_t t;
	struct tm *tm;
	t = time(NULL);
	tm = localtime(&t);
	if (!tm)
		return -1;
	if (!force && (tm->tm_min == last_minute))
		return 0;
	sprintf(buf, "%02d:%02d", tm->tm_hour, tm->tm_min);
	w = gfx_chars_width(font, buf);
	if (last_w) {
		gfx_draw_bg(bg, BOARD_X - POOL_SPACE - last_w, 
			SCREEN_H - gfx_font_height(font), last_w, gfx_font_height(font));
	}
	gfx_draw_chars(font, buf, BOARD_X - POOL_SPACE - w, SCREEN_H - gfx_font_height(font));
	gfx_update(BOARD_X - POOL_SPACE - _max(w, last_w), SCREEN_H - gfx_font_height(font),
		_max(w, last_w), gfx_font_height(font));
	last_w = w;
	last_minute = tm->tm_min;
//	fprintf(stderr,"Times going\n");
	return 1;
}
#endif

static int get_word(const char *str, char *word)
{
	int parsed = 0;
	while (*str && *str != '\n' && *str != ' ') {
		*word = *str;
		word ++;
		str ++;
		parsed ++;
	}
	while (*str == ' ') {
		str ++;
		parsed ++;
	}
	*word = 0;
	return parsed;
}
#define BALL_W 40
#define BALL_H 40
static struct _gfx_word {
	const char *name;
	img_t	*img;
} gfx_words[] = {
	{ "ball_color1", &balls[0][ALPHA_STEPS - 1] },
	{ "ball_color2", &balls[1][ALPHA_STEPS - 1] },
	{ "ball_color3", &balls[2][ALPHA_STEPS - 1] },
	{ "ball_color4", &balls[3][ALPHA_STEPS - 1] },
	{ "ball_color5", &balls[4][ALPHA_STEPS - 1] },
	{ "ball_color6", &balls[5][ALPHA_STEPS - 1] },
	{ "ball_color7", &balls[6][ALPHA_STEPS - 1] },
	{ "ball_joker", &balls[7][ALPHA_STEPS - 1] }, 
	{ "ball_bomb", &balls[8][ALPHA_STEPS - 1] },
	{ "ball_brush", &balls[9][ALPHA_STEPS - 1] }, 
	{ "ball_boom", &balls[10][ALPHA_STEPS - 1] },
#ifndef MAEMO
	{ "pb_logo", &pb_logo },
#endif	
	{ NULL },
};
static void show_info_status(void);
			
static img_t gfx_word(const char *word)
{
	int i = 0;
	while (gfx_words[i].name) {
		if (!strcmp(gfx_words[i].name, word)) {
			return *gfx_words[i].img;
		}
		i ++;
	}
	return NULL;
}
static const char *game_print(const char *str)
{
	int i;
	int x = BOARD_X;
	int y = BOARD_Y;
	int h = gfx_font_height(font);
	const char *ptr = str;
	while (*ptr == '\n')
		ptr ++;
	while (*ptr) {
		int w;
		i = 0;
		w = 0;
		h = gfx_font_height(font);
		while (w < BOARD_WIDTH) {
			char word[128];
			int rc;
			img_t img;
			rc = get_word(ptr, word);
//			fprintf(stderr,"get Word:%s:%d\n", word, rc);	
			img = gfx_word(word);
			if (img) {
				w += gfx_img_w(img);
			} else {
				w += gfx_chars_width(font, word);
			}
			if (w > BOARD_WIDTH)
				break;
			if (img) {
				gfx_draw(img, x, y);
				x += gfx_img_w(img) + gfx_chars_width(font, " ");
				h = _max(h, gfx_img_h(img));
			} else {	
				strcat(word, " ");	
				gfx_draw_chars(font, word, x, y);
				w += gfx_chars_width(font, " ");
				x += gfx_chars_width(font, word);
			}
			ptr += rc;
			if (*ptr == '\n') {
				ptr ++;
				break;
			}
//			fprintf(stderr,"Word:%s\n", word);	
		}
		x = BOARD_X;
		y += h;
		if (y >= BOARD_Y + BOARD_HEIGHT - 2*h) {
			gfx_draw_chars(font, "MORE", BOARD_X, BOARD_Y + BOARD_HEIGHT - h);
			break;
		}
	}
	return ptr;
}

char info_text[] = " -= Color Lines v0.6 =-\n\n"
	"Try to arrange balls of the same color in vertical, "
	"horizontal or diagonal lines."
	"To move a ball click on it to select, "
	"then click on destination square. Once line has five or more balls "
	"of same color, that line is removed from the board and you earn score points. "
	"After each turn three new balls randomly added to the board.\n\n"
	"There are four bonus balls.\n\n"
	"ball_joker is a joker that can be used like any color ball. Joker also multiply score by two.\n\n"
	"ball_bomb acts like a joker, but when applied it also removes all balls of the same color from the board.\n\n"
	"ball_brush paints all nearest balls in the same color.\n\n"
	"ball_boom is a bomb. It always does \"boom\"!!!\n\n"
	"The game is over when the board is filled up.\n\n"
	"CODE: Peter Kosyh <gloomy_pda@mail.ru>\n\n"
	"GRAPHICS: Peter Kosyh and some files from www.openclipart.org\n\n"
	"SOUNDS: Stealed from some linux games...\n\n"
	"MUSIC: \"Satellite One\" by Purple Motion.\n\n"
	"FIRST TESTING & ADVICES: my wife Ola, Sergey Kalichev...\n\n"
#ifndef MAEMO	
	"PORTING TO M$ Windoze: Ilja Ryndin from Pinebrush\n pb_logo www.pinebrush.com\n\n"
#else
	"PORTING TO M$ Windoze: Ilja Ryndin from Pinebrush\n(www.pinebrush.com)\n\n"
#endif	
	"SPECIAL THANX: All UNIX world... \n\n"
	" Good Luck!";
	
static const char *cur_text = info_text;

static int show_info_window(void)
{
	game_lock();
	if (!bg_saved) {
		bg_saved = gfx_grab_screen(BOARD_X - TILE_WIDTH - POOL_SPACE, BOARD_Y, 
			BOARD_WIDTH + TILE_WIDTH + POOL_SPACE, BOARD_HEIGHT);
		if (!bg_saved) {
			game_unlock();
			return 0;
		}
	}
	if (!*cur_text) {
		cur_text = info_text;
		game_unlock();
		return 1;
	}
	gfx_draw_bg(bg, BOARD_X - TILE_WIDTH - POOL_SPACE, BOARD_Y, 
			BOARD_WIDTH + TILE_WIDTH + POOL_SPACE, BOARD_HEIGHT);
#ifdef SHOW_CLOCK
	show_time(1);		
#endif	
	cur_text = game_print(cur_text);
	gfx_update(BOARD_X - TILE_WIDTH - POOL_SPACE, BOARD_Y, 
			BOARD_WIDTH + TILE_WIDTH + POOL_SPACE, BOARD_HEIGHT);
	g_info_window = 1;
	show_info_status();
	game_unlock();
	return 0;
}

static void hide_info_window(void)
{
	game_lock();
	gfx_draw(bg_saved, BOARD_X - TILE_WIDTH - POOL_SPACE, BOARD_Y);
#ifdef SHOW_CLOCK
	show_time(1);
#endif	
	gfx_update(BOARD_X - TILE_WIDTH - POOL_SPACE, BOARD_Y, 
			BOARD_WIDTH + TILE_WIDTH + POOL_SPACE, BOARD_HEIGHT);
	bg_saved = 0;
	gfx_free_image(bg_saved);
	g_info_window = 0;
	show_info_status();
	game_unlock();	
}

static int info_window(int x, int y)
{
	static const char *last_text = NULL;
	if (x >= BOARD_X && y >= BOARD_Y && x < BOARD_X + BOARD_WIDTH &&
		y < BOARD_Y + BOARD_HEIGHT ) {
		last_text = cur_text;
		if (show_info_window()) {
			show_info_window();
//			hide_info_window();
		}
		return 1;
	}
	cur_text = last_text;
	if (!cur_text)
		cur_text = info_text;
	hide_info_window();
	if (x >= info_x && x < info_x + info_w && y > info_y && y < info_y + info_h) {	
		return 1;
	}
	return 0;	
}

static int running = 1;

#define HISCORES_NR	5
static int	game_hiscores[HISCORES_NR] = { 50, 40, 30, 20, 10 };

enum {
	fadein = 1,
	fadeout,
	changing, 
	jumping, 
	moving,
} effect_t;

typedef struct {
	int used;
	int draw_needed;
	int x;
	int y;
	int tx;
	int ty;
	int effect;
	int step;
	int id;
	cell_t	cell;
	cell_t	cell_from;
} gfx_ball_t;

gfx_ball_t	game_board[BOARD_W][BOARD_H];
gfx_ball_t	game_pool[POOL_SIZE];

void draw_cell(int x, int y);
void update_cell(int x, int y);
void update_all(void);
void draw_ball(int n, int x, int y);
void draw_ball_offset(int n, int x, int y, int dx, int dy);
void draw_ball_alpha(int n, int x, int y, int alpha);
void draw_ball_size(int n, int x, int y, int size);
void update_cells(int x1, int y1, int x2, int y2);
void draw_ball_jump(int n, int x, int y, int num);
static void show_score(void);
static int check_hiscores(int score);
static void show_hiscores(void);
static void game_message(const char *str, int board);
static int game_restart(void);
static int game_loadhiscores(const char *path);
static int game_savehiscores(const char *path);
static int game_loadprefs(const char *path);
static int game_saveprefs(const char *path);
static int set_volume(int x, int y);
static int music_switch(int x, int y);
static void show_music_status(void);

static int g_music = 1;
static int g_prefs = 0;

static void enable_effect(int x, int y, int effect)
{
	gfx_ball_t *b;
	if (x == -1)
		b = &game_pool[y];	
	else 
		b = &game_board[x][y];
	b->x = x;
	b->y = y;
	b->used = 1;
	b->draw_needed = 1;
	b->effect = effect;
	b->step = 0;
}

static void disable_effect(int x, int y)
{
	gfx_ball_t *b;
	if (x == -1)
		b = &game_pool[y];	
	else 
		b = &game_board[x][y];
	b->draw_needed = 1;
	b->effect = 0;
}

int game_move_ball(void)
{
	static int x, y;
	int tx, ty;
	int m, s;
	
	m = board_moved(&tx, &ty);
	s = board_selected(&x, &y);
	if (!s && !m)
		return 0;

	if (s && !game_board[x][y].effect && game_board[x][y].cell && game_board[x][y].used) {
		enable_effect(x, y, jumping);
		game_board[x][y].step = 0;
	}
	
	if (m && !game_board[tx][ty].cell) {
		moving_nr ++;
		game_board[tx][ty].used = 0;
		game_board[tx][ty].cell = game_board[x][y].cell;
		enable_effect(tx, ty, moving);
		game_board[tx][ty].tx = tx;
		game_board[tx][ty].ty = ty;
		game_board[tx][ty].id = y * BOARD_W + x;
		game_board[tx][ty].x = x;
		game_board[tx][ty].y = y;
		disable_effect(x, y);
		game_board[x][y].draw_needed = 0;
		game_board[x][y].used = 0;
		game_board[x][y].cell = 0;
	}
	return 1;	
}

int game_process_pool(void)
{
	int rc = 0;
	int x;
	gfx_ball_t *b;
	for (x = 0; x < POOL_SIZE; x++) {
		cell_t	c;
		b = &game_pool[x];
		c = pool_cell(x);
		if (c && !b->cell && !b->effect) { /* appearing */
			enable_effect(-1, x, fadein);
			b->cell = c;
			rc ++;
		} else if (!c && b->cell && !b->effect) { /* disappearing */
			enable_effect(-1, x, fadeout);
			rc ++;
		}
	}
	return rc;
}

int game_process_board(void)
{
	int rc = 0;
	int x, y;
	gfx_ball_t *b;
	int fadeout_nr = 0;
	int paint_nr = 0;
	int boom_nr = 0;
	for (y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x++) {
			cell_t	c;
			b = &game_board[x][y];
			c = board_cell(x, y);
			if (c && !b->cell && !b->effect) { /* appearing */
				enable_effect(x, y, fadein);
				b->cell = c;
				rc ++;
			} else if (c && b->cell && c != b->cell && !b->effect){
//				paint_nr ++;
				enable_effect(x, y, changing);
				b->cell_from = b->cell;
				b->cell = c;
				rc ++;
			} else if (!c && b->cell && (!b->effect || b->effect == jumping)) { /* disappearing */
				
				if (b->cell == ball_boom) {
//					snd_play(SND_BOOM, 1);
					boom_nr ++;
				} else if (b->cell == ball_brush) {
//					snd_play(SND_PAINT, 1);	
					paint_nr ++;
				} else	
					fadeout_nr ++;	
				enable_effect(x, y, fadeout);
				rc ++;
			}
		}
	}
	if (boom_nr)
		snd_play(SND_BOOM, 1);
	else if (paint_nr)
		snd_play(SND_PAINT, 1);
	else if (fadeout_nr)
		snd_play(SND_FADEOUT, 1);
	return rc;
}
int game_init(void)
{
	int x, y;
	memset(game_board, 0, sizeof(game_board));
	memset(game_pool, 0, sizeof(game_pool));
	for (y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x++) {
			game_board[x][y].draw_needed = 0;
		}
	}
	return 0;
}

int game_display_board(void)
{
	int x, y, tmpx, tmpy, x1, y1;
	int rc = 0, dx, dy;
	int dist;
	for (y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x++) {
			gfx_ball_t *b;
			b = &game_board[x][y];
			if (!b->draw_needed)	
				continue;
//			fprintf(stderr,"In:x:%d y:%d c:%d(%d)\n", x, y, b->cell, b->effect);	
			rc ++;	
			switch (b->effect) {
			case 0:
				rc --;
				draw_cell(x, y);
				if (b->used)
					draw_ball(b->cell - 1, x, y); 
				else
					b->cell = 0;	
				update_cell(x, y);
				b->draw_needed = 0;
				break;
			case fadein:
//				if (moving_nr)
//					break;
				rc --;
				if (board_path(b->x, b->y))
					break;
				draw_cell(b->x, b->y);
				draw_ball_size(b->cell - 1, b->x, b->y, b->step); 
				b->step ++;
				if (b->step >= SIZE_STEPS) {
					disable_effect(x, y);
				}
				update_cell(b->x, b->y);
				break;
			case jumping:
				rc --;
				draw_cell(b->x, b->y);
				dist = b->step % (3*JUMP_STEPS);
				if (dist < JUMP_STEPS)
					draw_ball_jump(b->cell - 1, b->x, b->y, dist);
				else if (dist >= JUMP_STEPS && (dist < 2*JUMP_STEPS))
					draw_ball_jump(b->cell - 1, b->x, b->y, 2*JUMP_STEPS - dist - 1);
				else if (dist < 2*JUMP_STEPS + 5 && dist >= 2*JUMP_STEPS)
					draw_ball_offset(b->cell - 1, b->x, b->y, 0, 2*JUMP_STEPS - dist);
				else if (dist < 2*JUMP_STEPS + 10 && dist >= 2*JUMP_STEPS + 5)
					draw_ball_offset(b->cell - 1, b->x, b->y, 0, dist - 2*JUMP_STEPS - 10);
				b->step ++;
				update_cell(b->x, b->y);
				if (dist == 2 * JUMP_STEPS && (!board_selected(&tmpx, &tmpy) || tmpx != b->x || tmpy != b->y)) {
					disable_effect(x, y);
				} else if (b->step >= 3*JUMP_STEPS * 19 + 2*JUMP_STEPS) {
					//b->step = 0;
					disable_effect(x, y);
					board_select(-1, -1);
				}
				break;
			case moving:
//				rc --;
				x1 = b->x;
				y1 = b->y;
				draw_cell(b->x, b->y);
				board_follow_path(b->x, b->y, &tmpx, &tmpy, b->id);
				//fprintf(stderr,"%d %d\n", tmpx, tmpy);
				draw_cell(tmpx, tmpy);
				
				dist = abs(b->tx - b->x) + abs(b->ty - b->y);
				
				if (dist <= 2) {
					int d = (BALL_STEP * (TILE_WIDTH * dist - b->step)) / (TILE_WIDTH + TILE_WIDTH);
					if (!d)
						d = 1;
					b->step += d;
				} else
					b->step += BALL_STEP;
					
				dx = (tmpx - b->x) * b->step;
				dy = (tmpy - b->y) * b->step;
				
				if (abs(dx) >= TILE_WIDTH || abs(dy) >= TILE_HEIGHT) {
					board_clear_path(b->x, b->y);
					b->x = tmpx;
					b->y = tmpy;
					dx = dy = 0;
					b->step = 0;
				}
				draw_ball_offset(b->cell - 1, b->x, b->y, dx, dy); 
				
				update_cells(x1, y1, tmpx, tmpy); 
				//fprintf(stderr,"%d %d (%d %d)\n", tmpx, tmpy, dx, dy);

				if (b->x == b->tx && b->y == b->ty) {
					moving_nr --;
					board_clear_path(b->x, b->y);
					b->draw_needed = 1;
					b->effect = 0;
					b->used = 1;
				}
				break;
			case changing:
				draw_cell(b->x, b->y);
				draw_ball_alpha(b->cell_from - 1, b->x, b->y, 
					ALPHA_STEPS - b->step - 1); 
				draw_ball_alpha(b->cell - 1, b->x, b->y, b->step); 
				b->step ++;
				if (b->step >= ALPHA_STEPS - 1) {
					disable_effect(x, y);
//					b->used = 1;
				}
				update_cell(b->x, b->y);			
				break;		
			case fadeout:
				//rc --;
			//	if (moving_nr /*&& (b->step != ALPHA_STEPS -1)*/)
			//		break;
				draw_cell(b->x, b->y);
				draw_ball_alpha(b->cell - 1, b->x, b->y, ALPHA_STEPS - b->step - 1); 
				b->step ++;
				if (b->step == ALPHA_STEPS) {
					disable_effect(x, y);
					b->used = 0;
					b->cell = 0;
				}
				update_cell(b->x, b->y);
				break;
			}	
//			fprintf(stderr,"Out:x:%d y:%d c:%d(%d)\n", x, y, b->cell, b->effect);	

		}
	}
//	update_all();
	return rc;
}

int game_display_pool(void)
{
	int x;
	int rc = 0;
	for (x = 0; x < POOL_SIZE; x++) {
		gfx_ball_t *b;
		b = &game_pool[x];
		if (!b->draw_needed)	
			continue;
		rc ++;	
		switch (b->effect) {
		case 0:
			rc --;
			draw_cell(-1, x);
			if (b->used)
				draw_ball(b->cell - 1, -1, x); 
			else
				b->cell = 0;
			update_cell(-1, x);
			b->draw_needed = 0;
			break;
		case fadein:
//			if (moving_nr)
//				break;
			draw_cell(-1, b->y);
			draw_ball_size(b->cell - 1, -1, b->y, b->step); 
			b->step ++;
			if (b->step >= SIZE_STEPS) {
				disable_effect(-1, x);
			}
			update_cell(-1, x);
			break;
		case fadeout:
			rc --;
//			if (moving_nr)
//				break;
			draw_cell(-1, b->y);
			draw_ball_alpha(b->cell - 1, -1, b->y, ALPHA_STEPS - b->step - 1); 
			b->step ++;
			if (b->step == ALPHA_STEPS) {
				disable_effect(-1, x);
				b->used = 0;
				b->cell = 0;
			}
			update_cell(-1, b->y);
			break;
		}	
	}
	update_all();
	return rc;
}

#ifndef MAEMO
static int load_pb(void)
{
	pb_logo = gfx_load_image(GAMEDATADIR"gfx/pb_logo.png", 1);
	if (!pb_logo)
		return -1;
	return 0;	
}

static void free_pb(void)
{
	gfx_free_image(pb_logo);
}
#endif

static int load_balls(void)
{
	int i, k;
	img_t ball;
	ball = gfx_load_image(GAMEDATADIR"gfx/ball.png", 1);
	if (!ball)
		return -1;
	for (i = 1; i <= BALLS_NR + SPECIAL_BALLS; i++) {
		char fname[255];
		img_t color, new, alph, sized, jumped;
		if (i == ball_joker) {
			snprintf(fname, sizeof(fname), GAMEDATADIR"gfx/joker.png");
			color = NULL;
			new = gfx_load_image(fname, 1);
		} else if (i == ball_bomb) {
			new = gfx_load_image(GAMEDATADIR"gfx/atomic.png", 1);
			color = NULL;
		} else if (i == ball_brush) {
			new = gfx_load_image(GAMEDATADIR"gfx/paint.png", 1);
			color = NULL;
		} else if (i == ball_boom) {
			new = gfx_load_image(GAMEDATADIR"gfx/boom.png", 1);
			color = NULL;
		} else {
			snprintf(fname, sizeof(fname), GAMEDATADIR"gfx/color%d.png", i);
			color = gfx_load_image(fname, 1);
			if (!color)
				return -1;
			new = gfx_combine(ball, color);
		}
		if (!new)
			return -1;
		for (k = 1; k <= ALPHA_STEPS; k++) {
			alph = gfx_set_alpha(new, (255 * 100 ) / (ALPHA_STEPS * 100/k));
			balls[i - 1][k - 1] = alph;
		}
		for (k = 1; k <= SIZE_STEPS; k++) {
			float cff = (float)1.0 / ((float)SIZE_STEPS / (float)k);
			sized = gfx_scale(new, cff, cff);
			resized_balls[i - 1][k - 1] = sized;
		}
		for (k = 1; k <= JUMP_STEPS; k++) {
			float cff = 1.0 - (((float)(1.0 - JUMP_MAX)/(float)JUMP_STEPS) * k);
			jumped = gfx_scale(new, 1.0 + (1.0 - cff), cff);
			jumping_balls[i - 1][k - 1] = jumped;
		}
		gfx_free_image(new);	
		gfx_free_image(color);
	}
	gfx_free_image(ball);
	return 0;
}

void free_balls(void)
{
	int i, k;
	for (i = 1; i <= BALLS_NR + SPECIAL_BALLS; i++) {
		for (k = 1; k <= ALPHA_STEPS; k++) {
			gfx_free_image(balls[i - 1][k - 1]);
		}
		for (k = 1; k <= SIZE_STEPS; k++) {
			gfx_free_image(resized_balls[i - 1][k - 1]);
		}
		for (k = 1; k <= JUMP_STEPS; k++) {
			gfx_free_image(jumping_balls[i - 1][k - 1]);
		}
	}
}
static int cell_to_screen(int x, int y, int *ox, int *oy)
{
	if (x == -1) {
		*ox = BOARD_X - TILE_WIDTH - POOL_SPACE;
		*oy = y * TILE_HEIGHT + TILE_HEIGHT * (BOARD_H - POOL_SIZE)/2 + BOARD_Y;
		return 0;
	}
	*ox = x * TILE_WIDTH + BOARD_X;
	*oy = y * TILE_HEIGHT + BOARD_Y;
	return 0;
}

int load_cell(void)
{
#ifdef MAEMO
	cell = gfx_load_image(GAMEDATADIR"gfx/cell.png", 0);
#else
	cell = gfx_load_image(GAMEDATADIR"gfx/cell.png", 1);
#endif	
	if (!cell)
		return -1;
	return 0;	
}

static int load_music(void)
{
	music_on = gfx_load_image(GAMEDATADIR"gfx/music_on.png", 1);
	music_off = gfx_load_image(GAMEDATADIR"gfx/music_off.png", 1);
	if (!music_on || !music_off)
		return -1;
	music_w = gfx_img_w(music_on);
	music_h = gfx_img_h(music_on);	
	music_x = 0;
	music_y = SCREEN_H - music_h;
	return 0;	
}

static int music_switch(int x, int y)
{
	if (g_snd_disabled)
		return 0;
	if (x >= music_x && y >= music_y &&
		 x < music_x + music_w && y < music_y + music_h ) {
		g_music ^= 1;
		g_prefs = 1;
		if (g_music) {
			if (!snd_music_start())
			g_music = 0;
		} 
		if (!g_music)
			snd_music_stop();
		show_music_status();	
		return 1;
	} 
	return 0;
}
static void free_music(void)
{
	gfx_free_image(music_on);
	gfx_free_image(music_off);
}

static int load_info(void)
{
	info_on = gfx_load_image(GAMEDATADIR"gfx/info_on.png", 1);
	info_off = gfx_load_image(GAMEDATADIR"gfx/info_off.png", 1);
	if (!info_on || !info_off)
		return -1;
	info_w = gfx_img_w(info_off);
	info_h = gfx_img_h(info_off);	
	info_x = 0; //BOARD_X - info_w;
	info_y = music_y - info_h;
	return 0;	
}

static int vol_off;
static int load_volume(void)
{
	vol_empty = gfx_load_image(GAMEDATADIR"gfx/vol_empty.png", 1);
	vol_full = gfx_load_image(GAMEDATADIR"gfx/vol_full.png", 1);
	if (!vol_empty || !vol_full)
		return -1;
	vol_w = gfx_img_w(vol_empty);
	vol_h = gfx_img_h(vol_empty);	
	vol_off = vol_w / 4;
	return 0;	
}

static void free_volume(void)
{
	gfx_free_image(vol_empty);
	gfx_free_image(vol_full);
}


static void show_volume()
{
	int x = music_w;
	int y = SCREEN_H - music_h;
	int w;
	game_lock();
	w = g_volume * (vol_w - vol_off) / 256;
	game_unlock();
	gfx_draw_bg(bg, x, y, vol_w, vol_h);
	gfx_draw(vol_empty, x, y);
	if (!g_volume)
		gfx_draw_wh(vol_full, x, y, 0, gfx_img_h(vol_full));
	else	
		gfx_draw_wh(vol_full, x, y, w + vol_off, gfx_img_h(vol_full));
	gfx_update(x, y, vol_w, vol_h);
}

static int set_volume(int x, int y)
{
	int disp;
	
	if (g_snd_disabled)
		return 0;
		
	if (x < music_w || y < SCREEN_H - music_h || x > music_w + vol_w || y > SCREEN_H)
		return 0;
	disp = x - music_w;
	if (disp < vol_off)
		disp = vol_off;
	else if (disp > vol_w)
		disp = vol_w;	
	disp -= vol_off;	
	game_lock();
	g_volume = (256 * disp) / (vol_w - vol_off);
	game_unlock();
	snd_volume(g_volume);
	if (!g_volume)
		snd_music_stop();
	else if (g_music)
		snd_music_start();	
	show_volume();
	g_prefs = 1;	
	return 1;
}

static void free_info(void)
{
	gfx_free_image(info_on);
	gfx_free_image(info_off);
}

int load_bg(void)
{
	bg = gfx_load_image(GAMEDATADIR"gfx/bg.png", 0);
	if (!bg)
		return -1;
	return 0;	
}

void free_cell(void)
{
	gfx_free_image(cell);
}

void free_bg(void)
{
	gfx_free_image(bg);
}

void draw_cell(int x, int y)
{
	int nx, ny;
	cell_to_screen(x, y, &nx, &ny);
#ifndef MAEMO	
	gfx_draw_bg(bg, nx, ny, TILE_WIDTH, TILE_HEIGHT);
#endif	
	gfx_draw(cell, nx, ny);
}


void update_cell(int x, int y)
{
	update_needed = 1;
	int nx, ny;
	cell_to_screen(x, y, &nx, &ny);
#ifdef MAEMO	
	gfx_update(nx, ny, TILE_WIDTH, TILE_HEIGHT);
#endif	
}

void update_cells(int x1, int y1, int x2, int y2)
{
	update_needed = 1;
	int nx1, ny1;
	int nx2, ny2;
	int tmp;
	if (x1 > x2) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}
	if (y1 > y2) {
		tmp = y2;
		y2 = y1;
		y1 = tmp;
	}
	cell_to_screen(x1, y1, &nx1, &ny1);
	cell_to_screen(x2 + 1, y2 + 1, &nx2, &ny2);
#ifdef MAEMO	
	gfx_update(nx1, ny1, nx2 - nx1, ny2 - ny1);
#endif	
}

void mark_cells_dirty(int x1, int y1, int x2, int y2)
{
//	update_needed = 1;
	int x;
	int tmp;
	if (x1 > x2) {
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}
	if (y1 > y2) {
		tmp = y2;
		y2 = y1;
		y1 = tmp;
	}
	x = x1;
	for (; y1 <= y2; y1++) {
		for (x1 = x; x1 <= x2; x1++)
			game_board[x1][y1].draw_needed = 1;
	}
}

void update_all(void)
{
#ifndef MAEMO
	if (update_needed)
		gfx_update(BOARD_X - TILE_WIDTH - POOL_SPACE, 
			BOARD_Y, BOARD_WIDTH + TILE_WIDTH + POOL_SPACE, BOARD_HEIGHT);
#endif		
	update_needed = 0;
}


void draw_ball_alpha_offset(int n, int alpha, int x, int y, int dx, int dy)
{
	int nx, ny;
	cell_to_screen(x, y, &nx, &ny);
	gfx_draw(balls[n][alpha], nx + 5 + dx, ny + 5 + dy);
}

void draw_ball_alpha(int n, int x, int y, int alpha)
{
	draw_ball_alpha_offset(n, alpha, x, y, 0, 0);
}

void draw_ball_offset(int n, int x, int y, int dx, int dy)
{
	draw_ball_alpha_offset(n, ALPHA_STEPS - 1, x, y, dx, dy);
}

void draw_ball(int n, int x, int y)
{
	draw_ball_offset(n, x, y, 0, 0);
}

void draw_ball_size(int n, int x, int y, int size)
{
	int nx, ny;
	SDL_Surface *img = (SDL_Surface *)resized_balls[n][size];
	int diff = ((TILE_WIDTH - img->w) /2);
	cell_to_screen(x, y, &nx, &ny);
	gfx_draw(img, nx + diff, ny + diff);
}

void draw_ball_jump(int n, int x, int y, int num)
{
	int nx, ny;
	SDL_Surface *img = (SDL_Surface *)jumping_balls[n][num];
	int diff = (40 - img->h) + 5;
	int diffx = ((TILE_WIDTH - img->w) /2);
	cell_to_screen(x, y, &nx, &ny);
	gfx_draw(img, nx + diffx, ny + diff);
}

static void fetch_game_board(void)
{
	int x, y;
	for (y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x++) {
			game_board[x][y].cell = board_cell(x, y);
			if (game_board[x][y].cell) {
				game_board[x][y].x = x;
				game_board[x][y].y = y;
				game_board[x][y].used = 1;
			}
		}
	}
}

static void draw_board(void)
{
	int x, y;
	for (y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x++) {
			draw_cell(x, y);
			if (game_board[x][y].cell && game_board[x][y].used) {
				draw_ball(game_board[x][y].cell - 1, x, y); 
			}
		}
	}
}

static int game_loop() {
    // Main loop
	SDL_Event event;
	int x,y;
	while (running) {
		if (SDL_WaitEvent(&event)) {
			if (event.key.state == SDL_PRESSED) {
				if (event.key.keysym.sym == SDLK_ESCAPE
#ifdef MAEMO				
				|| event.key.keysym.sym == SDLK_F4
				|| event.key.keysym.sym == SDLK_F5 
				|| event.key.keysym.sym == SDLK_F6
#endif
				) {
				
					game_lock();
					running = 0;
					game_unlock();
				}
			}
			switch (event.type) { 
// Quit the game
			case SDL_QUIT:
				running = 0;
				break;
// Button pressed
			case SDL_MOUSEMOTION:
				x = event.button.x;
				y = event.button.y;
				game_lock();
				if (g_mouse_down) {
					set_volume(x, y);
				}
				game_unlock();
				break;
			case SDL_MOUSEBUTTONUP:	
				game_lock();
				g_mouse_down = 0;
				game_unlock();
				break;
			case SDL_MOUSEBUTTONDOWN:
				game_lock();
				g_mouse_down = 1;
				game_unlock();
				x = event.button.x;
				y = event.button.y;
				if (set_volume(x, y))
					break;
				if (music_switch(x, y))
					break;	
				if (g_info_window) {
					if (info_window(x, y))
						break;
//					g_info_window ^= 1;
//					hide_info_window();	
//					break;
				}
		
				if (x >= restart_x && y >= restart_y && x < restart_x + restart_w &&
					y < restart_y + restart_h)  {
					if (board_running() && check_hiscores(board_score())) {
						snd_play(SND_HISCORE, 1);
					} else if (board_running()) {
						snd_play(SND_GAMEOVER, 1);
					}
					game_restart();
				} else if (x >= info_x && x < info_x + info_w && 
					y >= info_y && y < info_y + info_h) {	
					show_info_window();
				} else if (x >= BOARD_X && y >= BOARD_Y &&
					x < (BOARD_X + BOARD_WIDTH) &&
					y < (BOARD_Y + BOARD_HEIGHT)) {
					if (board_running()) {
						x = (x - BOARD_X) / TILE_WIDTH;
						y = (y - BOARD_Y) / TILE_HEIGHT;
						board_select(x, y);
					} else {
						game_restart();
					}
				}
	 	             	break;
	            	}
		}
		
	}
	if (board_running())
		board_save(SAVE_PATH);
	return 0;
}
static int game_over = 0;
static Uint32 callback(Uint32 interval)
{
	game_lock();
	if (!running) {
		game_unlock();
		return 0;
	}
#ifdef SHOW_CLOCK
	show_time(0);
#endif	
	if (g_info_window) {
		game_unlock();
		return interval;
	}

	game_move_ball();
	game_process_board();
	game_process_pool();
	show_score();
//	if ((pos = check_hiscores(board_score()))) {
//		show_hiscores(pos);
//	}
	game_display_pool();
//		pthread_mutex_unlock(&game_lock);
//		return interval;
//	}
	if (!game_display_board()) { /* nothing todo */
		board_logic();
		if (!board_running() && !update_needed && !game_over) {
			game_over  = 1;
			game_message("Game Over!", 1);			
			if (check_hiscores(board_score())) {
				snd_play(SND_HISCORE, 1);
				show_hiscores();
			} else {
				snd_play(SND_GAMEOVER, 1);
			}
			remove(SAVE_PATH);
		}
	}
	
	update_all();
	game_unlock();
	return interval;
}

static int cur_score = -1;
static int cur_mul = 0;
static int score_x = SCORE_X + ((SCORE_W) / 2);
static int score_w = 0;
#define BONUS_TIMER 40 // 40
#define BONUS_BLINKS 4 // 5

static void show_score(void)
{
	char buff[64];
	int w, h, x;
	int new_score = board_score();
	static int timer = 0;
	
	if (board_score_mul() < cur_mul)
		cur_mul = board_score_mul();

	if (new_score > cur_score || cur_score == -1) {
		h = gfx_font_height(font);
		if ((board_score_mul() > cur_mul) && (board_score_mul() > 1) && !(timer % BONUS_BLINKS)) {
			if (!timer) {
				snd_play(SND_BONUS, 1);
			}
			snprintf(buff, sizeof(buff), "Bonus x%d", board_score_mul());
			w = gfx_chars_width(font, buff);
			x = SCORE_X + ((SCORE_W - w) / 2);
			gfx_draw_bg(bg, _min(score_x, x), SCORE_Y, _max(score_w, w), h);
			if ((timer / BONUS_BLINKS) & 1)
				gfx_draw_chars(font, buff, x, SCORE_Y);
			gfx_update(_min(score_x, x), SCORE_Y, _max(score_w, w), h);
			score_x = x;
			score_w = w;
			if (!timer)
				timer = BONUS_TIMER;
		} else if (!timer) {

			snprintf(buff, sizeof(buff), "SCORE:%d", ++ cur_score);
			if (cur_score != new_score) {
				snd_play(SND_CLICK, 1);
			}
			w = gfx_chars_width(font, buff);
			x = SCORE_X + ((SCORE_W - w) / 2);
			gfx_draw_bg(bg, _min(score_x, x), SCORE_Y, _max(score_w, w), h);
			gfx_draw_chars(font, buff, x, SCORE_Y);
			gfx_update(_min(score_x, x), SCORE_Y, _max(score_w, w), h);
			score_x = x;
			score_w = w;
		}
	}

	if (timer == 1) {
		cur_mul = board_score_mul();
	}
	if (timer)
		timer --;
}

static int game_savehiscores(const char *path)
{
	int i;
	FILE *f = fopen(path, "w");
	if (!f)
		return -1;

	for (i = 0; i < HISCORES_NR; i++) {
		if (fwrite(&game_hiscores[i], sizeof(game_hiscores[i]), 1, f) != 1)
			goto err;
	}
	fclose(f);
	return 0;
err:		
	fclose(f);
	return -1;
}

static int game_loadhiscores(const char *path)
{
	int i;
	FILE *f = fopen(path, "r");
	if (!f)
		return -1;

	for (i = 0; i < HISCORES_NR; i++) {
		if (fread(&game_hiscores[i], sizeof(game_hiscores[i]), 1, f) != 1)
			goto err;
	}
	fclose(f);
	return 0;
err:		
	fclose(f);
	return -1;
}

static int game_loadprefs(const char *path)
{
	FILE *f = fopen(path, "r");
	if (!f)
		return -1;
	if (fread(&g_music, sizeof(g_music), 1, f) != 1)
		goto err;
	if (fread(&g_volume, sizeof(g_volume), 1, f) != 1)
		goto err;
	if (g_volume > 256 || g_volume < 0)
		g_volume = 256;
	fclose(f);
	return 0;
err:		
	fclose(f);
	return -1;
}

static int game_saveprefs(const char *path)
{
	FILE *f = fopen(path, "w");
	if (!f)
		return -1;
	if (fwrite(&g_music, sizeof(g_music), 1, f) != 1)
		goto err;
	if (fwrite(&g_volume, sizeof(g_volume), 1, f) != 1)
		goto err;
	fclose(f);
	return -1;
err:		
	fclose(f);
	return -1;
}

static int check_hiscores(int score)
{
	int i, k;
	for (i = 0; i < HISCORES_NR; i++) {
		if (score > game_hiscores[i]) {
			for (k = HISCORES_NR - 1; k > i; k --) {
				game_hiscores[k] = game_hiscores[k - 1];
			}
			game_hiscores[i] = score;
			game_savehiscores(SCORES_PATH);
			return i + 1;
		}
	}
	return 0;
}

static void show_hiscores(void)
{
	char buff[64];
	int w, i;
	int h = gfx_font_height(font);
	gfx_draw_bg(bg, SCORES_X, SCORES_Y, SCORES_W, HISCORES_NR * h);
	for (i = 0; i < HISCORES_NR; i++) {
		snprintf(buff, sizeof(buff),"%d", i + 1);
		w = gfx_chars_width(font, buff);
		snprintf(buff, sizeof(buff),"%d.", i + 1);
		gfx_draw_chars(font, buff, SCORES_X + FONT_WIDTH - w, SCORES_Y + i * h);
		snprintf(buff, sizeof(buff),"%d", game_hiscores[i]);	
		w = gfx_chars_width(font, buff);
		gfx_draw_chars(font, buff, SCORES_X + SCORES_W - w, SCORES_Y + i * h);
	}
	gfx_update(SCORES_X, SCORES_Y, SCORES_W, HISCORES_NR * h);
}

static void game_message(const char *str, int board)
{
	int w = gfx_chars_width(font, str);
	int x = BOARD_X + (BOARD_WIDTH - w) / 2;
	int y = BOARD_Y + (BOARD_HEIGHT - gfx_font_height(font)) / 2;
	if (!board) {
		x = ( SCREEN_W - w )/ 2;
		y = (SCREEN_H - gfx_font_height(font)) / 2;
	}
	gfx_draw_chars(font, str, x, y);
	gfx_update(x, y, w, gfx_font_height(font));
}

static void draw_buttons(void)
{
	char restart[] = "Restart";
	int w = gfx_chars_width(font, restart);
	int h = gfx_font_height(font);
	restart_x = SCORES_X + (SCORES_W - w)/2;
	restart_y = SCREEN_H - h * 2 - h/2;
	restart_w = w;
	restart_h = h;
	gfx_draw_chars(font, restart, restart_x, restart_y);
	return;
}

static int game_prep(void)
{
	game_lock();
	gfx_draw_bg(bg, 0, 0, SCREEN_W, SCREEN_H);
	draw_buttons();
	gfx_update(0, 0, SCREEN_W, SCREEN_H);
	show_hiscores();
	show_music_status();
	show_volume();
	show_info_status();
	score_x = SCORE_X + ((SCORE_W) / 2);
	score_w = 0;
	game_unlock();
	return 0;
}


static void show_music_status(void)
{
	gfx_draw_bg(bg, music_x, music_y, music_w, music_h);
	if (g_music)
		gfx_draw(music_on, music_x, music_y);
	else	
		gfx_draw(music_off, music_x, music_y);
	gfx_update(music_x, music_y, music_w, music_h);
	return;
}

static void show_info_status(void)
{
	gfx_draw_bg(bg, info_x, info_y, info_w, info_h);
	if (g_info_window)
		gfx_draw(info_on, info_x, info_y);
	else	
		gfx_draw(info_off, info_x, info_y);
	gfx_update(info_x, info_y, info_w, info_h);
	return;
}


static int game_restart(void)
{
	game_lock();
	update_needed = 0;
	game_over = 0;
	board_init();
	game_init();
	draw_board();
	gfx_update(BOARD_X, BOARD_Y, BOARD_WIDTH, BOARD_HEIGHT);
	cur_score = -1;
	cur_mul = 0;
	show_score();
	show_hiscores();
#ifdef SHOW_CLOCK
	show_time(1);
#endif	
	game_unlock();
	return 0;
}

static int game_load(void)
{
	game_lock();
	update_needed = 0;
	board_init();
	if (board_load(SAVE_PATH)) {
		game_unlock();
		return -1;
	}
	game_over = 0;
	game_init();
	fetch_game_board();
	draw_board();
	gfx_update(BOARD_X, BOARD_Y, BOARD_WIDTH, BOARD_HEIGHT);
	cur_score = board_score() - 1;
	cur_mul = board_score_mul();
	show_score();
	show_hiscores();
#ifdef SHOW_CLOCK
	show_time(1);
#endif	
	game_unlock();
	return 0;
}

#ifndef WINDOWS
int main(int argc, char **argv)
#else
int WINAPI WinMain (HINSTANCE hInstance,
                        HINSTANCE hPrevInstance, 
                        PSTR szCmdLine, 
                        int iCmdShow)
#endif
{
#ifdef MAEMO
	putenv("SDL_VIDEO_X11_WMCLASS=lines");
#endif
	game_mutex = SDL_CreateMutex();
	if (gfx_init())
		return 1;
	if (snd_init()) {
		g_snd_disabled = 1;	
	}
	game_loadprefs(PREFS_PATH);
	if (!g_snd_disabled)
		snd_volume(g_volume);
	else
		g_volume = 0;	
	if (g_music) {
		if (!snd_music_start())
			g_music = 0;
	}

	font = gfx_load_font(GAMEDATADIR"gfx/fnt.png", FONT_WIDTH);
	if (!font)
		return 1;
	game_message("Loading...", 0);	

	if (load_bg())
		return 1;
	if (load_balls())
		return 1;
	if (load_cell())
		return 1;
	if (load_volume())
		return 1;
	if (load_music())
		return 1;
	if (load_info())
		return 1;
#ifndef	MAEMO
	if (load_pb())
		return 1;
#endif
	game_loadhiscores(SCORES_PATH);
	
	game_prep();
	if (game_load())
		game_restart();
	SDL_SetTimer(20, callback);
	game_loop();

	game_lock();
	free_bg();
	free_cell();
	free_balls();
	free_music();
	free_info();
	free_volume();
#ifndef MAEMO
	free_pb();
#endif	
	game_unlock();
	snd_done();
	gfx_done();
	gfx_font_free(font);
	SDL_DestroyMutex(game_mutex);
	if (g_prefs)
		game_saveprefs(PREFS_PATH);
	return 0;
}

#ifndef MAEMO
char *get_path(int n)
{
	static char path[255];

#ifndef WINDOWS
	struct passwd *pw;
	pw = getpwuid(getuid());
	if (!pw) {
		strcpy(path, "/tmp");
	} else 
		snprintf(path, sizeof(path) - 32, "%s", pw->pw_dir);
#else
	static char appdir[MAX_PATH];

	SHGetFolderPath( NULL, 
		CSIDL_FLAG_CREATE | CSIDL_LOCAL_APPDATA,
		NULL,
		0, 
		(LPTSTR)appdir );

	if ( !strlen( appdir ) )
	{
		strcpy( path, "." );
	}
	else
                    {
		strcpy( path, appdir );
                    }
#endif

	switch (n) {
	case 0:
		strcat(path,"/.lines-save");
		break;
	case 1:
		strcat(path,"/.lines-scores");
		break;	
	case 2:
		strcat(path,"/.lines");
		break;	
	}
	return path;
}
#endif
