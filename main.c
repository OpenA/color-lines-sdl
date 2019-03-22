#include "main.h"
#include "board.h"
#include "graphics.h"
#include "sound.h"

#ifdef MAEMO
	#define SIZE_STEPS  8 // 16
	#define SAVE_PATH   "/home/user/.config/color-lines/save"
	#define SCORES_PATH "/home/user/.config/color-lines/scores"
	#define PREFS_PATH  "/home/user/.config/color-lines/prefs"
#else
	#define SIZE_STEPS  16
	static char SAVE_PATH   [ PATH_MAX ];
	static char SCORES_PATH [ PATH_MAX ];
	static char PREFS_PATH  [ PATH_MAX ];
#endif

#define POOL_SPACE  8
#define FONT_WIDTH  24
#define SCORES_X    60
#define SCORES_Y    225
#define SCORES_W   (BOARD_X - TILE_WIDTH - SCORES_X - 40)
#define SCORE_X     0
#define SCORE_Y     175
#define SCORE_W    (BOARD_X - 23)
#define ALPHA_STEPS 16
#define BALLS_NR   (COLORS_NR + BONUSES_NR)
#define JUMP_STEPS  8
#define JUMP_MAX    0.8
#define BALL_STEP   25
#define TILE_WIDTH  50
#define TILE_HEIGHT 50

#define BOARD_X      (800 - 450 - 15)
#define BOARD_Y      (15)
#define BOARD_WIDTH  (BOARD_W * TILE_WIDTH)
#define BOARD_HEIGHT (BOARD_H * TILE_HEIGHT)
#define HISCORES_NR	  5
#define BONUS_BLINKS  4
#define BONUS_TIMER  40

#define BALL_JOKER  8
#define BALL_BOMB   9
#define BALL_BRUSH 10
#define BALL_W     40
#define BALL_H     40

static struct __SET__ {
	short volume;
	char music;
	bool loop;
	int lastTime;
} Settings  = {
	.volume = 256,
	.music  = 0,
	.loop   = false
};

static struct __STAT__ {
	bool running, game_over, update_needed, store_prefs;
} status = {
	.store_prefs   = false,
	.update_needed = false,
	.game_over     = false,
	.running       = true
};

typedef struct __ELS__ {
	char name[ 24 ];
	int x, y, w, h;
	img_t on, off;
	float temp;
	bool touch, hook;
} elemen_t;

static elemen_t Restart = { .name = "Restart" };
static elemen_t Board;
static elemen_t Track   = { .temp = 14 };
static elemen_t Timer   = { .name = "00:00", .temp = 0.5 };
static elemen_t Music   = { .name = "music" };
static elemen_t Loop    = { .name = "loop" };
static elemen_t Info    = { .name = "info" };
static elemen_t Vol     = { .name = "vol" };

SDL_mutex *game_mutex;

void game_lock(void)
{
	SDL_mutexP(game_mutex);
}

void game_unlock(void)
{
	SDL_mutexV(game_mutex);
}

static int FONT_HEIGHT;
int   moving_nr = 0;
fnt_t font;
#ifndef MAEMO
img_t pb_logo = NULL;
#endif
img_t bg_saved = NULL;
img_t balls[BALLS_NR][ALPHA_STEPS];
img_t resized_balls[BALLS_NR][SIZE_STEPS];
img_t jumping_balls[BALLS_NR][JUMP_STEPS];
img_t cell, bg;

bool alpha = true;

#ifdef MAEMO
	alpha = false;
#endif

static bool _Is_onElement(elemen_t target, int x, int y)
{
	return !(x < target.x || y < target.y || x >= target.x + target.w || y >= target.y + target.h);
}

