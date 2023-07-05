#include "src/misc/epm_includes.h"

#include "src/input/input.h"

#include "src/draw/draw.h"
#include "src/draw/default_layout.h"
#include "src/draw/colors.h"

#include "src/draw/viewport/viewport.h"
#include "src/draw/viewport/viewport_internal.h"

Viewport viewports[NUM_VP];
Viewport *active_viewport = NULL;

// cyclical & ordered list of all active viewport modes, including those not
// currently mapped to a viewport. cycle_next_viewport_mode called on a viewport
// will make that viewport display the next mode in the mode_ring that is not
// already being displayed.
//static VPI mode_ring[32];

WindowNode *const viewport_wrap_nodes[NUM_VP] = {
    [VP_NONE] = NULL,
    [VP_TL]   = &viewports[VP_TL]  .wrap_node,
    [VP_TR]   = &viewports[VP_TR]  .wrap_node,
    [VP_BR]   = &viewports[VP_BR]  .wrap_node,
    [VP_BL]   = &viewports[VP_BL]  .wrap_node,
    [VP_L]    = &viewports[VP_L]   .wrap_node,
    [VP_R]    = &viewports[VP_R]   .wrap_node,
    [VP_T]    = &viewports[VP_T]   .wrap_node,
    [VP_B]    = &viewports[VP_B]   .wrap_node,
    [VP_FULL] = &viewports[VP_FULL].wrap_node,
};
WindowNode *const viewport_bar_nodes[NUM_VP] = {
    [VP_NONE] = NULL,
    [VP_TL]   = &viewports[VP_TL]  .bar_node,
    [VP_TR]   = &viewports[VP_TR]  .bar_node,
    [VP_BR]   = &viewports[VP_BR]  .bar_node,
    [VP_BL]   = &viewports[VP_BL]  .bar_node,
    [VP_L]    = &viewports[VP_L]   .bar_node,
    [VP_R]    = &viewports[VP_R]   .bar_node,
    [VP_T]    = &viewports[VP_T]   .bar_node,
    [VP_B]    = &viewports[VP_B]   .bar_node,
    [VP_FULL] = &viewports[VP_FULL].bar_node,
};
WindowNode *const viewport_VPI_nodes[NUM_VP] = {
    [VP_NONE] = NULL,
    [VP_TL]   = &viewports[VP_TL]  .VPI_node,
    [VP_TR]   = &viewports[VP_TR]  .VPI_node,
    [VP_BR]   = &viewports[VP_BR]  .VPI_node,
    [VP_BL]   = &viewports[VP_BL]  .VPI_node,
    [VP_L]    = &viewports[VP_L]   .VPI_node,
    [VP_R]    = &viewports[VP_R]   .VPI_node,
    [VP_T]    = &viewports[VP_T]   .VPI_node,
    [VP_B]    = &viewports[VP_B]   .VPI_node,
    [VP_FULL] = &viewports[VP_FULL].VPI_node,
};

static void draw_view_container(Window *win);
static Window view_container_window;

static WindowNode _view_container_node = {.win = &view_container_window};
WindowNode *const view_container_node = &_view_container_node;

