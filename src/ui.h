#ifndef _UI_H_
# include "cstr.h"
# include "config.h"
# include "../graphics.c"
# define _UI_H_

#define el_img  SDL_Surface*
#define el_rect SDL_Rect

union el_attr { int i; float f; };

typedef struct {
	struct el_rect rect;
	union  el_attr value;
	struct el_img  img;
	unsigned char  flags;
	cstr_t text;
} elem_t;

#define IS_ON_ELEM(el, px, py) !(\
	px < (el)->rect.x || px >= (el)->rect.x + (el)->rect.w ||\
	py < (el)->rect.y || py >= (el)->rect.y + (el)->rect.h )

# define NEW_BOUNDS_RECT(_x,_y,_w,_h) (struct el_rect)\
	{ .x = _x, .y = _y, .w = _w, .h = _h }

# define ui_set_el_ivalue(el,_i) ((el)->value.i = _i)
# define ui_set_el_fvalue(el,_f) ((el)->value.f = _f)
# define ui_set_el_bounds(el,_r) ((el)->rect    = _r)

# define ui_get_el_bounds(el)    ((el)->rect)
# define ui_get_el_ivalue(el)    ((el)->value.i)
# define ui_get_el_fvalue(el)    ((el)->value.f)
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
	        b1 = NEW_BOUNDS_RECT(0, 0, b0.w, b0.h),
	        b2 = NEW_BOUNDS_RECT(0, b0.h, fill_w, b0.h);

	SDL_UpperBlit(bg     , &b0, blank, &b0);
	SDL_UpperBlit(el->img, &b1, blank, &b0);
	SDL_UpperBlit(el->img, &b2, blank, &b0);
}
/* draw trigger icon */
static inline void ui_draw_Tico(elem_t *el, el_img bg, el_img blank, bool is_on)
{
	el_rect b0 = ui_get_el_bounds(el),
	        b1 = NEW_BOUNDS_RECT(0, 0, b0.w, b0.h),
	        b2 = NEW_BOUNDS_RECT(0, b0.h, b0.w, b0.h);

	SDL_UpperBlit(bg     , &b0, blank, &b0);
	SDL_UpperBlit(el->img, &b1, blank, &b0);
	if (is_on)
		SDL_UpperBlit(el->img, &b2, blank, &b0);
}

#endif