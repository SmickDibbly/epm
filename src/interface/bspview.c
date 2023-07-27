#include "src/misc/epm_includes.h"
#include "src/draw/window/window.h"
#include "src/world/geometry.h"
#include "src/world/world.h"
#include "src/draw/viewport/viewport_internal.h"

typedef struct ColoredEdge {
    Edge e;
    zgl_Color color;    
} ColoredEdge;

typedef struct DiagramNode DiagramNode;
struct DiagramNode {
    DiagramNode *L;
    DiagramNode *R;
    DiagramNode *thread;
    int mod;    
    zgl_Pixel *pixel;
};

#undef x_of
#define x_of(P_DNODE) ((P_DNODE)->pixel->x)

#undef y_of
#define y_of(P_DNODE) ((P_DNODE)->pixel->y)

static void construct_diagram(void);

static size_t num_nodes = 0;
static DiagramNode *dnodes = NULL;
static zgl_Pixel *vertices = NULL;

static size_t num_edges;
static ColoredEdge edges[4096];

static zgl_Pixel hovered_grid_point = {-1, -1};

void update_BSPView(void) {
    
    if ( ! (g_world.worldflags & WF_LOADED_BSPGEO)) {
        epm_Log(LT_WARN, "Cannot update BSPView when no BSP exists.");
        return;
    }
    
    construct_diagram();
}

static void onMap_BSPView(ViewportInterface *self, Viewport *p_VP) {
    //Window *win = &p_VP->VPI_win;

    dibassert(self->i_VPI == VPI_BSPVIEW);

    update_BSPView();
}

static size_t i_curr_v = 0;
static int const mul = 8;

static void draw_BSPView(Window *win) {
    if ( ! (g_world.worldflags & WF_LOADED_BSPGEO)) {    
        return;
    }
    
    zgl_Pixel pixel2;
    zgl_PixelRect rect;
    
    if (hovered_grid_point.x >= 0 && hovered_grid_point.y >= 0) {
        zgl_GrayRect(g_scr, hovered_grid_point.x, hovered_grid_point.y, mul, mul, 0x44);
    }    
    
    pixel2.x = mul*vertices[i_curr_v].x + win->rect.x;
    pixel2.y = mul*vertices[i_curr_v].y + win->rect.y;
    rect = (zgl_PixelRect){pixel2.x - (mul>>1),
                           pixel2.y - (mul>>1),
                           mul,
                           mul};
    zgl_GrayRect(g_scr, rect.x, rect.y, rect.w, rect.h, 0x77);
   

    zgl_Pixel pixel0;
    zgl_Pixel pixel1;
    for (size_t i_e = 0; i_e < num_edges; i_e++) {
        size_t i_v0 = edges[i_e].e.i_v0;
        size_t i_v1 = edges[i_e].e.i_v1;
        pixel0.x = mul*vertices[i_v0].x + win->rect.x;
        pixel0.y = mul*vertices[i_v0].y + win->rect.y;
        pixel1.x = mul*vertices[i_v1].x + win->rect.x;
        pixel1.y = mul*vertices[i_v1].y + win->rect.y;
        zgl_PixelSeg seg = {pixel0, pixel1};
        zglDraw_PixelSeg(g_scr, &win->rect, &seg, edges[i_e].color);
    }
    
    zgl_Pixel pixel;
    for (size_t i_v = 0; i_v < num_nodes; i_v++) {
        pixel.x = mul*vertices[i_v].x + win->rect.x;
        pixel.y = mul*vertices[i_v].y + win->rect.y;
        if (i_v == i_curr_v)
            zglDraw_PixelDot(g_scr, &win->rect, &pixel, 0xFFFFFF);
        else
            zglDraw_PixelDot(g_scr, &win->rect, &pixel, 0x00FFFF);
    }


    /*
    int len_lcont = 0;
    int len_rcont = 0;

    int *lcont = construct_contour(dnodes + 0, gt, 0, &len_lcont, NULL);
    int *rcont = construct_contour(dnodes + 0, lt, 0, &len_rcont, NULL);

    printf(" Left contour: [");
    for (int i = 0; i < len_lcont; i++) {
        printf("%i, ", lcont[i]);
    }
    printf("]\n");
    zgl_Free(lcont);
    
    printf("Right contour: [");
    for (int i = 0; i < len_rcont; i++) {
        printf("%i, ", rcont[i]);
    }
    printf("]\n\n");
    zgl_Free(rcont);
    */
}

