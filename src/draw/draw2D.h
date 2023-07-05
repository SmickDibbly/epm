#ifndef DRAW2D_H
#define DRAW2D_H

#include "src/misc/epm_includes.h"
#include "src/draw/draw.h"
#include "src/draw/window/window.h"

typedef enum Plane {
    PLN_TOP = 0,
    PLN_SIDE = 1,
    PLN_FRONT = 2,
} Plane;

typedef struct View2D {
    Fix64Vec_2D center;
    Fix64Rect_2D worldbox;
    zgl_mPixelRect screenbox;
    uint8_t i_x, i_y;
    
    uint32_t i_zoom; // index into zoom LUT
    Fix64 gridres_world;
    uint8_t gridres_world_shift;
    UFix32 gridres_screen;
    uint16_t num_vert_lines;
    uint16_t num_hori_lines;

    int wiregeo;
} View2D;

extern void set_zoom_level(View2D *v2d);
extern void zoom_level_up(Plane tsf);
extern void zoom_level_down(Plane tsf);
extern void draw_View2D(Window *win);
extern void scroll(Window *win, int dx, int dy, Plane tsf);

#endif /* DRAW2D_H */
