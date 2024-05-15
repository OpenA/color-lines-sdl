#include "main.h"
#include "src/game.c"

/* Game Hiscores */
static record_t Records[HISCORES_NR] = {
	DEFAULT_RECORD(50), DEFAULT_RECORD(40),
	DEFAULT_RECORD(30), DEFAULT_RECORD(20), DEFAULT_RECORD(10)
};

/* BGM Plylist */
static struct playlist {
	const char *title, *file;
} trackList[] = { MUSIC_PLAYLIST };

static window_t Window;
static sound_t Sound;
static prefs_t Prefs = { DEFAULT_PREFS };
static zfont_t Fonts[FONT_NAME_N];

static elem_t SfxVol, SfxMute, PlayMode, OptsBtn, MyScore, HelpGuide,
              BgmVol, BgmMute, PlayList, HelpBtn, Restart;
static desk_t Board;
static move_t Move;
static path_t GameDir;
static el_img SVGs[UI_BITMAPS_L];

struct _STAT_ {
	bool quit:1, locked:1, paused:1, game_over:1;
	char brd_state;
};

typedef enum _HOOK_ {
	hT_Free,
	hT_Track_Loop,
	hT_Info_Help,
	hT_Timer_Stop,
	hT_Sfx_Mute,   hT_Bgm_Mute,
	hT_Sfx_Volume, hT_Bgm_Volume,
} hook_e;

static struct _PLAYER_ {
	unsigned score:21, dmul  :7, store_records:1, update_need :1, _0:2;
	unsigned scmax:21, blinks:7, store_prefs  :1, change_track:1, _1:2;
} Player = {
	.store_prefs   = false, .score  = 0x1FFFFFu,
	.store_records = false, .dmul   = 0x7F,
	.change_track  = false, .blinks = 0,
	.update_need   = false, .scmax  = 0
};
static typecr_t timer_pos;

int   moving_nr = 0;

static struct {
	el_img alpha[BALL_ALPHA_N];
	el_img sizes[BALL_SIZES_N];
	el_img jumps[BALL_JUMPS_N];
} Balls[BALLS_COUNT_L];

#define iBG     SVGs[UI_Bg]
#define iCELL   SVGs[UI_Cell]
#define iSCREEN SVGs[UI_Blank]
#define iBOX    SVGs[UI_SetBox]
#define iSHOT   SVGs[UI_Snapshot]
#define tFANCY &Fonts[FNT_Fancy]
#define tPIXIL &Fonts[FNT_Pixil]
#define tLIMON &Fonts[FNT_Limon]

#define scroll_info(el,_y,_s) ui_draw_scroll(el, iBG, iSCREEN, 0,_y,_s)

# define restore_game_screen(_r) ui_draw_source(iSHOT, _r, iSCREEN, _r.x,_r.y)
static void snap_game_screen() {
	el_rect r = ui_new_el_rect(0, 0, GAME_SCREEN_W, GAME_SCREEN_H );
	if(!iSHOT)
		iSHOT = GFX_CpySurfFormat(iSCREEN, r.w, r.h),
		/* ~ */ GFX_DisableAlpha(iSHOT);
	ui_draw_source(iSCREEN, r, iSHOT, 0, 0);
}

#define iget_W(el) el.on->w
#define iget_H(el) el.on->h

#define draw_Msg(str, s) \
	game_board_msg(str, sizeof(str) - 1, s)

static inline void draw_Track_title(bool y) {
	el_img img = ui_get_el_bitmap(&PlayList);
	el_rect or = ui_get_el_bounds(&PlayList);
	/* ~ */ ui_draw_source(iBG, or, iSCREEN, or.x, or.y);
	if (y)  ui_scale_image(img, iSCREEN, or.x, or.y, .5f);
}

