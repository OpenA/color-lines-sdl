#ifndef _CONFIG_H_
# define CL_VERSION_MAJOR 1
# define CL_VERSION_MINOR 4
# define CL_VERSION_STRING "1.4"
# define _CONFIG_H_

# define CL_RECORDS_NAME "records"
# define CL_SESSION_NAME "session"
# define CL_PREFS_NAME   "settings.cfg"

# define CFG_P_SFX "SFXVolume="
# define CFG_P_BGM "BGMVolume="
# define CFG_P_MUS "PlayTrack="
# define CFG_GET_STR(_N) "["_N"]\n"CFG_P_SFX"%i\n"CFG_P_BGM"%i\n"CFG_P_MUS"%i\n"

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

# define SVG_DIR "vector"
# define SVG_IMAGES_N 9
# define SVG_IMAGE_GET(i) (const char[][16]){\
	"bg0.svg",\
	"cell.svg",\
	"sbox.svg",\
	"gear.svg",\
	"cl_balls.svg",\
	"progress.svg",\
	"volume.svg",\
	"sound.svg",\
	"music.svg" }[i]

#endif //_CONFIG_H_
