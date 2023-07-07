#include "src/misc/epm_includes.h"

#include "src/input/input.h"

#include "src/draw/draw.h"
#include "src/draw/colors.h"
#include "src/draw/default_layout.h"
#include "src/draw/text.h"

#include "src/draw/window/window_registry.h"

#include "src/draw/viewport/viewport.h"
#include "src/draw/viewport/viewport_internal.h"

#include "src/draw/draw3D.h"

#define GRAY_TOP    118
#define GRAY_BODY   88
#define GRAY_BOTTOM 58

zgl_PixelArray *icons22;

epm_Result load_icons(void) {
    icons22 = zgl_ReadBMP("../assets/icons.bmp");

    return EPM_SUCCESS;
}

epm_Result unload_icons(void) {
    zgl_DestroyPixelArray(icons22);

    return EPM_SUCCESS;
}

#define ICON_W 22

static zgl_Pixel const btnofs_interface   = {3 + 0*(ICON_W + 3), 3};

static zgl_Pixel const icon_topview_off   = {0*ICON_W, 0*ICON_W};
static zgl_Pixel const icon_topview_on    = {1*ICON_W, 0*ICON_W};

static zgl_Pixel const icon_sideview_off  = {0*ICON_W, 1*ICON_W};
static zgl_Pixel const icon_sideview_on   = {1*ICON_W, 1*ICON_W};

static zgl_Pixel const icon_frontview_off = {0*ICON_W, 2*ICON_W};
static zgl_Pixel const icon_frontview_on  = {1*ICON_W, 2*ICON_W};

static zgl_Pixel const icon_3Dview_off    = {0*ICON_W, 3*ICON_W};
static zgl_Pixel const icon_3Dview_on     = {1*ICON_W, 3*ICON_W};

static zgl_Pixel const icon_light_off     = {0*ICON_W, 4*ICON_W};
static zgl_Pixel const icon_light_on      = {1*ICON_W, 4*ICON_W};

static zgl_Pixel const icon_logview       = {0*ICON_W, 5*ICON_W};
static zgl_Pixel const icon_console       = {1*ICON_W, 5*ICON_W};

static zgl_Pixel const icon_wirenopoly    = {0*ICON_W, 6*ICON_W};
static zgl_Pixel const icon_wireoverlay   = {1*ICON_W, 6*ICON_W};
static zgl_Pixel const icon_wireoff       = {0*ICON_W, 7*ICON_W};

static void set_button_icon(ViewbarButton *btn, zgl_Pixel pos) {
    btn->icon_pos = pos;
}

static void updateButton_light_mode(ViewbarButton *self, Viewport *vp) {
    switch (g_settings.light_mode) {
    case LIGHT_OFF:
        set_button_icon(self, icon_light_off);
        break;
    case LIGHT_VBRI:
        set_button_icon(self, icon_light_on);
        break;
    case LIGHT_FBRI:
        set_button_icon(self, icon_light_on);
        break;
    case LIGHT_BOTH:
        set_button_icon(self, icon_light_on);
        break;
    }
}

static void updateButton_wire_mode(ViewbarButton *self, Viewport *vp) {
    switch (g_settings.wire_mode) {
    case WIRE_OFF:
        set_button_icon(self, icon_wireoff);
        break;
    case WIRE_NO_POLY:
        set_button_icon(self, icon_wirenopoly);
        break;
    case WIRE_OVERLAY:
        set_button_icon(self, icon_wireoverlay);
        break;
    }
}

static void updateButton_proj_mode(ViewbarButton *self, Viewport *vp) {
    switch (g_settings.proj_mode) {
    case PROJ_ORTHOGRAPHIC:
        set_button_icon(self, icon_wireoff);
        break;
    case PROJ_PERSPECTIVE:
        set_button_icon(self, icon_wirenopoly);
        break;
    }
}

static void onClickButton_light_mode(ViewbarButton *self, Viewport *vp) {
    epm_SetLightMode((g_settings.light_mode + 1) % NUM_LIGHT);
}

