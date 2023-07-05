#ifndef DRAW_H
#define DRAW_H

#include "zigil/zigil.h"

extern zgl_PixelArray *g_scr;
extern zgl_Color *g_scr_pixels;
extern zgl_Pixit g_scr_w;
extern zgl_Pixit g_scr_h;

typedef struct DepthPixel {
    Fix64Vec_2D XY;
    Fix64 z;
} DepthPixel;

#endif /* DRAW_H */
