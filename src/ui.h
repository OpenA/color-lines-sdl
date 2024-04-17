#ifndef _UI_H_
# include "cstr.h"
# include "cl_defines.h"
# include "../graphics.c"
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

typedef struct {
	struct el_rect rect;
	union  el_attr value;
	struct el_img  img;
	unsigned char  flags;
	unsigned short px,py;
} elem_t;

#define ui_has_el_on_coords(el, px, py) !(\
	px < (el)->rect.x || px >= (el)->rect.x + (el)->rect.w ||\
	py < (el)->rect.y || py >= (el)->rect.y + (el)->rect.h )

# define ui_new_el_rect(_x,_y,_w,_h) (struct el_rect){ .x = _x, .y = _y, .w = _w, .h = _h }

# define ui_set_el_value(el,A,_v)((el)->value.A = _v)
# define ui_set_el_bounds(el,_r) ((el)->rect    = _r)

# define ui_get_el_bounds(el)    ((el)->rect)
# define ui_get_el_value(el,A)   ((el)->value.A)
# define ui_get_el_width(el)     ((el)->img->w)
# define ui_get_el_height(el)    ((el)->img->h)

# define ui_has_el_flag(el,FL)   ((el)->flags & FL)
# define ui_add_el_flag(el,FL)   ((el)->flags |= FL)
# define ui_del_el_flag(el,FL)   ((el)->flags &= ~FL)

static inline el_img ui_load_image(cstr_t file)
{	
	el_img img = IMG_Load(file);

	if (img) {
		SDL_SetColorKey(img, SDL_HasColorKey(img), img->format->format);
	}
	return img;
}
/* draw horisontal bar */
static inline void ui_draw_Hbar(elem_t *el, el_img bg, el_img blank, int fill_w)
{
	el_rect b0 = ui_get_el_bounds(el),
	        b1 = ui_new_el_rect(0, 0, b0.w, b0.h),
	        b2 = ui_new_el_rect(0, b0.h, fill_w, b0.h);

	SDL_UpperBlit(bg     , &b0, blank, &b0);
	SDL_UpperBlit(el->img, &b1, blank, &b0);
	SDL_UpperBlit(el->img, &b2, blank, &b0);
}
/* draw trigger icon */
static inline void ui_draw_Tico(elem_t *el, el_img bg, el_img blank, bool is_on)
{
	el_rect b0 = ui_get_el_bounds(el),
	        b1 = ui_new_el_rect(0, 0, b0.w, b0.h),
	        b2 = ui_new_el_rect(0, b0.h, b0.w, b0.h);

	SDL_UpperBlit(bg     , &b0, blank, &b0);
	SDL_UpperBlit(el->img, &b1, blank, &b0);
	if (is_on)
		SDL_UpperBlit(el->img, &b2, blank, &b0);
}

/** u8x4 ~ u16x2
 `| charW | lineH | padding | space |`
 `| --- width --- | ---- offset --- |`*/
typedef union {
	struct { unsigned char  padding, space, charW, lineH; };
	struct { unsigned short offset, width; };
} measure_t;

/** u16x2 ~ u16x2
 `| - offsetX - | - offsetY - |`
 `| -- width -- | -- height - |`*/
typedef union {
	struct { unsigned short offsetX, offsetY; };
	struct { unsigned short width, height; };
} typecr_t;

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

# define ui_draw_ctxt(fnt, txt) ui_make_text(fnt, txt, sizeof(txt) - 1)

extern typecr_t ui_draw_char(zfont_t *fnt, el_img out, const char c, typecr_t p);
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

#endif