static void do_PointerPress_BSPView(Window *win, zgl_PointerPressEvent *evt) {
    int x, y;
    x = evt->x;
    y = evt->y;

    for (size_t i_v = 0; i_v < num_nodes; i_v++) {
        zgl_Pixel pixel = vertices[i_v];
        pixel.x = mul*pixel.x + win->rect.x;
        pixel.y = mul*pixel.y + win->rect.y;
        if (pixel.x - (mul>>1) <= x && x <= pixel.x + ((mul>>1)-1) &&
            pixel.y - (mul>>1) <= y && y <= pixel.y + ((mul>>1)-1)) {
            //BSPNode *node = &world.bsp->nodes[i_v];
            i_curr_v = i_v;
            extern size_t g_i_acting_root;
            g_i_acting_root = i_v;
        }
    }
}

static void do_PointerRelease_BSPView(Window *win, zgl_PointerReleaseEvent *evt) {

}

static void do_PointerMotion_BSPView(Window *win, zgl_PointerMotionEvent *evt) {
    zgl_Pixel pixel;

    pixel = zgl_GetPointerPos();
    pixel.x -= win->rect.x + (mul>>1);
    pixel.y -= win->rect.y + (mul>>1);
    if (0 <= pixel.x && pixel.x <= win->rect.w-1 &&
        0 <= pixel.y && pixel.y <= win->rect.h-1) {
        
        pixel.x = pixel.x - (pixel.x & (mul-1));
        pixel.y = pixel.y - (pixel.y & (mul-1));

        pixel.x += win->rect.x + (mul>>1);
        pixel.y += win->rect.y + (mul>>1);
        
        if (g_scr_pixels[(pixel.x + (mul>>1)) + (pixel.y + (mul>>1))*g_scr->w]) {
            hovered_grid_point = pixel;
        }
        else {
            hovered_grid_point = (zgl_Pixel){-1, -1};
        }
    }    
}

static void do_PointerEnter_BSPView(Window *win, zgl_PointerEnterEvent *evt) {

}

static void do_PointerLeave_BSPView(Window *win, zgl_PointerLeaveEvent *evt) {
    
}

static void do_KeyPress_BSPView(Window *win, zgl_KeyPressEvent *evt) {
    //    zgl_KeyCode zk = evt->zk;

}

#include "src/draw/viewport/viewport_internal.h"
ViewportInterface interface_BSPView = {
    .i_VPI = VPI_BSPVIEW,
    .mapped_i_VP = VP_NONE,
    .windata = NULL,
    .onUnmap = NULL,
    .onMap   = onMap_BSPView,
    .winfncs = {
        .draw             = draw_BSPView,
        .onPointerPress   = do_PointerPress_BSPView,
        .onPointerRelease = do_PointerRelease_BSPView,
        .onPointerMotion  = do_PointerMotion_BSPView,
        .onPointerEnter   = do_PointerEnter_BSPView,
        .onPointerLeave   = do_PointerLeave_BSPView,
        .onKeyPress       = do_KeyPress_BSPView,
        NULL,
    },
    .name = "BSPView"
};



//////////////////////////////////////////////////////////////////////////

typedef struct State {
    DiagramNode *L;
    DiagramNode *R;
    int max_offset;
    int Loffset;
    int Roffset;
    DiagramNode *Louter;
    DiagramNode *Router;
} State;

static void layout(DiagramNode *root);
static DiagramNode *setup(DiagramNode *node, int depth);
static int fix_subtrees(DiagramNode *L, DiagramNode *R);
static State make_contour(DiagramNode *L, DiagramNode *R,
                          int max_offset, int Loffset, int Roffset,
                          DiagramNode *Louter, DiagramNode *Router);
static inline DiagramNode *next_left(DiagramNode *node);
static inline DiagramNode *next_right(DiagramNode *node);
    

static DiagramNode *addmods(DiagramNode *node, int modsum);

#define DNODE_PTR_TO_IDX(DNODE) ((DNODE) - dnodes)
#define BSPNODE_PTR_TO_IDX(BSPNODE) ((BSPNODE) - g_world.geo_bsp->nodes)

