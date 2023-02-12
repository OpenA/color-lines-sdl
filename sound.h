#ifndef __SOUND_H__
#define __SOUND_H__

#define SND_CLICK    0
#define SND_BONUS    1
#define SND_HISCORE  2
#define SND_GAMEOVER 3
#define SND_BOOM     4
#define SND_FADEOUT  5
#define SND_PAINT    6
#define TRACKS_COUNT 26

extern bool snd_init(const char *snd_path);
extern void snd_done(void);
extern void snd_play(int sample, int cnt);
extern void snd_music_start(short num, char *name, const char *path);
extern void snd_music_stop(void);
extern void snd_volume(short vol);
#endif
