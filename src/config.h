#ifndef _CONFIG_H_
# define CL_VERSION_MAJOR 1
# define CL_VERSION_MINOR 3
# define CL_VERSION_STRING "1.3"
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
# define MUSIC_TRACKS_LIST() \
	{"organ.mod"        , "PSC - Organ"             },\
	{"lastninja.mod"    , "PSC - Last Ninja"        },\
\
	{"satellite.s3m"    , "Purple Motion - Satellite one"},\
\
	{"allnightalone.mod", "CORE - All Night Alone"  },\
	{"aloneinaworld.xm" , "CORE - Alone in a World!"},\
	{"boondocks.xm"     , "CORE - Boondocks"        },\
	{"crossfire.xm"     , "CORE - CrossFire"        },\
	{"crystalhammer.mod", "CORE - Crystal Hammer"   },\
	{"flying.it"        , "CORE - ~~ Flying ~~"     },\
	{"hidninja.xm"      , "CORE - (( Hidden Ninja ))"},\
	{"hipchip2.xm"      , "CORE - HipChip2"         },\
	{"ihaveakey.xm"     , "CORE - I Have The Key to..."},\
	{"insidemyoldpc.mod", "CORE - Inside My Oldskool PC"},\
	{"jtechno.xm"       , "CORE - J-Techno"         },\
	{"kilobyte.xm"      , "CORE - Kilobyte"         },\
	{"meeting.xm"       , "CORE - ## meeting ##"    },\
	{"melancolie.s3m"   , "CORE - MELANCOLIE of S.UMMER"},\
	{"modernsurf.xm"    , "CORE - Modern Surf"      },\
	{"monastery.xm"     , "CORE - Mary's Monastery" },\
	{"tikob_ii.mod"     , "CORE - Ticob II"         },\
	{"ringsofmedusa.xm" , "CORE - Rings of Medusa"  },\
	{"rusinahumppa.mod" , "CORE - Rusinahumppa"     },\
	{"4stonewalls.it"   , "CORE - Four Stone Walls" },\
	{"offencyt.xm"      , "CORE - Offence you too"  },\
	{"someone.mod"      , "CORE - One on Someone"   },\
	{"skogensdjur.xm"   , "CORE - Skogens Djur 011" },\
	{"trackerode.xm"    , "CORE - Ode to Tracker"   },\
	{"xmas.xm"          , "CORE - X-mas Spirit"     },\
	{"wonderstar.xm"    , "CORE - Wonderstar"       },\
	{"denkade.xm"       , "CORE - Nice Fruit/DenkaDe"},\
	{"cryobed.xm"       , "CORE - Mr.Spock's Cryo-Bed"},\
	{"genchip.xm"       , "CORE - Generation (C)hip"}

# define UI_DIR "vector"
# define UI_ELEMENTS_N 5
# define UI_ELEMENT_GET(i) (const char[][13]){\
	"bg0.svg",\
	"cell.svg",\
	"cl_balls.svg",\
	"volume.svg",\
	"music.svg" }[i]

#endif //_CONFIG_H_
