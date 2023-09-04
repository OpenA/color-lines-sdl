#include <math.h>
#include "graphics.h"
#include "src/cstr.h"

static SDL_Window *sdlWindow;
static SDL_Renderer *sdlRenderer;
static SDL_Texture *sdlTexture;
static SDL_Surface *screen;

img_t gfx_grab_screen(int x, int y, int w, int h)
{
	SDL_Rect dest, src;
	SDL_Surface * img;
	if (!(img = SDL_CreateRGBSurface(0, w, h, 16, 0xF800, 0x7E0, 0x1F, 0)))
		return NULL;
	src.x = x; dest.x = 0;
	src.y = y; dest.y = 0;
	src.w = dest.w = w;
	src.h = dest.h = h;
	SDL_BlitSurface(screen, &src, img, &dest);
	return img;
}

img_t gfx_set_alpha(img_t src, int alpha)
{
	SDL_Surface * img;

	if (!(img = SDL_ConvertSurfaceFormat((SDL_Surface*)src, SDL_PIXELFORMAT_RGBA8888, 0)))
		return NULL;
	
	Uint32 *ptr = (Uint32*)img->pixels;
	Uint32 size = img->w * img->h;
	for (; size > 0 ; size--, ptr++) {
		Uint8 r, g, b, a;
		Uint32 col = *ptr;
		SDL_GetRGBA(col, img->format, &r, &g, &b, &a);
		col = SDL_MapRGBA(img->format, r, g, b, a * alpha / 255);
		*ptr = col;
	}
	return img;
}

img_t gfx_combine(img_t src, img_t dst)
{
	SDL_Surface * new;
	if (!(new = SDL_ConvertSurfaceFormat(dst, SDL_PIXELFORMAT_RGBA8888, 0)))
		return NULL;
	SDL_BlitSurface((SDL_Surface *)src, NULL, new, NULL);
	return new;
}

img_t gfx_load_image(const char *file, SDL_bool has_alpha)
{	
	SDL_Surface *img, *surf;

	if (!(img = IMG_Load(file))) {
		fprintf(stderr, "graphics.c: File not found - '%s'\n", file);
		return NULL;
	}
	if (SDL_TRUE == has_alpha) {
		SDL_SetColorKey(img, SDL_HasColorKey(img), img->format->format);
		return img;
	}
	// Create hardware surface
	if (!(surf = SDL_CreateRGBSurface(0, img->w, img->h, 16, 0xF800, 0x7E0, 0x1F, 0))) {
		SDL_FreeSurface(img);
		fprintf(stderr, "graphics.c: Error creating surface!\n");
		return NULL;
	}
	SDL_BlitSurface(img, NULL, surf, NULL);
	SDL_FreeSurface(img);
	return surf;
}

typedef struct {
	img_t font;
	int	w, h, n;
	int	widths[256];
	int	disp[256];
} font_t;

font_t * font;
TTF_Font *ttf;

Uint32 sge_GetPixel(SDL_Surface *surface, Sint16 x, Sint16 y);

static int fnt_calc_idx(int n, char c)
{
	int idx = (unsigned char)c;
#ifdef RUSSIAN
	if (idx >= 160) {
//		fprintf(stderr, "%d->%d\n", idx, idx - 65);
		idx -= 65;
	}
#endif
	if (idx > (n + 0x20) || idx < 0x20) {
//		fprintf(stderr, "%d\n", idx);
		return -1;
	}
	return (idx - 0x20 - 1);
}

int gfx_draw_char(const char c, int x, int y, float s)
{
	SDL_Surface *pixbuf;
	SDL_Rect dest, src;
	
	if (c == ' ')
		return font->w / 2;

	int idx = fnt_calc_idx(font->n, c);
	if (idx < 0)
		return 0;
	
	pixbuf = (SDL_Surface *)(font->font);
	dest.x = x;
	dest.y = y;
	
	src.y = 0;
	src.x = idx * font->w + (font->disp[idx]);
	src.w = dest.w = font->widths[idx];
	src.h = dest.h = font->h;
	
	if (s && s != 1) {
		SDL_Rect scl;
		scl.x = 0; scl.w = dest.w = (int)(dest.w * s);
		scl.y = 0; scl.h = dest.h = (int)(dest.h * s);
		SDL_Surface *scaled = SDL_CreateRGBSurface(
			pixbuf->flags, scl.w, scl.h, 32,
			pixbuf->format->Rmask,
			pixbuf->format->Gmask,
			pixbuf->format->Bmask,
			pixbuf->format->Amask);
		SDL_BlitScaled(pixbuf, &src, scaled, &scl);
		SDL_BlitSurface(scaled, &scl, screen, &dest);
		//fprintf(stderr, "scaled: %f\n", s);
	} else
		SDL_BlitSurface(pixbuf, &src, screen, &dest);
	return dest.w;
}

