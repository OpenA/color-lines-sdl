#include "main.h"
#include "src/game.c"

#define GFX_UPDATE    0x321
#define SCORES_W     (BOARD_X - TILE_W - SCORES_X - 40)
#define SCORE_W      (BOARD_X - 23)
#define BOARD_WIDTH  (BOARD_W * TILE_W)
#define BOARD_HEIGHT (BOARD_H * TILE_H)

char path_gfx[ SYS_PATH_L ];

/* Game Hiscores */
static record_t Records[HISCORES_NR] = {
	DEFAULT_RECORD(50), DEFAULT_RECORD(40),
	DEFAULT_RECORD(30), DEFAULT_RECORD(20), DEFAULT_RECORD(10)
};

/* BGM Plylist */
static struct playlist {
	const char *file, *title;
} trackList[] = { MUSIC_TRACKS_LIST() };

static sound_t Sound;
static prefs_t Prefs = DEFAULT_PREFS();
static desk_t Board;
static move_t Move;
static path_t GameDir;
static elem_t Elem[UI_ELEMENTS_N];

static struct __STAT__ {
	bool running, game_over, update_needed, store_prefs, store_hiscore;
} status = {
	.store_hiscore = false,
	.store_prefs   = false,
	.update_needed = false,
	.game_over     = false,
	.running       = true
};

typedef struct __ELS__ {
	char name[ 128 ];
	int x, y, w, h;
	img_t on, off;
	float temp;
	bool touch, hook;
} elemen_t;

static elemen_t Restart = { .name = "Restart" };
static elemen_t Score   = { .x = SCORE_X + SCORE_W / 2, .y = SCORE_Y };
static elemen_t Track   = { .temp = 14 };
static elemen_t Timer   = { .name = "00:00", .temp = 0.5 };
static elemen_t Loop    = { .name = "loop" };
static elemen_t Info    = { .name = "info" };

int   moving_nr = 0;
img_t pb_logo = NULL;
img_t bg_saved = NULL;
img_t balls[BALLS_NR][ALPHA_STEPS];
img_t resized_balls[BALLS_NR][SIZE_STEPS];
img_t jumping_balls[BALLS_NR][JUMP_STEPS];

#define iBG   Elem[UI_Bg  ].img
#define iCELL Elem[UI_Cell].img

#define is_OnElem(el,x,y) !( \
	x < el.x || y < el.y || x >= el.x + el.w || y >= el.y + el.h \
)

#define iget_W(el) el.on->w
#define iget_H(el) el.on->h
#define free_Img(el) \
	gfx_free_image(el.on), gfx_free_image(el.off)

#define draw_Button(el, pss)\
	gfx_draw_bg(iBG, el.x, el.y, el.w, el.h);\
	gfx_draw((pss ? el.on : el.off), el.x, el.y)

#define draw_Msg(str, s) {\
	int w = gfx_chars_width(str), x = BOARD_X + (BOARD_WIDTH  - w) / 2;\
	int h = gfx_font_height()   , y = BOARD_Y + (BOARD_HEIGHT - h) / 2;\
	gfx_draw_text(str, x, y, s);\
}

static void draw_Track_title(void)
{
	gfx_draw_bg(iBG, Track.x, Track.y, Track.w, Track.h);
	Track.on = gfx_draw_ttf_text(Track.name);
	Track.w  = gfx_img_w(Track.on);
	Track.h  = gfx_img_h(Track.on);
	gfx_draw_wh(Track.on, Track.x, Track.y, Track.w, Track.h);
	gfx_update();
}

/* <==== Game Timer ====> */
SDL_TimerID gameTimerId;

Uint32 draw_Timer_digit(Uint32 ival, void *_)
{
	unsigned int t = board_get_time(&Board) + (ival > 0);
	                 board_set_time(&Board, t);
	
	sprintf(Timer.name, "%02d:%02d", (int)(t / 60), t % 60);
	gfx_draw_bg(iBG, Timer.x, Timer.y, Timer.w, Timer.h);
	
	Timer.w = gfx_chars_width(Timer.name) * Timer.temp;
	Timer.x = SCREEN_W - Timer.w;
	
	gfx_draw_text(Timer.name, Timer.x, Timer.y, Timer.temp);
	status.update_needed = true;
	
	return ival;
}
void start_GameTimer()
{
	gameTimerId = SDL_AddTimer(1e3, draw_Timer_digit, NULL);
}
void stop_GameTimer()
{
	SDL_RemoveTimer( gameTimerId );
} /* <=== END ===> */

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
	{ "ball_flush" , &balls[8][ALPHA_STEPS - 1] },
	{ "ball_brush" , &balls[9][ALPHA_STEPS - 1] }, 
	{ "ball_bomb1" , &balls[10][ALPHA_STEPS - 1] },
	{ "pb_logo", &pb_logo },
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
		int h = gfx_font_height();
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
				w += gfx_chars_width(word);
			}
			if (w > BOARD_WIDTH)
				break;
			if (img) {
				gfx_draw(img, x, y);
				x += gfx_img_w(img) + gfx_chars_width(" ");
				h = _max(h, gfx_img_h(img));
			} else {	
				strcat(word, " ");	
				gfx_draw_text(word, x, y, 0);
				w += gfx_chars_width(" ");
				x += gfx_chars_width(word);
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
			gfx_draw_text("MORE", BOARD_X, BOARD_Y + BOARD_HEIGHT - h, 0);
			break;
		}
	}
	return ptr;
}

