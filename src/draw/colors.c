#include "src/draw/colors.h"
#include "src/system/config_reader.h"
#include <stdlib.h>

#define COLOR_PLAYER 0x44FF44
#define COLOR_PLAYER_COLLISION_CIRCLE 0x44FF44
#define COLOR_PLAYER_COLLISION_BOX 0x44FF44
#define COLOR_GRID_BACKGROUND 0x3F3F3F
#define COLOR_GRID_PRIMARY_WIRE 0x606060
#define COLOR_GRID_SECONDARY_WIRE 0x404040
#define COLOR_GRID_AXIS_CENTER_WIRE 0x808080
#define COLOR_GRID_AXIS_EDGE_WIRE 0x606060
#define COLOR_DEFAULT_WIRE 0xFF0587

World2DColors world2D_colors = {
    .player = COLOR_PLAYER,
    .player_collision_circle = COLOR_PLAYER_COLLISION_CIRCLE,
    .player_collision_box = COLOR_PLAYER_COLLISION_BOX,
    .grid_background = COLOR_GRID_BACKGROUND,
    .grid_primary_wire = COLOR_GRID_PRIMARY_WIRE,
    .grid_secondary_wire = COLOR_GRID_SECONDARY_WIRE,
    .grid_axis_center_wire = COLOR_GRID_AXIS_CENTER_WIRE,
    .grid_axis_edge_wire = COLOR_GRID_AXIS_EDGE_WIRE,
    .default_wire = COLOR_DEFAULT_WIRE,
};

zgl_Color color_view_bg = 0x999999;
zgl_Color color_nothing = 0x000000;

zgl_Color color_sidebar_bg = 0x6C6C6C;
zgl_Color color_viewbar_bg = 0x585858;
zgl_Color color_view_container_bg = 0x828282;
zgl_Color color_view_container_border = 0x313131;

zgl_Color color_brushframe = 0xF83298;
zgl_Color color_brush = 0x9766b2;
zgl_Color color_selected_brush = 0xd1bbdd;

static ConfigMapEntry const cme_world2D[9] = {
    [0] = {"Default", &world2D_colors.default_wire},
    [1] = {"Player", &world2D_colors.player},
    [2] = {"PlayerCollisionCircle", &world2D_colors.player_collision_circle},
    [3] = {"PlayerCollisionBox", &world2D_colors.player_collision_box},
    [4] = {"Background", &world2D_colors.grid_background},
    [5] = {"Primary", &world2D_colors.grid_primary_wire},
    [6] = {"Secondary", &world2D_colors.grid_secondary_wire},
    [7] = {"AxisCenter", &world2D_colors.grid_axis_center_wire},
    [8] = {"AxisOffCenter", &world2D_colors.grid_axis_edge_wire},
};

static ConfigMapEntry const cme_colors[4] = {
    [0] = {"ViewContainerBackground", &color_view_container_bg},
    [1] = {"ViewContainerBorder", &color_view_container_border},
    [2] = {"ViewbarBackground", &color_viewbar_bg},
    [3] = {"SidebarBackground", &color_sidebar_bg},
};

static void color_str_to_color(void *var, char const *color_str) {
    zgl_Color *clr = (zgl_Color *)var;
    *clr = str_to_i32(color_str, NULL, 16);
}

void read_color_config(void) {
    read_config_data("View2D.Color", 9, cme_world2D, color_str_to_color);

    read_config_data("Color", 4, cme_colors, color_str_to_color);    
}