int gfx_chars_width(const char *str)
{
	const int Wd = font->w / 2;
	const int Ni = font->n;

	int result = 0;

	for (int i = 0; i < strlen(str); i++) {
		if (str[i] == ' ') {
			result += Wd;
			continue;
		}
		int idx = fnt_calc_idx(Ni, str[i]);
		if (idx >= 0) {
			result += font->widths[idx] + 1;
		}
	}
	return result;
}

int gfx_draw_text(const char *str, int x, int y, float s)
{
	int x_orig = x;
	for (int i = 0; i < strlen(str); i++) {
		x += gfx_draw_char(str[i], x, y, s);
	}
	return x - x_orig;
}

int gfx_font_height(void)
{
	return font->h;
}

img_t gfx_draw_ttf_text(char *text)
{
	SDL_Color color = { 0x00, 0x00, 0x00 };
	SDL_Surface* mask;

	if(!ttf || !(mask = TTF_RenderText_Solid(ttf, text, color))) {
		fprintf(stderr, "graphics.c: Can't draw TTF text '%s'\n", text);
		return NULL;
	}
	color.r = 0xB8; color.g = 0x7B;
	
	SDL_Rect dest, src;
	SDL_Surface* fill = TTF_RenderText_Solid(ttf, text, color);

	SDL_Surface* pixbuf = SDL_CreateRGBSurfaceWithFormat(0,
		(src.w = dest.w = fill->w) + 1,
		(src.h = dest.h = fill->h) + 1,
		8, SDL_PIXELFORMAT_RGBA8888
	);
	src.x = 0; dest.x = 1;
	src.y = 0; dest.y = 1;
	
	SDL_BlitSurface(mask, &src, pixbuf, &dest);
	SDL_BlitSurface(fill, &src, pixbuf, &src);
	SDL_FreeSurface(mask);
	SDL_FreeSurface(fill);

	return pixbuf;
}

font_t *gfx_load_font(const char *file, const int cW)
{
	SDL_Surface *img = gfx_load_image(file, SDL_TRUE);
	
	if (!img)
		return NULL;
	
	font_t *font = malloc(sizeof(font_t));
	
	if (!font) {
		gfx_free_image(img);
		return NULL;
	}
	const int cN = img->w / cW;
	const int cH = img->h;

	font->h = cH;
	font->w = cW;
	font->n = cN;
	font->font = img;
	
	for (int i = 0; i < cN; i++) {
		
		font->widths[i] = 0;
		font->disp[i]   = cW;
		
		for (int y = 0; y < cH; y++) {
			
			int displ, dispr;
			Uint32 pixel;
			Uint8 r, g, b, a;
			
			for (displ = 0; displ < cW; displ++) {
				pixel = sge_GetPixel(img, i * cW + displ, y);
				SDL_GetRGBA(pixel, img->format, &r, &g, &b, &a);
				if (a)
					break;
			}
			for (dispr = cW - 1; dispr >= 0; dispr--) {
				pixel = sge_GetPixel(img, i * cW + dispr, y);
				SDL_GetRGBA(pixel, img->format, &r, &g, &b, &a);
				if (a)
					break;
			}
			if (displ < font->disp[i])
				font->disp[i] = displ;
			if (dispr >= displ) {
				if ((dispr - displ + 1) > font->widths[i])
					font->widths[i] = dispr - displ + 1;
			}
		}
	}
	return font;
}

void gfx_font_free(void)
{
	gfx_free_image(font->font);
	free(font);
}

