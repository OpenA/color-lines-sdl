#ifndef _CONFIG_H_
# define CL_VERSION_MAJOR 1
# define CL_VERSION_MINOR 4
# define CL_VERSION_STRING "1.4"
# define _CONFIG_H_

# define CL_RECORDS_NAME "records"
# define CL_SESSION_NAME "session"
# define CL_PREFS_NAME   "prefs"

# define SOUND_DIR "sounds"
# define SOUND_EFFECTS_N 7
# define SOUND_EFFECTS_GET(i) (const char[][13]){\
	"click.wav",\
	"fadeout.wav",\
	"bonus.wav",\
	"boom.wav",\
	"paint.wav",\
	"hiscore.wav",\
	"gameover.wav" }[i]

# define MUSIC_DIR "music"
# define MUSIC_TRACKS_N 32

# include "autogen_conf.h"

# define FONT_DIR "vector"
# define FONT_NAME_N 3
# define FONT_NAME_GET(i) (const char[][16]){\
	"fnt_fancy.svg",\
	"fnt_pixil.svg",\
	"fnt_limon.svg" }[i]

# define UI_DIR "vector"
# define UI_ELEMENTS_N 5
# define UI_ELEMENT_GET(i) (const char[][13]){\
	"bg0.svg",\
	"cell.svg",\
	"cl_balls.svg",\
	"volume.svg",\
	"music.svg" }[i]

#endif //_CONFIG_H_