void init_viewports(void) {
    active_viewport = &viewports[VP_TL];

    // some initialization is common to all viewports
    for (VPCode i_VP = 0; i_VP < NUM_VP; i_VP++) {
        Viewport *p_VP = &viewports[i_VP];
        
        p_VP->i_VP = i_VP;

        p_VP->wrap_win.data = p_VP;
        p_VP->wrap_win.focusable = false;
        p_VP->wrap_win.winfncs = (WindowFunctions){NULL};
        p_VP->wrap_node.win = &p_VP->wrap_win;
        
        p_VP->bar_win.data = p_VP;
        p_VP->bar_win.focusable = false; // viewbar takes pointer input, no kbd
        p_VP->bar_win.winfncs = winfncs_viewbar;
        p_VP->bar_node.win = &p_VP->bar_win;
        
        p_VP->VPI_win.data = p_VP;
        p_VP->VPI_win.focusable = true;
        p_VP->VPI_win.winfncs = (WindowFunctions){NULL};
        p_VP->VPI_node.win = &p_VP->VPI_win;
        p_VP->mapped_p_VPI = NULL;
    }
    
    Viewport *p_VP;
    
    p_VP = &viewports[VP_NONE];
    strcpy(p_VP->name, "NULL");
    
    strcpy(p_VP->wrap_win.name, "Viewport NULL");
    p_VP->wrap_win.rect.x = 0;
    p_VP->wrap_win.rect.y = 0;
    p_VP->wrap_win.rect.w = 0;
    p_VP->wrap_win.rect.h = 0;
    
    strcpy(p_VP->bar_win.name, "Viewport NULL: Bar");
    p_VP->bar_win.rect.x = 0;
    p_VP->bar_win.rect.y = 0;
    p_VP->bar_win.rect.w = 0;
    p_VP->bar_win.rect.h = 0;
    
    strcpy(p_VP->VPI_win.name, "Viewport NULL: View");
    p_VP->VPI_win.rect.x = 0;
    p_VP->VPI_win.rect.y = 0;
    p_VP->VPI_win.rect.w = 0;
    p_VP->VPI_win.rect.h = 0;


    p_VP = &viewports[VP_TL];
    strcpy(p_VP->name, "Top-Left");
    
    strcpy(p_VP->wrap_win.name, "Viewport Top-Left");
    p_VP->wrap_win.rect.x = vp_tl_x;
    p_VP->wrap_win.rect.y = vp_tl_y;
    p_VP->wrap_win.rect.w = vp_tl_w;
    p_VP->wrap_win.rect.h = vp_tl_h;
    
    strcpy(p_VP->bar_win.name, "Viewport Top-Left: Bar");
    p_VP->bar_win.rect.x = vp_tl_bar_x;
    p_VP->bar_win.rect.y = vp_tl_bar_y;
    p_VP->bar_win.rect.w = vp_tl_bar_w;
    p_VP->bar_win.rect.h = vp_bar_h;
    
    strcpy(p_VP->VPI_win.name, "Viewport Top-Left: View");
    p_VP->VPI_win.rect.x = vp_tl_view_x;
    p_VP->VPI_win.rect.y = vp_tl_view_y;
    p_VP->VPI_win.rect.w = vp_tl_view_w;
    p_VP->VPI_win.rect.h = vp_tl_view_h;


    p_VP = &viewports[VP_TR];
    strcpy(p_VP->name, "Top-Right");
    
    strcpy(p_VP->wrap_win.name, "Viewport Top-Right");
    p_VP->wrap_win.rect.x = vp_tr_x;
    p_VP->wrap_win.rect.y = vp_tr_y;
    p_VP->wrap_win.rect.w = vp_tr_w;
    p_VP->wrap_win.rect.h = vp_tr_h;
    
    strcpy(p_VP->bar_win.name, "Viewport Top-Right: Bar");
    p_VP->bar_win.rect.x = vp_tr_bar_x;
    p_VP->bar_win.rect.y = vp_tr_bar_y;
    p_VP->bar_win.rect.w = vp_tr_bar_w;
    p_VP->bar_win.rect.h = vp_bar_h;
    
    strcpy(p_VP->VPI_win.name, "Viewport Top-Right: View");
    p_VP->VPI_win.rect.x = vp_tr_view_x;
    p_VP->VPI_win.rect.y = vp_tr_view_y;
    p_VP->VPI_win.rect.w = vp_tr_view_w;
    p_VP->VPI_win.rect.h = vp_tr_view_h;


    p_VP = &viewports[VP_BR];
    strcpy(p_VP->name, "Bottom-Right");
        
    strcpy(p_VP->wrap_win.name, "Viewport Bottom-Right");
    p_VP->wrap_win.rect.x = vp_br_x;
    p_VP->wrap_win.rect.y = vp_br_y;
    p_VP->wrap_win.rect.w = vp_br_w;
    p_VP->wrap_win.rect.h = vp_br_h;
    
    strcpy(p_VP->bar_win.name, "Viewport Bottom-Right: Bar");
    p_VP->bar_win.rect.x = vp_br_bar_x;
    p_VP->bar_win.rect.y = vp_br_bar_y;
    p_VP->bar_win.rect.w = vp_br_bar_w;
    p_VP->bar_win.rect.h = vp_bar_h;
    
    strcpy(p_VP->VPI_win.name, "Viewport Bottom-Right: View");
    p_VP->VPI_win.rect.x = vp_br_view_x;
    p_VP->VPI_win.rect.y = vp_br_view_y;
    p_VP->VPI_win.rect.w = vp_br_view_w;
    p_VP->VPI_win.rect.h = vp_br_view_h;



    p_VP = &viewports[VP_BL];
    strcpy(p_VP->name, "Bottom-Left");
    
    strcpy(p_VP->wrap_win.name, "Viewport Bottom-Left");
    p_VP->wrap_win.rect.x = vp_bl_x;
    p_VP->wrap_win.rect.y = vp_bl_y;
    p_VP->wrap_win.rect.w = vp_bl_w;
    p_VP->wrap_win.rect.h = vp_bl_h;
    
    strcpy(p_VP->bar_win.name, "Viewport Bottom-Left: Bar");
    p_VP->bar_win.rect.x = vp_bl_bar_x;
    p_VP->bar_win.rect.y = vp_bl_bar_y;
    p_VP->bar_win.rect.w = vp_bl_bar_w;
    p_VP->bar_win.rect.h = vp_bar_h;
    
    strcpy(p_VP->VPI_win.name, "Viewport Bottom-Left: View");
    p_VP->VPI_win.rect.x = vp_bl_view_x;
    p_VP->VPI_win.rect.y = vp_bl_view_y;
    p_VP->VPI_win.rect.w = vp_bl_view_w;
    p_VP->VPI_win.rect.h = vp_bl_view_h;


    p_VP = &viewports[VP_L];
    strcpy(p_VP->name, "Left");
    
    strcpy(p_VP->wrap_win.name, "Viewport Left");
    p_VP->wrap_win.rect.x = vp_l_x;
    p_VP->wrap_win.rect.y = vp_l_y;
    p_VP->wrap_win.rect.w = vp_l_w;
    p_VP->wrap_win.rect.h = vp_l_h;
    
    strcpy(p_VP->bar_win.name, "Viewport Left: Bar");
    p_VP->bar_win.rect.x = vp_l_bar_x;
    p_VP->bar_win.rect.y = vp_l_bar_y;
    p_VP->bar_win.rect.w = vp_l_bar_w;
    p_VP->bar_win.rect.h = vp_bar_h;
    
    strcpy(p_VP->VPI_win.name, "Viewport Left: View");
    p_VP->VPI_win.rect.x = vp_l_view_x;
    p_VP->VPI_win.rect.y = vp_l_view_y;
    p_VP->VPI_win.rect.w = vp_l_view_w;
    p_VP->VPI_win.rect.h = vp_l_view_h;


    p_VP = &viewports[VP_R];
    strcpy(p_VP->name, "Right");
    
    strcpy(p_VP->wrap_win.name, "Viewport Right");
    p_VP->wrap_win.rect.x = vp_r_x;
    p_VP->wrap_win.rect.y = vp_r_y;
    p_VP->wrap_win.rect.w = vp_r_w;
    p_VP->wrap_win.rect.h = vp_r_h;
    
    strcpy(p_VP->bar_win.name, "Viewport Right: Bar");
    p_VP->bar_win.rect.x = vp_r_bar_x;
    p_VP->bar_win.rect.y = vp_r_bar_y;
    p_VP->bar_win.rect.w = vp_r_bar_w;
    p_VP->bar_win.rect.h = vp_bar_h;
    
    strcpy(p_VP->VPI_win.name, "Viewport Right: View");
    p_VP->VPI_win.rect.x = vp_r_view_x;
    p_VP->VPI_win.rect.y = vp_r_view_y;
    p_VP->VPI_win.rect.w = vp_r_view_w;
    p_VP->VPI_win.rect.h = vp_r_view_h;


    p_VP = &viewports[VP_T];
    strcpy(p_VP->name, "Top");
        
    strcpy(p_VP->wrap_win.name, "Viewport Top");
    p_VP->wrap_win.rect.x = vp_t_x;
    p_VP->wrap_win.rect.y = vp_t_y;
    p_VP->wrap_win.rect.w = vp_t_w;
    p_VP->wrap_win.rect.h = vp_t_h;
    
    strcpy(p_VP->bar_win.name, "Viewport Top: Bar");
    p_VP->bar_win.rect.x = vp_t_bar_x;
    p_VP->bar_win.rect.y = vp_t_bar_y;
    p_VP->bar_win.rect.w = vp_t_bar_w;
    p_VP->bar_win.rect.h = vp_bar_h;
    
    strcpy(p_VP->VPI_win.name, "Viewport Top: View");
    p_VP->VPI_win.rect.x = vp_t_view_x;
    p_VP->VPI_win.rect.y = vp_t_view_y;
    p_VP->VPI_win.rect.w = vp_t_view_w;
    p_VP->VPI_win.rect.h = vp_t_view_h;


    p_VP = &viewports[VP_B];
    strcpy(p_VP->name, "Bottom");
    
    strcpy(p_VP->wrap_win.name, "Viewport Bottom");
    p_VP->wrap_win.rect.x = vp_b_x;
    p_VP->wrap_win.rect.y = vp_b_y;
    p_VP->wrap_win.rect.w = vp_b_w;
    p_VP->wrap_win.rect.h = vp_b_h;
    
    strcpy(p_VP->bar_win.name, "Viewport Bottom: Bar");
    p_VP->bar_win.rect.x = vp_b_bar_x;
    p_VP->bar_win.rect.y = vp_b_bar_y;
    p_VP->bar_win.rect.w = vp_b_bar_w;
    p_VP->bar_win.rect.h = vp_bar_h;
    
    strcpy(p_VP->VPI_win.name, "Viewport Bottom: View");
    p_VP->VPI_win.rect.x = vp_b_view_x;
    p_VP->VPI_win.rect.y = vp_b_view_y;
    p_VP->VPI_win.rect.w = vp_b_view_w;
    p_VP->VPI_win.rect.h = vp_b_view_h;


    p_VP = &viewports[VP_FULL];
    strcpy(p_VP->name, "Full");
    
    strcpy(p_VP->wrap_win.name, "Viewport Full");
    p_VP->wrap_win.rect.x = vpfull_x;
    p_VP->wrap_win.rect.y = vpfull_y;
    p_VP->wrap_win.rect.w = vpfull_w;
    p_VP->wrap_win.rect.h = vpfull_h;
    
    strcpy(p_VP->bar_win.name, "Viewport Full: Bar");
    p_VP->bar_win.rect.x = vpfull_bar_x;
    p_VP->bar_win.rect.y = vpfull_bar_y;
    p_VP->bar_win.rect.w = vpfull_bar_w;
    p_VP->bar_win.rect.h = vp_bar_h;
    
    strcpy(p_VP->VPI_win.name, "Viewport Full: View");
    p_VP->VPI_win.rect.x = vpfull_view_x;
    p_VP->VPI_win.rect.y = vpfull_view_y;
    p_VP->VPI_win.rect.w = vpfull_view_w;
    p_VP->VPI_win.rect.h = vpfull_view_h;

  
    for (VPCode i_VP = 0; i_VP < NUM_VP; i_VP++) {
        Window *win;
        
        win = &viewports[i_VP].wrap_win;        
        win->mrect.x = win->rect.x<<16;
        win->mrect.y = win->rect.y<<16;
        win->mrect.w = win->rect.w<<16;
        win->mrect.h = win->rect.h<<16;

        win = &viewports[i_VP].bar_win;
        win->mrect.x = win->rect.x<<16;
        win->mrect.y = win->rect.y<<16;
        win->mrect.w = win->rect.w<<16;
        win->mrect.h = win->rect.h<<16;

        win = &viewports[i_VP].VPI_win;
        win->mrect.x = win->rect.x<<16;
        win->mrect.y = win->rect.y<<16;
        win->mrect.w = win->rect.w<<16;
        win->mrect.h = win->rect.h<<16;
    }
    

    Window *win = &view_container_window;
    strcpy(win->name, "Viewport Container");
    win->rect.x = view_container_x;
    win->rect.y = view_container_y;
    win->rect.w = view_container_w;
    win->rect.h = view_container_h;
    win->mrect.x = view_container_x<<16;
    win->mrect.y = view_container_y<<16;
    win->mrect.w = view_container_w<<16;
    win->mrect.h = view_container_h<<16;
    win->focusable = false;
    win->cursor = ZC_arrow;
    win->winfncs.draw = draw_view_container;
    win->winfncs.onPointerPress = NULL;
    win->winfncs.onPointerRelease = NULL;
    win->winfncs.onPointerMotion = NULL;
    win->winfncs.onPointerEnter = NULL;
    win->winfncs.onPointerLeave = NULL;
}


