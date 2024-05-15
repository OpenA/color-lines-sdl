#include "ui.h"

# define FONT_G_WIDTH   g.charW
# define FONT_G_HEIGHT  g.lineH
# define FONT_G_SPACE   g.space
# define FONT_G_PADDING g.padding

typecr_t ui_text_rect(zfont_t *fnt, cstr_t txt, const int len)
{
	measure_t g = ui_font_get_measures(fnt);
	int i,p,w,h = FONT_G_HEIGHT;

	for (i = p = w = 0; i < len && txt[i] != '\0'; i++) {
		/*~*/if (txt[i] == '\n')
			h += FONT_G_PADDING + FONT_G_HEIGHT, p = 0;
		else if (txt[i] == ' ')
			p += FONT_G_PADDING + FONT_G_SPACE;
		else if (txt[i] > 0x20)
			p += FONT_G_PADDING + ui_font_get_letter_width(fnt, txt[i] - 0x21);
		if (p > w)
			w = p;
	}
	return ui_new_rectsz(w,h);
}

inline typecr_t ui_draw_char(zfont_t *fnt, unsigned char c, unsigned char f, el_img out, typecr_t p)
{
	measure_t g = ui_font_get_measures(fnt);

	/*~~*/ if (c == '\n') {
		p.offsetY += FONT_G_PADDING + FONT_G_HEIGHT;
		p.offsetX  = 0;
	} else if (c == ' ') {
		p.offsetX += FONT_G_PADDING + FONT_G_SPACE;
	} else if (c > 0x20) {
		int i = c - 0x21,
		    y = i / FONT_LETTER_N * FONT_G_HEIGHT,
		    x = ui_font_get_letter_offset(fnt, i),
		    w = ui_font_get_letter_width(fnt, i);

		el_rect ir = ui_new_el_rect(x, y, w, FONT_G_HEIGHT);
		if (c != f)
			ui_draw_source(fnt->bitmap, ir, out, p.offsetX, p.offsetY);
		p.offsetX += FONT_G_PADDING + w;
	}
	return p;
}

inline void ui_draw_text(zfont_t *fnt, cstr_t txt, const int len, el_img out, typecr_t sp)
{
	typecr_t p = sp;
	for (int i = 0; i < len && txt[i] != '\0'; i++) {
		p = ui_draw_char(fnt, txt[i], ' ', out, p);
		if (txt[i] == '\n')
			p.offsetX = sp.offsetX;
	}
}

el_img ui_make_text(zfont_t *fnt, cstr_t txt, const int len)
{
	typecr_t m = ui_text_rect(fnt, txt, len);
	el_img img = SDL_CreateRGBSurface(0, m.width, m.height, 32,
		fnt->bitmap->format->Rmask,
		fnt->bitmap->format->Gmask,
		fnt->bitmap->format->Bmask,
		fnt->bitmap->format->Amask);

	typecr_t p = ui_new_offset(0,0);
	for (int i = 0; i < len && txt[i] != '\0'; i++)
		p = ui_draw_char(fnt, txt[i], ' ', img, p);
	return img;
}

void ui_init_font(zfont_t *fnt, el_img bitmap, measure_t g)
{
	Uint32 *pixels = bitmap->pixels,
	         Amask = bitmap->format->Amask;
	   fnt->bitmap = bitmap;
	   fnt->dims   = g;

	int i,j,n,k, y,x, o,w, ax;
	measure_t m;

	for (n = 0; n < FONT_LETTER_L; n++)
		ui_font_set_letter(fnt, n, -1, 0);
	for (k = j = 0; j < FONT_LETTER_R; j++) {
# pragma loop count(28)
		for (y = 0; y < FONT_G_HEIGHT; y++, ax = 0)
		for (i = 0; i < FONT_LETTER_N; i++, ax += FONT_G_WIDTH) {
			 o = w = 0, m = fnt->lmap[(n = j * FONT_LETTER_N + i)];
# pragma loop count(24)
			for (x = 0; x < FONT_G_WIDTH ; x++, k++) {
				if (pixels[k] & Amask)
					w = x;
				else if (w == 0)
					o = x;
			}
			if (m.offset > (o + ax))
				m.offset = (o + ax);
			if (m.width  < (w - o + 1) && w > o)
				m.width  = (w - o + 1);
			fnt->lmap[n] = m;
		}
	}
}