Uint32 upd_main_time(Uint32 ival, void *_)
{
	unsigned int t = board_get_time(&Board) + (ival > 0);
	char tdigit[16]; board_set_time(&Board, t);

	char s = (t % 60),
		 m = (t / 60), f = ' ';
	if (t >= (60 * 60))
		 s = m % 60, m = t / (60 * 60), f = t % 2 ? ' ' : ':';

	sprintf(tdigit, "%02d : %02d", m, s);

	typecr_t p = ui_new_offset(
		SCORE_TAB_X + SCORE_TAB_W / 2 - timer_pos.offsetX,
		SCORE_TAB_ROW(2)
	);
	el_rect or = ui_new_el_rect(
		/* - - - - */ p.offsetX, p.offsetY,
		SCORE_TAB_W - p.offsetX, timer_pos.offsetY
	);
	ui_draw_source(iBG, or, iSCREEN, or.x, or.y);

	for (int i = 0; i < 7; i++)
		p = ui_draw_char(tPIXIL, tdigit[i], f, iSCREEN, p);

	Player.update_need = true;
	return ival;
}

const char help_text[] = CL_HELP,
          about_text[] = "   -= Color Lines "CL_VERSION_STRING" =-\n"CL_ABOUT"";

static void toggle_game_help(bool show)
{
	el_rect ir = ui_new_el_rect(
		INFO_X, GAME_BOARD_Y,
		INFO_W, GAME_BOARD_H );
	if(!HelpGuide.img && show) {
		HelpGuide.img  = ui_make_text(&Fonts[FNT_Limon], help_text , sizeof(help_text)),
		ui_draw_image(Balls[ball_joker-1].alpha[ALPHA_STEPS-1], HelpGuide.img, 0, INFO_Y_JOCKER);
		ui_draw_image(Balls[ball_flush-1].alpha[ALPHA_STEPS-1], HelpGuide.img, 0, INFO_Y_FLUSH );
		ui_draw_image(Balls[ball_brush-1].alpha[ALPHA_STEPS-1], HelpGuide.img, 0, INFO_Y_BRUSH );
		ui_draw_image(Balls[ball_bomb1-1].alpha[ALPHA_STEPS-1], HelpGuide.img, 0, INFO_Y_BOMB  );
	}
	HelpBtn.fill.offsetY = show ? FONT_LIMON_HEIGHT : 0;
	ui_draw_el_ico(&HelpBtn, iBG, iSCREEN);
	if (show) {
		snap_game_screen();
		scroll_info(&HelpGuide, 0, false);
	} else {
		restore_game_screen(ir);
	}
}

