#include "main.h"
#include "src/game.c"

#define SCORES_W     (BOARD_X - TILE_W - SCORES_X - 40)
#define SCORE_W      (BOARD_X - 23)
#define UI_BITMAPS_L (1+SVG_IMAGES_N)

char path_gfx[ SYS_PATH_L ];

/* Game Hiscores */
static record_t Records[HISCORES_NR] = {
	DEFAULT_RECORD(50), DEFAULT_RECORD(40),
	DEFAULT_RECORD(30), DEFAULT_RECORD(20), DEFAULT_RECORD(10)
};

/* BGM Plylist */
static struct playlist {
	const char *title, *file;
} trackList[] = { MUSIC_PLAYLIST };

static sound_t Sound;
static prefs_t Prefs = DEFAULT_PREFS();
static zfont_t Fonts[FONT_NAME_N];

static elem_t SfxVol, SfxMute, MyScore, HelpGuide,
              BgmVol, BgmMute, Restart;
static desk_t Board;
static move_t Move;
static path_t GameDir;
static el_img SVGs[UI_BITMAPS_L];

struct __STAT__ {
	bool quit, locked, paused, game_over;
};

enum {
	hT_Free,
	hT_Sfx_Volume,
	hT_Bgm_Volume,
	hT_Info_Help,
	hT_Timer_Stop,
};