/* fills horisontal/vertical bar with fill attribute */
void ui_draw_el_bar(elem_t *el, el_img restrict bg, el_img restrict out, fill_m mode)
{
	el_img img = ui_get_el_bitmap(el);
	el_rect r0 = ui_get_el_bounds(el),
	        r1 = ui_new_el_rect(0, 0, r0.w, r0.h),
	        r2 = ui_new_el_rect(0, 0, el->fill.width, el->fill.height);

	bool fb = r2.w && r2.h;
	if ( fb )
		r2.w = to_min(r2.w, r0.w),
		r2.h = to_min(r2.h, r0.h);

	switch (fb ? mode : -1) {
	case HR_Outside: r2.x = r0.w - r2.w;
	case HL_Outside: r2.y = r0.h;
		break;
	case HR_Inside:  r1.x = r0.w - r2.w;
	case HL_Inside:
		r1.w = r2.w, r2.w = r0.w,
		r1.h = r2.h, r2.h = r1.y = r0.h;
		break;
	default: /* skip */
	}
	if (bg) SDL_UpperBlit(bg , &r0, out, &r0);
	/* ~ */ SDL_UpperBlit(img, &r1, out, &r0);
	if (fb) SDL_UpperBlit(img, &r2, out, &r0);
}

/* draw trigger icon */
void ui_draw_el_ico(elem_t *el, el_img restrict bg, el_img restrict out)
{
	el_img img = ui_get_el_bitmap(el);
	el_rect or = ui_get_el_bounds(el),
	        ir = ui_new_el_rect(el->fill.offsetX, el->fill.offsetY, or.w, or.h);

	if (bg) SDL_UpperBlit(bg , &or, out, &or);
	/* ~ */ SDL_UpperBlit(img, &ir, out, &or);
}

void ui_draw_scroll(elem_t *el, el_img restrict bg, el_img restrict out, int x, int y, bool save_pos)
{
	el_img img = ui_get_el_bitmap(el);
	el_rect or = ui_get_el_bounds(el),
	        ir = ui_new_el_rect(x, y, or.w, or.h);

//	ir.x += el->fill.offsetX;
	ir.y += el->fill.offsetY;

	if (save_pos) {
//		el->fill.offsetX = ir.x > or.w ? (ir.x = or.w) : ir.x < 0 ? (ir.x = 0) : ir.x;
		el->fill.offsetY = ir.y > or.h ? (ir.y = or.h) : ir.y < 0 ? (ir.y = 0) : ir.y;
	}
	SDL_UpperBlit(bg , &or, out, &or);
	SDL_UpperBlit(img, &ir, out, &or);
}

int ui_win_create(window_t *win, int scr_w, int scr_h)
{
	int ec = !SDL_Init(
		SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS
	);
	if (ec && (ec = !SDL_CreateWindowAndRenderer(scr_w, scr_h,
#ifdef CL_MOBILE
		SDL_WINDOW_FULLSCREEN_DESKTOP
#else
		SDL_WINDOW_RESIZABLE
#endif
	, &win->sdlWin, &win->render))
	) {
		SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY , "linear" );
		SDL_RenderSetLogicalSize( win->render, scr_w, scr_h );
		win->stream = SDL_CreateTexture( win->render,
			SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STREAMING,
		scr_w, scr_h );
	}
	return ec;
}

# undef FONT_G_WIDTH
# undef FONT_G_HEIGHT
# undef FONT_G_SPACE
# undef FONT_G_PADDING
