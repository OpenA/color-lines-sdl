#include <stdbool.h>
#ifndef __SOUND_H__
#define __SOUND_H__

#define SND_CLICK    0
#define SND_BONUS    1
#define SND_HISCORE  2
#define SND_GAMEOVER 3
#define SND_BOOM     4
#define SND_FADEOUT  5
#define SND_PAINT    6

extern bool snd_init(char *g_datadir);
extern void snd_done(void);
extern void snd_play(int sample, int cnt);
extern bool snd_music_start(void);
extern void snd_music_stop(void);
extern void snd_volume(int vol);
#endif
