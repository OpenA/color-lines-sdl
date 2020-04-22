#include <SDL.h>
#include <SDL_thread.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef WINDOWS
	#include <windows.h>
	#include <shlobj.h>
	#ifndef PATH_MAX
		#define PATH_MAX MAX_PATH
	#endif
#else
	#include <limits.h>
	#include <unistd.h>
	#include <pwd.h>
	
	#ifdef MACOS
		#include <mach-o/dyld.h>
		//#include <sys/syslimits.h>
	#endif
#endif

#define _max(a,b) (((a)>(b))?(a):(b))   // developer: 'max' was a global define, so it was replaced to '_max'
#define _min(a,b) (((a)<(b))?(a):(b))   // developer: 'min' was a global define, so it was replaced to '_min'

#define SCREEN_W 800
#define SCREEN_H 480

extern void track_switch(void);
char GAME_DIR[ PATH_MAX ];