void epm_SetActiveVP(VPCode new_i_VP) {
    if (!active_viewport) {
        active_viewport = &viewports[new_i_VP];
        return;
    }

    active_viewport = &viewports[new_i_VP];
}

WindowNode *epm_PrevActiveVP(void) {
    if ( ! active_viewport) return NULL;
    
    ViewportLayout const *p_VPL = &viewport_layouts[i_curr_VPL];
    int i_cycle = p_VPL->VP_vec[active_viewport->i_VP];
    if (i_cycle == -1) return NULL;
            
    active_viewport = &viewports[p_VPL->VP_cycle[(i_cycle + 1) % p_VPL->VP_cycle_len]];
    epm_SetInputFocus(&active_viewport->VPI_node);
    
    return &active_viewport->VPI_node;
}

WindowNode *epm_NextActiveVP(void) {
    if ( ! active_viewport) return NULL;
    
    ViewportLayout const *p_VPL = &viewport_layouts[i_curr_VPL];
    int i_cycle = p_VPL->VP_vec[active_viewport->i_VP];
    if (i_cycle == -1) return NULL;
    
    size_t len = p_VPL->VP_cycle_len;
    active_viewport = &viewports[p_VPL->VP_cycle[(i_cycle + len - 1) % len]];
    epm_SetInputFocus(&active_viewport->VPI_node);
    
    return &active_viewport->VPI_node;
}

