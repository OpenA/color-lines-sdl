#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

typedef void* img_t;
typedef void* fnt_t;
extern bool gfx_init(char *g_datadir);
extern void gfx_update(void);
extern void gfx_done(void);
extern img_t gfx_load_ttf_font(char *font, char *text, int px);
extern void gfx_clear(int x, int y, int w, int h);
extern void gfx_draw(img_t pixmap, int x, int y);
extern void gfx_draw_wh(img_t p, int x, int y, int w, int h);
extern img_t gfx_grab_screen(int x, int y, int w, int h);
extern img_t gfx_load_image(char *filename, bool transparent);
extern void gfx_free_image(img_t pixmap);
extern int gfx_img_w(img_t pixmap);
extern int gfx_img_h(img_t pixmap);
extern img_t gfx_combine(img_t src, img_t dst);
extern img_t gfx_set_alpha(img_t src, int alpha);
extern img_t gfx_scale(img_t src, float xscale, float yscale);
extern void gfx_draw_bg(img_t p, int x, int y, int width, int height);
extern fnt_t gfx_load_font(char *fname, int w);
extern int gfx_draw_char(fnt_t f, const char c, int x, int y, float scale);
extern int gfx_draw_chars(fnt_t f, const char *str, int x, int y, float scale);
extern int gfx_chars_width(fnt_t f, const char *str);
extern int gfx_font_height(fnt_t f);
extern void gfx_font_free(fnt_t f);
#endif
