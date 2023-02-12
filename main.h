#ifndef MAIN_H

#include <stdbool.h>
#include "exstr.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef WINDOWS
	#include <windows.h>
	#include <shlobj.h>
	#ifndef PATH_MAX
		#define PATH_MAX MAX_PATH
	#endif

	#define GetConfigPath(dir) \
		SHGetFolderPath(\
			NULL, CSIDL_FLAG_CREATE | CSIDL_LOCAL_APPDATA,\
			NULL, 0, (LPTSTR)dir)
#else
	#include <limits.h>
	#include <unistd.h>
	#include <pwd.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#ifdef MACOS
		#include <mach-o/dyld.h>
	#endif

	#define GetConfigPath(dir) {\
		struct passwd *pw = getpwuid(getuid());\
		strcat(strcpy(dir, pw ? pw->pw_dir : "/tmp"), "/.config");\
		if(access(dir, F_OK ) == -1)\
		    mkdir(dir, 0755 );\
	}
#endif

#define _max(a,b) (((a)>(b))?(a):(b))   // developer: 'max' was a global define, so it was replaced to '_max'
#define _min(a,b) (((a)<(b))?(a):(b))   // developer: 'min' was a global define, so it was replaced to '_min'


#define SCREEN_W  800
#define SCREEN_H  480
#define BOARD_X  (800 - 450 - 15)
#define BOARD_Y   (15)
#define SCORES_X   60
#define SCORES_Y  225
#define SCORE_X     0
#define SCORE_Y   175
#define BALL_W     40
#define BALL_H     40
#define TILE_W     50
#define TILE_H     50

#define SIZE_STEPS   16
#define POOL_SPACE    8
#define ALPHA_STEPS  16
#define JUMP_STEPS    8
#define JUMP_MAX     .8
#define BALL_STEP    25
#define HISCORES_NR	  5
#define BONUS_BLINKS  4
#define BONUS_TIMER  40


extern void track_switch(void);

#endif //MAIN_H