static void draw_view_container(Window *win) {
    (void)win;
    Window *vp_tl = viewport_wrap_nodes[VP_TL]->win;

    // all layouts have the following 4-pixel border
    zgl_GrayRect(g_scr,
                 view_container_window.rect.x,
                 view_container_window.rect.y,
                 view_container_window.rect.w,
                 4,
                 color_view_container_bg & 0xFF);
    
    zgl_GrayRect(g_scr,
                 view_container_window.rect.x,
                 view_container_window.rect.y + view_container_window.rect.h - 4,
                 view_container_window.rect.w,
                 4,
                 color_view_container_bg & 0xFF);
    
    zgl_GrayRect(g_scr,
                 view_container_window.rect.x,
                 view_container_window.rect.y,
                 4,
                 view_container_window.rect.h,
                 color_view_container_bg & 0xFF);

    zgl_GrayRect(g_scr,
                 view_container_window.rect.x + view_container_window.rect.w - 4,
                 view_container_window.rect.y,
                 4,
                 view_container_window.rect.h,
                 color_view_container_bg & 0xFF);


    switch (i_curr_VPL) {
    case VPL_QUAD: {
        // middle horizontal
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x,
                     view_container_window.rect.y + vp_tl->rect.h + 4,
                     view_container_window.rect.w,
                     4,
                     color_view_container_bg & 0xFF);

        // middle vertical
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x + vp_tl->rect.w + 4,
                     view_container_window.rect.y,
                     4,
                     view_container_window.rect.h,
                     color_view_container_bg & 0xFF);
        
    }
        break;
    case VPL_TRI_L: {
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x + vp_tl->rect.w + 4,
                     view_container_window.rect.y,
                     4,
                     view_container_window.rect.h,
                     color_view_container_bg & 0xFF);
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x + vp_tl->rect.w + 4,
                     view_container_window.rect.y + vp_tl->rect.h + 4,
                     view_container_window.rect.w/2,
                     4,
                     color_view_container_bg & 0xFF);
        
    }
        break;
    case VPL_TRI_R: {
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x + vp_tl->rect.w + 4,
                     view_container_window.rect.y,
                     4,
                     view_container_window.rect.h,
                     color_view_container_bg & 0xFF);
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x,
                     view_container_window.rect.y + vp_tl->rect.h + 4,
                     view_container_window.rect.w/2,
                     4,
                     color_view_container_bg & 0xFF);
    }
        break;
    case VPL_TRI_T: {
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x,
                     view_container_window.rect.y + vp_tl->rect.h + 4,
                     view_container_window.rect.w,
                     4,
                     color_view_container_bg & 0xFF);
        // middle vertical
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x + vp_tl->rect.w + 4,
                     view_container_window.rect.y + vp_tl->rect.h + 4,
                     4,
                     view_container_window.rect.h/2,
                     color_view_container_bg & 0xFF);
    }
        break;
    case VPL_TRI_B: {
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x,
                     view_container_window.rect.y + vp_tl->rect.h + 4,
                     view_container_window.rect.w,
                     4,
                     color_view_container_bg & 0xFF);
        // middle vertical
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x + vp_tl->rect.w + 4,
                     view_container_window.rect.y,
                     4,
                     view_container_window.rect.h/2,
                     color_view_container_bg & 0xFF);
    }
        break;
    case VPL_DUO_LR: {
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x + vp_tl->rect.w + 4,
                     view_container_window.rect.y,
                     4,
                     view_container_window.rect.h,
                     color_view_container_bg & 0xFF);
    }
        break;
    case VPL_DUO_TB: {
        zgl_GrayRect(g_scr,
                     view_container_window.rect.x,
                     view_container_window.rect.y + vp_tl->rect.h + 4,
                     view_container_window.rect.w,
                     4,
                     color_view_container_bg & 0xFF);
    }
        break;
    case VPL_MONO: // nothing more to do in MONO layout, but maybe will add
                   // something
        break;
    default:
        dibassert(false);
    }

    ViewportLayout const *p_VPL = &viewport_layouts[i_curr_VPL];
    for (size_t i_VP_cycle = 0; i_VP_cycle < p_VPL->VP_cycle_len; i_VP_cycle++) {
        zgl_PixelRect *rect = &viewports[p_VPL->VP_cycle[i_VP_cycle]].wrap_win.rect;
        zglDraw_PixelRect_Outline
            (g_scr, &(zgl_PixelRect){rect->x-1, rect->y-1, rect->w+2, rect->h+2},
             color_view_container_border);
    }

    zgl_FillRect(g_scr, view_container_window.rect.x, view_container_window.rect.y, 1, view_container_window.rect.h,
                 color_view_container_border);
    
    active_viewport = NULL;
    extern WindowNode *input_focus;
    for (VPCode i_VP = 0; i_VP < NUM_VP; i_VP++) {
        if (input_focus == &viewports[i_VP].VPI_node) {
            epm_SetActiveVP(i_VP);
            break;
        }
    }
    
    if (active_viewport) {
        zgl_PixelRect *rect = &active_viewport->wrap_win.rect;
        zglDraw_PixelRect_Outline
            (g_scr, &(zgl_PixelRect){rect->x-1, rect->y-1, rect->w+2, rect->h+2},
             0xFF0000);
        zglDraw_PixelRect_Outline
            (g_scr, &(zgl_PixelRect){rect->x-2, rect->y-2, rect->w+4, rect->h+4},
             0xFF0000);
    }
}