static bool load_Img_for(elemen_t *target)
{
	char ion[ 24 ], ioff[ 24 ];
	
	bool loaded = !strcmp(target->name, "vol");
	
	strcpy(ion,  target->name); strcat(ion, (loaded ? "_full.png"  : "_on.png" ));
	strcpy(ioff, target->name); strcat(ioff,(loaded ? "_empty.png" : "_off.png"));
	
	target->on  = gfx_load_image(ion,  true);
	target->off = gfx_load_image(ioff, true);
	
	if ((loaded = target->on && target->off)) {
		target->w = gfx_img_w(target->off);
		target->h = gfx_img_h(target->off);
	}
	return !loaded;
}

static void draw_Button_for(elemen_t *target, bool enabled)
{
	gfx_draw_bg(bg, target->x, target->y, target->w, target->h);
	gfx_draw((enabled ? target->on : target->off), target->x, target->y);
	gfx_update();
}

static void free_Img_for(elemen_t *target)
{
	gfx_free_image(target->on);
	gfx_free_image(target->off);
}

static void draw_Volume_bar(void)
{
	game_lock();
	int bar = Settings.volume ? Settings.volume * Vol.w / 256 : 0;
	game_unlock();
	gfx_draw_bg(bg, Vol.x, Vol.y, Vol.w, Vol.h);
	gfx_draw(Vol.off, Vol.x, Vol.y);
	gfx_draw_wh(Vol.on, Vol.x, Vol.y, bar, Vol.h);
	gfx_update();
}

static void draw_Track_title(void)
{
	gfx_draw_bg(bg, Track.x, Track.y, Track.w, Track.h);
	Track.on = gfx_load_ttf_font("manaspc.ttf", Track.name, Track.temp);
	Track.w  = gfx_img_w(Track.on);
	Track.h  = gfx_img_h(Track.on);
	gfx_draw_wh(Track.on, Track.x, Track.y, Track.w, Track.h);
	gfx_update();
}

static Uint32 draw_Timer_digit(Uint32 interval, void *_null)
{
	Settings.lastTime++;
	
	sprintf(Timer.name, "%02d:%02d", (int)(Settings.lastTime / 60), Settings.lastTime % 60);
	gfx_draw_bg(bg, Timer.x, Timer.y, Timer.w, Timer.h);
	
	Timer.w = gfx_chars_width(font, Timer.name) * Timer.temp;
	Timer.x = SCREEN_W - Timer.w;
	
	gfx_draw_chars(font, Timer.name, Timer.x, Timer.y, Timer.temp);
	gfx_update();
	
	return interval;
}

static void go_GameTimer(bool start)
{
	static SDL_TimerID gameTimerId;
	
	if (start)
		gameTimerId = SDL_AddTimer(1000, &draw_Timer_digit, NULL);
	else
		SDL_RemoveTimer( gameTimerId );
}

static void start_GameTimer()
{
	go_GameTimer( true );
}
static void stop_GameTimer()
{
	go_GameTimer( false );
}

static int get_word(const char *str, char *word)
{
	int parsed = 0;
	while (*str && *str != '\n' && *str != ' ') {
		*word = *str;
		word   ++;
		str    ++;
		parsed ++;
	}
	while (*str == ' ') {
		str    ++;
		parsed ++;
	}
	*word = 0;
	return parsed;
}

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
	{ "ball_joker" , &balls[7][ALPHA_STEPS - 1] }, 
	{ "ball_bomb"  , &balls[8][ALPHA_STEPS - 1] },
	{ "ball_brush" , &balls[9][ALPHA_STEPS - 1] }, 
	{ "ball_boom" , &balls[10][ALPHA_STEPS - 1] },
#ifndef MAEMO
	{ "pb_logo", &pb_logo },
#endif	
	{ NULL },
};

