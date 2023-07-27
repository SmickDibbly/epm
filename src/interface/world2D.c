#include "src/misc/epm_includes.h"
#include "src/draw/window/window.h"
#include "src/world/selection.h"
#include "src/draw/draw2D.h"
#include "src/draw/text.h"

static Plane const top_plane = PLN_TOP;
static Plane const side_plane = PLN_SIDE;
static Plane const front_plane = PLN_FRONT;

typedef struct coordmap {
    int i_x;
    int i_y;
} coordmap;

static coordmap g_v2Dmap[3] = {
    [PLN_TOP] = {
        .i_x = I_X,
        .i_y = I_Y,
    },
    [PLN_SIDE] = {
        .i_x = I_Y,
        .i_y = I_Z,
    },
    [PLN_FRONT] = {
        .i_x = I_X,
        .i_y = I_Z,
    },
};

// Input Modes (IM)
// Top-level
#define IM_DEFAULT 0
#define IM_PREDRAG 1
#define IM_DRAG 2
static int input_mode = IM_DEFAULT;

// Submodes of IM_DRAG
#define IM_DRAG_VIEWPAN 0
#define IM_DRAG_BRUSHPAN 1
#define IM_DRAG_BRUSHPAN_X 2
#define IM_DRAG_BRUSHPAN_Y 3
static int drag_mode = IM_DRAG_VIEWPAN;

extern void grab_input(Window *win);
extern void ungrab_input(void);

static void draw_World2D(Window *win) {
    draw_View2D(win);
    extern void worldpoint_from_screenpoint(Fix32Vec_2D mapscr_pt, WorldVec *world_pt, Plane tsf);
    Plane tsf = *(Plane *)(win->data);
    
    zgl_Pixel pos = zgl_GetPointerPos();
    zgl_mPixel mpos = mpixel_from_pixel(pos);
    char buf[256] = {'\0'};
    WorldVec vec = {{0}};
    worldpoint_from_screenpoint(mpos, &vec,  tsf);
    if (tsf == PLN_TOP) {
        snprintf(buf, 255, "Cursor (world): (%s, %s, %s)",
                 fmt_fix_d(x_of(vec), 16, 4),
                 fmt_fix_d(y_of(vec), 16, 4),
                 "Z");
    }
    else if (tsf == PLN_SIDE) {
        snprintf(buf, 255, "Cursor (world): (%s, %s, %s)",
                 "X",
                 fmt_fix_d(y_of(vec), 16, 4),
                 fmt_fix_d(z_of(vec), 16, 4));        
    }
    else if (tsf == PLN_FRONT) {
        snprintf(buf, 255, "Cursor (world): (%s, %s, %s)",
                 fmt_fix_d(x_of(vec), 16, 4),
                 "Y",
                 fmt_fix_d(z_of(vec), 16, 4));
    }
    else {
        dibassert(false);
    }
    
    draw_BMPFont_string(g_scr, NULL, buf,
                        win->rect.x+8, win->rect.y+4, FC_IBMVGA, 0xFFFFFF);    
}


/* -------------------------------------------------------------------------- */


static Fix32 brushpan_acc_x = 0;
static Fix32 brushpan_acc_y = 0;
static void compute_drag_submode(Window *win, uint8_t mod_flags, uint8_t but_flags) {
        bool 
        LEFT  = but_flags & ZGL_MOUSELEFT_MASK,
        RIGHT = but_flags & ZGL_MOUSERIGHT_MASK,
        SHIFT = mod_flags & ZGL_SHIFT_MASK,
        CTRL  = mod_flags & ZGL_CTRL_MASK;

    if ( ! (LEFT || RIGHT)) {
        //ungrab_input();
        input_mode = IM_DEFAULT;
    }
    else {
        //grab_input(win);
        input_mode = IM_DRAG;
        if (SHIFT || CTRL) {
            if (LEFT && RIGHT) {
                drag_mode = IM_DRAG_BRUSHPAN;
                brushpan_acc_x = 0;
                brushpan_acc_y = 0;
            }
            else if (LEFT) {
                drag_mode = IM_DRAG_BRUSHPAN_X;
                brushpan_acc_x = 0;
                brushpan_acc_y = 0;
            }
            else /*if (RIGHT)*/ {
                drag_mode = IM_DRAG_BRUSHPAN_Y;
                brushpan_acc_x = 0;
                brushpan_acc_y = 0;
            }
        }
        else {
            if (LEFT && RIGHT) {
                drag_mode = IM_DRAG_VIEWPAN;
            }
            else if (LEFT) {
                drag_mode = IM_DRAG_VIEWPAN;
            }
            else /*if (RIGHT)*/ {
                drag_mode = IM_DRAG_VIEWPAN;
            }
        }    
    }
}

