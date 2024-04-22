#ifndef _GFX_SDL_H_
# include <SDL2/SDL.h>
# include <SDL2/SDL_image.h>
# define _GFX_SDL_H_

typedef union GFX_Range {
	struct { Sint16 x_min, x_max, y_min, y_max; };
	struct { Uint16 x1, x2, y1, y2; };
} GFX_Range;

typedef struct {
	const float sa, ca;
} GFX_Radi;

# define GFX_PI 3.1414926535
/* Convert Angle deg to radians. */
# define GFX_toRadians(_a) (GFX_Radi){\
	.sa = SDL_sin((_a)*GFX_PI/180.0),\
	.ca = SDL_cos((_a)*GFX_PI/180.0)\
}
# define GFX_NewRange(_x,_X,_y,_Y) (GFX_Range){ .x1 =_x, .x2 =_X, .y1 =_y, .y2 =_Y }
# define GFX_RectToRange(_r)       (GFX_Range){\
	.x_min = _r.x, .x_max = (_r.x + _r.w) - 1,\
	.y_min = _r.y, .y_max = (_r.y + _r.h) - 1\
}
# define GFX_GetPixel(src,_x,_y)    ((Uint32 *)src->pixels)[(_y) * src->pitch / 4 + (_x)]
# define GFX_SetPixel(dst,_x,_y,_c) ((Uint32 *)dst->pixels)[(_y) * dst->pitch / 4 + (_x)] = _c

# define GFX_DisableAlpha(src) \
		 SDL_SetColorKey(src, SDL_FALSE, src->format->format)
# define GFX_DrawPoint(rend,_x,_y) \
		 SDL_RenderDrawPoint(rend,_x,_y)
# define GFX_DrawHline(rend,_x1,_x2,_y) \
		 SDL_RenderDrawLine(rend,_x1,_y,_x2,_y)
# define GFX_DrawVline(rend,_x,_y1,_y2) \
		 SDL_RenderDrawLine(rend,_x,_y1,_x,_y2)
# define GFX_SetDrawRGB(rend,_r,_g,_b) \
		 SDL_SetRenderDrawColor(rend,_r,_g,_b,0xFF)
# define GFX_SetDrawRGBA(rend,_r,_g,_b,_a) \
		 SDL_SetRenderDrawColor(rend,_r,_g,_b,_a)
# define GFX_CpySurfFormat(src,_w,_h) \
		 SDL_CreateRGBSurfaceWithFormat(0,_w,_h, 32, src->format->format)

static inline void GFX_StrokeCircle(SDL_Renderer *rend, int cx, int cy, int radii)
{
	// if the first pixel in the screen is represented by (0,0) (which is in sdl)
	// remember that the beginning of the circle is not in the middle of the pixel
	// but to the left-top from it:
	for (int x = radii, er = -radii, y = 0; x >= y; y++)
	{
		SDL_RenderDrawPoint(rend, (cx + x), (cy + y));
		SDL_RenderDrawPoint(rend, (cx + y), (cy + x));
		if (x != 0) {
			SDL_RenderDrawPoint(rend, (cx - x), (cy + y));
			SDL_RenderDrawPoint(rend, (cx + y), (cy - x));
		}
		if (y != 0) {
			SDL_RenderDrawPoint(rend, (cx + x), (cy - y));
			SDL_RenderDrawPoint(rend, (cx - y), (cy + x));
		}
		if (x != 0 && y != 0) {
			SDL_RenderDrawPoint(rend, (cx - x), (cy - y));
			SDL_RenderDrawPoint(rend, (cx - y), (cy - x));
		}
		if((er += y + y + 1) >= 0)
			er -= x + x - 2, x--;
	}
}

