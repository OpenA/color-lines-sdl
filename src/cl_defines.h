#ifndef _CL_DEFINES_H_
# define to_max(a,b) (a)>(b)?(a):(b) // 'max' was a global define
# define to_min(a,b) (a)<(b)?(a):(b) // 'min' was a global define
# define _CL_DEFINES_H_

# define to_range(w,i,imx) (w * (i * 100 / imx) / 100)

# define conf_param_add(cfg, FL) ((cfg)->flags |=  FL)
# define conf_param_has(cfg, FL) ((cfg)->flags &   FL)
# define conf_param_del(cfg, FL) ((cfg)->flags &= ~FL)

// Fonts Defines
# define FONT_LETTER_R 3
# define FONT_LETTER_N 32
# define FONT_LETTER_L (FONT_LETTER_N * FONT_LETTER_R)

# define FONT_FANCY_PADDING 0
# define FONT_FANCY_WIDTH  24
# define FONT_FANCY_HEIGHT 28
# define FONT_FANCY_SPACE   8

# define FONT_PIXIL_PADDING 2
# define FONT_PIXIL_WIDTH  28
# define FONT_PIXIL_HEIGHT 28
# define FONT_PIXIL_SPACE   6

# define FONT_LIMON_PADDING 0
# define FONT_LIMON_WIDTH  24
# define FONT_LIMON_HEIGHT 28
# define FONT_LIMON_SPACE  12

# define FONT_GET_HLINE(_N) (FONT_##_N##_HEIGHT + FONT_##_N##_PADDING*2)

// Board Defines
# define BOARD_DESK_W  9
# define BOARD_DESK_H  9
# define BOARD_POOL_N  3
# define BOARD_DESK_N (BOARD_DESK_W * BOARD_DESK_H)

# define BOARD_BALL_W 40
# define BOARD_BALL_H 40
# define BOARD_TILE_W 50
# define BOARD_TILE_H 50

// Balls Defines
# define BALL_COLOR_N  7
# define BALL_BONUS_N  4
# define BALL_BONUS_D  5
# define BALL_COLOR_D  5

# define BALLS_COUNT_L (BALL_COLOR_N+BALL_BONUS_N)

# define BALL_ALPHA_N 16
# define BALL_ALPHA_S (0xFF / BALL_ALPHA_N)
# define BALL_SIZES_N 16
# define BALL_SIZES_S (1.0f / BALL_SIZES_N)
# define BALL_JUMPS_N 8
# define BALL_JUMPS_S (1.0f - BALL_JUMPS_N * 0.1f) / BALL_JUMPS_N

// Game Defines
# define GAME_SCREEN_W 800
# define GAME_SCREEN_H 480
# define GAME_SCREEN_P 15

# define GAME_BOARD_W (BOARD_TILE_W * BOARD_DESK_W)
# define GAME_BOARD_H (BOARD_TILE_H * BOARD_DESK_H)
# define GAME_BOARD_X (GAME_SCREEN_W - GAME_BOARD_W - 15)
# define GAME_BOARD_Y 15

# define IS_OVER_GAME_BOARD(x,y) (\
	x > GAME_BOARD_X && x < GAME_BOARD_X + GAME_BOARD_W &&\
	y > GAME_BOARD_Y && y < GAME_BOARD_Y + GAME_BOARD_H)

# define SCORE_TAB_X GAME_SCREEN_P
# define SCORE_TAB_Y 75
# define SCORE_TAB_W 175
# define SCORE_TAB_ROW(_n) (SCORE_TAB_Y + FONT_GET_HLINE(FANCY) *_n)

// Info Defines
# define INFO_X (GAME_BOARD_X - BOARD_TILE_W - BOARD_POOL_N - 20)
# define INFO_W (GAME_BOARD_W + BOARD_TILE_W + BOARD_POOL_N + 20)

# define INFO_Y_JOCKER FONT_GET_HLINE(FANCY) * 18
# define INFO_Y_FLUSH  FONT_GET_HLINE(FANCY) * 21
# define INFO_Y_BRUSH  FONT_GET_HLINE(FANCY) * 24
# define INFO_Y_BOMB   FONT_GET_HLINE(FANCY) * 27

#endif //_CL_DEFINES_