char info_text[] = " -= Color Lines v"CL_VERSION_STRING". =-\n\n"
	"Try to arrange balls of the same color in vertical, "
	"horizontal or diagonal lines."
	"To move a ball click on it to select, "
	"then click on destination square. Once line has five or more balls "
	"of same color, that line is removed from the board and you earn score points. "
	"After each turn three new balls randomly added to the board.\n\n"
	"There are four bonus balls.\n\n"
	"ball_joker is a joker that can be used like any color ball. Joker also multiply score by two.\n\n"
	"ball_flush acts like a joker, but when applied it also removes all balls of the same color from the board.\n\n"
	"ball_brush paints all nearest balls in the same color.\n\n"
	"ball_bomb1 is a bomb. It always does \"boom\"!!!\n\n"
	"The game is over when the board is filled up.\n\n"
	"CODE: Peter Kosyh <gloomy_pda@mail.ru>\n\n"
	"GRAPHICS: Peter Kosyh and some files from www.openclipart.org\n\n"
	"SOUNDS: Stealed from some linux games...\n\n"
	"MUSIC: \"Satellite One\" by Purple Motion.\n\n"
	"FIRST TESTING & ADVICES: my wife Ola, Sergey Kalichev...\n\n"
	"PORTING TO M$ Windoze: Ilja Ryndin from Pinebrush\n pb_logo www.pinebrush.com\n\n"
	"SPECIAL THANX: All UNIX world... \n\n"
	" Good Luck!";

static const char *cur_text  = info_text;
static const char *last_text = NULL;

static bool show_info_window(void)
{
	last_text = cur_text;
	if (!bg_saved && !(
		 bg_saved = gfx_grab_screen(
			BOARD_X - TILE_W - POOL_SPACE,
			BOARD_Y,
			BOARD_WIDTH + TILE_W + POOL_SPACE,
			BOARD_HEIGHT))){
	} else if (!*cur_text) {
		cur_text = info_text;
	} else {
		gfx_draw_bg(iBG, BOARD_X - TILE_W - POOL_SPACE, BOARD_Y, 
				BOARD_WIDTH + TILE_W + POOL_SPACE, BOARD_HEIGHT);
		stop_GameTimer();
		cur_text = game_print(cur_text);
		draw_Button(Info, (Info.hook = true));
		return true;
	}
	return false;
}

static bool hide_info_window(void)
{
	cur_text = last_text ?: info_text;
	gfx_draw(bg_saved, BOARD_X - TILE_W - POOL_SPACE, BOARD_Y);
	draw_Button(Info, (Info.hook = false));
	gfx_free_image(bg_saved);
	bg_saved = 0;
	start_GameTimer();
	return true;
}

typedef enum {
	standing = 0,
	fadein,
	fadeout,
	changing,
	jumping,
	moving,
} effec_t;

typedef struct {
	cell_t cell, cell_from;
	effec_t effect, step;
	bool reUse, reDraw;
	int x, y, tx, ty, id;
} pool_t;

pool_t game_board[BOARD_W][BOARD_H];
pool_t game_pool[POOL_SIZE];

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
static void game_restart(bool clean);
static int set_volume(int x);

static void enable_effect(int x, int y, int effect)
{
	pool_t *b = (x == -1 ? &game_pool[y] : &game_board[x][y]);
	b->x = x;
	b->y = y;
	b->reUse  = true;
	b->reDraw = true;
	b->effect = effect;
	b->step   = 0;
}

static void disable_effect(int x, int y)
{
	pool_t *b = (x == -1 ? &game_pool[y] : &game_board[x][y]);
	b->reDraw = true;
	b->effect = 0;
}

void game_move_ball(void)
{
	static int x, y;
	int tx = Move.to.x,
	    ty = Move.to.y;

	ball_t b = board_get_selected(&Board, &Move, &x, &y);

	if (b != no_ball && !game_board[x][y].effect && game_board[x][y].cell && game_board[x][y].reUse) {
		enable_effect(x, y, jumping);
		game_board[x][y].step = 0;
	}
	if (board_has_moves(&Move) && !game_board[tx][ty].cell) {
		moving_nr++;
		game_board[tx][ty].reUse = false;
		game_board[tx][ty].cell = game_board[x][y].cell;
		enable_effect(tx, ty, moving);
		game_board[tx][ty].tx = tx;
		game_board[tx][ty].ty = ty;
		game_board[tx][ty].id = NEW_MPATH_ID(x,y);
		game_board[tx][ty].x = x;
		game_board[tx][ty].y = y;
		disable_effect(x, y);
		game_board[x][y].reDraw = false;
		game_board[x][y].reUse = false;
		game_board[x][y].cell = 0;
	}
}

