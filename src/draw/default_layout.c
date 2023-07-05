#include "src/draw/default_layout.h"
#include "src/draw/draw.h"
#include "src/misc/epm_includes.h"

int menubar_x;
int menubar_y;
int menubar_w;
int menubar_h;

int sidebar_x;
int sidebar_y;
int sidebar_w;
int sidebar_h;

int view_container_x;
int view_container_y;
int view_container_w;
int view_container_h;

int vp_bar_h;

int vp_tl_x;
int vp_tl_y;
int vp_tl_w;
int vp_tl_h;
int vp_tl_bar_x;
int vp_tl_bar_y;
int vp_tl_bar_w;
int vp_tl_view_x;
int vp_tl_view_y;
int vp_tl_view_w;
int vp_tl_view_h;

int vp_tr_x;
int vp_tr_y;
int vp_tr_w;
int vp_tr_h;
int vp_tr_bar_x;
int vp_tr_bar_y;
int vp_tr_bar_w;
int vp_tr_view_x;
int vp_tr_view_y;
int vp_tr_view_w;
int vp_tr_view_h;

int vp_br_x;
int vp_br_y;
int vp_br_w;
int vp_br_h;
int vp_br_bar_x;
int vp_br_bar_y;
int vp_br_bar_w;
int vp_br_view_x;
int vp_br_view_y;
int vp_br_view_w;
int vp_br_view_h;

int vp_bl_x;
int vp_bl_y;
int vp_bl_w;
int vp_bl_h;
int vp_bl_bar_x;
int vp_bl_bar_y;
int vp_bl_bar_w;
int vp_bl_view_x;
int vp_bl_view_y;
int vp_bl_view_w;
int vp_bl_view_h;

int vp_l_x;
int vp_l_y;
int vp_l_w;
int vp_l_h;
int vp_l_bar_x;
int vp_l_bar_y;
int vp_l_bar_w;
int vp_l_view_x;
int vp_l_view_y;
int vp_l_view_w;
int vp_l_view_h;

int vp_r_x;
int vp_r_y;
int vp_r_w;
int vp_r_h;
int vp_r_bar_x;
int vp_r_bar_y;
int vp_r_bar_w;
int vp_r_view_x;
int vp_r_view_y;
int vp_r_view_w;
int vp_r_view_h;

int vp_t_x;
int vp_t_y;
int vp_t_w;
int vp_t_h;
int vp_t_bar_x;
int vp_t_bar_y;
int vp_t_bar_w;
int vp_t_view_x;
int vp_t_view_y;
int vp_t_view_w;
int vp_t_view_h;

int vp_b_x;
int vp_b_y;
int vp_b_w;
int vp_b_h;
int vp_b_bar_x;
int vp_b_bar_y;
int vp_b_bar_w;
int vp_b_view_x;
int vp_b_view_y;
int vp_b_view_w;
int vp_b_view_h;

int vpfull_x;
int vpfull_y;
int vpfull_w;
int vpfull_h;
int vpfull_bar_x;
int vpfull_bar_y;
int vpfull_bar_w;
int vpfull_view_x;
int vpfull_view_y;
int vpfull_view_w;
int vpfull_view_h;