static void onClickButton_wire_mode(ViewbarButton *self, Viewport *vp) {
    epm_SetWireMode((g_settings.wire_mode + 1) % NUM_WIRE);
}

static void onClickButton_proj_mode(ViewbarButton *self, Viewport *vp) {
    epm_SetProjectionMode((g_settings.proj_mode + 1) % NUM_PROJ);
}

static void onClickButton_WorldTop(ViewbarButton *self, Viewport *vp) {
    epm_SetVPInterface(vp->i_VP, VPI_WORLDTOP);
}

static void onClickButton_WorldSide(ViewbarButton *self, Viewport *vp) {
    epm_SetVPInterface(vp->i_VP, VPI_WORLDSIDE);
}

static void onClickButton_WorldFront(ViewbarButton *self, Viewport *vp) {
    epm_SetVPInterface(vp->i_VP, VPI_WORLDFRONT);
}

static void onClickButton_World3D(ViewbarButton *self, Viewport *vp) {
    epm_SetVPInterface(vp->i_VP, VPI_WORLD3D);
}


static ViewbarButton const btn_light_mode = {
    .onClickButton = onClickButton_light_mode,
    .updateButton = updateButton_light_mode,
    .icon_pos = icon_light_on,
    .offset = {0,0}
};
static ViewbarButton const btn_wire_mode = {
    .onClickButton = onClickButton_wire_mode,
    .updateButton = updateButton_wire_mode,
    .icon_pos = icon_wireoff,
    .offset = {0},
};
static ViewbarButton const btn_proj_mode = {
    .onClickButton = onClickButton_proj_mode,
    .updateButton = updateButton_proj_mode,
    .icon_pos = {0,0},
    .offset = {0},
};
static ViewbarButton const btn_top = {
    .onClickButton = onClickButton_WorldTop,
    .updateButton = NULL,
    .icon_pos = icon_topview_off,
    .offset = {0},
};
static ViewbarButton const btn_side = {
    .onClickButton = onClickButton_WorldSide,
    .updateButton = NULL,
    .icon_pos = icon_sideview_off,
    .offset = {0},
};
static ViewbarButton const btn_front = {
    .onClickButton = onClickButton_WorldFront,
    .updateButton = NULL,
    .icon_pos = icon_frontview_off,
    .offset = {0},
};
static ViewbarButton const btn_3D = {
    .onClickButton = onClickButton_World3D,
    .updateButton = NULL,
    .icon_pos = icon_3Dview_on,
    .offset = {0},
};


static ViewbarSpec barspec_Monitor = {
    .icon = icon_logview,
    .num_subbtns = 0,
    //    .subbtns = {},
    .num_relbtns = 0,
    //    .relbtns = {},
};
static ViewbarSpec barspec_World3D = {
    .icon = icon_3Dview_on,
    .num_subbtns = 3,
    .subbtns = {btn_light_mode, btn_wire_mode, btn_proj_mode},
    .num_relbtns = 4,
    .relbtns = {btn_top, btn_side, btn_front, btn_3D},
};
static ViewbarSpec barspec_WorldTop = {
    .icon = icon_topview_on,
    .num_subbtns = 0,
    .subbtns = {{0}},
    .num_relbtns = 4,
    .relbtns = {btn_top, btn_side, btn_front, btn_3D},
};
static ViewbarSpec barspec_WorldSide = {
    .icon = icon_sideview_on,
    .num_subbtns = 0,
    .subbtns = {{0}},
    .num_relbtns = 4,
    .relbtns = {btn_top, btn_side, btn_front, btn_3D},
};
static ViewbarSpec barspec_WorldFront = {
    .icon = icon_frontview_on,
    .num_subbtns = 0,
    .subbtns = {{0}},
    .num_relbtns = 4,
    .relbtns = {btn_top, btn_side, btn_front, btn_3D},
};
static ViewbarSpec barspec_Log = {
    .icon = icon_logview,
    .num_subbtns = 0,
    .subbtns = {{0}},
    .num_relbtns = 0,
    .relbtns = {{0}},
};
static ViewbarSpec barspec_FileSelect = {
    .icon = {0,0},
    .num_subbtns = 0,
    .subbtns = {{0}},
    .num_relbtns = 0,
    .relbtns = {{0}},
};
static ViewbarSpec barspec_Console = {
    .icon = icon_console,
    .num_subbtns = 0,
    .subbtns = {{0}},
    .num_relbtns = 0,
    .relbtns = {{0}},
};
static ViewbarSpec barspec_TexView = {
    .icon = {0,0},
    .num_subbtns = 0,
    .subbtns = {{0}},
    .num_relbtns = 0,
    .relbtns = {{0}},
};
static ViewbarSpec barspec_BSPView = {
    .icon = {0,0},
    .num_subbtns = 0,
    .subbtns = {{0}},
    .num_relbtns = 0,
    .relbtns = {{0}},
};