void gfx_draw_bg(img_t pix, int x, int y, int w, int h) {
	SDL_Rect dest, src;
	src.x = dest.x = x, src.w = dest.w = w;
	src.y = dest.y = y, src.h = dest.h = h;
	SDL_UpperBlit(pix, &src, screen, &dest);
}

void gfx_draw(img_t pix, int x, int y) {
	SDL_Rect rect = { .x = x, .y = y, .w = pix->w, .h = pix->h };\
	SDL_UpperBlit(pix, NULL, screen, &rect);\
}

void gfx_draw_wh(img_t pix, int x, int y, int w, int h) {
	SDL_Rect dest, src;
	src.x = 0, dest.x = x, src.w = dest.w = w;
	src.y = 0, dest.y = y, src.h = dest.h = h;
	SDL_UpperBlit(pix, &src, screen, &dest);
}

void gfx_clear(int x, int y, int w, int h)
{
	SDL_Rect dest;
	dest.x = x; dest.y = y;
	dest.w = w; dest.h = h;
	SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));
}

SDL_bool gfx_init(const char *gfx_dir, int scr_w, int scr_h)
{
	if (SDL_CreateWindowAndRenderer(scr_w, scr_h,
#ifdef CL_MOBILE
		SDL_WINDOW_FULLSCREEN_DESKTOP
#else
		SDL_WINDOW_RESIZABLE
#endif
	, &sdlWindow, &sdlRenderer)) {
		return SDL_TRUE;
	}
	
	SDL_SetWindowTitle( sdlWindow, "Color Lines" );
	SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY , "linear" );
	SDL_RenderSetLogicalSize( sdlRenderer, scr_w, scr_h );
	
	sdlTexture = SDL_CreateTexture( sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, scr_w, scr_h );
	screen     = SDL_CreateRGBSurface( 0, scr_w, scr_h, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000 );
	
	gfx_clear(0, 0, scr_w, scr_h);
#ifdef CL_MOBILE
	SDL_ShowCursor(SDL_DISABLE);
#else
	SDL_Surface * icon;
	char ipath[ SYS_PATH_L ];

	if ((icon = IMG_Load( _strcomb(ipath, gfx_dir, "joker.png") ))) {
		SDL_SetWindowIcon( sdlWindow, icon );
	}
#endif
	if (TTF_Init()) {
		fprintf(stderr, "graphics.c: Couldn't initialize TTF - '%s'\n", TTF_GetError());
	} else {
		ttf = TTF_OpenFont( _strcomb(ipath, gfx_dir, "manaspc.ttf"), TTF_PX );
	}
	if (!(font = gfx_load_font( _strcomb(ipath, gfx_dir, "fnt.png"), FONT_WIDTH )))
		return SDL_TRUE;
	return SDL_FALSE;
}

void gfx_update(void) {
	SDL_UpdateTexture( sdlTexture, NULL, screen->pixels, screen->pitch );
	SDL_RenderClear( sdlRenderer );
	SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, NULL );
	SDL_RenderPresent( sdlRenderer );
}

void gfx_done(void)
{
	TTF_CloseFont(ttf);
	TTF_Quit();
	gfx_font_free();
}

/* code from sge */
#ifndef PI
	#define PI 3.1414926535
#endif

void _PutPixel24(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color)
{
	Uint8 *pix = (Uint8 *)surface->pixels + y * surface->pitch + x * 3;
	
	/* Gack - slow, but endian correct */
	*(pix+surface->format->Rshift/8) = color>>surface->format->Rshift;
	*(pix+surface->format->Gshift/8) = color>>surface->format->Gshift;
	*(pix+surface->format->Bshift/8) = color>>surface->format->Bshift;
	*(pix+surface->format->Ashift/8) = color>>surface->format->Ashift;
}
void _PutPixel32(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color)
{
	*((Uint32 *)surface->pixels + y*surface->pitch/4 + x) = color;
}

void _PutPixelX(SDL_Surface *dest,Sint16 x,Sint16 y,Uint32 color)
{
	switch ( dest->format->BytesPerPixel ) {
	case 1:
		*((Uint8 *)dest->pixels + y*dest->pitch + x) = color;
		break;
	case 2:
		*((Uint16 *)dest->pixels + y*dest->pitch/2 + x) = color;
		break;
	case 3:
		_PutPixel24(dest,x,y,color);
		break;
	case 4:
		*((Uint32 *)dest->pixels + y*dest->pitch/4 + x) = color;
		break;
	}
}