void configure_default_layout(void) {
    dibassert( ! (view_container_w & 1) && ! (view_container_h & 1));
    
    menubar_x = 0;
    menubar_y = 0;
    menubar_w = g_scr_w;
    menubar_h = 28;

    sidebar_x = 0;
    sidebar_y = menubar_h;
    sidebar_w = 64;
    sidebar_h = g_scr_h - sidebar_y;

    view_container_x = sidebar_w;
    view_container_y = menubar_h;
    view_container_w = g_scr_w - view_container_x;
    view_container_h = g_scr_h - view_container_y;

    vp_bar_h = 28;
    
    vp_tl_x = view_container_x + 4;
    vp_tl_y = view_container_y + 4;
    vp_tl_w = view_container_w/2 - 6;
    vp_tl_h = view_container_h/2 - 6;
    vp_tl_bar_x = vp_tl_x;
    vp_tl_bar_y = vp_tl_y;
    vp_tl_bar_w = vp_tl_w;
    vp_tl_view_x = vp_tl_x;
    vp_tl_view_y = vp_tl_y + vp_bar_h;
    vp_tl_view_w = vp_tl_w;
    vp_tl_view_h = vp_tl_h - vp_bar_h;

    vp_tr_x = view_container_x + view_container_w/2 + 2;
    vp_tr_y = view_container_y + 4;
    vp_tr_w = view_container_w/2 - 6;
    vp_tr_h = view_container_h/2 - 6;    
    vp_tr_bar_x = vp_tr_x;
    vp_tr_bar_y = vp_tr_y;
    vp_tr_bar_w = vp_tr_w;
    vp_tr_view_x = vp_tr_x;
    vp_tr_view_y = vp_tr_y + vp_bar_h;
    vp_tr_view_w = vp_tr_w;
    vp_tr_view_h = vp_tr_h - vp_bar_h;
    
    vp_br_x = view_container_x + view_container_w/2 + 2;
    vp_br_y = view_container_y + view_container_h/2 + 2;
    vp_br_w = view_container_w/2 - 6;
    vp_br_h = view_container_h/2 - 6;
    vp_br_bar_x = vp_br_x;
    vp_br_bar_y = vp_br_y;
    vp_br_bar_w = vp_br_w;
    vp_br_view_x = vp_br_x;
    vp_br_view_y = vp_br_y + vp_bar_h;
    vp_br_view_w = vp_br_w;
    vp_br_view_h = vp_br_h - vp_bar_h;

    vp_bl_x = view_container_x + 4;
    vp_bl_y = view_container_y + view_container_h/2 + 2;
    vp_bl_w = view_container_w/2 - 6;
    vp_bl_h = view_container_h/2 - 6;
    vp_bl_bar_x = vp_bl_x;
    vp_bl_bar_y = vp_bl_y;
    vp_bl_bar_w = vp_bl_w;
    vp_bl_view_x = vp_bl_x;
    vp_bl_view_y = vp_bl_y + vp_bar_h;
    vp_bl_view_w = vp_bl_w;
    vp_bl_view_h = vp_bl_h - vp_bar_h;

    vp_l_x = view_container_x + 4;
    vp_l_y = view_container_y + 4;
    vp_l_w = view_container_w/2 - 6;
    vp_l_h = view_container_h - 8;
    vp_l_bar_x = vp_l_x;
    vp_l_bar_y = vp_l_y;
    vp_l_bar_w = vp_l_w;
    vp_l_view_x = vp_l_x;
    vp_l_view_y = vp_l_y + vp_bar_h;
    vp_l_view_w = vp_l_w;
    vp_l_view_h = vp_l_h - vp_bar_h;

    vp_r_x = view_container_x + view_container_w/2 + 2;
    vp_r_y = view_container_y + 4;
    vp_r_w = view_container_w/2 - 6;
    vp_r_h = view_container_h - 8;
    vp_r_bar_x = vp_r_x;
    vp_r_bar_y = vp_r_y;
    vp_r_bar_w = vp_r_w;
    vp_r_view_x = vp_r_x;
    vp_r_view_y = vp_r_y + vp_bar_h;
    vp_r_view_w = vp_r_w;
    vp_r_view_h = vp_r_h - vp_bar_h;

    vp_t_x = view_container_x + 4;
    vp_t_y = view_container_y + 4;
    vp_t_w = view_container_w - 8;
    vp_t_h = view_container_h/2 - 6;
    vp_t_bar_x = vp_t_x;
    vp_t_bar_y = vp_t_y;
    vp_t_bar_w = vp_t_w;
    vp_t_view_x = vp_t_x;
    vp_t_view_y = vp_t_y + vp_bar_h;
    vp_t_view_w = vp_t_w;
    vp_t_view_h = vp_t_h - vp_bar_h;

    vp_b_x = view_container_x + 4;
    vp_b_y = view_container_y + view_container_h/2 + 2;
    vp_b_w = view_container_w - 8;
    vp_b_h = view_container_h/2 - 6;
    vp_b_bar_x = vp_b_x;
    vp_b_bar_y = vp_b_y;
    vp_b_bar_w = vp_b_w;
    vp_b_view_x = vp_b_x;
    vp_b_view_y = vp_b_y + vp_bar_h;
    vp_b_view_w = vp_b_w;
    vp_b_view_h = vp_b_h - vp_bar_h;
    
    vpfull_x = view_container_x + 4;
    vpfull_y = view_container_y + 4;
    vpfull_w = view_container_w - 8;
    vpfull_h = view_container_h - 8;
    vpfull_bar_x = vpfull_x;
    vpfull_bar_y = vpfull_y;
    vpfull_bar_w = vpfull_w;
    vpfull_view_x = vpfull_x;
    vpfull_view_y = vpfull_y + vp_bar_h;
    vpfull_view_w = vpfull_w;
    vpfull_view_h = vpfull_h - vp_bar_h;
}