void game_process_pool(void)
{
	for (int x = 0; x < BOARD_POOL_N; x++) {

		cell_t  c = x < Move.pool_i ? no_ball : board_get_pool(&Board, x);
		pool_t *b = &game_pool[x];

		if (c && !b->cell && !b->effect) {
			// appearing
			enable_effect(-1, x, fadein);
			b->cell = c;

		} else if (!c && b->cell && !b->effect) {
			// disappearing
			enable_effect(-1, x, fadeout);
		}
	}
}

void game_process_board(void)
{
	int x, y, play_snd = 0;

	for (y = 0; y < BOARD_DESK_H; y++) {
		for (x = 0; x < BOARD_DESK_W; x++) {

			cell_t  c = board_get_cell(&Board, x, y) & MSK_BALL;
			pool_t *b = &game_board[x][y];

			if (c && !b->cell && !b->effect) {
				// appearing
				enable_effect(x, y, fadein);
				b->cell = c;

			} else if (c && b->cell && c != b->cell && !b->effect) {
				// staging
				enable_effect(x, y, changing);
				b->cell_from = b->cell;
				b->cell = c;

			} else if (!c && b->cell && (!b->effect || b->effect == jumping)) {
				// disappearing
				enable_effect(x, y, fadeout);
				if (b->cell == ball_bomb1 ) {
					play_snd = SND_Boom;
				} else
				if (b->cell == ball_brush) {
					play_snd = SND_Paint;
				} else
				if (play_snd == 0) {
					play_snd = SND_Fadeout;
				}
			}
		}
	}
	if (play_snd)
		sound_sfx_play(&Sound, play_snd, 1);
}

unsigned short game_display_board(void)
{
	unsigned short out = 0;

	int tmpx, tmpy, x1, y1, dx, dy, dist;
	pool_t *b;

	for (int y = 0; y < BOARD_H; y++) {
		for (int x = 0; x < BOARD_W; x++) {

			b = &game_board[x][y];

			if (!b->reDraw)	
				continue;

			out++;

			switch (b->effect) {
			case standing:
				out--;
				draw_cell(x, y);
				if (b->reUse) {
					draw_ball(b->cell - 1, x, y);
				} else
					b->cell = 0;
				update_cell(x, y);
				b->reDraw = false;
				break;
			case fadein:
				out--;
				if (!board_has_mpath(&Move, b->x, b->y)) {
					draw_cell(b->x, b->y);
					draw_ball_size(b->cell - 1, b->x, b->y, b->step); 
					b->step ++;
					if (b->step >= SIZE_STEPS) {
						disable_effect(x, y);
					}
					update_cell(b->x, b->y);
				}
				break;
			case jumping:
				out--;
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
				b->step++;
				update_cell(b->x, b->y);
				if (dist == 2 * JUMP_STEPS && (!board_get_selected(&Board, &Move, &tmpx, &tmpy) || tmpx != b->x || tmpy != b->y)) {
					disable_effect(x, y);
				} else if (b->step >= 3*JUMP_STEPS * 19 + 2*JUMP_STEPS) {
					disable_effect(x, y);
					board_del_select(&Move);
				}
				break;
			case moving:
				x1 = b->x;
				y1 = b->y;
				draw_cell(b->x, b->y);
				board_follow_path(&Move, b->x, b->y, &tmpx, &tmpy, b->id);
				draw_cell(tmpx, tmpy);
				dist = abs(b->tx - b->x) + abs(b->ty - b->y);
				if (dist <= 2) {
					b->step += (BALL_STEP * (TILE_W * dist - b->step)) / (TILE_W * 2) ?: 1;
				} else
					b->step += BALL_STEP;
				dx = (tmpx - b->x) * b->step;
				dy = (tmpy - b->y) * b->step;
				if (abs(dx) >= TILE_W || abs(dy) >= TILE_H) {
					board_del_mpath(&Move, b->x, b->y);
					b->x = tmpx;
					b->y = tmpy;
					dx = dy = 0;
					b->step = 0;
				}
				draw_ball_offset(b->cell - 1, b->x, b->y, dx, dy); 
				update_cells(x1, y1, tmpx, tmpy);
				if (b->x == b->tx && b->y == b->ty) {
					moving_nr--;
					board_del_mpath(&Move, b->x, b->y);
					b->reDraw = true;
					b->effect = 0;
					b->reUse  = true;
				}
				break;
			case changing:
				draw_cell(b->x, b->y);
				draw_ball_alpha(b->cell_from - 1, b->x, b->y, 
					ALPHA_STEPS - b->step - 1); 
				draw_ball_alpha(b->cell - 1, b->x, b->y, b->step); 
				b->step++;
				if (b->step >= ALPHA_STEPS - 1) {
					disable_effect(x, y);
				}
				update_cell(b->x, b->y);
				break;
			case fadeout:
				draw_cell(b->x, b->y);
				draw_ball_alpha(b->cell - 1, b->x, b->y, ALPHA_STEPS - b->step - 1); 
				b->step++;
				if (b->step == ALPHA_STEPS) {
					disable_effect(x, y);
					b->reUse = false;
					b->cell  = 0;
				}
				update_cell(b->x, b->y);
			default:
  				break;
			}
		}
	}
	return out;
}

