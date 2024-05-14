#ifndef _UI_H_
# include "cstr.h"
# include "cl_defines.h"
# include "gfx_sdl.h"
# define _UI_H_

#define el_img  SDL_Surface*
#define el_rect SDL_Rect

typedef enum {
	/* Left/Right Horisontal */
	HL_Outside, HR_Outside,
	HL_Inside , HR_Inside,
	/* Bottom/Top Vertical */
	VB_Outside, VT_Outside,
	VB_Inside , VT_Inside,
} fill_m;

// A uiversal attribute for 32 bit value
typedef union el_attr {
	struct { short h0, h1; };
	struct { signed i0; };
	struct { float f0; };
} el_attr;

/** u16x2 ~ u16x2
 `| - offsetX - | - offsetY - |`
 `| -- width -- | -- height - |`*/
typedef union typecr {
	struct { signed short offsetX, offsetY; };
	struct { unsigned short width, height; };
} typecr_t;

typedef struct elem {
	union  typecr  pos, rect, fill;
	union  el_attr value;
	struct el_img  img;
} elem_t;

#define ui_is_on_el_rect(el,_x,_y) !(\
	_x < (el)->pos.offsetX || _x >= ((el)->pos.offsetX + (el)->rect.width) ||\
	_y < (el)->pos.offsetY || _y >= ((el)->pos.offsetY + (el)->rect.height) )

# define ui_new_rectsz(_w,_h) (typecr_t){ .width   = _w, .height  = _h }
# define ui_new_offset(_x,_y) (typecr_t){ .offsetX = _x, .offsetY = _y }

# define ui_get_img_Hcrect(img) (typecr_t){ .width = (img)->w, .height = ((img)->h/2) }
# define ui_get_img_Vcrect(img) (typecr_t){ .height = (img)->h, .width = ((img)->w/2) }

# define ui_new_el_rect(_x,_y,_w,_h) (struct el_rect){\
	.x = _x, .y = _y, .w = _w, .h = _h\
}
# define ui_set_el_bounds(el,_r,_p,fw,fh) \
	(el)->rect =_r, (el)->pos =_p, (el)->fill.width =fw, (el)->fill.height =fh\

# define ui_get_el_bitmap(el) ((el)->img)
# define ui_get_el_bounds(el) ui_new_el_rect(\
	(el)->pos.offsetX, (el)->pos.offsetY, (el)->rect.width, (el)->rect.height\
)

static inline el_img ui_load_image(cstr_t file, SDL_bool transparent)
{
	el_img img = IMG_Load(file);
	if   ( img ) SDL_SetColorKey(img, transparent, img->format->format);
	return img;
}

extern void ui_draw_scroll(elem_t *el, el_img restrict bg, el_img restrict out, int x, int y, bool save_pos);
extern void ui_draw_el_bar(elem_t *el, el_img restrict bg, el_img restrict out, fill_m mode);
extern void ui_draw_el_ico(elem_t *el, el_img restrict bg, el_img restrict out);

/** u8x4 ~ u16x2
 `| charW | lineH | padding | space |`
 `| --- width --- | ---- offset --- |`*/
typedef union {
	struct { unsigned char  padding, space, charW, lineH; };
	struct { unsigned short offset, width; };
} measure_t;

typedef struct {
	el_img bitmap;
	measure_t dims, lmap[FONT_LETTER_N * FONT_LETTER_R];
} zfont_t;

# define ui_font_set_letter(fnt,n,_o,_w)  (fnt)->lmap[n].offset =_o, (fnt)->lmap[n].width =_w
# define ui_font_get_letter_width(fnt,n)  (fnt)->lmap[n].width
# define ui_font_get_letter_offset(fnt,n) (fnt)->lmap[n].offset
# define ui_font_get_measures(fnt)        (fnt)->dims
# define ui_font_new_measures(_w,_h,_s,_p)(measure_t){ .charW =_w, .lineH =_h, .space =_s, .padding =_p }

# define ui_make_cstr(fnt, txt) ui_make_text(fnt, txt, sizeof(txt) - 1)
# define ui_cstr_rect(fnt, txt) ui_text_rect(fnt, txt, sizeof(txt) - 1)
# define ui_draw_cstr(fnt, txt, out,p) ui_draw_text(fnt, txt, sizeof(txt) - 1, out, p);

extern typecr_t ui_draw_char(zfont_t *fnt, unsigned char c, unsigned char f, el_img out, typecr_t p);
extern     void ui_init_font(zfont_t *fnt, el_img bitmap, measure_t g);
extern typecr_t ui_text_rect(zfont_t *fnt, cstr_t txt, const int len);
extern     void ui_draw_text(zfont_t *fnt, cstr_t txt, const int len, el_img out, typecr_t p);
inline   el_img ui_make_text(zfont_t *fnt, cstr_t txt, const int len);

# define ui_free_image(img) \
	SDL_FreeSurface(img), img = NULL
# define ui_draw_image(img, out, ox, oy) \
	SDL_UpperBlit(img, NULL, out, &ui_new_el_rect(ox,oy,(img)->w,(img)->h))
# define ui_scale_image(img, out, ox, oy, s) \
	SDL_UpperBlitScaled(img, NULL, out, &ui_new_el_rect(ox,oy,(img)->w*s,(img)->h*s))
# define ui_draw_source(img, ir, out, ox, oy) \
	SDL_UpperBlit(img, &ir, out, &ui_new_el_rect(ox,oy,ir.w,ir.h))
# define ui_scale_source(img, ir, out, ox, oy, s) \
	SDL_UpperBlitScaled(img, &ir, out, &ui_new_el_rect(ox,oy,ir.w*s,ir.h*s))

typedef struct {
	SDL_Window   *sdlWin;
	SDL_Renderer *render;
	SDL_Texture  *stream;
} window_t;

extern signed int  ui_win_create(window_t *win, int scr_w, int scr_h);
static inline void ui_win_update(window_t *win, el_img res) {
	SDL_UpdateTexture( win->stream, NULL, res->pixels, res->pitch );
	SDL_RenderClear  ( win->render );
	SDL_RenderCopy   ( win->render, win->stream, NULL, NULL );
	SDL_RenderPresent( win->render );
};
static inline void ui_win_destroy(window_t *win) {
//	SDL_DestroyTexture ( win->stream ), win->stream = NULL;
	SDL_DestroyRenderer( win->render ), win->render = NULL;
	SDL_DestroyWindow  ( win->sdlWin ), win->sdlWin = NULL;
	SDL_Quit();
};
static inline void ui_push_event(int code) {
	SDL_Event usr_ev;
	usr_ev.type = SDL_USEREVENT;
	usr_ev.user.code = code;
	SDL_PushEvent(&usr_ev);
}

#endif