static inline void game_board_msg(cstr_t str, const int len, float s) {

	typecr_t p, m = ui_text_rect(&Fonts[FNT_Fancy], str, len);
	    p.offsetX = GAME_BOARD_X + (GAME_BOARD_W - m.width ) / 2,
	    p.offsetY = GAME_BOARD_Y + (GAME_BOARD_H - m.height) / 2;

	for (int i = 0; i < len; i++)
		p = ui_draw_char(&Fonts[FNT_Fancy], str[i], ' ', iSCREEN, p);
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
static bool upd_main_score(bool reset);
static bool check_hiscores(int score);
static void upd_main_hiscores();
static void prep_main_board(bool is_new);

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

static void play_bgm_track(unsigned char n, path_t game_dir)
{
	if (n >= MUSIC_TRACKS_N)
		return;

	cstr_t file = trackList[n].file,
	      title = trackList[n].title;

	sys_make_dir_path(&game_dir, MUSIC_DIR);

	if (sound_bgm_play(&Sound, n, sys_make_file_path(&game_dir, file))) {
		sound_set_bgm_onEnd(track_switch);
		el_img img = PlayList.img;
		if (PlayList.value.h0 != n || !img) {
			PlayList.value.h0 = n;
			if (img) ui_free_image(img);
			PlayList.img = ui_make_text(&Fonts[FNT_Pixil], title, 255);
		}
	}
}

static inline void draw_play_mode(bool lt)
{
	unsigned char c = '~' + 1 + lt;
	typecr_t p = PlayMode.pos;
	p.offsetX += PlayMode.rect.width - FONT_LIMON_WIDTH;
	el_rect ir = ui_new_el_rect(p.offsetX, p.offsetY, FONT_LIMON_WIDTH, FONT_LIMON_HEIGHT);
	ui_draw_source(iBOX, ir, iSCREEN, ir.x, ir.y);
	ui_draw_char(tLIMON, c, ' ', iSCREEN, p);
}

void track_switch() {
	int  tnum = Prefs.mus_num;
	bool change = Player.change_track || !Prefs.mus_loop;

	if (change) {
		if((tnum + 1) >= MUSIC_TRACKS_N)
			tnum = 0;
		else
			tnum++;
		Prefs.mus_num = tnum;
		Player.store_prefs = true;
	}
	play_bgm_track(tnum, GameDir);
	if (change)
		draw_Track_title(true);
}

int change_main_volume(elem_t *el, int x)
{
	el_rect vol = ui_get_el_bounds(el);
	int fill_w  = x - vol.x, val_i;

	if (x >= (vol.x + vol.w))
		val_i = 100, fill_w = vol.w;
	else if (fill_w <= 1)
		val_i = 1, fill_w = 0;
	else
		val_i = (fill_w * 100) / vol.w;
	el->fill.width = fill_w;
	ui_draw_el_bar(el, iBOX, iSCREEN, HL_Outside);
	return val_i;
}

void draw_cell(int x, int y)
{
	el_rect ir = ui_new_el_rect(0,0, BOARD_TILE_W, BOARD_TILE_H);
	int nx, ny;
	cell_to_screen(x, y, &nx, &ny);
	ui_draw_source(iCELL, ir, iSCREEN, nx, ny);
}

void update_cell(int x, int y)
{
	Player.update_need = true;
	int nx, ny;
	cell_to_screen(x, y, &nx, &ny);
}

void update_cells(int x1, int y1, int x2, int y2)
{
	Player.update_need = true;
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
	ui_draw_image(img, iSCREEN, nx + 5 + dx, ny + 5 + dy);
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
	ui_draw_image(img, iSCREEN, nx + diff, ny + diff);
}

void draw_ball_jump(int b, int x, int y, int i)
{
	int nx, ny;
	el_img img = Balls[b].jumps[i];
	int diff  = (40 - img->h) + 5;
	int diffx = (TILE_W - img->w) / 2;
	cell_to_screen(x, y, &nx, &ny);
	ui_draw_image(img, iSCREEN, nx + diffx, ny + diff);
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
	struct _STAT_ *st = frame, ld = *st;
	int code = 0;

	if (ld.quit)
		return 0;
	if (!ld.paused && !ld.locked) {
		game_move_ball();
		game_process_board();
		game_process_pool();
		upd_main_score(false);
		game_display_pool();
		if (!game_display_board() && !ld.game_over) { /* nothing todo */
			if (!(st->brd_state = board_next_move(&Board, &Move))) {
				if (check_hiscores(board_get_score(&Board))) {
					sound_sfx_play(&Sound, SND_Hiscore, 1);
					upd_main_hiscores();
				} else {
					sound_sfx_play(&Sound, SND_Gameover, 1);
				}
				draw_Msg("Game Over!", 0);
				code = hT_Timer_Stop;
				st->game_over = true;
			}
		}
		if (code || Player.update_need)
			ui_push_event(code), Player.update_need = false;
	}
	return ival;
}

static void toggle_main_prefs(bool show, bool snap)
{
	el_rect sr = ui_new_el_rect(0, 0, GAME_SCREEN_W, GAME_SCREEN_H );
	if (show) {
		if (snap)
			snap_game_screen();
		ui_draw_source(iBOX, sr, iSCREEN, 0, 0);
		ui_draw_el_bar(&BgmVol , NULL, iSCREEN, HL_Outside);
		ui_draw_el_bar(&SfxVol , NULL, iSCREEN, HL_Outside);
		ui_draw_el_bar(&BgmMute, NULL, iSCREEN, HL_Outside);
		ui_draw_el_bar(&SfxMute, NULL, iSCREEN, HL_Outside);
		draw_play_mode(Prefs.mus_loop);
	} else {
		restore_game_screen(sr);
	}
}

static hook_e nook_main_prefs(int x, int y) {
	return (
		ui_is_on_el_rect(&PlayMode, x, y) ? hT_Track_Loop :
		ui_is_on_el_rect(&SfxVol  , x, y) ? hT_Sfx_Volume :
		ui_is_on_el_rect(&SfxMute , x, y) ? hT_Sfx_Mute :
		ui_is_on_el_rect(&BgmVol  , x, y) ? hT_Bgm_Volume :
		ui_is_on_el_rect(&BgmMute , x, y) ? hT_Bgm_Mute : hT_Free
	);
}

static void handle_main_prefs(hook_e ht, int x, int y)
{
	switch (ht) {
	case hT_Track_Loop:
		draw_play_mode(Prefs.mus_loop ^= 1);
		break;
	case hT_Sfx_Volume:
		/**/x = change_main_volume(&SfxVol, x);
		if (x != Prefs.sfx_vol) {
# ifdef DEBUG
			printf("SFX volume change: %i w%d\n", INT_VOL_STEP(x), SfxVol.fill.width);
# endif
			if (sound_has_mix_ready(&Sound))
				sound_set_sfx_volume(&Sound, x);
			Prefs.sfx_vol = x;
		}
		break;
	case hT_Sfx_Mute:
		if (Prefs.sfx_mute ^= 1) {
			conf_param_add(&Sound, FL_SFX_MUTE);
			SfxMute.fill.width = 0;
		} else {
			conf_param_del(&Sound, FL_SFX_MUTE);
			SfxMute.fill.width = -1;
		}
		ui_draw_el_bar(&SfxMute, iBOX, iSCREEN, HL_Outside);
		break;
	case hT_Bgm_Volume:
		/**/x = change_main_volume(&BgmVol, x);
		if (x != Prefs.bgm_vol) {
# ifdef DEBUG
			printf("BGM volume change: %i w%d\n", INT_VOL_STEP(x), BgmVol.fill.width);
# endif
			if (sound_has_mix_ready(&Sound))
				sound_set_bgm_volume(&Sound, x);
			Prefs.bgm_vol = x;
		}
		break;
	case hT_Bgm_Mute:
		if (Prefs.bgm_mute ^= 1) {
			if (sound_has_mix_ready(&Sound))
				sound_bgm_stop(&Sound);
			BgmMute.fill.width = 0;
		} else {
			play_bgm_track(Prefs.mus_num, GameDir);
			BgmMute.fill.width = -1;
		}
		ui_draw_el_bar(&BgmMute, iBOX, iSCREEN, HL_Outside);
		break;
	case hT_Free: case hT_Info_Help: case hT_Timer_Stop:
	default:
		return;
	}
	Player.store_prefs = true;
}

static void loop_main_start()
{
	struct _STAT_ frame = {
		.quit      = false,
		.game_over = false,
		.brd_state = 0,
		.locked    = false,
		.paused    = false
	};

	SDL_Event event;
	SDL_TimerID timer_id = 0;
	Uint32 t, c, hook_ht = 0;

	int x, sx = 0, mx = 0,
	    y, sy = 0, my = 0;

	bool refresh = false, paused = false,
	     running = true, options = false;

/* <==== Game Timer ====> */
# define start_Game_Timer() frame.paused = false, timer_id = SDL_AddTimer(1e3, upd_main_time, NULL)
# define  stop_Game_Timer() frame.paused = true , /* ~~~~ */ SDL_RemoveTimer( timer_id )
# define  lock_Main_Frame() frame.locked = true , timer_id = 0
# define ulock_Main_Frame() frame.locked = frame.game_over = false

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
					stop_Game_Timer(), frame.paused = false;
				break;
			case SDL_QUIT: // Quit the game
				frame.quit = true;
				refresh = running = false;
				break;
			case SDL_MOUSEMOTION:
				mx = x, my = y;
				if (hook_ht >= hT_Sfx_Volume)
					handle_main_prefs(hook_ht,x,y);
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
				if (ui_is_on_el_rect(&OptsBtn, x, y)) {
					if (options ^= 1) {
						if (!paused)
							stop_Game_Timer();
						toggle_main_prefs(true, !paused);
					} else {
						toggle_main_prefs(false, false);
						if (!paused)
						     start_Game_Timer();
						else scroll_info(&HelpGuide, 0, false);
					}
				} else if (options) {
					if ((hook_ht = nook_main_prefs(x,y)))
						handle_main_prefs(hook_ht, x,y);
					else
						refresh = false;
				} else
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
				} else if (ui_is_on_el_rect(&HelpBtn, x, y)) {
					if (paused ^= 1) {
						stop_Game_Timer();
						toggle_game_help(true);
					} else {
						toggle_game_help(false);
						start_Game_Timer();
					}
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
				} else if (ui_is_on_el_rect(&PlayList, x, y)) {
					Player.change_track = true;
					track_switch();
					Player.change_track = false;
				} else
					refresh = false;
				break;
			case SDL_MOUSEWHEEL:
				if (options && ui_is_on_el_rect(&SfxVol, mx,my)) {
					handle_main_prefs(hT_Sfx_Volume, SfxVol.pos.offsetX+SfxVol.fill.width + (-2*x), 0);
				} else
				if (options && ui_is_on_el_rect(&BgmVol, mx,my)) {
					handle_main_prefs(hT_Bgm_Volume, BgmVol.pos.offsetX+BgmVol.fill.width + (-2*x), 0);
				} else
				if (paused && IS_OVER_GAME_BOARD(mx,my)) {
					scroll_info(&HelpGuide, (-FONT_LIMON_HEIGHT*x), true);
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
			if (refresh)
				ui_win_update(&Window, iSCREEN);
		}
	}
	stop_Game_Timer();
	board_wait_finish(&Board, &Move);
}