void draw_viewbar(Window *win) {
    Viewport *p_VP = (Viewport *)win->data;

    int32_t
        win_x = win->rect.x,
        win_y = win->rect.y,
        win_w = win->rect.w,
        win_h = win->rect.h;

    zgl_Pixel btn_interface = {
        .x = win_x + btnofs_interface.x,
        .y = win_y + btnofs_interface.y,
    };
    
    zgl_GrayRect(g_scr, win_x, win_y, win_w, win_h, color_viewbar_bg & 0xFF);

    ViewbarSpec *barspec;
    
    switch (p_VP->mapped_p_VPI->i_VPI) {
    case VPI_MONITOR:
        barspec = &barspec_Monitor;
        break;
    case VPI_WORLD3D:
        barspec = &barspec_World3D;
        break;
    case VPI_WORLDTOP:
        barspec = &barspec_WorldTop;
        break;
    case VPI_WORLDSIDE:
        barspec = &barspec_WorldSide;
        break;
    case VPI_WORLDFRONT:
        barspec = &barspec_WorldFront;
        break;
    case VPI_FILESELECT:
        barspec = &barspec_FileSelect;
        break;
    case VPI_LOGVIEW:
        barspec = &barspec_Log;
        break;
    case VPI_CONSOLE:
        barspec = &barspec_Console;
        break;
    case VPI_TEXVIEW:
        barspec = &barspec_TexView;
        break;
    case VPI_BSPVIEW:
        barspec = &barspec_BSPView;
        break;
    default:
        return;
    }
    
    /* Interface icon */
    zgl_Blit(g_scr, btn_interface.x, btn_interface.y,
             icons22, barspec->icon.x, barspec->icon.y,
             ICON_W, ICON_W);
        
    /* Interface-specific icons */
    for (uint8_t i_btn = 0; i_btn < barspec->num_subbtns; i_btn++) {
        ViewbarButton *btn = barspec->subbtns + i_btn;
        btn->offset = (zgl_Pixel){3 + (i_btn+1)*(ICON_W + 3), 3};

        if (btn->updateButton) btn->updateButton(btn, p_VP);
        
        zgl_Blit(g_scr, win_x + btn->offset.x, win_y + btn->offset.y,
                 icons22, btn->icon_pos.x, btn->icon_pos.y,
                 ICON_W, ICON_W);
    }

    /* Related interface icons */
    for (uint8_t i_btn = 0; i_btn < barspec->num_relbtns; i_btn++) {
        ViewbarButton *btn = barspec->relbtns + i_btn;
        btn->offset = (zgl_Pixel){- 12 - (i_btn + 1)*(ICON_W + 3), 3};

        if (btn->updateButton) btn->updateButton(btn, p_VP);

        zgl_Blit(g_scr, win_x + win_w + btn->offset.x, win_y + btn->offset.y,
                 icons22, btn->icon_pos.x, btn->icon_pos.y,
                 ICON_W, ICON_W);
    }

    uint8_t const shadow_x_ofs = 1;
    uint8_t const shadow_y_ofs = 2;    
    zgl_Pixit len = (zgl_Pixit)strlen(p_VP->mapped_p_VPI->name);
    draw_BMPFont_string(g_scr, &win->rect, p_VP->mapped_p_VPI->name,
                        win_x + win_w/2 - (len*8)/2 + shadow_x_ofs,
                        win_y + win_h/2 - 8 + shadow_y_ofs,
                        FC_IBMVGA, 0x000000);
    draw_BMPFont_string(g_scr, &win->rect, p_VP->mapped_p_VPI->name,
                        win_x + win_w/2 - (len*8)/2,
                        win_y + win_h/2 - 8,
                        FC_IBMVGA, 0xCCCCCC);
}