static img_t gfx_word(const char *word)
{
	int i = 0;
	while (gfx_words[i].name) {
		if (!strcmp(gfx_words[i].name, word)) {
			return *gfx_words[i].img;
		}
		i++;
	}
	return NULL;
}
static const char *game_print(const char *str)
{
	int x = BOARD_X;
	int y = BOARD_Y;
	const char *ptr = str;
	while (*ptr == '\n')
		ptr++;
	while (*ptr) {
		int w = 0;
		int h = FONT_HEIGHT;
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
				gfx_draw_chars(font, word, x, y, 0);
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
		x  = BOARD_X;
		y += h;
		if (y >= BOARD_Y + BOARD_HEIGHT - 2*h) {
			gfx_draw_chars(font, "MORE", BOARD_X, BOARD_Y + BOARD_HEIGHT - h, 0);
			break;
		}
	}
	return ptr;
}

char info_text[] = " -= Color Lines v"CL_VER" =-\n\n"
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

static const char *cur_text  = info_text;
static const char *last_text = NULL;

static void show_info_window(void)
{
	last_text = cur_text;
	game_lock();
	if (!bg_saved && !(
		 bg_saved = gfx_grab_screen(
			BOARD_X - TILE_WIDTH - POOL_SPACE,
			BOARD_Y,
			BOARD_WIDTH + TILE_WIDTH + POOL_SPACE,
			BOARD_HEIGHT))){
	} else if (!*cur_text) {
		cur_text = info_text;
	} else {
		gfx_draw_bg(bg, BOARD_X - TILE_WIDTH - POOL_SPACE, BOARD_Y, 
				BOARD_WIDTH + TILE_WIDTH + POOL_SPACE, BOARD_HEIGHT);
		stop_GameTimer();
		cur_text = game_print(cur_text);
		draw_Button_for(&Info, (Info.hook = true));
	}
	game_unlock();
}

static void hide_info_window(void)
{
	cur_text = last_text ?: info_text;
	game_lock();
	gfx_draw(bg_saved, BOARD_X - TILE_WIDTH - POOL_SPACE, BOARD_Y);
	gfx_update();
	bg_saved = 0;
	gfx_free_image(bg_saved);
	draw_Button_for(&Info, (Info.hook = false));
	start_GameTimer();
	game_unlock();
}

static int game_hiscores[HISCORES_NR] = { 50, 40, 30, 20, 10 };

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

