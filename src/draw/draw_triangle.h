#ifndef DRAW_TRIANGLE_H
#define DRAW_TRIANGLE_H

#include "src/draw/draw.h"
#include "src/draw/window/window.h"

// weird includes
#include "src/world/world.h"

typedef struct ScreenTri {
    DepthPixel *vpxl[3];
    zgl_mPixel *vtxl[3];
    uint8_t vbri[3];
    
    zgl_PixelArray *ftex;
    Fix32 fbri;
    zgl_Color fclr;

    uint8_t shft;
} ScreenTri;

extern void reset_depth_buffer(void);

extern bool g_all_pixels_drawn;
extern void reset_rasterizer(Window *win);


#define RASTERFLAG_TEXEL         0x01
#define RASTERFLAG_VBRI          0x02
#define RASTERFLAG_FBRI          0x04
#define RASTERFLAG_HILIT         0x08
#define RASTERFLAG_COUNT         0x10
#define RASTERFLAG_SKY           0x20
#define RASTERFLAG_ORTHO         0x80
extern uint8_t g_rasterflags;

extern void draw_Triangle(Window *win, ScreenTri *tri);

#endif /* DRAW_TRIANGLE_H */