void game_display_pool(void)
{
	pool_t *b;

	for (int x = 0; x < POOL_SIZE; x++) {

		b = &game_pool[x];

		if (!b->reDraw)
			continue;

		switch (b->effect) {
		case 0:
			draw_cell(-1, x);
			if (b->reUse)
				draw_ball(b->cell - 1, -1, x); 
			else
				b->cell = 0;
			update_cell(-1, x);
			b->reDraw = false;
			break;
		case fadein:
			draw_cell(-1, b->y);
			draw_ball_size(b->cell - 1, -1, b->y, b->step); 
			b->step++;
			if (b->step >= SIZE_STEPS) {
				disable_effect(-1, x);
			}
			update_cell(-1, x);
			break;
		case fadeout:
			draw_cell(-1, b->y);
			draw_ball_alpha(b->cell - 1, -1, b->y, ALPHA_STEPS - b->step - 1); 
			b->step ++;
			if (b->step == ALPHA_STEPS) {
				disable_effect(-1, x);
				b->reUse = false;
				b->cell = 0;
			}
			update_cell(-1, b->y);
		default:
  			break;
		}	
	}
}

# define load_ball(path, ball, n) \
	ball = gfx_load_image(path, SDL_TRUE);\
	push_ball(n, ball);\
	gfx_free_image(ball);

void push_ball(ball_t b, img_t ball)
{
	int k, n = b - 1;

	balls[n][ALPHA_STEPS - 1] = ball;

	for (k = 1; k < ALPHA_STEPS; k++) {
		int a = (255 * 100) / (ALPHA_STEPS * 100 / k);
		balls[n][k - 1] = gfx_set_alpha(ball, a);
	}
	for (k = 1; k <= SIZE_STEPS; k++) {
		float xs = (float)1.0 / ((float)SIZE_STEPS / (float)k);
		resized_balls[n][k - 1] = gfx_scale(ball, xs, xs);
	}
	for (k = 1; k <= JUMP_STEPS; k++) {
		float ys = 1.0 - (((float)(1.0 - JUMP_MAX) / (float)JUMP_STEPS) * k),
		      xs = 1.0 + (1.0 - ys);
		jumping_balls[n][k - 1] = gfx_scale(ball, xs, ys);
	}
}

static void cell_to_screen(int x, int y, int *ox, int *oy)
{
	if (x == -1) {
		*ox = BOARD_X - TILE_W - POOL_SPACE;
		*oy = y * TILE_H + TILE_H * (BOARD_H - POOL_SIZE)/2 + BOARD_Y;
	} else {
		*ox = x * TILE_W + BOARD_X;
		*oy = y * TILE_H + BOARD_Y;
	}
}

static void music_start(int n, path_t game_dir)
{
	if ( n > MUSIC_TRACKS_N || !(Prefs.flags & FL_PREF_BGM_PLAY))
		return;

	cstr_t file = trackList[n].file,
	      title = trackList[n].title;

	sys_set_dpath(game_dir, MUSIC_DIR);

	if (sound_bgm_play(&Sound, n, sys_get_spath(game_dir, file))) {
		sound_set_bgm_onEnd(track_switch);
		strcpy(Track.name, title);
	}
}

static void toggle_music()
{
	bool has_play = game_prefs_has(&Prefs, FL_PREF_BGM_PLAY);
	     has_play ^= 1;

	if (has_play) {
		game_prefs_add(&Prefs, FL_PREF_BGM_PLAY);
		music_start(Prefs.track_num, GameDir);
		if (!Track.on)
			draw_Track_title();
	} else {
		game_prefs_del(&Prefs, FL_PREF_BGM_PLAY);
		sound_bgm_stop(&Sound);
	}
	status.store_prefs = true;
	ui_draw_Tico(&Elem[UI_Music], iBG, screen, has_play);
}

static void toggle_loop() {

	bool loop = game_prefs_has(&Prefs, FL_PREF_BGM_LOOP);
	     loop ^= 1;

	if (loop)
		game_prefs_add(&Prefs, FL_PREF_BGM_LOOP);
	else
		game_prefs_del(&Prefs, FL_PREF_BGM_LOOP);
	status.store_prefs = true;
	draw_Button(Loop, loop);
}

