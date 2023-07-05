#ifndef XSPANS_H
#define XSPANS_H

#include "src/misc/epm_includes.h"
#include "window/window.h"

#define MAX_SCREEN_HEIGHT 1080

extern uint16_t g_xspans[2*MAX_SCREEN_HEIGHT];

extern void compute_xspans
(Window *win, zgl_Pixel v0, zgl_Pixel v1, zgl_Pixel v2,
 uint16_t *out_min_y, uint16_t *out_max_y);

extern void print_xspans(uint16_t min_y, uint16_t max_y);

#endif /* XSPANS_H */