static inline void GFX_FillCircle(SDL_Renderer *rend, int cx, int cy, int radii)
{
	// Note that there is more to altering the bitrate of this 
	// method than just changing this value.  See how pixels are
	// altered at the following web page for tips:
	//   http://www.libsdl.org/intro.en/usingvideo.html
	for (int y = 1, x = 2 * radii; y <= radii; y++)
	{
		// This loop is unrolled a bit, only iterating through half of the
		// height of the circle.  The result is used to draw a scan line and
		// its mirror image below it.

		// The following formula has been simplified from our original.  We
		// are using half of the width of the circle because we are provided
		// with a center and we need left/right coordinates.
		int d = (int)SDL_sqrtf(x * y - y * y);

		SDL_RenderDrawLine(rend, (cx - d), (cy + y - radii), (cx + d), (cy + y - radii));
		SDL_RenderDrawLine(rend, (cx - d), (cy - y + radii), (cx + d), (cy - y + radii));
	}
}

# define ONE 2048 // 1 in Fixed-point
# define TWO 4096 // 2 in Fixed-point
# define SIF 8192.0f // 2^13 = 8192
# define COF 8192.2f

/*
  Helper function to sge_transform()
  Returns the bounding box
*/
static inline GFX_Range GFX_CalcRange(GFX_Range scp, GFX_Range pos, GFX_Radi theta, float xs, float ys)
{
	Sint16 x, rx, xmax = 0, xmin = 0,
	       y, ry, ymax = 0, ymin = 0;

	// We don't really need fixed-point here
	// but why not?
	Sint32 const istx = (Sint32)(theta.sa * xs * SIF); /* Inverse transform */
	Sint32 const ictx = (Sint32)(theta.ca * xs * COF);
	Sint32 const isty = (Sint32)(theta.sa * ys * SIF);
	Sint32 const icty = (Sint32)(theta.ca * ys * COF);
	Sint16 const xXxX[] = { scp.x_min, scp.x_max, scp.x_min, scp.x_max };
	Sint16 const yYYy[] = { scp.y_min, scp.y_max, scp.y_max, scp.y_min };

	// Calculate the four corner points
	for (int i = 0; i < 4; i++) {
		rx = xXxX[i] - pos.x1;
		ry = yYYy[i] - pos.y1;
		 x = (Sint16)(((ictx*rx - isty*ry) >> 13) + pos.x2);
		 y = (Sint16)(((icty*ry + istx*rx) >> 13) + pos.y2);

		if (x > xmax) xmax = x; else
		if (x < xmin) xmin = x;
		if (y > ymax) ymax = y; else
		if (y < ymin) ymin = y;
	}
	// Better safe than sorry...
	return GFX_NewRange(xmin-1, xmax+1, ymin-1, ymax+1);
}