void track_switch(void)
{
	int  tnum = Prefs.track_num;
	bool loop = game_prefs_has(&Prefs, FL_PREF_BGM_LOOP),
	     pass = false;

	if (tnum >= 0 && !(loop && !Track.hook)) {
		if((tnum + 1) >= MUSIC_TRACKS_N)
			tnum = 0;
		else
			tnum++;
		Prefs.track_num = tnum;
		status.store_prefs = pass = true;
	}
	music_start(tnum, GameDir);
	if (pass) {
		gfx_free_image(Track.on);
		draw_Track_title();
	}
}

static int set_volume(int x)
{
	int vol_i, vol_w = ui_get_el_bounds(&Elem[UI_Volume]).w,
	    pos_x, mus_w = ui_get_el_bounds(&Elem[UI_Music ]).w;

	if (x <= mus_w)
		pos_x = 0,
		vol_i = 0;
	else if (x >= (mus_w + vol_w))
		pos_x = vol_w,
		vol_i = 100;
	else
		pos_x = x - mus_w,
		vol_i = (pos_x * 100) / vol_w;
# ifdef DEBUG
	if (Prefs.bgm_vol != vol_i)
		printf("volume change: %d (%f)\n", vol_i, FP_VOL_STEP * vol_i);
# endif
	if(!sound_has_not_ready(&Sound)) {
		sound_set_bgm_volume(&Sound, vol_i);
		sound_set_sfx_volume(&Sound, vol_i);
	}
	ui_set_el_ivalue(&Elem[UI_Volume], mus_w + pos_x);
	ui_draw_Hbar(&Elem[UI_Volume], iBG, screen, pos_x);
	Prefs.bgm_vol = vol_i;
	Prefs.sfx_vol = vol_i;
	status.store_prefs = true;
	return pos_x + mus_w;
}

void draw_cell(int x, int y)
{
	int nx, ny;
	cell_to_screen(x, y, &nx, &ny);
	gfx_draw_bg(iBG, nx, ny, TILE_W, TILE_H);
	gfx_draw(iCELL, nx, ny);
}

void update_cell(int x, int y)
{
	status.update_needed = true;
	int nx, ny;
	cell_to_screen(x, y, &nx, &ny);
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
			game_board[x1][y1].reDraw = true;
	}
}

void update_all(void)
{
	if (status.update_needed) {
		SDL_Event upd_event;
		upd_event.type = GFX_UPDATE;
		SDL_PushEvent(&upd_event);
	}
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
	int diff = (TILE_W - img->w) / 2;
	cell_to_screen(x, y, &nx, &ny);
	gfx_draw(img, nx + diff, ny + diff);
}

void draw_ball_jump(int n, int x, int y, int num)
{
	int nx, ny;
	SDL_Surface *img = (SDL_Surface *)jumping_balls[n][num];
	int diff  = (40 - img->h) + 5;
	int diffx = (TILE_W - img->w) / 2;
	cell_to_screen(x, y, &nx, &ny);
	gfx_draw(img, nx + diffx, ny + diff);
}

static int fetch_game_board(bool rel)
{
	memset(game_board, 0, sizeof( game_board ));
	memset(game_pool , 0, sizeof( game_pool  ));

	int x, y, nb = 0;

	for (y = 0; y < BOARD_H; y++) {
		for (x = 0; x < BOARD_W; x++) {
			cell_t b = board_get_cell(&Board, x, y);
			if ((b & MSK_BALL) != no_ball) {
				game_board[x][y].x = x;
				game_board[x][y].y = y;
				game_board[x][y].reUse = true;
				nb++;
			}
			game_board[x][y].cell = b;
			game_board[x][y].reDraw = false;
		}
	}
	return nb;
}

static void draw_board(void)
{
	for (int x, y = 0; y < BOARD_H; y ++) {
		for (x = 0; x < BOARD_W; x++) {
			draw_cell(x, y);
			if (game_board[x][y].cell && game_board[x][y].reUse) {
				draw_ball(game_board[x][y].cell - 1, x, y); 
			}
		}
	}
}

Uint32 gameHandler(Uint32 interval, void *_)
{
	if (status.running) {
		if (!Info.hook) {
			game_move_ball();
			game_process_board();
			game_process_pool();
			show_score();
			game_display_pool();
			if (!game_display_board()) { /* nothing todo */
				if (!board_next_move(&Board, &Move) && !status.update_needed && !status.game_over) {
					stop_GameTimer();
					draw_Msg("Game Over!", 0);
					status.game_over = true;
					if (check_hiscores(board_get_score(&Board))) {
						sound_sfx_play(&Sound, SND_Hiscore, 1);
						show_hiscores();
					} else {
						sound_sfx_play(&Sound, SND_Gameover, 1);
					}
					status.update_needed = true;
				}
			}
			update_all();
		}
	} else
		return 0;
	return interval;
}

