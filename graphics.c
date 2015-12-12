#include <SDL.h>
#include <SDL_image.h>
#include "graphics.h"
#include "math.h"
	
static SDL_Surface *screen;

void gfx_free_image(img_t p)
{
	SDL_FreeSurface((SDL_Surface *)p);
}

int	gfx_img_w(img_t pixmap)
{
	if (!pixmap)
		return 0;
	return ((SDL_Surface *)pixmap)->w;
}

int	gfx_img_h(img_t pixmap)
{
	if (!pixmap)
		return 0;
	return ((SDL_Surface *)pixmap)->h;
}

img_t	gfx_grab_screen(int x, int y, int w, int h)
{
	SDL_Rect dst, src;
	SDL_Surface *img = SDL_CreateRGBSurface(SDL_HWSURFACE, w, h, 16, 0xF800, 0x7E0, 0x1F, 0);
	if (!img)
		return NULL;	
	src.x = x;
	src.y = y;
	src.w = w;
	src.h = h;
	dst.x = 0;
	dst.y = 0;
	dst.w = w;
	dst.h = h;	
	SDL_BlitSurface(screen, &src, img, &dst);
	return img;
}

img_t gfx_set_alpha(img_t src, int alpha)
{
	Uint32 *ptr;
	Uint32 col;
	int size;
	
	SDL_Surface *img = SDL_DisplayFormatAlpha((SDL_Surface*)src);
	if (!img)
		return NULL;	
	ptr = (Uint32*)img->pixels;
	size = img->w * img->h;
	while (size --) {
		Uint8 r, g, b, a;
		col = *ptr;
		SDL_GetRGBA(col, img->format, &r, &g, &b, &a);
		col = SDL_MapRGBA(img->format, r, g, b, a * alpha / 255);
		*ptr = col;
		ptr ++;
	}
	return img;
}

img_t gfx_combine(img_t src, img_t dst)
{
	img_t new;
	new = SDL_DisplayFormatAlpha(dst);
	if (!new)
		return NULL;
	SDL_BlitSurface((SDL_Surface *)src, NULL, (SDL_Surface *)new, NULL);
	return new;	
}

img_t gfx_load_image(char *filename, int transparent)
{

	SDL_Surface *img, *img2;
	img = IMG_Load(filename);
	if (!img) {
		fprintf(stderr, "File not found: %s\n", filename);
		return NULL;
	}
	if (transparent && transparent != 2) {
		SDL_SetColorKey(img,  SDL_RLEACCEL, img->format->colorkey);
		return img;
	} 
    // Create hardware surface
    	img2 = SDL_CreateRGBSurface(SDL_HWSURFACE | (transparent)?SDL_SRCCOLORKEY:0, 
		img->w, img->h, 16, 0xF800, 0x7E0, 0x1F, 0);
	if (!img2) {
		SDL_FreeSurface(img);
		fprintf(stderr, "Error creating surface!\n");
		return NULL;
	}
	
	if (transparent)
		SDL_SetColorKey(img2,  SDL_SRCCOLORKEY | SDL_RLEACCEL, 0xF81F);

	SDL_SetAlpha(img2, 0, 0);
	SDL_BlitSurface(img, NULL, img2, NULL);
	SDL_FreeSurface(img);
	return img2;
}

typedef struct {
	int	h;
	int	w;
	img_t	font;
	int	n;
	int	widths[256];
	int	disp[256];
} _font_t;

Uint32 sge_GetPixel(SDL_Surface *surface, Sint16 x, Sint16 y);

static int fnt_calc_idx(fnt_t f, char c)
{
	_font_t *fnt = (_font_t *)f;
	int idx = (unsigned char)c;
#ifdef RUSSIAN
	if (idx >= 160) {
//		fprintf(stderr, "%d->%d\n", idx, idx - 65);
		idx -= 65;
	}
#endif		
	if (idx > (fnt->n + 32) || idx < 32) {
//		fprintf(stderr, "%d\n", idx);
		return -1;
	}
	idx = idx - (int)' ' - 1;
	return idx;
}
int gfx_draw_char(fnt_t f, const char c, int x, int y)
{
	_font_t *fnt = (_font_t *)f;
	int idx;
	int from;

	SDL_Surface *pixbuf;
	SDL_Rect dest, src;

	if (c == ' ')
		return fnt->w / 2;
	
	idx = fnt_calc_idx(f, c);
	if (idx < 0)
		return 0;
	from = (idx) * fnt->w + (fnt->disp[idx]);

	pixbuf = (SDL_Surface *)(fnt->font);
	
	src.x = from; src.y = 0;
	src.w = fnt->widths[idx]; src.h = fnt->h;
	dest.x = x; dest.y = y; 
	dest.w = fnt->widths[idx]; dest.h = fnt->h;
	SDL_BlitSurface(pixbuf, &src, screen, &dest);
	return dest.w;
}

int gfx_chars_width(fnt_t f, const char *str)
{
	_font_t *fnt = (_font_t *)f;
	int len = strlen(str);
	int i; int x = 0;
	int idx, from;
	for (i = 0; i < len ; i++ ) {
		if (str[i] == ' ') {
			x += fnt->w / 2;
			continue;
		}
		idx = fnt_calc_idx(f, str[i]);
		if (idx < 0)
			continue;
		from = (idx) * fnt->w + (fnt->disp[idx]);
		x += fnt->widths[idx] + 1;
	}
	return x;
}

int gfx_draw_chars(fnt_t f, const char *str, int x, int y)
{
	int len = strlen(str);
	int i; int x_orig = x;
	for (i = 0; i < len ; i++ ) {
		x += gfx_draw_char(f, str[i], x, y);
	}
	return x - x_orig;
}