static bool upd_main_score(bool reset)
{
	typecr_t p = ui_new_offset ( SCORE_TAB_X, SCORE_TAB_Y );
	el_rect or = ui_new_el_rect( SCORE_TAB_X, SCORE_TAB_Y, SCORE_TAB_W, FONT_FANCY_HEIGHT );

	char ni_bonus = reset ? 0 : Player.blinks, txt[24];
	int cur_score = reset ? 0 : Player.score;
	int new_score = board_get_score(&Board),
	   l, new_mul = board_get_delta(&Board) - 1,
	   diff_score = new_score - cur_score;

	bool do_sound = false,
	     incr_mul = new_mul > Player.dmul,
	     do_fills = reset ? true  : false,
	     do_clear = reset ? true  : false;

	if (reset) {
		Player.blinks = Player.dmul = Player.score = 0;
	} else if (ni_bonus) {
		do_fills = (ni_bonus / BONUS_BLINKS) & 1,
		do_clear = !do_fills;
		if (incr_mul)
			Player.blinks = BONUS_TIMER,
			Player.dmul = new_mul;
		else
			Player.blinks = ni_bonus - 1;
	} else {
		if (incr_mul && new_mul > 1)
			Player.blinks = ni_bonus = BONUS_TIMER,
			do_sound = do_clear = do_fills = true;
		else if (diff_score) {
			Player.score = cur_score = (
				   0 > diff_score ? new_score    :
				  20 > diff_score ? cur_score+1  :
				 200 > diff_score ? cur_score+10 :
				2000 > diff_score ? cur_score+100: cur_score+1000
			),
			do_clear = do_fills = true;
		}
		Player.dmul = new_mul;
	}
	if (do_clear)
		ui_draw_source(iBG, or, iSCREEN, or.x, or.y);
	if (do_sound)
		sound_sfx_play(&Sound, SND_Bonus, 1);
	if (do_fills) {
		l = snprintf(txt, sizeof(txt), (ni_bonus ? "Bonus x %d" : "PLAYER: %d"), (ni_bonus ? new_mul : cur_score));
		ui_draw_text(tFANCY, txt, l, iSCREEN, p);
		MyScore.fill.width = to_range(MyScore.rect.width, cur_score, Player.scmax);
		ui_draw_el_bar(&MyScore, iBG, iSCREEN, HL_Inside);
	}
	if (do_clear || do_fills)
		Player.update_need = true;
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
			Player.store_records = true;
			break;
		}
	}
	return stat;
}