Uint32 sge_GetPixel(SDL_Surface *surface, Sint16 x, Sint16 y)
{
	if(x < 0 || x >= surface->w || y < 0 || y >= surface->h)
		return 0;
	
	switch (surface->format->BytesPerPixel) {
		case 1: { /* Assuming 8-bpp */
			return *((Uint8 *)surface->pixels + y*surface->pitch + x);
		}
		break;
		
		case 2: { /* Probably 15-bpp or 16-bpp */
			return *((Uint16 *)surface->pixels + y*surface->pitch/2 + x);
		}
		break;
		
		case 3: { /* Slow 24-bpp mode, usually not used */
			Uint8 *pix;
			int shift;
			Uint32 color=0;
			
			pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;
			shift = surface->format->Rshift;
			color = *(pix+shift/8)<<shift;
			shift = surface->format->Gshift;
			color|= *(pix+shift/8)<<shift;
			shift = surface->format->Bshift;
			color|= *(pix+shift/8)<<shift;
			shift = surface->format->Ashift;
			color|= *(pix+shift/8)<<shift;
			return color;
		}
		break;
		
		case 4: { /* Probably 32-bpp */
			return *((Uint32 *)surface->pixels + y*surface->pitch/4 + x);
		}
		break;
	}
	return 0;
}

/*
*  Macro to get clipping
*/
#if SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) >= \
	SDL_VERSIONNUM(1, 1, 5)
	#define sge_clip_xmin(pnt) pnt->clip_rect.x
	#define sge_clip_xmax(pnt) pnt->clip_rect.x + pnt->clip_rect.w-1
	#define sge_clip_ymin(pnt) pnt->clip_rect.y
	#define sge_clip_ymax(pnt) pnt->clip_rect.y + pnt->clip_rect.h-1
#else
	#define sge_clip_xmin(pnt) pnt->clip_minx
	#define sge_clip_xmax(pnt) pnt->clip_maxx
	#define sge_clip_ymin(pnt) pnt->clip_miny
	#define sge_clip_ymax(pnt) pnt->clip_maxy
#endif

#define SWAP(x,y,temp) temp=x;x=y;y=temp
//==================================================================================
// Helper function to sge_transform()
// Returns the bounding box
//==================================================================================
void _calcRect(SDL_Surface *src, SDL_Surface *dst, float theta, float xscale, float yscale, Uint16 px, Uint16 py, Uint16 qx, Uint16 qy, Sint16 *xmin, Sint16 *ymin, Sint16 *xmax, Sint16 *ymax)
{
	Sint16 x, y, rx, ry;
	Sint32 istx, ictx, isty, icty;
	// Clip to src surface
	Sint16 sxmin = sge_clip_xmin(src);
	Sint16 sxmax = sge_clip_xmax(src);
	Sint16 symin = sge_clip_ymin(src);
	Sint16 symax = sge_clip_ymax(src);
	Sint16 sx[]={sxmin, sxmax, sxmin, sxmax};
	Sint16 sy[]={symin, symax, symax, symin};
	
	// We don't really need fixed-point here
	// but why not?
	istx = (Sint32)((sin(theta)*xscale) * 8192.0); /* Inverse transform */
	ictx = (Sint32)((cos(theta)*xscale) * 8192.2);
	isty = (Sint32)((sin(theta)*yscale) * 8192.0);
	icty = (Sint32)((cos(theta)*yscale) * 8192.2);

	//Calculate the four corner points
	for (int i = 0; i < 4; i++) {
		rx = sx[i] - px;
		ry = sy[i] - py;
		
		x = (Sint16)(((ictx*rx - isty*ry) >> 13) + qx);
		y = (Sint16)(((icty*ry + istx*rx) >> 13) + qy);
		
		
		if (i == 0) {
			*xmax = *xmin = x;
			*ymax = *ymin = y;
		} else {
			if (x > *xmax)
				*xmax=x;
			else if (x < *xmin)
				*xmin=x;
				
			if (y > *ymax)
				*ymax = y;
			else if (y < *ymin)
				*ymin = y;
		}
	}
	
	//Better safe than sorry...
	*xmin -= 1;
	*ymin -= 1;
	*xmax += 1;
	*ymax += 1;
	
	//Clip to dst surface
	if( !dst )
		return;
	if( *xmin < sge_clip_xmin(dst) )
		*xmin = sge_clip_xmin(dst);
	if( *xmax > sge_clip_xmax(dst) )
		*xmax = sge_clip_xmax(dst);
	if( *ymin < sge_clip_ymin(dst) )
		*ymin = sge_clip_ymin(dst);
	if( *ymax > sge_clip_ymax(dst) )
		*ymax = sge_clip_ymax(dst);
}