static void game_loop() {
	// Main loop
	SDL_Event event;

	bool Board_touch = false, refresh = false,
	       Vol_touch = false, running = true,
	       Vol_hook  = false;

	SDL_AddTimer(20, &gameHandler, NULL);
	
	while (status.running) {
		if (SDL_WaitEvent(&event)) {
			//fprintf(stderr,"event %d\n", event.type);
			int x = event.button.x;
			int y = event.button.y;
			if (event.key.state == SDL_PRESSED) {
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					status.running = false;
				}
			}
			switch (event.type) {
			case GFX_UPDATE: refresh = true;
				break;
			case SDL_QUIT: // Quit the game
				status.running = false;
				break;
			case SDL_MOUSEMOTION:
				Board_touch = !(x < BOARD_X || y < BOARD_Y || x >= BOARD_X + BOARD_WIDTH || y >= BOARD_Y + BOARD_HEIGHT);
				  Vol_touch = IS_ON_ELEM(&Elem[UI_Volume], x, y);
				if (Vol_hook)
					set_volume(x), refresh = true;
				break;
			case SDL_MOUSEBUTTONUP:
				Track.hook = Vol_hook = false;
				break;
			case SDL_MOUSEBUTTONDOWN: // Button pressed
				if (Board_touch) {
					if (Info.hook) {
						refresh = show_info_window();
					} else if (!board_has_over(&Move)) {
						board_select_ball(&Board, &Move,
							(x - BOARD_X) / TILE_W,
							(y - BOARD_Y) / TILE_H);
					} else {
						stop_GameTimer();
						game_restart(false);
					}
				}
				else if (Vol_touch) {
					Vol_hook = refresh = true;
					set_volume(x);
				}
				else if (is_OnElem(Restart, x, y)) {
					stop_GameTimer();
					board_wait_finish(&Board, &Move);
					if (!board_has_over(&Move)) {
						bool ok = check_hiscores(board_get_score(&Board));
						sound_sfx_play(&Sound, ok ? SND_Hiscore : SND_Gameover, 1);
					}
					game_restart(false);
				}
				else if (IS_ON_ELEM(&Elem[UI_Music], x, y)) {
					toggle_music();
					refresh = true;
				}
				else if ((Track.hook = is_OnElem(Track, x, y))) {
					track_switch();
				}
				else if (is_OnElem(Loop, x, y)) {
					toggle_loop();
					refresh = true;
				}
				else if (is_OnElem(Info, x, y)) {
					refresh = Info.hook ? hide_info_window() : show_info_window();
				}
				break;
			case SDL_MOUSEWHEEL:
				if (Vol_touch) {
					x = ui_get_el_ivalue(&Elem[UI_Volume]) + (x ?: y);
					set_volume(x);
					refresh = true;
				} else if (Board_touch && Info.hook) {
					refresh = show_info_window();
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
			if (refresh)
				gfx_update(), refresh = false;
		}
	}
	stop_GameTimer();
	board_wait_finish(&Board, &Move);
}

int cur_score = -1;
int cur_mul = 0;

static void show_score(void)
{
	static unsigned short timer = 0;

	int w, x, h   = gfx_font_height();
	int new_score = board_get_score(&Board),
	         dmul = board_get_delta(&Board) - 1;

	if (dmul < cur_mul)
		cur_mul = dmul;

	if (new_score > cur_score || cur_score == -1) {
		if (dmul > cur_mul && dmul > 1 && !(timer % BONUS_BLINKS)) {
			snprintf(Score.name, sizeof(Score.name), "Bonus x%d", dmul);
			w = gfx_chars_width(Score.name);
			x = SCORE_X + ((SCORE_W - w) / 2);
			gfx_draw_bg(iBG, _min(Score.x, x), SCORE_Y, _max(Score.w, w), h);
			if ((timer / BONUS_BLINKS) & 1)
				gfx_draw_text(Score.name, x, SCORE_Y, 0);
			status.update_needed = true;
			Score.x = x;
			Score.w = w;
			if (!timer) {
				sound_sfx_play(&Sound, SND_Bonus, 1);
				timer = BONUS_TIMER;
			}
		} else if (!timer) {
			snprintf(Score.name, sizeof(Score.name), "SCORE: %d", ++cur_score);
			w = gfx_chars_width(Score.name);
			x = SCORE_X + ((SCORE_W - w) / 2);
			gfx_draw_bg(iBG, _min(Score.x, x), SCORE_Y, _max(Score.w, w), h);
			gfx_draw_text(Score.name, x, SCORE_Y, 0);
			status.update_needed = true;
			Score.x = x;
			Score.w = w;
			if (cur_score != new_score) {
				sound_sfx_play(&Sound, SND_Click, 1);
			}
		}
	}
	if (timer == 1) {
		cur_mul = dmul;
	}
	if (timer)
		timer--;
}

static bool check_hiscores(int score)
{
	bool stat = false;
	for (int i = 0; i < HISCORES_NR; i++) {
		if ((stat = score > Records[i].hiscore)) {
			for (int k = HISCORES_NR - 1; k > i; k--) {
				Records[k].hiscore = Records[k - 1].hiscore;
			}
			Records[i].hiscore = score;
			status.store_hiscore = true;
			break;
		}
	}
	return stat;
}

static void show_hiscores(void)
{
	char buff[64];
	int w, h = gfx_font_height();
	gfx_draw_bg(iBG, SCORES_X, SCORES_Y, SCORES_W, HISCORES_NR * h);
	for (int i = 0; i < HISCORES_NR; i++) {
		snprintf(buff, sizeof(buff),"%d", i + 1);
		w = gfx_chars_width(buff);
		snprintf(buff, sizeof(buff),"%d.", i + 1);
		gfx_draw_text(buff, SCORES_X + FONT_WIDTH - w, SCORES_Y + i * h, 0);
		snprintf(buff, sizeof(buff),"%d", Records[i].hiscore);
		w = gfx_chars_width(buff);
		gfx_draw_text(buff, SCORES_X + SCORES_W - w, SCORES_Y + i * h, 0);
	}
}

bool load_game_ui(path_t game_dir)
{
	char path[ SYS_PATH_L ], cname[14];
	const size_t si = strlen( strcpy(path, path_gfx) );

	int i, b = ball1;

	sys_set_dpath(game_dir, UI_DIR);

# define _load_img(el, LIT) \
	_strrepl(path, si, LIT);\
	el = gfx_load_image(path, SDL_TRUE)

	/* draw background */
	Elem[UI_Bg].img = ui_load_image(sys_get_fpath(game_dir, UI_ELEMENT_GET(0)));
	_load_img(pb_logo, "pb_logo.png");

	gfx_draw_bg(iBG, 0, 0, SCREEN_W, SCREEN_H);
	draw_Msg("Loading...", 0);
	gfx_update();

# ifdef DEBUG
	printf("- check image in: %s\n", game_dir.path);
# endif
	for (i = 1; i < UI_ELEMENTS_N; i++) {
		cstr_t f = sys_get_fpath(game_dir, UI_ELEMENT_GET(i));
		Elem[i].img = ui_load_image(f);
# ifdef DEBUG
		printf("|~ load image %i: %s\n", i, f);
# endif
	}
	el_rect rg0 = NEW_BOUNDS_RECT(240, 40, 40, 40);
	el_img bC = Elem[UI_BallsCollect].img;
	Uint32 dP = bC->format->BitsPerPixel,
	       mR = bC->format->Rmask,
	       mG = bC->format->Gmask,
	       mB = bC->format->Bmask,
	       mA = bC->format->Amask;

	for (i = 0; i < COLORS_NR; i++, b++)
	{
		el_rect rc1 = NEW_BOUNDS_RECT(i*40, 0, 40, 40);
		el_img  col = SDL_CreateRGBSurface(0, 40, 40, dP, mR, mG, mB, mA);

		SDL_UpperBlit(bC, &rc1, col, NULL);
		SDL_UpperBlit(bC, &rg0, col, NULL);

		push_ball(b, col);
	}
	for (i = 0; i < BONUSES_NR; i++, b++)
	{
		el_rect rc1 = NEW_BOUNDS_RECT(i*40, 40, 40, 40);
		el_img  col = SDL_CreateRGBSurface(0, 40, 40, dP, mR, mG, mB, mA);

		SDL_UpperBlit(bC, &rc1, col, NULL);
		if (b < ball_bomb1)
			SDL_UpperBlit(bC, &rg0, col, NULL);

		push_ball(b, col);
	}
	_load_img( Loop.on ,  "loop_on.png"  );
	_load_img( Loop.off,  "loop_off.png" );
	_load_img( Info.on ,  "info_on.png"  );
	_load_img( Info.off,  "info_off.png" );

	return false;
}

void free_game_ui(void) {
	gfx_free_image(pb_logo);

	/* free elems */
	for (int i = 0; i < UI_ELEMENTS_N; i++) {
		gfx_free_image(Elem[i].img);
	}
	/* free balls */
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
	};
	free_Img( Info  );
	free_Img( Loop  );
}