static void do_PointerPress_drag(Window *win, zgl_PointerPressEvent *evt) {
    compute_drag_submode(win, evt->mod_flags, evt->but_flags);
        
    return;
}



static void do_PointerPress_predrag(Window *win, zgl_PointerPressEvent *evt) {
    zgl_LongKeyCode lk = evt->lk;
    (void)lk;
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
    /*
    else if (key == LK_POINTER_LEFT) {
        win->dragged = true;
        zgl_SetCursor(ZC_drag);
        win->last_ptr_x = evt->x;
        win->last_ptr_y = evt->y;
    }
    */
    else {
        switch (input_mode) {
        case IM_DEFAULT:
            input_mode = IM_PREDRAG;
            break;
        case IM_PREDRAG:
            do_PointerPress_predrag(win, evt);
            break;
        case IM_DRAG:
            do_PointerPress_drag(win, evt);
            break;
        }
    } 
}
static void do_PointerRelease_World2D(Window *win, zgl_PointerReleaseEvent *evt) {
    zgl_LongKeyCode lk = evt->lk;

    switch (input_mode) {
    case IM_PREDRAG:
        ungrab_input();
        input_mode = IM_DEFAULT;
        if (lk == LK_POINTER_LEFT) {
            if (evt->mod_flags & ZGL_SHIFT_MASK) {
                //select_brush(win, zgl_GetPointerPos());
            }
            else if (evt->mod_flags & ZGL_CTRL_MASK) {
                //select_face(win, zgl_GetPointerPos());
            }
            else {
                //select_one_face(win, zgl_GetPointerPos());
            }
        }
        else if (lk == LK_POINTER_RIGHT) {
            //select_vertex(win, zgl_GetPointerPos());
        }
        break;
    case IM_DRAG:
        compute_drag_submode(win, evt->mod_flags, evt->but_flags);
        break;
    case IM_DEFAULT:
    default:
        break;
    }
    
    zgl_SetCursor(ZC_cross);
}




