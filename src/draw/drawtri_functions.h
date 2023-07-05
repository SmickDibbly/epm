#ifndef DRAWTRI_FUNCTIONS_H
#define DRAWTRI_FUNCTIONS_H

#include "src/misc/epm_includes.h"
#include "src/draw/window/window.h"
#include "src/draw/draw_triangle.h"

typedef uint32_t (*fn_draw_Triangle)(Window *win, ScreenTri *tri);

#define TRIFLAG_NONE  0x00
#define TRIFLAG_TEXEL 0x01
#define TRIFLAG_VBRI  0x02
#define TRIFLAG_FBRI  0x04
#define TRIFLAG_HILIT 0x08
#define TRIFLAG_COUNT 0x10
#define TRIFLAG_SKY   0x20

#define TRIFLAG_ORTHO  0x80

extern fn_draw_Triangle drawtri_table[256];

#endif /* DRAWTRI_FUNCTIONS_H */