#define TRANSFORM_GENERIC_AA \
	Uint8 R, G, B, A, R1, G1, B1, A1=0, R2, G2, B2, A2=0, R3, G3, B3, A3=0, R4, G4, B4, A4=0; \
	Sint32 wx, wy, p1, p2, p3, p4;\
\
	Sint32 const one = 2048;   /* 1 in Fixed-point */ \
	Sint32 const two = 2*2048; /* 2 in Fixed-point */ \
\
	for (y=ymin; y<ymax; y++){ \
		dy = y - qy; \
\
		sx = (Sint32)(ctdx  + stx*dy + mx);  /* Compute source anchor points */ \
		sy = (Sint32)(cty*dy - stdx  + my); \
\
		for (x=xmin; x<xmax; x++){ \
			rx=(Sint16)(sx >> 13);  /* Convert from fixed-point */ \
			ry=(Sint16)(sy >> 13); \
\
			/* Make sure the source pixel is actually in the source image. */ \
			if( (rx>=sxmin) && (rx+1<=sxmax) && (ry>=symin) && (ry+1<=symax) ){ \
				wx = (sx & 0x00001FFF) >> 2;  /* (float(x) - int(x)) / 4 */ \
				wy = (sy & 0x00001FFF) >> 2;\
\
				p4 = wx+wy;\
				p3 = one-wx+wy;\
				p2 = wx+one-wy;\
				p1 = two-wx-wy;\
\
				SDL_GetRGBA(sge_GetPixel(src,rx,  ry), src->format, &R1, &G1, &B1, &A1);\
				SDL_GetRGBA(sge_GetPixel(src,rx+1,ry), src->format, &R2, &G2, &B2, &A2);\
				SDL_GetRGBA(sge_GetPixel(src,rx,  ry+1), src->format, &R3, &G3, &B3, &A3);\
				SDL_GetRGBA(sge_GetPixel(src,rx+1,ry+1), src->format, &R4, &G4, &B4, &A4);\
\
				/* Calculate the average */\
				R = (p1*R1 + p2*R2 + p3*R3 + p4*R4)>>13;\
				G = (p1*G1 + p2*G2 + p3*G3 + p4*G4)>>13;\
				B = (p1*B1 + p2*B2 + p3*B3 + p4*B4)>>13;\
				A = (p1*A1 + p2*A2 + p3*A3 + p4*A4)>>13;\
\
				_PutPixelX(dst,x,y,SDL_MapRGBA(dst->format, R, G, B, A)); \
				\
			} \
			sx += ctx;  /* Incremental transformations */ \
			sy -= sty; \
		} \
	} 

Uint8 _sge_lock = 1;

