#ifndef COLORS_H
#define COLORS_H

#include "zigil/zigil.h"

typedef struct World2DColors {
    zgl_Color player;
    zgl_Color player_collision_circle;
    zgl_Color player_collision_box;
    zgl_Color grid_background;
    zgl_Color grid_primary_wire;
    zgl_Color grid_secondary_wire;
    zgl_Color grid_axis_center_wire;
    zgl_Color grid_axis_edge_wire;
    zgl_Color default_wire;
} World2DColors;

extern World2DColors world2D_colors;

extern zgl_Color color_nothing;
extern zgl_Color color_sidebar_bg;
extern zgl_Color color_viewbar_bg;
extern zgl_Color color_view_container_bg;
extern zgl_Color color_view_container_border;
extern zgl_Color color_brushframe;
extern zgl_Color color_brush;
extern zgl_Color color_selected_brush;

extern void read_color_config(void);

#endif /* COLORS_H */