static void game_prep(void)
{
	srand(time(NULL));

	int fnt_h = gfx_font_height(),
	    fnt_w = gfx_chars_width(Restart.name);
	int mus_w = ui_get_el_width(&Elem[UI_Music]),
	    mus_h = ui_get_el_height(&Elem[UI_Music]) / 2;
	int vol_w = ui_get_el_width(&Elem[UI_Volume]),
	    vol_h = ui_get_el_height(&Elem[UI_Volume]) / 2;

	Restart.w = fnt_w, Restart.x = SCORES_X + (SCORES_W - fnt_w) / 2;
	Restart.h = fnt_h, Restart.y = SCREEN_H - fnt_h * 2 - fnt_h  / 2;
	gfx_draw_text(Restart.name, Restart.x, Restart.y, 0);

	Timer.x = SCREEN_W - (Timer.w = gfx_chars_width(Timer.name) * Timer.temp);
	Timer.y = SCREEN_H - (Timer.h = fnt_h * Timer.temp);
	gfx_draw_text(Timer.name, Timer.x, Timer.y, Timer.temp);

	bool has_on = game_prefs_has(&Prefs, FL_PREF_BGM_PLAY);
	ui_set_el_bounds(&Elem[UI_Music], NEW_BOUNDS_RECT(0, SCREEN_H - mus_h, mus_w, mus_h));
	ui_draw_Tico(&Elem[UI_Music], iBG, screen, has_on);

	Loop.w = iget_W(Loop), Loop.x = mus_w + 5;
	Loop.h = iget_H(Loop), Loop.y = SCREEN_H - mus_h - Loop.h;
	draw_Button(Loop, game_prefs_has(&Prefs, FL_PREF_BGM_LOOP));

	Info.w = iget_W(Info), Info.x = 0;
	Info.h = iget_H(Info), Info.y = SCREEN_H - mus_h - Info.h;
	draw_Button(Info, Info.hook);

	int fill_w = Prefs.bgm_vol * vol_w / 100;
	ui_set_el_bounds(&Elem[UI_Volume], NEW_BOUNDS_RECT(mus_w, SCREEN_H - vol_h, vol_w, vol_h));
	ui_set_el_ivalue(&Elem[UI_Volume], mus_w + fill_w);
	ui_draw_Hbar(&Elem[UI_Volume], iBG, screen, fill_w);

	Track.x = mus_w + vol_w + Track.temp / 2;
	Track.y = SCREEN_H - Track.temp - Track.temp / 2;

	if (Track.name[0] != '\0') {
		Track.on = gfx_draw_ttf_text(Track.name);
		Track.w  = iget_W(Track);
		Track.h  = iget_H(Track);
		gfx_draw_wh(Track.on, Track.x, Track.y, Track.w, Track.h);
	}
}

