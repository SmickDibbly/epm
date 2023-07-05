#include "src/misc/epm_includes.h"

#include "src/draw/colors.h"
#include "src/draw/draw.h"
#include "src/draw/window/window.h"
#include "src/draw/default_layout.h"

static void draw_sidebar(Window *win);

static Window sidebar_win;
WindowNode _sidebar_node = {.win = &sidebar_win};
WindowNode *const sidebar_node = &_sidebar_node;

void init_sidebar(void) {
    Window *win = &sidebar_win;
    
    strcpy(win->name, "Sidebar");
    win->rect.x = sidebar_x;
    win->rect.y = sidebar_y;
    win->rect.w = sidebar_w;
    win->rect.h = sidebar_h;
    win->mrect.x = sidebar_x;
    win->mrect.y = sidebar_y;
    win->mrect.w = sidebar_w;
    win->mrect.h = sidebar_h;
    win->cursor = ZC_arrow;
    win->focusable = true,
    win->winfncs.draw = draw_sidebar;
    win->winfncs.onPointerPress = NULL;
    win->winfncs.onPointerRelease = NULL;
    win->winfncs.onPointerMotion = NULL;
    win->winfncs.onPointerEnter = NULL;
    win->winfncs.onPointerLeave = NULL;
}

static void draw_sidebar(Window *win) {
    zgl_FillRect(g_scr, win->rect.x, win->rect.y, win->rect.w, win->rect.h, color_sidebar_bg);
}
