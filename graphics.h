#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#define FONT_WIDTH 24
#define TTF_PX     14

typedef void* img_t;
extern bool gfx_init  (void);
extern void gfx_update(void);
extern void gfx_done  (void);
extern void gfx_clear(int x, int y, int w, int h);
extern void gfx_draw(img_t pixmap, int x, int y);
extern void gfx_draw_wh(img_t p, int x, int y, int w, int h);
extern img_t gfx_grab_screen(int x, int y, int w, int h);
extern img_t gfx_draw_ttf_text(char *text);
extern img_t gfx_load_image(const char *file, bool alpha);
extern void gfx_free_image(img_t pixmap);
extern int gfx_img_w(img_t pixmap);
extern int gfx_img_h(img_t pixmap);
extern img_t gfx_combine(img_t src, img_t dst);
extern img_t gfx_set_alpha(img_t src, int alpha);
extern img_t gfx_scale(img_t src, float xscale, float yscale);
extern void gfx_draw_bg(img_t p, int x, int y, int w, int h);

extern int gfx_draw_char  ( const char  c,   int x, int y, float s);
extern int gfx_draw_text  ( const char *str, int x, int y, float s);
extern int gfx_chars_width( const char *str);
extern int gfx_font_height( void );
#endif
