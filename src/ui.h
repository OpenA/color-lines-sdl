#ifndef _UI_H_
# include "cstr.h"
# include "cl_defines.h"
# include "gfx_sdl.h"
# define _UI_H_

#define el_img  SDL_Surface*
#define el_rect SDL_Rect

# define ATYPE_FP  f0
# define ATYPE_INT i0
# define ATYPE_SH1 h0
# define ATYPE_SH2 h1

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
	struct { unsigned short offsetX, offsetY; };
	struct { unsigned short width, height; };
} typecr_t;

typedef struct elem {
	struct el_rect rect;
	union  el_attr value;
	struct el_img  img;
	unsigned char  flags;
	union  typecr  fill;
} elem_t;

#define ui_is_on_el_rect(el, px, py) !(\
	px < (el)->rect.x || px >= (el)->rect.x + (el)->rect.w ||\
	py < (el)->rect.y || py >= (el)->rect.y + (el)->rect.h )

# define ui_new_el_rect(_x,_y,_w,_h) (struct el_rect){ .x = _x, .y = _y, .w = _w, .h = _h }

# define ui_set_el_value(el,A,_v)((el)->value.A = _v)
# define ui_set_el_bounds(el,_x,_y,_w,_h)\
	(el)->rect.x =_x, (el)->rect.y =_y,\
	(el)->rect.w =_w, (el)->rect.h =_h

# define ui_get_el_bitmap(el)    ((el)->img)
# define ui_get_el_bounds(el)    ((el)->rect)
# define ui_get_el_value(el,A)   ((el)->value.A)

# define ui_get_img_width(img)   ((img)->w)
# define ui_get_img_height(img)  ((img)->h)

# define ui_has_el_flag(el,FL)   ((el)->flags & FL)
# define ui_add_el_flag(el,FL)   ((el)->flags |= FL)
# define ui_del_el_flag(el,FL)   ((el)->flags &= ~FL)

static inline el_img ui_load_image(cstr_t file, SDL_bool transparent)
{
	el_img img = IMG_Load(file);
	if   ( img ) SDL_SetColorKey(img, transparent, img->format->format);
	return img;
}
/* fills horisontal/vertical bar with fill attribute */
static inline void ui_draw_el_bar(elem_t *el, el_img bg, el_img out)
{
	typecr_t fill = el->fill;

	el_rect or = ui_get_el_bounds(el),
	        ir = ui_new_el_rect(0, 0, or.w, or.h);

	if (bg) SDL_UpperBlit(bg, &or, out, &or);
	/* ~ */ SDL_UpperBlit(el->img, &ir, out, &or);

	if (fill.width && fill.height) {
		ir.y = or.h, ir.x = 0;
		ir.w = to_min(fill.width, or.w);
		ir.h = to_min(fill.height, or.h);
		SDL_UpperBlit(el->img, &ir, out, &or);
	}
}
/* draw trigger icon */
static inline void ui_draw_el_ico(elem_t *el, el_img bg, el_img out)
{
	typecr_t fill = el->fill;

	el_rect or = ui_get_el_bounds(el),
	        ir = ui_new_el_rect(fill.offsetX, fill.offsetY, or.w, or.h);

	if (bg) SDL_UpperBlit(bg, &or, out, &or);
	/* ~ */ SDL_UpperBlit(el->img, &ir, out, &or);
}

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
# define ui_font_new_typecaret(xw,yh)      (typecr_t){ .offsetX = xw, .offsetY = yh }

# define ui_make_cstr(fnt, txt) ui_make_text(fnt, txt, sizeof(txt) - 1)
# define ui_cstr_rect(fnt, txt) ui_text_rect(fnt, txt, sizeof(txt) - 1)

extern typecr_t ui_draw_char(zfont_t *fnt, const char c, const char f, el_img out, typecr_t p);
extern     void ui_init_font(zfont_t *fnt, el_img bitmap, measure_t g);
extern typecr_t ui_text_rect(zfont_t *fnt, cstr_t txt, const int len);
extern     void ui_draw_text(zfont_t *fnt, cstr_t txt, const int len, el_img out, typecr_t p);
inline   el_img ui_make_text(zfont_t *fnt, cstr_t txt, const int len);

# define ui_free_image(img) \
	SDL_FreeSurface(img), img = NULL
# define ui_draw_image(img, out, ox, oy) \
	ui_draw_source(img, ui_new_el_rect(0,0,(img)->w,(img)->h), out, ox, oy)
# define ui_scale_image(img, out, ox, oy, s) \
	ui_scale_source(img, ui_new_el_rect(0,0,(img)->w,(img)->h), out, ox, oy, s)

static inline void ui_draw_source(el_img img, el_rect ir, el_img out, int ox, int oy)
{
	el_rect or = ui_new_el_rect( ox, oy, ir.w, ir.h );
	SDL_UpperBlit(img, &ir, out, &or);
}

static inline void ui_scale_source(el_img img, el_rect ir, el_img out, int ox, int oy, float s)
{
	el_rect or = ui_new_el_rect( ox, oy, ir.w*s, ir.h*s );
	SDL_UpperBlitScaled(img, &ir, out, &or);
}

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
	SDL_DestroyTexture ( win->stream ), win->stream = NULL;
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