static void construct_diagram(void) {
    num_nodes = g_world.geo_bsp->num_nodes;
    if (num_nodes == 0) return;
    if (vertices) zgl_Free(vertices);
    if (dnodes) zgl_Free(dnodes);
    
    vertices = zgl_Malloc(num_nodes * sizeof(*vertices));
    dnodes = zgl_Malloc(num_nodes * sizeof(*dnodes));

    num_edges = 0;
    for (size_t i_node = 0; i_node < num_nodes; i_node++) {
        DiagramNode *dnode = dnodes + i_node;
        dnode->pixel = vertices + i_node;
        
        BSPNode *bspnode = g_world.geo_bsp->nodes + i_node;
        
        if (bspnode->front) {
            size_t i_L = BSPNODE_PTR_TO_IDX(bspnode->front);
            dnode->L = dnodes + i_L;
            
            ColoredEdge *edge = &edges[num_edges];
            num_edges++;
            edge->e.i_v0 = i_node;
            edge->e.i_v1 = i_L;
            edge->color = 0x00FF00;
        }
        else {
            dnode->L = NULL;
        }

        if (bspnode->back) {
            size_t i_R = BSPNODE_PTR_TO_IDX(bspnode->back);
            dnode->R = dnodes + i_R;

            ColoredEdge *edge = &edges[num_edges];
            num_edges++;
            edge->e.i_v0 = i_node;
            edge->e.i_v1 = i_R;
            edge->color = 0xFF0000;
        }
        else {
            dnode->R = NULL;
        }

        dnode->thread = NULL;
        dnode->mod = 0;
    }

    layout(dnodes + 0);
}

static void layout(DiagramNode *root) {
    setup(dnodes + 0, 1); // second argument is depth, but can be used as global y offset.
    addmods(dnodes + 0, 1); // second argument is "mod", but can be used as global x offset
}

static DiagramNode *setup(DiagramNode *node, int depth) {
    y_of(node) = depth;
    
    if (node->L && node->R) { // both L and R
        DiagramNode *L = setup(node->L, depth+1);
        DiagramNode *R = setup(node->R, depth+1);

        x_of(node) = fix_subtrees(L, R);
    }
    else if (node->L) { // only L
        x_of(node) = x_of(setup(node->L, depth+1));
    }
    else if (node->R) { // only R
        x_of(node) = x_of(setup(node->R, depth+1));
    }
    else { // neither L nor R
        x_of(node) = 0;
    }

    return node;
}

static int fix_subtrees(DiagramNode *L, DiagramNode *R) {
    State s = make_contour(L, R, -1, 0, 0, NULL, NULL);
    int diff = s.max_offset;
    
    diff += 1;
    diff += (x_of(R) + diff + x_of(L)) % 2;
    
    R->mod = diff;
    x_of(R) += diff;

    if (R->L || R->R) {
        s.Roffset += diff;
    }

    if (s.R && ! s.L) {
        s.Louter->thread = s.R;
        s.Louter->mod = s.Roffset - s.Loffset;
    }
    else if (s.L && ! s.R) {
        s.Router->thread = s.L;
        s.Router->mod = s.Loffset - s.Roffset;
    }

    return (x_of(L) + x_of(R)) / 2;
}

static State make_contour(DiagramNode *L, DiagramNode *R, int max_offset,
                        int Loffset, int Roffset,
                        DiagramNode *Louter, DiagramNode *Router) {
    int delta = (x_of(L) + Loffset) - (x_of(R) + Roffset);

    if (max_offset == -1 || delta > max_offset) {
        max_offset = delta;
    }
    
    if ( ! Louter) {
        Louter = L;
    }
    if ( ! Router) {
        Router = R;
    }
    
    DiagramNode *lo = next_left(Louter);
    DiagramNode *li = next_right(L);
    DiagramNode *ri = next_left(R);
    DiagramNode *ro = next_right(Router);

    if (li && ri) {
        Loffset += L->mod;
        Roffset += R->mod;
        return make_contour(li, ri, max_offset, Loffset, Roffset, lo, ro);
    }

    return (State){li, ri, max_offset, Loffset, Roffset, Louter, Router};
}

static inline DiagramNode *next_right(DiagramNode *node) {
    if (node->thread) return node->thread;
    if (node->R) return node->R;
    if (node->L) return node->L;
    return NULL;
}

static inline DiagramNode *next_left(DiagramNode *node) {
    if (node->thread) return node->thread;
    if (node->L) return node->L;
    if (node->R) return node->R;
    return NULL;
}

static DiagramNode *addmods(DiagramNode *node, int mod) {
    x_of(node) += mod;

    if (node->L) {
        addmods(node->L, mod + node->mod);
    }
    if (node->R) {
        addmods(node->R, mod + node->mod);
    }

    return node;
}