static void upd_main_hiscores()
{
	char buff[64];
	typecr_t m,p = ui_new_offset(SCORE_TAB_X, SCORE_TAB_ROW(2) + FONT_GET_HLINE(PIXIL));
	elem_t hitab = {
		.img  = SVGs[UI_ProgressBar],
		.pos  = p,
		.rect = MyScore.rect,
		.fill = ui_new_rectsz(-1,-1)
	};
	int nR1, i, w = MyScore.rect.width,
		nR2, r,
		nR3, l;

	for(i = nR1 = nR2 = nR3 = 0; i < HISCORES_NR; i++) {
		r = Records[i].hiscore;
		if (nR1 < r) nR1 = r; else
		if (nR2 < r) nR2 = r; else
		if (nR3 < r) nR3 = r;
	}
	l = snprintf(buff, sizeof(buff), "LEADER: %i\n\nSILVER: %i\n\nTHIRDY: %i", nR1,nR2,nR3),
	m = ui_text_rect(tFANCY, buff, l);

	el_rect ir = ui_new_el_rect(p.offsetX, p.offsetY, m.width, m.height);
	ui_draw_source(iBG, ir, iSCREEN, p.offsetX, p.offsetY);
	ui_draw_text(tFANCY, buff, l, iSCREEN, p);

	/* global store */
	Player.scmax = nR1;

	hitab.pos.offsetY = FONT_GET_HLINE(PIXIL) + SCORE_TAB_ROW(3);
	ui_draw_el_bar(&hitab, iBG, iSCREEN, HR_Inside);

	hitab.pos.offsetY = FONT_GET_HLINE(PIXIL) + SCORE_TAB_ROW(5);
	hitab.fill.width = to_range(w, nR2, nR1);
	ui_draw_el_bar(&hitab, iBG, iSCREEN, HR_Inside);

	hitab.pos.offsetY = FONT_GET_HLINE(PIXIL) + SCORE_TAB_ROW(7);
	hitab.fill.width = to_range(w, nR3, nR1);
	ui_draw_el_bar(&hitab, iBG, iSCREEN, HR_Inside);
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
	ui_free_image( PlayList .img );              MyScore.img = NULL;
	ui_free_image( Restart  .img );
	ui_free_image( PlayMode  .img );
	ui_free_image( HelpBtn  .img );
}