static void do_PointerMotion_World2D(Window *win, zgl_PointerMotionEvent *evt) {
    Plane tsf = *(Plane *)(win->data);
    int dx = evt->dx;
    int dy = evt->dy;
       
    switch (input_mode) {
    case IM_DEFAULT:
        zgl_SetCursor(ZC_cross);
        break;
    case IM_PREDRAG:
        compute_drag_submode(win, evt->mod_flags, evt->but_flags);
        break;
    case IM_DRAG:
        zgl_SetCursor(ZC_drag);
        switch (drag_mode) {
        case IM_DRAG_VIEWPAN:
            scroll(win, dx, dy, tsf);    
            break;
        case IM_DRAG_BRUSHPAN: {
            Fix32 wdx, wdy;
            
            compute_scroll(win, dx, dy, tsf, &wdx, &wdy);
            wdx = -wdx;
            
            if (sel_brush.head == NULL) {
                BrushSel_add(g_frame);
            }
            extern void snap_brush_selection(void);
            snap_brush_selection();
            goto DoX;

            DoX:
            brushpan_acc_x += wdx;

            extern uint32_t snap;
            if ( ABS(brushpan_acc_x) <= snap) goto DoY;

            if (brushpan_acc_x < 0) {
                wdx = (brushpan_acc_x & ~(snap-1)) + snap;
                brushpan_acc_x = (brushpan_acc_x & (snap-1)) - snap;
            }
            else {
                wdx = (brushpan_acc_x & ~(snap-1));
                brushpan_acc_x = (brushpan_acc_x & (snap-1));
            }

            brushsel_POR.v[g_v2Dmap[tsf].i_x] += wdx;
            for (SelectionNode *node = sel_brush.head; node; node = node->next) {
                Brush *brush = node->obj;
                brush->POR.v[g_v2Dmap[tsf].i_x] += wdx;
                
                WorldVec *v = brush->verts;
                for (size_t i_v = 0; i_v < brush->num_verts; i_v++) {
                    v[i_v].v[g_v2Dmap[tsf].i_x] += wdx;
                }
            }
            
            if (LK_states[LK_LSHIFT]) cam.pos.v[g_v2Dmap[tsf].i_x] += wdx;

            DoY:
            brushpan_acc_y += wdy;

            extern uint32_t snap;
            if ( ABS(brushpan_acc_y) <= snap) break;

            if (brushpan_acc_y < 0) {
                wdy = (brushpan_acc_y & ~(snap-1)) + snap;
                brushpan_acc_y = (brushpan_acc_y & (snap-1)) - snap;
            }
            else {
                wdy = (brushpan_acc_y & ~(snap-1));
                brushpan_acc_y = (brushpan_acc_y & (snap-1));
            }

            brushsel_POR.v[g_v2Dmap[tsf].i_y] += wdy;
            for (SelectionNode *node = sel_brush.head; node; node = node->next) {
                Brush *brush = node->obj;
                brush->POR.v[g_v2Dmap[tsf].i_y] += wdy;
                
                WorldVec *v = brush->verts;
                for (size_t i_v = 0; i_v < brush->num_verts; i_v++) {
                    v[i_v].v[g_v2Dmap[tsf].i_y] += wdy;
                }
            }
            
            if (LK_states[LK_LSHIFT]) cam.pos.v[g_v2Dmap[tsf].i_y] += wdy;
            
        }
            break;
        case IM_DRAG_BRUSHPAN_X: {
            Fix32 wdx, wdy_ignored;
            
            compute_scroll(win, dx, dy, tsf, &wdx, &wdy_ignored);
            wdx = -wdx;
            
            if (sel_brush.head == NULL) {
                BrushSel_add(g_frame);
            }
            extern void snap_brush_selection(void);
            snap_brush_selection();
            
            brushpan_acc_x += wdx;

            extern uint32_t snap;
            if ( ABS(brushpan_acc_x) <= snap) break;

            if (brushpan_acc_x < 0) {
                wdx = (brushpan_acc_x & ~(snap-1)) + snap;
                brushpan_acc_x = (brushpan_acc_x & (snap-1)) - snap;
            }
            else {
                wdx = (brushpan_acc_x & ~(snap-1));
                brushpan_acc_x = (brushpan_acc_x & (snap-1));
            }

            brushsel_POR.v[g_v2Dmap[tsf].i_x] += wdx;
            for (SelectionNode *node = sel_brush.head; node; node = node->next) {
                Brush *brush = node->obj;
                brush->POR.v[g_v2Dmap[tsf].i_x] += wdx;
                
                WorldVec *v = brush->verts;
                for (size_t i_v = 0; i_v < brush->num_verts; i_v++) {
                    v[i_v].v[g_v2Dmap[tsf].i_x] += wdx;
                }
            }
            
            if (LK_states[LK_LSHIFT]) cam.pos.v[g_v2Dmap[tsf].i_x] += wdx;
        }
            break;
        case IM_DRAG_BRUSHPAN_Y: {
            Fix32 wdx_ignored, wdy;
            
            compute_scroll(win, dx, dy, tsf, &wdx_ignored, &wdy);
            //wdy = -wdy;
            
            if (sel_brush.head == NULL) {
                BrushSel_add(g_frame);
            }
            extern void snap_brush_selection(void);
            snap_brush_selection();
            
            brushpan_acc_y += wdy;

            extern uint32_t snap;
            if ( ABS(brushpan_acc_y) <= snap) break;

            if (brushpan_acc_y < 0) {
                wdy = (brushpan_acc_y & ~(snap-1)) + snap;
                brushpan_acc_y = (brushpan_acc_y & (snap-1)) - snap;
            }
            else {
                wdy = (brushpan_acc_y & ~(snap-1));
                brushpan_acc_y = (brushpan_acc_y & (snap-1));
            }

            brushsel_POR.v[g_v2Dmap[tsf].i_y] += wdy;
            for (SelectionNode *node = sel_brush.head; node; node = node->next) {
                Brush *brush = node->obj;
                brush->POR.v[g_v2Dmap[tsf].i_y] += wdy;
                
                WorldVec *v = brush->verts;
                for (size_t i_v = 0; i_v < brush->num_verts; i_v++) {
                    v[i_v].v[g_v2Dmap[tsf].i_y] += wdy;
                }
            }
            
            if (LK_states[LK_LSHIFT]) cam.pos.v[g_v2Dmap[tsf].i_y] += wdy;
        }
            break;
        }
        break;
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