void do_PointerPress_viewbar(Window *win, zgl_PointerPressEvent *evt) {
    Viewport *p_VP = (Viewport *)win->data;

    int32_t x = evt->x;
    int32_t y = evt->y;
    
    int32_t
        win_x = win->rect.x,
        win_y = win->rect.y,
        win_w = win->rect.w;
        //        win_h = win->rect.h;

    zgl_Pixel btn_interface = {
        .x = win_x + btnofs_interface.x,
        .y = win_y + btnofs_interface.y,
    };


    ViewbarSpec *barspec = NULL;
    
    switch (p_VP->mapped_p_VPI->i_VPI) {
    case VPI_WORLD3D:
        barspec = &barspec_World3D;
        break;
    case VPI_WORLDTOP:
        barspec = &barspec_WorldTop;
        break;
    case VPI_WORLDSIDE:
        barspec = &barspec_WorldSide;
        break;
    case VPI_WORLDFRONT:
        barspec = &barspec_WorldFront;
        break;
    case VPI_FILESELECT:
        barspec = &barspec_FileSelect;
        break;
    case VPI_LOGVIEW:
        barspec = &barspec_Log;
        break;
    case VPI_CONSOLE:
        barspec = &barspec_Console;
        break;
    case VPI_TEXVIEW:
        barspec = &barspec_TexView;
        break;
    case VPI_BSPVIEW:
        barspec = &barspec_BSPView;
        break;
    default:
        return;
    }

    zgl_Pixel pos;
    ViewbarButton *btn = NULL;
    
    /* Interface icon */
    pos = btn_interface;
    if (x >= pos.x && x <= pos.x + ICON_W &&
        y >= pos.y && y <= pos.y + ICON_W) {
        epm_Log(LT_WARN, "This button has no effect yet.");

        goto Hit;
    }
        
    /* Interface-specific icons */
    for (uint8_t i_btn = 0; i_btn < barspec->num_subbtns; i_btn++) {
        btn = barspec->subbtns + i_btn;
        btn->offset = (zgl_Pixel){3 + (i_btn+1)*(ICON_W + 3), 3};
        
        pos.x = win_x + btn->offset.x;
        pos.y = win_y + btn->offset.y;
        if (x >= pos.x && x <= pos.x + ICON_W &&
            y >= pos.y && y <= pos.y + ICON_W) {

            goto Hit;
        }
    }

    /* Related interface icons */
    for (uint8_t i_btn = 0; i_btn < barspec->num_relbtns; i_btn++) {
        btn = barspec->relbtns + i_btn;
        btn->offset = (zgl_Pixel){- 12 - (i_btn + 1)*(ICON_W + 3), 3};

        pos.x = win_x + win_w + btn->offset.x;
        pos.y = win_y + btn->offset.y;
        if (x >= pos.x && x <= pos.x + ICON_W &&
            y >= pos.y && y <= pos.y + ICON_W) {
            
            goto Hit;
        }
    }

    return;

 Hit:
    if (btn && btn->onClickButton) {
        btn->onClickButton(btn, p_VP);
    }
    
    epm_SetInputFocus(&p_VP->VPI_node);
}




/* -------------------------------------------------------------------------- */

WindowFunctions winfncs_viewbar = {
    .draw = draw_viewbar,
    .onPointerPress = do_PointerPress_viewbar,
    .onPointerRelease = NULL,
    .onPointerMotion = NULL,
    .onPointerEnter = NULL,
    .onPointerLeave = NULL,
    .onKeyPress = NULL,
    .onKeyRelease = NULL,
};