static void prep_main_ui(void)
{
	srand(time(NULL));
# define SOUND_TXT "SFX\n\nBGM\n\n"
# define PLMOD_TXT "PLAY MODE: "

	HelpBtn .img = ui_make_cstr(&Fonts[FNT_Fancy], "{?}\n{x}");
	Restart .img = ui_make_cstr(&Fonts[FNT_Pixil], "Restart\n");
	MyScore .img = SVGs[UI_ProgressBar];
	BgmVol  .img = SVGs[UI_Volume];
	BgmMute .img = SVGs[UI_Music];
	SfxVol  .img = SVGs[UI_Volume];
	SfxMute .img = SVGs[UI_Sound];
	OptsBtn .img = SVGs[UI_Gear];

	typecr_t bar_r = ui_get_img_Hcrect(SVGs[UI_ProgressBar]),
			 vol_r = ui_get_img_Hcrect(SVGs[UI_Volume]),
			 bgm_r = ui_get_img_Hcrect(SVGs[UI_Music]),
			 sfx_r = ui_get_img_Hcrect(SVGs[UI_Sound]),
			 opt_r = ui_get_img_Hcrect(SVGs[UI_Gear]),
			 hlp_r = ui_get_img_Hcrect(HelpBtn.img),
			 rst_r = ui_get_img_Hcrect(Restart.img),

	ply_r = ui_cstr_rect(tLIMON, PLMOD_TXT),
	zzz_r = ui_cstr_rect(tLIMON, SOUND_TXT),
	inf_r = ui_new_rectsz(INFO_W, GAME_BOARD_H),

	trk_p = ui_new_offset(opt_r.width + 15, GAME_SCREEN_H - FONT_PIXIL_HEIGHT),
	trk_r = ui_new_rectsz(GAME_BOARD_X - trk_p.width, FONT_PIXIL_HEIGHT),

	hlp_p = ui_new_offset(5, 5),
	inf_p = ui_new_offset(INFO_X, GAME_BOARD_Y),
	opt_p = ui_new_offset(5  /*---*/ , GAME_SCREEN_H - opt_r.height - 5),
	sfv_p = ui_new_offset(zzz_r.width+ GAME_SCREEN_P, GAME_SCREEN_P),
	bgv_p = ui_new_offset(zzz_r.width+ GAME_SCREEN_P, GAME_SCREEN_P + FONT_GET_HLINE(LIMON)*2),
	ply_p = ui_new_offset(-5 /*---*/ + GAME_SCREEN_P, GAME_SCREEN_P + FONT_GET_HLINE(LIMON)*4),
	sfx_p = ui_new_offset(vol_r.width+ sfv_p.offsetX, sfv_p.offsetY),
	bgm_p = ui_new_offset(vol_r.width+ bgv_p.offsetX, bgv_p.offsetY),
	rst_p = ui_new_offset(SCORE_TAB_X+ SCORE_TAB_W/2 - rst_r.width/2, FONT_GET_HLINE(PIXIL) + SCORE_TAB_ROW(8)),
	bar_p = ui_new_offset(SCORE_TAB_X, SCORE_TAB_ROW(1));

	short sX = Prefs.sfx_mute - 1, fX = Prefs.sfx_vol * vol_r.width / 100,
	      bX = Prefs.bgm_mute - 1, vX = Prefs.bgm_vol * vol_r.width / 100;

	ply_r.width += FONT_LIMON_WIDTH;

	ui_set_el_bounds(&OptsBtn , opt_r, opt_p,  0,  0);
	ui_set_el_bounds(&SfxMute , sfx_r, sfx_p, sX, -1);
	ui_set_el_bounds(&SfxVol  , vol_r, sfv_p, fX, -1);
	ui_set_el_bounds(&BgmMute , bgm_r, bgm_p, bX, -1);
	ui_set_el_bounds(&BgmVol  , vol_r, bgv_p, vX, -1);
	ui_set_el_bounds(&Restart , rst_r, rst_p,  0,  0);
	ui_set_el_bounds(&PlayMode, ply_r, ply_p,  0,  0);
	ui_set_el_bounds(&HelpBtn , hlp_r, hlp_p,  0,  0);
	ui_set_el_bounds(&MyScore , bar_r, bar_p,  0, -1);
	ui_set_el_bounds(&HelpGuide,inf_r, inf_p,  0,  0);
	ui_set_el_bounds(&PlayList, trk_r, trk_p,  0,  0);

	ui_draw_image(Restart.img, iSCREEN, rst_p.offsetX, rst_p.offsetY);

	timer_pos = ui_cstr_rect(tPIXIL, "00 : 00");
	timer_pos.offsetX /= 2;

	ui_draw_cstr(tLIMON, SOUND_TXT PLMOD_TXT, iBOX, ui_new_offset(GAME_SCREEN_P-5, GAME_SCREEN_P));
	ui_draw_el_ico(&OptsBtn, NULL, iSCREEN), OptsBtn.fill.offsetY = opt_r.width;
	ui_draw_el_ico(&OptsBtn, NULL, iBOX);
	ui_draw_el_ico(&HelpBtn ,NULL, iSCREEN);

	if (PlayList.img)
		draw_Track_title(true);
# undef SOUND_TXT
# undef PLMOD_TXT
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
	upd_main_time(0, NULL);
	upd_main_hiscores();
	upd_main_score(is_new);
}