static struct {
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

static elemen_t Score   = { .x = SCORE_X + SCORE_W / 2, .y = SCORE_Y };
static elemen_t Track   = { .temp = 14 };
static elemen_t Timer   = { .name = "00:00", .temp = 0.5 };
static elemen_t Loop    = { .name = "loop" };
static elemen_t Info    = { .name = "info" };

int   moving_nr = 0;
img_t bg_saved = NULL;

static struct {
	el_img alpha[BALL_ALPHA_N];
	el_img sizes[BALL_SIZES_N];
	el_img jumps[BALL_JUMPS_N];
} Balls[BALLS_COUNT_L];

#define iBG     SVGs[UI_Bg]
#define iCELL   SVGs[UI_Cell]
#define iSCREEN SVGs[UI_Blank]

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

#define draw_Msg(str, s) \
	game_board_msg(str, sizeof(str) - 1, s)

static void draw_Track_title(void)
{
	gfx_draw_bg(iBG, Track.x, Track.y, Track.w, Track.h);
	Track.on = ui_make_text(&Fonts[FNT_Pixil], Track.name, sizeof(Track.name));
	Track.w  = gfx_img_w(Track.on);
	Track.h  = gfx_img_h(Track.on);
	ui_scale_image(Track.on, screen, Track.x, Track.y, 0.5);
	gfx_update();
}

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

const char help_text[] = CL_HELP,
          about_text[] = "   -= Color Lines "CL_VERSION_STRING" =-\n"CL_ABOUT"";

static void scroll_info(elem_t *el, int oy, bool save_y)
{
	gfx_draw_bg(iBG, INFO_X, GAME_BOARD_Y, INFO_W, GAME_BOARD_H);

	el_rect r = ui_get_el_bounds(el);
	     r.y += oy;

	if (save_y) {
		if (r.y > GAME_BOARD_H) {
			r.y = GAME_BOARD_H;
		} else if (r.y < 0) {
			r.y = 0;
		}
		el->rect.y = r.y;
	}
	ui_draw_source(el->img, r, screen, INFO_X, GAME_BOARD_Y);
}

static void toggle_game_pause(bool pause)
{
	el_rect ir = ui_new_el_rect(
		INFO_X, GAME_BOARD_Y,
		INFO_W, GAME_BOARD_H );
	if (pause) {
		if(!bg_saved)
			bg_saved = SDL_CreateRGBSurface(0, ir.w, ir.h, 16, 0xF800, 0x7E0, 0x1F, 0);
		ui_draw_source(screen, ir, bg_saved, 0, 0);
	} else {
		ir.x = ir.y = 0;
		ui_draw_source(bg_saved, ir, screen, INFO_X, GAME_BOARD_Y);
	}
}

static inline void game_board_msg(cstr_t str, const int len, float scale) {

	typecr_t p, m = ui_text_rect(&Fonts[FNT_Fancy], str, len);
	    p.offsetX = GAME_BOARD_X + (GAME_BOARD_W - m.width ) / 2,
	    p.offsetY = GAME_BOARD_Y + (GAME_BOARD_H - m.height) / 2;

	for (int i = 0; i < len; i++)
		p = ui_draw_char(&Fonts[FNT_Fancy], screen, str[i], p);
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
void draw_ball(int n, int x, int y);
void draw_ball_offset(int n, int x, int y, int dx, int dy);
void draw_ball_alpha(int n, int x, int y, int alpha);
void draw_ball_size(int n, int x, int y, int size);
void update_cells(int x1, int y1, int x2, int y2);
void draw_ball_jump(int n, int x, int y, int num);
static void show_score(void);
static bool check_hiscores(int score);
static void show_hiscores(void);
static void prep_main_board(bool is_new);
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
	ui_draw_Tico(&BgmMute, iBG, screen, has_play);
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
	int vol_i, vol_w = BgmVol.rect.w,
	    pos_x, mus_w = BgmMute.rect.w;

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
	ui_set_el_value(&BgmVol, ATYPE_INT, mus_w + pos_x);
	ui_draw_Hbar(&BgmVol, iBG, screen, pos_x);
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

void draw_ball_alpha_offset(int b, int i, int x, int y, int dx, int dy)
{
	int nx, ny;
	el_img img = Balls[b].alpha[i];
	cell_to_screen(x, y, &nx, &ny);
	gfx_draw(img, nx + 5 + dx, ny + 5 + dy);
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

void draw_ball_size(int b, int x, int y, int i)
{
	int nx, ny;
	el_img img = Balls[b].sizes[i];
	int diff = (TILE_W - img->w) / 2;
	cell_to_screen(x, y, &nx, &ny);
	gfx_draw(img, nx + diff, ny + diff);
}

void draw_ball_jump(int b, int x, int y, int i)
{
	int nx, ny;
	el_img img = Balls[b].jumps[i];
	int diff  = (40 - img->h) + 5;
	int diffx = (TILE_W - img->w) / 2;
	cell_to_screen(x, y, &nx, &ny);
	gfx_draw(img, nx + diffx, ny + diff);
}

static int fetch_game_board()
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

Uint32 frame_handler(Uint32 ival, void *frame)
{
	struct __STAT__ *st = frame, ld = *st;
	int code = 0;

	if (ld.quit)
		return 0;
	if (!ld.paused && !ld.locked) {
		game_move_ball();
		game_process_board();
		game_process_pool();
		show_score();
		game_display_pool();
		if (!game_display_board() && !ld.game_over) { /* nothing todo */
			if (!board_next_move(&Board, &Move)) {
				if (check_hiscores(board_get_score(&Board))) {
					sound_sfx_play(&Sound, SND_Hiscore, 1);
					show_hiscores();
				} else {
					sound_sfx_play(&Sound, SND_Gameover, 1);
				}
				draw_Msg("Game Over!", 0);
				code = hT_Timer_Stop;
				st->game_over = true;
			}
		}
		if (code || status.update_needed)
			ui_push_event(code), status.update_needed = false;
	}
	return ival;
}

static void loop_main_start()
{
	struct __STAT__ frame = {
		.quit      = false,
		.game_over = false,
		.locked    = false,
		.paused    = false
	};

	SDL_Event event;
	SDL_TimerID timer_id = 0;
	Uint32 t, c, hook_ht = 0;

	int x, sx = 0, mx = 0,
	    y, sy = 0, my = 0;

	bool refresh = false, paused = false,
	     running = true;

/* <==== Game Timer ====> */
# define start_Game_Timer() timer_id = SDL_AddTimer(1e3, draw_Timer_digit, NULL)
# define  stop_Game_Timer() /*------*/ SDL_RemoveTimer( timer_id )
# define  lock_Main_Frame() timer_id = 0, frame.locked = true
# define ulock_Main_Frame() /*------*/    frame.locked = false

	SDL_AddTimer(20, frame_handler, &frame);
	start_Game_Timer();

	while (running) {
		if (SDL_WaitEvent(&event)) {
			x = event.button.x;
			y = event.button.y;
			c = event.user.code;
			t = event.key.state == SDL_PRESSED &&
				event.key.keysym.sym == SDLK_ESCAPE ? SDL_QUIT :
				event.type;

			refresh = true;

			switch (t) {
			case SDL_USEREVENT:
				if (c == hT_Timer_Stop)
					stop_Game_Timer();
				break;
			case SDL_QUIT: // Quit the game
				frame.quit = true;
				refresh = running = false;
				break;
			case SDL_MOUSEMOTION:
				mx = x, my = y;
				if (hook_ht == hT_Bgm_Volume)
					set_volume(x);
				else if (hook_ht == hT_Info_Help)
					scroll_info(&HelpGuide, sy - y, false);
				else
					refresh = false;
				break;
			case SDL_MOUSEBUTTONUP:
				if (hook_ht == hT_Info_Help)
					scroll_info(&HelpGuide, sy - y, true);
				else
					refresh = false;
				sx = sy = hook_ht = 0;
				break;
			case SDL_MOUSEBUTTONDOWN: // Button pressed
				sx = x, sy = y;
				if (IS_OVER_GAME_BOARD(x,y)) {
					if (paused) {
						hook_ht = hT_Info_Help;
					} else if (!board_has_over(&Move)) {
						board_select_ball(&Board, &Move,
							(x - BOARD_X) / TILE_W,
							(y - BOARD_Y) / TILE_H);
					} else if (timer_id) {
						lock_Main_Frame();
						prep_main_board(true);
						start_Game_Timer();
						ulock_Main_Frame();
					} else
						refresh = false;
				} else if (ui_is_on_el_rect(&BgmVol, x, y)) {
					hook_ht = hT_Bgm_Volume;
					set_volume(x);
				} else if (ui_is_on_el_rect(&Restart, x, y)) {
					if (timer_id) {
						stop_Game_Timer();
						lock_Main_Frame();
						board_wait_finish(&Board, &Move);
						if (!board_has_over(&Move)) {
							bool ok = check_hiscores(board_get_score(&Board));
							sound_sfx_play(&Sound, ok ? SND_Hiscore : SND_Gameover, 1);
						}
						prep_main_board(true);
						start_Game_Timer();
						ulock_Main_Frame();
					} else
						refresh = false;
				} else if (ui_is_on_el_rect(&BgmMute, x, y)) {
					toggle_music();
				}
				else if ((Track.hook = is_OnElem(Track, x, y))) {
					track_switch();
				}
				else if (is_OnElem(Loop, x, y)) {
					toggle_loop();
				}
				else if (is_OnElem(Info, x, y)) {
					if (paused ^= 1) {
						stop_Game_Timer();
						toggle_game_pause(frame.paused = true);
						scroll_info(&HelpGuide, 0, false);
						draw_Button(Info, true);
					} else {
						toggle_game_pause(frame.paused = false);
						draw_Button(Info, false);
						start_Game_Timer();
					}
				} else
					refresh = false;
				break;
			case SDL_MOUSEWHEEL:
				if (ui_is_on_el_rect(&BgmVol, mx,my)) {
					set_volume(ui_get_el_value(&BgmVol, ATYPE_INT) + (x ?: y));
				} else
				if (IS_OVER_GAME_BOARD(mx,my) && paused && x) {
					scroll_info(&HelpGuide, -FONT_LIMON_HEIGHT * x, true);
				} else
					refresh = false;
				break;
			case SDL_WINDOWEVENT:
				c = event.window.event,
				refresh = c == SDL_WINDOWEVENT_SIZE_CHANGED ||
				          c == SDL_WINDOWEVENT_MINIMIZED ||
				          c == SDL_WINDOWEVENT_MAXIMIZED;
				break;
			default: refresh = false;
			}
			printf ("%i refrsh; winev %i\n", refresh, t);
			if (refresh)
				gfx_update();
		}
	}
	stop_Game_Timer();
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

void prep_main_balls()
{
	int j,x,y,a,b = 0,n = BALL_COLOR_N;

	float xs,ys;
	GFX_Radi angle = { 0.0f, 1.0f }; // GFX_toRadians(0);

	el_rect dbr = ui_new_el_rect(
		BOARD_BALL_H*(BALL_COLOR_N -1 ),
		BOARD_BALL_H, BOARD_BALL_W, BOARD_BALL_H);

	for (y = 0; y < BOARD_BALL_H*2; y += BOARD_BALL_H, n = BALL_BONUS_N)
	for (x = 0; x < BOARD_BALL_W*n; x += BOARD_BALL_W, b++)
	{
		el_rect sbr = ui_new_el_rect(x, y, BOARD_BALL_W, BOARD_BALL_H);
		el_img ball = GFX_CpySurfFormat(SVGs[UI_BallsCollect], BOARD_BALL_W, BOARD_BALL_H);

		SDL_UpperBlit(SVGs[UI_BallsCollect], &sbr, ball, NULL);
		if ((b+1) < ball_bomb1)
			SDL_UpperBlit(SVGs[UI_BallsCollect], &dbr, ball, NULL);

		Balls[b].alpha[BALL_ALPHA_N - 1] = ball;

		for (j = 1, a = BALL_ALPHA_S; j < BALL_ALPHA_N; j++, a += BALL_ALPHA_S) {
			Balls[b].alpha[j-1] = GFX_CpyWithAlpha(ball, a);
		}
		for (j = 0, xs = BALL_SIZES_S; j < BALL_SIZES_N; j++, xs += BALL_SIZES_S) {
			Balls[b].sizes[j] = GFX_CpyWithTransform(ball, angle, xs, xs);
		}
		for (j = 1; j <= BALL_JUMPS_N; j++) {
			ys = 1.0 - (((float)(1.0 - JUMP_MAX) / (float)JUMP_STEPS) * j),
			xs = 1.0 + (1.0 - ys);
			Balls[b].jumps[j-1] = GFX_CpyWithTransform(ball, angle, xs, ys);
		}
	}
}

static void free_main_ui()
{
	int i,k;
	/* free images */
	for (i = 0; i <  UI_BITMAPS_L; i++) ui_free_image(SVGs[i]);
	/* free fonts */
	for (i = 0; i <   FONT_NAME_N; i++) ui_free_image(Fonts[i].bitmap);
	/* free balls */
	for (i = 0; i < BALLS_COUNT_L; i++) {
		for(k=0; k < BALL_ALPHA_N; k++) ui_free_image(Balls[i].alpha[k]);
		for(k=0; k < BALL_SIZES_N; k++) ui_free_image(Balls[i].sizes[k]);
		for(k=0; k < BALL_JUMPS_N; k++) ui_free_image(Balls[i].jumps[k]);
	};
	ui_free_image( HelpGuide.img ); BgmVol.img = BgmMute.img = NULL;
	ui_free_image( Restart  .img );
}

static void prep_main_ui(void)
{
	srand(time(NULL));

	Restart.img = ui_draw_ctxt(&Fonts[FNT_Fancy], "Restart");
	BgmVol .img = SVGs[UI_Volume];
	BgmMute.img = SVGs[UI_Music];

	int mus_w = ui_get_img_width (SVGs[UI_Music]),
	    mus_h = ui_get_img_height(SVGs[UI_Music]) / 2,
	    vol_w = ui_get_img_width (SVGs[UI_Volume]),
	    vol_h = ui_get_img_height(SVGs[UI_Volume]) / 2,
	    res_w = ui_get_img_width (Restart.img),
	    res_h = ui_get_img_height(Restart.img);

 	int res_x = SCORES_X + (SCORES_W - res_w) / 2,
 	    res_y = SCREEN_H - res_h * 2 - res_h  / 2;

	ui_set_el_bounds(&Restart, res_x, res_y, res_w, res_h);
	ui_draw_image(Restart.img, screen, res_x, res_y);

	Timer.x = SCREEN_W - (Timer.w = gfx_chars_width(Timer.name) * Timer.temp);
	Timer.y = SCREEN_H - (Timer.h = res_h * Timer.temp);
	gfx_draw_text(Timer.name, Timer.x, Timer.y, Timer.temp);

	bool has_on = game_prefs_has(&Prefs, FL_PREF_BGM_PLAY);
	ui_set_el_bounds(&BgmMute, 0, SCREEN_H - mus_h, mus_w, mus_h);
	ui_draw_Tico(&BgmMute, iBG, screen, has_on);

	Loop.on = ui_draw_ctxt(&Fonts[FNT_Pixil], "LT");
	Loop.off = ui_draw_ctxt(&Fonts[FNT_Pixil], "PL");
	Info.on = ui_draw_ctxt(&Fonts[FNT_Fancy], "{x}");
	Info.off = ui_draw_ctxt(&Fonts[FNT_Fancy], "{?}");

	Loop.w = iget_W(Loop), Loop.x = mus_w + 5;
	Loop.h = iget_H(Loop), Loop.y = SCREEN_H - mus_h - Loop.h;
	draw_Button(Loop, game_prefs_has(&Prefs, FL_PREF_BGM_LOOP));

	Info.w = iget_W(Info), Info.x = 5;
	Info.h = iget_H(Info), Info.y = 5;
	draw_Button(Info, Info.hook);

	int fill_w = Prefs.bgm_vol * vol_w / 100;
	ui_set_el_bounds(&BgmVol, mus_w, SCREEN_H - vol_h, vol_w, vol_h);
	ui_set_el_value (&BgmVol, ATYPE_INT, mus_w + fill_w);
	ui_draw_Hbar(&BgmVol, iBG, screen, fill_w);

	Track.x = mus_w + vol_w + Track.temp / 2;
	Track.y = SCREEN_H - Track.temp - Track.temp / 2;

	if (Track.name[0] != '\0') {
		Track.on = ui_make_text(&Fonts[FNT_Pixil], Track.name, sizeof(Track.name));
		Track.w  = iget_W(Track);
		Track.h  = iget_H(Track);
		ui_scale_image(Track.on, screen, Track.x, Track.y, 0.5);
	}
	if(!HelpGuide.img) {
		HelpGuide.img  = ui_make_text(&Fonts[FNT_Limon], help_text , sizeof(help_text)),
		HelpGuide.rect = ui_new_el_rect(0, 0, INFO_W, GAME_BOARD_H);

		ui_draw_image(Balls[ball_joker-1].alpha[ALPHA_STEPS-1], HelpGuide.img, 0, INFO_Y_JOCKER);
		ui_draw_image(Balls[ball_flush-1].alpha[ALPHA_STEPS-1], HelpGuide.img, 0, INFO_Y_FLUSH );
		ui_draw_image(Balls[ball_brush-1].alpha[ALPHA_STEPS-1], HelpGuide.img, 0, INFO_Y_BRUSH );
		ui_draw_image(Balls[ball_bomb1-1].alpha[ALPHA_STEPS-1], HelpGuide.img, 0, INFO_Y_BOMB  );
	}
}

static void prep_main_board(bool is_new)
{
	if (is_new) {
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
	board_init_move(&Move , fetch_game_board());
	draw_board();
	cur_score = is_new ? -1 : board_get_score(&Board) - 1;
	cur_mul   = is_new ?  0 : board_get_delta(&Board) - 1;
	draw_Timer_digit(0, NULL);
	show_score();
	show_hiscores();
	status.update_needed = status.game_over = false;
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
	// Load fonts
	if (!game_load_fonts(Fonts, GameDir)) {
		puts("Can't load game fonts\n");
		return -1;
	}
	// Initialize Graphic and UI
	if (gfx_init(path_gfx, SCREEN_W, SCREEN_H)) {
		fprintf(stderr, "graphics.c: Unable to create %dx%d window - '%s'\n", SCREEN_W, SCREEN_H, SDL_GetError());
		return -1;
	}
	if (!game_load_images(SVGs, GameDir)) {
		puts("Can't load game images\n");
		return -1;
	}
	ui_draw_image(iBG, screen, 0, 0);
	draw_Msg("Loading...",0);
	gfx_update();

	if (game_init_sound(&Sound, &Prefs, GameDir))
		music_start(Prefs.track_num, GameDir);

	prep_main_balls();
	prep_main_ui();
	prep_main_board(!rel);
	loop_main_start();
	/* END GAME CODE HERE */
# ifndef DEBUG
	// save game progress
	    game_save_session(&Board , sys_get_fpath(cfg_dir, CL_SESSION_NAME));
	if (status.store_hiscore)
	    game_save_records(Records, sys_get_fpath(cfg_dir, CL_RECORDS_NAME));
	if (status.store_prefs)
	    game_save_settings(&Prefs, sys_get_fpath(cfg_dir, CL_PREFS_NAME));
# endif
	free_main_ui();
	sound_close_done(&Sound);
	gfx_done();
	SDL_Quit();
	return 0;
}