gfx_ball_t game_board[BOARD_W][BOARD_H];
gfx_ball_t game_pool[POOL_SIZE];

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
static bool check_hiscores(int score);
static void show_hiscores(void);
static void game_message(const char *str, bool board);
static void game_restart(bool clean);
static void game_loadhiscores(const char *path);
static void game_savehiscores(const char *path);
static void game_loadprefs(const char *path);
static void game_saveprefs(const char *path);
static int set_volume(int x);

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
	bool m, s;
	
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
void game_init(void)
{
	memset(game_board, 0, sizeof(game_board));
	memset(game_pool, 0, sizeof(game_pool));
	for (int y = 0; y < BOARD_H; y ++) {
		for (int x = 0; x < BOARD_W; x++) {
			game_board[x][y].draw_needed = 0;
		}
	}
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
static bool load_pb(void)
{
	pb_logo = gfx_load_image("pb_logo.png", true);
	return !pb_logo;
}

static void free_pb(void)
{
	gfx_free_image(pb_logo);
}
#endif

static int load_balls(void)
{
	img_t ball = gfx_load_image("ball.png", true);
	if (!ball)
		return -1;
	for (int i = 1; i <= BALLS_NR; i++) {
		img_t color, new, alph, sized, jumped;
		if (i == ball_joker) {
			color = NULL;
			new = gfx_load_image("joker.png", true);
		} else if (i == ball_bomb) {
			new = gfx_load_image("atomic.png", true);
			color = NULL;
		} else if (i == ball_brush) {
			new = gfx_load_image("paint.png", true);
			color = NULL;
		} else if (i == ball_boom) {
			new = gfx_load_image("boom.png", true);
			color = NULL;
		} else {
			char fname[12];
			snprintf(fname, sizeof(fname), "color%d.png", i);
			color = gfx_load_image(fname, true);
			if (!color)
				return -1;
			new = gfx_combine(ball, color);
		}
		if (!new)
			return -1;
		for (int k = 1; k <= ALPHA_STEPS; k++) {
			alph = gfx_set_alpha(new, (255 * 100 ) / (ALPHA_STEPS * 100/k));
			balls[i - 1][k - 1] = alph;
		}
		for (int k = 1; k <= SIZE_STEPS; k++) {
			float cff = (float)1.0 / ((float)SIZE_STEPS / (float)k);
			sized = gfx_scale(new, cff, cff);
			resized_balls[i - 1][k - 1] = sized;
		}
		for (int k = 1; k <= JUMP_STEPS; k++) {
			float cff = 1.0 - (((float)(1.0 - JUMP_MAX) / (float)JUMP_STEPS) * k);
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
	for (int i = 0; i < BALLS_NR; i++) {
		for (int k = 0; k < ALPHA_STEPS; k++) {
			gfx_free_image(balls[i][k]);
		}
		for (int k = 0; k < SIZE_STEPS; k++) {
			gfx_free_image(resized_balls[i][k]);
		}
		for (int k = 0; k < JUMP_STEPS; k++) {
			gfx_free_image(jumping_balls[i][k]);
		}
	}
}

static void cell_to_screen(int x, int y, int *ox, int *oy)
{
	if (x == -1) {
		*ox = BOARD_X - TILE_WIDTH - POOL_SPACE;
		*oy = y * TILE_HEIGHT + TILE_HEIGHT * (BOARD_H - POOL_SIZE)/2 + BOARD_Y;
	} else {
		*ox = x * TILE_WIDTH + BOARD_X;
		*oy = y * TILE_HEIGHT + BOARD_Y;
	}
}

bool load_cell(void)
{
	cell = gfx_load_image("cell.png", alpha);
	return !cell;
}

static void music_switch(void)
{
	if (Settings.music == -1) {
		Settings.music = Music.temp;
		snd_music_start(Settings.music, Track.name);
		if (!Track.on)
			draw_Track_title();
	} else {
		Music.temp = Settings.music;
		Settings.music = -1;
		snd_music_stop();
	}
	status.store_prefs = true;
	draw_Button_for(&Music, (Settings.music > -1));
}

void track_switch(void)
{
	if ((status.store_prefs = Settings.music > -1)) {
		Settings.music += Settings.loop >> Track.hook ^ 1;
		if (Settings.music >= TRACKS_COUNT)
			Settings.music = 0;
		snd_music_start(Settings.music, Track.name);
		gfx_free_image(Track.on);
		draw_Track_title();
	}
}

static int set_volume(int x)
{
	int disp = x < Music.w ? 0 : x > (Vol.w + Music.w) ? Vol.w : x - Music.w;
	game_lock();
	Settings.volume = (256 * disp) / Vol.w;
	game_unlock();
	if (!Music.hook)
		snd_volume(Settings.volume);
	draw_Volume_bar();
	status.store_prefs = true;
	return disp + Music.w;
}

bool load_bg(void)
{
	bg = gfx_load_image("bg.png", false);
	return !bg;	
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
	status.update_needed = true;
	int nx, ny;
	cell_to_screen(x, y, &nx, &ny);
#ifdef MAEMO
	gfx_update();
#endif
}

void update_cells(int x1, int y1, int x2, int y2)
{
	status.update_needed = true;
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
	gfx_update();
#endif	
}

void mark_cells_dirty(int x1, int y1, int x2, int y2)
{
//	status.update_needed = true;
	int x, tmp;
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
	if (status.update_needed)
		gfx_update();
#endif
	status.update_needed = false;
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
	int diff = (TILE_WIDTH - img->w) / 2;
	cell_to_screen(x, y, &nx, &ny);
	gfx_draw(img, nx + diff, ny + diff);
}

void draw_ball_jump(int n, int x, int y, int num)
{
	int nx, ny;
	SDL_Surface *img = (SDL_Surface *)jumping_balls[n][num];
	int diff  = (40 - img->h) + 5;
	int diffx = (TILE_WIDTH - img->w) / 2;
	cell_to_screen(x, y, &nx, &ny);
	gfx_draw(img, nx + diffx, ny + diff);
}

static void fetch_game_board(void)
{
	for (int y = 0; y < BOARD_H; y++) {
		for (int x = 0; x < BOARD_W; x++) {
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
	for (int x, y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x++) {
			draw_cell(x, y);
			if (game_board[x][y].cell && game_board[x][y].used) {
				draw_ball(game_board[x][y].cell - 1, x, y); 
			}
		}
	}
}

static Uint32 refresh_screen(Uint32 interval, void *_null)
{
	if (status.running) {
		if (!Info.hook) {
			game_lock();
			game_move_ball();
			game_process_board();
			game_process_pool();
			show_score();
			game_display_pool();
			if (!game_display_board()) { /* nothing todo */
				board_logic();
				if (!board_running() && !status.update_needed && !status.game_over) {
					stop_GameTimer();
					game_message("Game Over!", (status.game_over = true));
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
		}
	} else
		interval = 0;
	return interval;
}

static void game_loop() {
	// Main loop
	SDL_Event event;
	
	SDL_AddTimer(20, &refresh_screen, NULL);
	
	while (status.running) {
		if (SDL_WaitEvent(&event)) {
			//fprintf(stderr,"event %d\n", event.type);
			int x = event.button.x;
			int y = event.button.y;
			if (event.key.state == SDL_PRESSED) {
				if (event.key.keysym.sym == SDLK_ESCAPE
#ifdef MAEMO
				|| event.key.keysym.sym == SDLK_F4
				|| event.key.keysym.sym == SDLK_F5
				|| event.key.keysym.sym == SDLK_F6
#endif
				) {
					game_lock();
					status.running = false;
					game_unlock();
				}
			}
			switch (event.type) {
			case SDL_QUIT: // Quit the game
				status.running = false;
				break;
			case SDL_MOUSEMOTION:
				Board.touch = !(x < BOARD_X || y < BOARD_Y || x >= BOARD_X + BOARD_WIDTH || y >= BOARD_Y + BOARD_HEIGHT);
				  Vol.touch = _Is_onElement(Vol, x, y);
				if (Vol.hook)
					Vol.temp = set_volume(x);
				break;
			case SDL_MOUSEBUTTONUP:
				Track.hook = Vol.hook = false;
				break;
			case SDL_MOUSEBUTTONDOWN: // Button pressed
				if (Board.touch) {
					if (Info.hook) {
						show_info_window();
					} else if (board_running()) {
						board_select(
							(x - BOARD_X) / TILE_WIDTH,
							(y - BOARD_Y) / TILE_HEIGHT);
					} else {
						game_restart(true);
					}
				}
				else if ((Vol.hook = Vol.touch)) {
					Vol.temp = set_volume(x);
				}
				else if (_Is_onElement(Restart, x, y)) {
					if (board_running()) {
						snd_play(
							check_hiscores(board_score()) ? SND_HISCORE : SND_GAMEOVER, 1);
					}
					game_restart(true);
				}
				else if (_Is_onElement(Music, x, y)) {
					music_switch();
				}
				else if ((Track.hook = _Is_onElement(Track, x, y))) {
					track_switch();
				}
				else if (_Is_onElement(Loop, x, y)) {
					draw_Button_for(&Loop, (Settings.loop ^= 1));
				}
				else if (_Is_onElement(Info, x, y)) {
					Info.hook ? hide_info_window() : show_info_window();
				}
				break;
			case SDL_MOUSEWHEEL:
				if (Vol.touch) {
					Vol.temp = set_volume(Vol.temp + (x ?: y));
				} else if (Board.touch && Info.hook) {
					show_info_window();
				}
				break;
			case SDL_WINDOWEVENT:
				switch(event.window.event) {
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				case SDL_WINDOWEVENT_MAXIMIZED:
				case SDL_WINDOWEVENT_MINIMIZED:
					status.update_needed = true;
				}
			}
		}
	}
	if (board_running())
		board_save(SAVE_PATH);
}

static int cur_score = -1;
static int cur_mul = 0;
static int score_x = SCORE_X + ((SCORE_W) / 2);
static int score_w = 0;

static void show_score(void)
{
	char buff[64];
	int w, x;
	int new_score = board_score();
	static int timer = 0;
	
	if (board_score_mul() < cur_mul)
		cur_mul = board_score_mul();
	
	if (new_score > cur_score || cur_score == -1) {
		if ((board_score_mul() > cur_mul) && (board_score_mul() > 1) && !(timer % BONUS_BLINKS)) {
			if (!timer) {
				snd_play(SND_BONUS, 1);
			}
			snprintf(buff, sizeof(buff), "Bonus x%d", board_score_mul());
			w = gfx_chars_width(font, buff);
			x = SCORE_X + ((SCORE_W - w) / 2);
			gfx_draw_bg(bg, _min(score_x, x), SCORE_Y, _max(score_w, w), FONT_HEIGHT);
			if ((timer / BONUS_BLINKS) & 1)
				gfx_draw_chars(font, buff, x, SCORE_Y, 0);
			gfx_update();
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
			gfx_draw_bg(bg, _min(score_x, x), SCORE_Y, _max(score_w, w), FONT_HEIGHT);
			gfx_draw_chars(font, buff, x, SCORE_Y, 0);
			gfx_update();
			score_x = x;
			score_w = w;
		}
	}
	if (timer == 1) {
		cur_mul = board_score_mul();
	}
	if (timer)
		timer--;
}

static void game_savehiscores(const char *path)
{
	FILE * file = fopen(SCORES_PATH, "w");
	if (file != NULL) {
		for (int i = 0; i < HISCORES_NR; i++) {
			if (fwrite(&game_hiscores[i], sizeof(game_hiscores[i]), 1, file) != 1)
				break;
		}
		fclose(file);
	}
}

static void game_loadhiscores(const char *path)
{
	FILE * file = fopen(SCORES_PATH, "r");
	if (file != NULL) {
		for (int i = 0; i < HISCORES_NR; i++) {
			if (fread(&game_hiscores[i], sizeof(game_hiscores[i]), 1, file) != 1)
				break;
		}
		fclose(file);
	}
}

static void game_loadprefs(const char *path)
{
	FILE * file = fopen(PREFS_PATH, "rb");
	if (file != NULL) {
		fread(&Settings, sizeof(struct __SET__), 1, file);
		fclose(file);
	}
}

static void game_saveprefs(const char *path)
{
	FILE * file = fopen(PREFS_PATH, "wb");
	if (file != NULL) {
		fwrite(&Settings, sizeof(struct __SET__), 1, file);
		fclose(file);
	}
}

static bool check_hiscores(int score)
{
	bool stat = false;
	for (int i = 0; i < HISCORES_NR; i++) {
		if ((stat = score > game_hiscores[i])) {
			for (int k = HISCORES_NR - 1; k > i; k--) {
				game_hiscores[k] = game_hiscores[k - 1];
			}
			game_hiscores[i] = score;
			game_savehiscores(SCORES_PATH);
			break;
		}
	}
	return stat;
}

static void show_hiscores(void)
{
	char buff[64];
	gfx_draw_bg(bg, SCORES_X, SCORES_Y, SCORES_W, HISCORES_NR * FONT_HEIGHT);
	for (int w, i = 0; i < HISCORES_NR; i++) {
		snprintf(buff, sizeof(buff),"%d", i + 1);
		w = gfx_chars_width(font, buff);
		snprintf(buff, sizeof(buff),"%d.", i + 1);
		gfx_draw_chars(font, buff, SCORES_X + FONT_WIDTH - w, SCORES_Y + i * FONT_HEIGHT, 0);
		snprintf(buff, sizeof(buff),"%d", game_hiscores[i]);
		w = gfx_chars_width(font, buff);
		gfx_draw_chars(font, buff, SCORES_X + SCORES_W - w, SCORES_Y + i * FONT_HEIGHT, 0);
	}
	gfx_update();
}

static void game_message(const char *str, bool board)
{
	int w = gfx_chars_width(font, str);
	int x = BOARD_X + (BOARD_WIDTH - w) / 2;
	int y = BOARD_Y + (BOARD_HEIGHT - FONT_HEIGHT) / 2;
	if (!board) {
		x = (SCREEN_W - w ) / 2;
		y = (SCREEN_H - FONT_HEIGHT) / 2;
	}
	gfx_draw_chars(font, str, x, y, 0);
	gfx_update();
}

static void game_prep(void)
{
	game_loadhiscores(SCORES_PATH);
	game_lock();
	
	srand(time(NULL));
	gfx_draw_bg(bg, 0, 0, SCREEN_W, SCREEN_H);
	
	Restart.w = gfx_chars_width(font, Restart.name);
	Restart.h = FONT_HEIGHT;
	Restart.x = SCORES_X + (SCORES_W - Restart.w) / 2;
	Restart.y = SCREEN_H - FONT_HEIGHT * 2 - FONT_HEIGHT / 2;
	gfx_draw_chars(font, Restart.name, Restart.x, Restart.y, 0);
	
	Track.x = Music.w + Vol.w + Track.temp / 2;
	Track.y = SCREEN_H - Track.temp - Track.temp / 2;
	//Track.off = gfx_load_ttf_font("manaspc.ttf", " ", Track.temp);
	
	Timer.w = gfx_chars_width(font, Timer.name) * Timer.temp;
	Timer.h = FONT_HEIGHT * Timer.temp;
	Timer.x = SCREEN_W - Timer.w;
	Timer.y = SCREEN_H - Timer.h;
	gfx_draw_chars(font, Timer.name, Timer.x, Timer.y, Timer.temp);
	
	show_hiscores();
	
	Music.temp = rand() % TRACKS_COUNT;
	  Vol.temp = Settings.volume * Vol.w / 256 + Music.w;
	
	Music.x = 0;          Music.y = SCREEN_H - Music.h;
	 Loop.x = Music.w + 5; Loop.y = SCREEN_H - Music.h - Loop.h;
	 Info.x = 0;           Info.y = SCREEN_H - Music.h - Info.h;
	  Vol.x = Music.w;      Vol.y = SCREEN_H - Vol.h;
	
	draw_Button_for(&Music, (Settings.music != -1));
	draw_Button_for(&Loop, Settings.loop);
	draw_Button_for(&Info, Info.hook);
	draw_Volume_bar();
	if(Track.name[0])
		draw_Track_title();
	score_x = SCORE_X + (SCORE_W / 2);
	score_w = 0;
	game_unlock();
	
	game_restart(
		board_load(SAVE_PATH)
	);
}

static void game_restart(bool clean)
{
	game_lock();
	stop_GameTimer();
	status.update_needed = status.game_over = false;
	cur_score = -1;
	cur_mul   = 0;
	if (!clean) {
		fetch_game_board();
		cur_score += board_score();
		cur_mul   += board_score_mul();
	} else {
		board_init();
		draw_Timer_digit((Settings.lastTime = -1), NULL);
	}
	game_init();
	draw_board();
	gfx_update();
	show_score();
	show_hiscores();
	start_GameTimer();
	game_unlock();
}

#if defined WINDOWS

int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    PSTR szCmdLine,
                    int iCmdShow) {

	char gameData_dir [ PATH_MAX ];
	size_t c = sizeof(gameData_dir);
	GetModuleFileName(NULL, gameData_dir, c);

	char config_dir[ PATH_MAX ];
	SHGetFolderPath( NULL,
		CSIDL_FLAG_CREATE | CSIDL_LOCAL_APPDATA,
		NULL,
		0,
		(LPTSTR)config_dir );
	
	strcpy( PREFS_PATH, ( strlen( config_dir ) ? config_dir : "." ));
	fprintf(stderr,"prefsdir:   %s\n", appdir);

#else
	
int main(int argc, char **argv) {
	
	char config_dir[ PATH_MAX ];
	struct passwd *pw = getpwuid(getuid());
	
	char gameData_dir[ PATH_MAX ];
	uint32_t c = sizeof(gameData_dir);
	
	#if defined MACOS
		_NSGetExecutablePath(gameData_dir, &c);
		puts("This is macOS");
	#elif defined LINUX
		if (readlink("/proc/self/exe", gameData_dir, c) != -1);
			puts("This is Linux");
	#elif defined SOLARIS
		strcpy(gameData_dir, getexecname());
		puts("This is Solaris");
	#elif defined FREEBSD
		sysctl({CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1}, 4, gameData_dir, &c, NULL, 0);
		puts("This is FreeBSD");
	#elif defined NETBSD
		if (readlink("/proc/curproc/exe", gameData_dir, c) != -1);
			puts("This is NetBSD");
	#else
		strncpy(gameData_dir, argv[0], c);
	#endif
	
	#if defined MAEMO
		putenv("SDL_VIDEO_X11_WMCLASS=lines");
		strcpy(config_dir, "/home/user/.config");
	#else
		strcpy(config_dir, (pw ? pw->pw_dir : "/tmp"));
		strcat(config_dir, "/.config");
		strcpy(SAVE_PATH  , config_dir); strcat(SAVE_PATH  , "/color-lines/save");
		strcpy(SCORES_PATH, config_dir); strcat(SCORES_PATH, "/color-lines/scores");
		strcpy(PREFS_PATH , config_dir); strcat(PREFS_PATH , "/color-lines/prefs");
	#endif
	
	if (access(config_dir, F_OK ) == -1)
		mkdir(config_dir, 0755);
	
	strcat(config_dir, "/color-lines");
	
	if (access(config_dir, F_OK ) == -1)
		mkdir(config_dir, 0755);
	
	do {
		c--;
	} while (gameData_dir[c] != '/');
	
	gameData_dir[c + 1] = '\0';
	
#endif
	
	game_mutex = SDL_CreateMutex();
	if (gfx_init(gameData_dir))
		return 1;
	
	game_loadprefs(PREFS_PATH);
	
	if (!(font = gfx_load_font("fnt.png", FONT_WIDTH)))
		return 1;
	
	FONT_HEIGHT = gfx_font_height(font);
	
	if (snd_init(gameData_dir)) {
		Music.hook = true;
	} else {
		snd_volume(Settings.volume);
		if (Settings.music > -1)
			snd_music_start(Settings.music, Track.name);
	}
	
	game_message("Loading...", false);
	
	if (load_bg() || load_balls() || load_cell())
		return 1;
#ifndef	MAEMO
	if (load_pb())
		return 1;
#endif
	
	if (load_Img_for(&Music) || load_Img_for(&Info) || load_Img_for(&Loop) || load_Img_for(&Vol))
		return 1;
	
	game_prep();
	game_loop();
	
	game_lock();
	stop_GameTimer();
	free_bg();
	free_cell();
	free_balls();
	free_Img_for( &Music );
	free_Img_for( &Info );
	free_Img_for( &Loop );
	free_Img_for( &Vol );
#ifndef MAEMO
	free_pb();
#endif
	game_unlock();
	snd_done();
	gfx_done();
	gfx_font_free(font);
	SDL_DestroyMutex(game_mutex);
	if (status.store_prefs)
		game_saveprefs(PREFS_PATH);
	return 0;
}
