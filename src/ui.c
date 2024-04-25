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
		if (txt[i] == '\n')
			h += FONT_G_PADDING + FONT_G_HEIGHT, p = 0;
		else
		if (txt[i] == ' ')
			p += FONT_G_PADDING + FONT_G_SPACE;
		else
			p += FONT_G_PADDING + ui_font_get_letter_width(fnt, txt[i] - 0x21);
		if (p > w)
			w = p;
	}
	return ui_font_new_typecaret(w,h);
}

inline typecr_t ui_draw_char(zfont_t *fnt, const char c, const char f, el_img out, typecr_t p)
{
	measure_t g = ui_font_get_measures(fnt);

	if (c == '\n') {
		p.offsetY += FONT_G_PADDING + FONT_G_HEIGHT;
		p.offsetX  = 0;
	} else
	if (c == ' ') {
		p.offsetX += FONT_G_PADDING + FONT_G_SPACE;
	} else {
		int i = c - 0x21,
		    n = i / FONT_LETTER_N,
		    l = i - FONT_LETTER_N * n,
		    x = l * FONT_G_WIDTH,
		    y = n * FONT_G_HEIGHT,
		    s = ui_font_get_letter_offset(fnt, i),
		    w = ui_font_get_letter_width(fnt, i);

		el_rect ir = ui_new_el_rect(s + x, y, w, FONT_G_HEIGHT);
		if (c != f)
			ui_draw_source(fnt->bitmap, ir, out, p.offsetX, p.offsetY);
		p.offsetX += FONT_G_PADDING + w;
	}
	return p;
}

inline void ui_draw_text(zfont_t *fnt, cstr_t txt, const int len, el_img out, typecr_t p)
{
	for (int i = 0; i < len && txt[i] != '\0'; i++)
		p = ui_draw_char(fnt, txt[i], ' ', out, p);
}

el_img ui_make_text(zfont_t *fnt, cstr_t txt, const int len)
{
	typecr_t m = ui_text_rect(fnt, txt, len);
	el_img img = SDL_CreateRGBSurface(0, m.width, m.height, 32,
		fnt->bitmap->format->Rmask,
		fnt->bitmap->format->Gmask,
		fnt->bitmap->format->Bmask,
		fnt->bitmap->format->Amask);

	ui_draw_text(fnt, txt, len, img, ui_font_new_typecaret(0,0));
	return img;
}

void ui_init_font(zfont_t *fnt, el_img bitmap, measure_t g)
{
	Uint32 *pixels = bitmap->pixels,
	         Amask = bitmap->format->Amask,
	         pitch = bitmap->pitch;
	   fnt->bitmap = bitmap;
	   fnt->dims   = g;

# define has_pixel_alpha(_x,_y) pixels[(_y) * pitch / 4 + (_x)] & Amask

	int i, j, n, y, l, r;

	for (n = j = 0; j < FONT_LETTER_R; j++) {
		for (i = 0; i < FONT_LETTER_N; i++, n++) {

			unsigned int o = -1, ax = i * FONT_G_WIDTH,
			             w =  0, ay = j * FONT_G_HEIGHT;

			for (y = 0; y < FONT_G_HEIGHT; y++) {
				for (l = 0; l < FONT_G_WIDTH; l++) {
					if (has_pixel_alpha(ax + l, ay + y))
						break;
				}
				for (r = FONT_G_WIDTH - 1; r >= 0; r--) {
					if (has_pixel_alpha(ax + r, ay + y))
						break;
				}
				if (l < o)
					o = l;
				if (r >= l && (r - l + 1) > w)
					w = r - l + 1;
			}
			ui_font_set_letter(fnt, n, o, w);
		}
	}
# undef has_pixel_alpha
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