static void game_restart(bool rel)
{
	status.update_needed = status.game_over = false;
	if (!rel) {
		board_init_desk(&Board);
		board_fill_pool(&Board);
# ifdef DEBUG
		board_dbg_desk(&Board);
# else
		for (int i = 0; i < BOARD_POOL_N; i++)
			board_fill_desk(&Board, i, BOARD_DESK_N - i);
		board_fill_pool(&Board);
# endif
	}
	board_init_move(&Move , fetch_game_board(rel));
	draw_board();
	cur_score = rel ? board_get_score(&Board) - 1 : -1;
	cur_mul   = rel ? board_get_delta(&Board) - 1 :  0;
	draw_Timer_digit(0, NULL);
	show_score();
	show_hiscores();
	gfx_update();
	start_GameTimer();
}

int main(int argc, char **argv) {

	path_t cfg_dir;
	bool rel = false;

#if CL_IMG_DIR && CL_SND_DIR
	strncat(path_gfx, CL_IMG_DIR, sizeof(CL_IMG_DIR));
#else

	if (!SysGetExecPath(&GameDir)) {
		puts("Can't fing game directory\n");
		return -1;
	}
# ifdef DEBUG
	printf("\n- found gamedir:  %s\n", GameDir.path);
# endif
	_strncomb(path_gfx, GameDir.path, "gfx/", GameDir.len);
#endif

	if (!SysAcessConfigPath(&cfg_dir, "color-lines")) {
		puts("Can't acess home config directory check you permissions\n");
		return -1;
	}
# ifdef DEBUG
	fprintf(stderr, "- found config:   %s\n\n %s\n\n", cfg_dir.path,
		"Note: in debug-mode progress will not be saved.");
# else
	rel = game_load_session(&Board , sys_get_fpath(cfg_dir, CL_SESSION_NAME));
	      game_load_records(Records, sys_get_fpath(cfg_dir, CL_RECORDS_NAME));
# endif
	// load settings before sound init
	/* */ game_load_settings(&Prefs, sys_get_fpath(cfg_dir, CL_PREFS_NAME));

	// Initialize SDL
	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return -1;
	}
	// Initialize Graphic and UI
	if (gfx_init(path_gfx, SCREEN_W, SCREEN_H) || load_game_ui(GameDir)) {
		free_game_ui();
		fprintf(stderr, "graphics.c: Unable to create %dx%d window - '%s'\n", SCREEN_W, SCREEN_H, SDL_GetError());
		return -1;
	}
	if (game_init_sound(&Sound, &Prefs, GameDir))
		music_start(Prefs.track_num, GameDir);
	game_prep();
	game_restart(rel);
	game_loop();
	/* END GAME CODE HERE */
	board_wait_finish(&Board, &Move);
# ifndef DEBUG
	// save game progress
	    game_save_session(&Board , sys_get_fpath(cfg_dir, CL_SESSION_NAME));
	if (status.store_hiscore)
	    game_save_records(Records, sys_get_fpath(cfg_dir, CL_RECORDS_NAME));
	if (status.store_prefs)
	    game_save_settings(&Prefs, sys_get_fpath(cfg_dir, CL_PREFS_NAME));
# endif
	free_game_ui();
	sound_close_done(&Sound);
	gfx_done();
	SDL_Quit();
	return 0;
}