int gfx_font_height(fnt_t f)
{
	_font_t	*font = (_font_t *)f;
	return font->h;	

}

fnt_t gfx_load_font(char *fname, int w)
{
	SDL_Surface *fn;
	int i;
	_font_t	*font;int y;
	fn = (SDL_Surface*)gfx_load_image(fname, 1);
	if (!fn)
		return NULL;
	font = malloc(sizeof(*font));
	if (!font) {
		gfx_free_image(fn);
		return NULL;
	}
	font->n = fn->w / w;
	font->h = fn->h;	
	font->w = w;
	font->font = fn;
	for (i = 0; i < font->n; i++) {
		font->widths[i] = 0;
		font->disp[i] = font->w;
		for (y = 0; y < font->h; y++) {
			int displ, dispr;
			Uint32 pixel;
			Uint8 r, g, b, a;		
			for (displ = 0; displ < font->w; displ ++) {
				pixel = sge_GetPixel(fn, i * font->w + displ, y);
				SDL_GetRGBA(pixel, fn->format, &r, &g, &b, &a);
				if (a)
					break;
			}
			for (dispr = font->w - 1; dispr >= 0; dispr --) {
				pixel = sge_GetPixel(fn, i * font->w + dispr, y);
				SDL_GetRGBA(pixel, fn->format, &r, &g, &b, &a);
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

void gfx_font_free(fnt_t f)
{
	_font_t	*font = (_font_t *)f;
	gfx_free_image(font->font);
	free(font);
	return;	
}

void gfx_draw_bg(img_t p, int x, int y, int width, int height)
{
	SDL_Surface *pixbuf = (SDL_Surface *)p;
	SDL_Rect dest, src;
	src.x = x;
	src.y = y;
	src.w = width;
	src.h = height;
	dest.x = x;
	dest.y = y; 
	dest.w = width; 
	dest.h = height;
	SDL_BlitSurface(pixbuf, &src, screen, &dest);
}

void gfx_draw(img_t p, int x, int y)
{
	SDL_Surface *pixbuf = (SDL_Surface *)p;
	SDL_Rect dest;
	dest.x = x;
	dest.y = y; 
	dest.w = pixbuf->w; 
	dest.h = pixbuf->h;
	SDL_BlitSurface(pixbuf, NULL, screen, &dest);
}

void gfx_draw_wh(img_t p, int x, int y, int w, int h)
{
	SDL_Surface *pixbuf = (SDL_Surface *)p;
	SDL_Rect dest, src;
	src.x = 0;
	src.y = 0; 
	src.w = w; 
	src.h = h;
	dest.x = x;
	dest.y = y; 
	dest.w = w; 
	dest.h = h;
	SDL_BlitSurface(pixbuf, &src, screen, &dest);
}

void gfx_clear(int x, int y, int w, int h)
{
	SDL_Rect dest;
	dest.x = x;
	dest.y = y; 
	dest.w = w; 
	dest.h = h;
	SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));
}

#ifndef MAEMO
static	SDL_Surface *icon;
#endif

int gfx_init(void)
{
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return -1;
  	}
#ifdef MAEMO	
	screen = SDL_SetVideoMode(800, 480, 16, SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_FULLSCREEN);
#else
	SDL_WM_SetCaption("Color Lines", NULL);
	icon = IMG_Load( GAMEDATADIR"icon/color-lines.png" );
	if (icon) {
		SDL_WM_SetIcon( icon, NULL );
	}
	screen = SDL_SetVideoMode(800, 480, 32, SDL_DOUBLEBUF | SDL_HWSURFACE);
#endif	
	if (screen == NULL) {
		fprintf(stderr, "Unable to set 800x480 video: %s\n", SDL_GetError());
		return -1;
	}
#ifdef MAEMO
	SDL_ShowCursor(SDL_DISABLE);
#endif	
	gfx_clear(0, 0, 800, 480);
	return 0;
}

void gfx_update(int x, int y, int w, int h) {
	SDL_UpdateRect(screen, x, y, w, h);
}

void gfx_done(void)
{
#ifndef MAEMO
//	SDL_FreeSurface(icon);
#endif	
	SDL_Quit();
}

/* code from sge */
#ifndef PI
	#define PI 3.1414926535
#endif

void _PutPixel24(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color)
{
	Uint8 *pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;

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
	if(x<0 || x>=surface->w || y<0 || y>=surface->h)
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
	int i;
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
	istx = (Sint32)((sin(theta)*xscale) * 8192.0);  /* Inverse transform */
	ictx = (Sint32)((cos(theta)*xscale) * 8192.2);
	isty = (Sint32)((sin(theta)*yscale) * 8192.0);
	icty = (Sint32)((cos(theta)*yscale) * 8192.2);

	//Calculate the four corner points
	for(i=0; i<4; i++){
		rx = sx[i] - px;
		ry = sy[i] - py;
		
		x = (Sint16)(((ictx*rx - isty*ry) >> 13) + qx);
		y = (Sint16)(((icty*ry + istx*rx) >> 13) + qy);
		
		
		if(i==0){
			*xmax = *xmin = x;
			*ymax = *ymin = y;
		}else{
			if(x>*xmax)
				*xmax=x;
			else if(x<*xmin)
				*xmin=x;
				
			if(y>*ymax)
				*ymax=y;
			else if(y<*ymin)
				*ymin=y;
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

Uint8 _sge_lock=1;

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
		xscale =  (float)(8192.0/maxint);
	else if( 8192.0/xscale < -maxint )
		xscale =  (float)(-8192.0/maxint);	
		
	if( 8192.0/yscale > maxint )
		yscale =  (float)(8192.0/maxint);
	else if( 8192.0/yscale < -maxint )
		yscale =  (float)(-8192.0/maxint);


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
	dest = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);
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