GFX_Range GFX_TransformAA(const SDL_Surface *src, GFX_Range pos, GFX_Radi theta, float xs, float ys, SDL_Surface *dst)
{
// Here we use 18.13 fixed point integer math
// Sint32 should have 31 usable bits and one for sign
	float const maxBits = SDL_pow(2,
		(sizeof(Sint32)*8 - 1 - 13) // 2^(31-13)
	);
	if ((SIF / xs) >  maxBits) xs =  SIF / maxBits; else 
	if ((SIF / xs) < -maxBits) xs = -SIF / maxBits;
	if ((SIF / ys) >  maxBits) ys =  SIF / maxBits; else
	if ((SIF / ys) < -maxBits) ys = -SIF / maxBits;

	// Fixed-point equivalents
	Sint32 const istx = (Sint32)((theta.sa / xs) * SIF);
	Sint32 const ictx = (Sint32)((theta.ca / xs) * SIF);
	Sint32 const isty = (Sint32)((theta.sa / ys) * SIF);
	Sint32 const icty = (Sint32)((theta.ca / ys) * SIF);
	Sint32 const imx  = (Sint32)(pos.x1) * (Sint32)SIF;
	Sint32 const imy  = (Sint32)(pos.y1) * (Sint32)SIF;
	
	Sint32 dy, sx, sy, wx, wy, p1, p2, p3, p4;
	Sint16 x, y, rx, ry, ex, ey;

	Uint8 R1,G1,B1,A1 = 0, R2,G2,B2,A2 = 0,
	      R3,G3,B3,A3 = 0, R4,G4,B4,A4 = 0;

	// Compute a bounding rectangle
	GFX_Range sClp = GFX_RectToRange(src->clip_rect),
			  dClp = GFX_RectToRange(dst->clip_rect),
			   box = GFX_CalcRange(sClp, pos, theta, xs, ys);

	if (box.x_min < dClp.x_min) box.x_min = dClp.x_min;
	if (box.x_max > dClp.x_max) box.x_max = dClp.x_max;
	if (box.y_min < dClp.y_min) box.y_min = dClp.y_min;
	if (box.y_max > dClp.y_max) box.y_max = dClp.y_max;

	// Some terms in the transform are constant
	Sint32 const   dx = box.x_min - pos.x2;
	Sint32 const ctdx = ictx * dx;
	Sint32 const stdx = isty * dx;

	for (y = box.y_min; y < box.y_max; y++) {
		dy = y - pos.y2, /* Compute source anchor points */
		sx = istx*dy + ctdx + imx,
		sy = icty*dy - stdx + imy;

		for (x = box.x_min; x < box.x_max; x++, sx += ictx, sy -= isty) { /* Incremental transformations */
			rx = (Sint16)(sx >> 13), ex = rx+1; /* Convert from fixed-point */
			ry = (Sint16)(sy >> 13), ey = ry+1;

			/* Make sure the source pixel is actually in the source image. */
			if (rx >= sClp.x_min && ex <= sClp.x_max &&
				ry >= sClp.y_min && ey <= sClp.y_max) {
				wx = (sx & 0x00001FFF) >> 2; /* (float(x) - int(x)) / 4 */
				wy = (sy & 0x00001FFF) >> 2;

				p1 = TWO-wx-wy, p2 = wx+ONE-wy;
				p3 = ONE-wx+wy, p4 = wx+wy;

				SDL_GetRGBA(GFX_GetPixel(src,rx,ry), src->format, &R1, &G1, &B1, &A1);
				SDL_GetRGBA(GFX_GetPixel(src,ex,ry), src->format, &R2, &G2, &B2, &A2);
				SDL_GetRGBA(GFX_GetPixel(src,rx,ey), src->format, &R3, &G3, &B3, &A3);
				SDL_GetRGBA(GFX_GetPixel(src,ex,ey), src->format, &R4, &G4, &B4, &A4);
				/* Calculate the average */
				Uint32 col = SDL_MapRGBA(dst->format,
					(p1*R1 + p2*R2 + p3*R3 + p4*R4)>>13,
					(p1*G1 + p2*G2 + p3*G3 + p4*G4)>>13,
					(p1*B1 + p2*B2 + p3*B3 + p4*B4)>>13,
					(p1*A1 + p2*A2 + p3*A3 + p4*A4)>>13
				);
				GFX_SetPixel(dst, x,y, col);
			}
		}
	}
	return box; // Return the bounding rectangle
}

static inline SDL_Surface *GFX_CpyWithTransform(const SDL_Surface *src, GFX_Radi theta, float xs, float ys)
{
	// Compute a bounding rectangle
	GFX_Range pos = GFX_NewRange(0,0,0,0),
	          scp = GFX_RectToRange(src->clip_rect),
	          box = GFX_CalcRange(scp, pos, theta, xs, ys);	

	int w = box.x_max - box.x_min + 1,
	    h = box.y_max - box.y_min + 1;

	SDL_Surface *dst = GFX_CpySurfFormat(src, w, h);

	pos.x2 = -box.x_min;
	pos.y2 = -box.y_min;

	GFX_TransformAA(src, pos, theta, xs, ys, dst);

	return dst;
}

static inline SDL_Surface *GFX_CpyWithAlpha(const SDL_Surface *src, int alpha)
{
	int x, w = src->w,
	    y, h = src->h;

	SDL_Surface *dst = GFX_CpySurfFormat(src, w, h);

	for (y = 0; y < h; y++)
	for (x = 0; x < w; x++) {
		Uint8 r,g,b,a;
		SDL_GetRGBA(GFX_GetPixel(src, x,y), src->format,&r,&g,&b,&a);
		GFX_SetPixel(dst, x,y, SDL_MapRGBA( src->format, r, g, b,(a*alpha / 255)));
	}
	return dst;
}

# undef ONE
# undef TWO
# undef SIF
# undef COF
#endif