static SUCESS parse_main_args(const int alen, cstr_t args[])
{
	cstr_t p = NULL;

	for (int i = 1; i < alen; i++) {
		char s = args[i][0],
		     c = args[i][1],
		     k = args[i][2] == '=';

		switch (s == '-' ? c : '\0') {
			case 'P':
				p = &args[i][2 + k];
				break;
			default:
				continue;
		}
# ifdef DEBUG
		printf("\n- found arg:  %c%c%s\n", s,c,p);
#endif
	}
	return p ? SysExtractArgPath(&GameDir, p, 1) : SysGetExecPath(&GameDir);
}

int main(int argc, cstr_t argv[]) {

	path_t cfg_dir;
	bool rel = false;

	if (!parse_main_args(argc, argv)) {
		puts("Can't fing game directory\n");
		return -1;
	}
# ifdef DEBUG
	printf("\n- found gamedir:  %s\n", GameDir.path);
#endif

	if (!SysAcessConfigPath(&cfg_dir)) {
		puts("Can't acess home config directory check you permissions\n");
		return -1;
	}
# ifdef DEBUG
	fprintf(stderr, "- found config:   %s\n\n %s\n\n", cfg_dir.path,
		"Note: in debug-mode progress will not be saved.");
# else
	rel = game_load_session(&Board , &cfg_dir);
	      game_load_records(Records, &cfg_dir);
# endif
	// load settings before sound init
	/* */ game_load_settings(&Prefs, &cfg_dir);

	// Initialize SDL
	if (!ui_win_create(&Window, GAME_SCREEN_W, GAME_SCREEN_H)) {
		printf("Couldn't create SDL Window: %s\n", SDL_GetError());
		return -1;
	}
	// Load fonts
	if (!game_load_fonts(Fonts, GameDir)) {
		puts("Can't load game fonts\n");
		return -1;
	}
	// Initialize Graphic and UI
	if (!game_load_images(SVGs, GameDir)) {
		puts("Can't load game images\n");
		return -1;
	}
	ui_draw_image(iBG, iSCREEN, 0, 0);
	draw_Msg("Loading...",0);
	ui_win_update(&Window, iSCREEN);

	if (game_init_sound(&Sound, &Prefs, GameDir) && !Prefs.bgm_mute)
		play_bgm_track(Prefs.mus_num, GameDir);

	prep_main_balls();
	prep_main_ui();
	prep_main_board(!rel);
	loop_main_start();
	/* END GAME CODE HERE */
# ifndef DEBUG
	// save game progress
	    game_save_session(&Board , &cfg_dir);
	if (Player.store_records)
	    game_save_records(Records, &cfg_dir);
	if (Player.store_prefs)
	    game_save_settings(&Prefs, &cfg_dir);
# endif
	free_main_ui();
	sound_close_done(&Sound);
	ui_win_destroy(&Window);
	return 0;
}