SDL_Rect sge_transformAA(SDL_Surface *src, SDL_Surface *dst, float angle, float xscale, float yscale ,Uint16 px, Uint16 py, Uint16 qx, Uint16 qy, Uint8 flags)
{
	Sint32 dy, sx, sy;
	Sint16 x, y, rx, ry;
	SDL_Rect r;
	r.x = r.y = r.w = r.h = 0;
	
	float theta = (float)(angle*PI/180.0);  /* Convert to radians.  */
	
	// Here we use 18.13 fixed point integer math
	// Sint32 should have 31 usable bits and one for sign
	// 2^13 = 8192
	
	// Check scales
	Sint32 maxint = (Sint32)(pow(2, sizeof(Sint32)*8 - 1 - 13));  // 2^(31-13)
	
	if( xscale == 0 || yscale == 0)
		return r;
	
	if( 8192.0/xscale > maxint )
		xscale = (float)(8192.0/maxint);
	else if( 8192.0/xscale < -maxint )
		xscale = (float)(-8192.0/maxint);	
	
	if( 8192.0/yscale > maxint )
		yscale = (float)(8192.0/maxint);
	else if( 8192.0/yscale < -maxint )
		yscale = (float)(-8192.0/maxint);
	
	// Fixed-point equivalents
	Sint32 const stx = (Sint32)((sin(theta)/xscale) * 8192.0);
	Sint32 const ctx = (Sint32)((cos(theta)/xscale) * 8192.0);
	Sint32 const sty = (Sint32)((sin(theta)/yscale) * 8192.0);
	Sint32 const cty = (Sint32)((cos(theta)/yscale) * 8192.0);
	Sint32 const mx = (Sint32)(px*8192.0); 
	Sint32 const my = (Sint32)(py*8192.0);
	
	// Compute a bounding rectangle
	Sint16 xmin=0, xmax=dst->w, ymin=0, ymax=dst->h;
	_calcRect(src, dst, theta, xscale, yscale, px, py, qx, qy, &xmin,&ymin, &xmax,&ymax);	
	
	// Clip to src surface
	Sint16 sxmin = sge_clip_xmin(src);
	Sint16 sxmax = sge_clip_xmax(src);
	Sint16 symin = sge_clip_ymin(src);
	Sint16 symax = sge_clip_ymax(src);
	
	// Some terms in the transform are constant
	Sint32 const dx = xmin - qx;
	Sint32 const ctdx = ctx*dx;
	Sint32 const stdx = sty*dx;
	
	// Lock surfaces... hopfully less than two needs locking!
	if ( SDL_MUSTLOCK(src) && _sge_lock )
		if ( SDL_LockSurface(src) < 0 )
			return r;
	if ( SDL_MUSTLOCK(dst) && _sge_lock ){
		if ( SDL_LockSurface(dst) < 0 ){
			if ( SDL_MUSTLOCK(src) && _sge_lock )
				SDL_UnlockSurface(src);
			return r;
		}
	}
	
	TRANSFORM_GENERIC_AA
	
	// Unlock surfaces
	if ( SDL_MUSTLOCK(src) && _sge_lock )
		SDL_UnlockSurface(src);
	if ( SDL_MUSTLOCK(dst) && _sge_lock )
		SDL_UnlockSurface(dst);
	
	//Return the bounding rectangle
	r.x=xmin; r.y=ymin; r.w=xmax-xmin; r.h=ymax-ymin;
	return r;
}

SDL_Rect sge_transform(SDL_Surface *src, SDL_Surface *dst, float angle, float xscale, float yscale, Uint16 px, Uint16 py, Uint16 qx, Uint16 qy, Uint8 flags)
{
	return sge_transformAA(src, dst, angle, xscale, yscale, px, py, qx, qy, flags);
}

SDL_Surface *sge_transform_surface(SDL_Surface *src, Uint32 bcol, float angle, float xscale, float yscale, Uint8 flags)
{
	float theta = (float)(angle*PI/180.0);  /* Convert to radians.  */
	
	// Compute a bounding rectangle
	Sint16 xmin=0, xmax=0, ymin=0, ymax=0;
	_calcRect(src, NULL, theta, xscale, yscale, 0, 0, 0, 0, &xmin,&ymin, &xmax,&ymax);	
	
	Sint16 w = xmax-xmin+1; 
	Sint16 h = ymax-ymin+1;
	
	Sint16 qx = -xmin;
	Sint16 qy = -ymin;
	
	SDL_Surface *dest;
	dest = SDL_CreateRGBSurface(0, w, h, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);
	if(!dest)
		return NULL;
	
//	sge_ClearSurface(dest,bcol);  //Set background color
	
	sge_transform(src, dest, angle, xscale, yscale, 0, 0, qx, qy, flags);
	
	return dest;
}

img_t gfx_scale(img_t src, float xscale, float yscale)
{
	return (img_t)sge_transform_surface((SDL_Surface *)src, 0, 0, xscale, yscale, 0);
}
