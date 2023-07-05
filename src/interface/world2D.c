#include "src/misc/epm_includes.h"
#include "src/draw/window/window.h"

#include "src/draw/draw2D.h"

static Plane const top_plane = PLN_TOP;
static Plane const side_plane = PLN_SIDE;
static Plane const front_plane = PLN_FRONT;

static void draw_World2D(Window *win) {
    draw_View2D(win);
}

static void do_PointerPress_World2D(Window *win, zgl_PointerPressEvent *evt) {
    Plane tsf = *(Plane *)(win->data);    
    zgl_LongKeyCode key = evt->lk;
    
    if (key == LK_POINTER_WHEELUP) {
        zoom_level_down(tsf);
    }
    else if (key == LK_POINTER_WHEELDOWN) {
        zoom_level_up(tsf);
    }
    else if (key == LK_POINTER_LEFT) {
        win->dragged = true;
        zgl_SetCursor(ZC_drag);
        win->last_ptr_x = evt->x;
        win->last_ptr_y = evt->y;
    }    
}
static void do_PointerRelease_World2D(Window *win, zgl_PointerReleaseEvent *evt) {
    zgl_LongKeyCode key = evt->lk;
        
    if (key == LK_POINTER_LEFT) {
        win->dragged = false;
        zgl_SetCursor(ZC_arrow);
    }
}
static void do_PointerMotion_World2D(Window *win, zgl_PointerMotionEvent *evt) {
    Plane tsf = *(Plane *)(win->data);
    
    if (win->dragged) {
        scroll(win, evt->dx, evt->dy, tsf);
    }
}
static void do_PointerEnter_World2D(Window *win, zgl_PointerEnterEvent *evt) {
    (void)win, (void)evt;
    zgl_SetCursor(ZC_arrow);
}
static void do_PointerLeave_World2D(Window *win, zgl_PointerLeaveEvent *evt) {
    (void)evt;
    
    if (win->dragged) {
        win->dragged = false;
    }
}



#include "src/draw/viewport/viewport_internal.h"
ViewportInterface interface_WorldTop = {
    .i_VPI = VPI_WORLDTOP,
    .mapped_i_VP = VP_NONE,
    .windata = (void *)&top_plane,
    .onUnmap = NULL,
    .onMap = NULL,
    .winfncs = {
        .draw = draw_World2D,
        .onPointerPress = do_PointerPress_World2D,
        .onPointerRelease = do_PointerRelease_World2D,
        .onPointerMotion = do_PointerMotion_World2D,
        .onPointerEnter = do_PointerEnter_World2D,
        .onPointerLeave = do_PointerLeave_World2D,
        NULL,
        NULL,
    },
    .name = "World 2D Top"
};

ViewportInterface interface_WorldSide = {
    .i_VPI = VPI_WORLDSIDE,
    .mapped_i_VP = VP_NONE,
    .windata = (void *)&side_plane,
    .onUnmap = NULL,
    .onMap = NULL,
    .winfncs = {
        .draw = draw_World2D,
        .onPointerPress = do_PointerPress_World2D,
        .onPointerRelease = do_PointerRelease_World2D,
        .onPointerMotion = do_PointerMotion_World2D,
        .onPointerEnter = do_PointerEnter_World2D,
        .onPointerLeave = do_PointerLeave_World2D,
        NULL,
        NULL,
    },
    .name = "World 2D Side"
};

ViewportInterface interface_WorldFront = {
    .i_VPI = VPI_WORLDFRONT,
    .mapped_i_VP = VP_NONE,
    .windata = (void *)&front_plane,
    .onUnmap = NULL,
    .onMap = NULL,
    .init = NULL,
    .term = NULL,
    .winfncs = {
        .draw = draw_World2D,
        .onPointerPress = do_PointerPress_World2D,
        .onPointerRelease = do_PointerRelease_World2D,
        .onPointerMotion = do_PointerMotion_World2D,
        .onPointerEnter = do_PointerEnter_World2D,
        .onPointerLeave = do_PointerLeave_World2D,
        NULL,
        NULL,
    },
    .name = "World 2D Front"
};
