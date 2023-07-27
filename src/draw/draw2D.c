#include "src/misc/epm_includes.h"
#include "src/draw/window/window.h"
#include "src/draw/colors.h"
#include "src/draw/default_layout.h"
#include "src/world/world.h"
#include "src/world/geometry.h"
#include "src/world/selection.h"
#include "src/draw/textures.h"
#include "src/draw/draw2D.h"
#include "src/draw/clip.h"

static void draw2D_wireframe(View2D *v2d, EdgeSet *eset, zgl_Color color, uint8_t flags);

#define WIREFLAGS_DOTTED 0x01
#define WIREFLAGS_NO_VERTICES 0x02

#define VIS_LEFT   0x04
#define VIS_RIGHT  0x08
#define VIS_ABOVE  0x10
#define VIS_BELOW  0x20

#define DEFAULT_ZOOM 50
#define MAX_ZOOM (1LL<<34)
#define MIN_ZOOM (1LL<<16) // TODO: lower this
#define NUM_ZOOMS 94

// A table of world widths, which together with window width determines the zoom level of the view.
static UFix64 const zoom_lut[NUM_ZOOMS] = {0X10000, 0X12492, 0X14E5D, 0X17E21, 0X1B4B8, 0X1F31B, 0X23A68, 0X28BE4, 0X2E904, 0X35372, 0X3CD14, 0X45816, 0X4F6F4, 0X5AC84, 0X67C04, 0X76929, 0X8782E, 0X9ADEB, 0XB0FE8, 0XCA476, 0XE72D0, 0X108336, 0X12DF19, 0X159141, 0X18A601, 0X1C2B6E, 0X2031A2, 0X24CB02, 0X2A0C94, 0X300E60, 0X36EBDB, 0X3EC468, 0X47BBE4, 0X51FB4D, 0X5DB17C, 0X6B13FB, 0X7A5FFA, 0X8BDB66, 0X9FD62B, 0XB6AB9E, 0XD0C422, 0XEE9702, 0X110AC94, 0X137A0A9, 0X1642553, 0X1970615, 0X1D12B85, 0X2139F73, 0X25F91A8, 0X2B65D52, 0X3198F39, 0X38AECD3, 0X40C7C5E, 0X4A08E22, 0X549C702, 0X60B2C94, 0X6E832F2, 0X7E4CC82, 0X9057C02, 0XA4F6926, 0XBC8782B, 0XD7764C3, 0XF63E0DE, 0X1196B7D9, 0X1419F6AE, 0X16F919EB, 0X1A414231, 0X1E017038, 0X224AC964, 0X2730E629, 0X2CCA2B9C, 0X333031D6, 0X3A8038F4, 0X42DBAECD, 0X4C68C7C5, 0X57532D73, 0X63CCC63A, 0X720E9966, 0X8259D3E2, 0X94F8F226, 0XAA4114BD, 0XC293856A, 0XDE5F73E6, 0XFE23F22B, 0X122723955, 0X14BF04185, 0X17B5BB898, 0X1B18D6540, 0X1EF7D0600, 0X23645BDB6, 0X2872B21F4, 0X2E39F023B, 0X34D48028C, 0X3C609277B};


static View2D view2D[3] = {
    [PLN_TOP] = {
        .i_x = I_X,
        .i_y = I_Y,
        .center = {0,0},
        .wiregeo = WIREGEO_BRUSH,
        .i_zoom = DEFAULT_ZOOM
    },
    [PLN_SIDE] = {
        .i_x = I_Y,
        .i_y = I_Z,
        .center = {0,0},
        .wiregeo = WIREGEO_BRUSH,
        .i_zoom = DEFAULT_ZOOM
    },
    [PLN_FRONT] = {
        .i_x = I_X,
        .i_y = I_Z,
        .center = {0,0},
        .wiregeo = WIREGEO_BRUSH,
        .i_zoom = DEFAULT_ZOOM
    },
};


static epm_Result draw_grid_lines(View2D *v2d);


/** Returns the least multiple of @mult which is greater than or equal to @x.
 */
static int64_t round_up_pow2(int64_t x, int64_t log2_mult) {
    int64_t absx = ABS(x);
    int64_t remainder = absx & ((1<<log2_mult) - 1);
    if (remainder == 0) return x;
    if (x < 0) return -(absx - remainder);
    else return x + (1<<log2_mult) - remainder;
}

/* -------------------------------------------------------------------------- */
// Transform between world and screen space. By construction, the "frustum" is
// an axis aligned box in the world. As such, much work shall be done in world
// space.

#define wit_from_mPixit(screen_dist, scr_ref, wit_ref) \
    FIX_MULDIV((screen_dist), (wit_ref), (scr_ref))

#define screendist_from_worlddist(world_dist, scr_ref, wit_ref) \
    (UFix32)FIX_MULDIV((world_dist), (scr_ref), (wit_ref))

static bool in_worldrect_tsf(WorldVec pt, WorldRect rect, View2D *v2d) {
    return ((pt.v[v2d->i_x] >= rect.v[v2d->i_x]) &&
            (pt.v[v2d->i_y] >= rect.v[v2d->i_y]) &&
            (pt.v[v2d->i_x] < rect.v[v2d->i_x]+rect.dv[v2d->i_x]) &&
            (pt.v[v2d->i_y] < rect.v[v2d->i_y]+rect.dv[v2d->i_y]));
}

static int screenpoint_from_worldpoint_64
(Fix64Vec_2D const *world_pt, zgl_mPixel *mapscr_pt, View2D *v2d) {
    mapscr_pt->x = (Fix32)(v2d->screenbox.x +
                    v2d->screenbox.w/2 +
                    FIX_MULDIV(world_pt->x - v2d->center.x,
                                 v2d->screenbox.w,
                                 v2d->worldbox.w));
    
    mapscr_pt->y = (Fix32)(v2d->screenbox.y +
                    v2d->screenbox.h/2 -
                    FIX_MULDIV(world_pt->y - v2d->center.y,
                                 v2d->screenbox.w,
                                 v2d->worldbox.w));
    
    return 0;
}

static int screenpoint_from_worldpoint
(WorldVec const *world_pt, Fix64Vec_2D *mapscr_pt, View2D *v2d) {
    mapscr_pt->x = (v2d->screenbox.x +
                    v2d->screenbox.w/2 +
                    FIX_MULDIV(world_pt->v[v2d->i_x] - v2d->center.x,
                                 v2d->screenbox.w,
                                 v2d->worldbox.w));
    
    mapscr_pt->y = (v2d->screenbox.y +
                    v2d->screenbox.h/2 -
                    FIX_MULDIV(world_pt->v[v2d->i_y] - v2d->center.y,
                                 v2d->screenbox.w,
                                 v2d->worldbox.w));
    
    return 0;
}

void worldpoint_from_screenpoint(Fix32Vec_2D mapscr_pt, WorldVec *world_pt, Plane tsf) {
    View2D *v2d = view2D + tsf;
    world_pt->v[v2d->i_x] = (Fix32)(v2d->center.x + FIX_MULDIV((mapscr_pt.x - v2d->screenbox.x - v2d->screenbox.w/2), v2d->worldbox.w, v2d->screenbox.w));

    world_pt->v[v2d->i_y] = (Fix32)(v2d->center.y - FIX_MULDIV((mapscr_pt.y - v2d->screenbox.y - v2d->screenbox.h/2), v2d->worldbox.w, v2d->screenbox.w));
}

static bool potentially_visible_seg(Fix64Seg_2D seg, zgl_mPixelRect rect) {
    return !(MAX(seg.pt0.x, seg.pt1.x) < rect.x ||
             MIN(seg.pt0.x, seg.pt1.x) > rect.x+rect.w ||
             MAX(seg.pt0.y, seg.pt1.y) < rect.y ||
             MIN(seg.pt0.y, seg.pt1.y) > rect.y+rect.h);
}

// TODO: Do the clipping of segments *before* transforming to screen. Instead,
// for each 3D segment, project it to the appropriate two dimensions by dropping
// a coordinate, and right then do a 2D clip to the world box similarly
// projected. From there, transform the clipped seg to screen. This will avoid
// the precision/overflows issues currently afflicted the algorithm.

#define WIREGEO_BRUSH 0
#define WIREGEO_TRI   1
#define WIREGEO_BSP   2
extern void epm_SetWireGeometry2D(int wiregeo);

static epm_Result draw2D_world(View2D *v2d) {
    if (v2d->wiregeo == WIREGEO_BRUSH) {
        for (BrushNode *node = g_world.geo_brush->head; node; node = node->next) {
            Brush *brush = node->brush;
            draw2D_wireframe(v2d, &EdgeSet_of(*brush), color_brush, 0);
        }
    }
    else if (v2d->wiregeo == WIREGEO_TRI) {
        draw2D_wireframe(v2d, &EdgeSet_of(*g_world.geo_prebsp), color_brush, 0);
    }
    else if (v2d->wiregeo == WIREGEO_BSP) {
        draw2D_wireframe(v2d, &EdgeSet_of(*g_world.geo_bsp), color_brush, 0);
    }
  
    return EPM_SUCCESS;
}


static inline Fix64Vec_2D worldspace_to_eyespace_2D(WorldVec in, View2D *v2d) {
    return (Fix64Vec_2D){.x = (Fix64)in.v[v2d->i_x],
                         .y = (Fix64)in.v[v2d->i_y]};
}

static inline void eyespace_to_screen_2D(Fix64Vec_2D e, zgl_mPixel *s, View2D *v2d) {
    s->x = (zgl_mPixit)(v2d->screenbox.x + v2d->screenbox.w/2 +
                        FIX_MULDIV(e.x - v2d->center.x,
                                   v2d->screenbox.w,
                                   v2d->worldbox.w));

    s->y = (zgl_mPixit)(v2d->screenbox.y + v2d->screenbox.h/2 -
                        FIX_MULDIV(e.y - v2d->center.y,
                                   v2d->screenbox.w,
                                   v2d->worldbox.w));
}


static inline uint8_t compute_viscode(Fix64Vec_2D v, View2D *v2d) {
    uint8_t viscode = 0;
    if (v.x < v2d->worldbox.x)
        viscode |= VIS_RIGHT;
    if (v.x > v2d->worldbox.x + v2d->worldbox.w-1)
        viscode |= VIS_LEFT;
    if (v.y < v2d->worldbox.y)
        viscode |= VIS_BELOW;
    if (v.y > v2d->worldbox.y + v2d->worldbox.h-1)
        viscode |= VIS_ABOVE;

    return viscode;
}

static void epm_TransformVertex_2D
(View2D *v2d, WorldVec v, Fix64Vec_2D *eye, uint8_t *vis, zgl_mPixel *s)
{
    *eye = worldspace_to_eyespace_2D(v, v2d);
    *vis = compute_viscode(*eye, v2d);
    eyespace_to_screen_2D(*eye, s, v2d);
}

static void draw2D_wireframe(View2D *v2d, EdgeSet *eset, zgl_Color color, uint8_t flags) {
    int (*fn_drawseg)(zgl_PixelArray *, const zgl_mPixelRect *,
                      const zgl_mPixel, const zgl_mPixel, const zgl_Color);
    fn_drawseg =
        flags & WIREFLAGS_DOTTED ?
        zglDraw_mPixelSeg_Dotted :
        zglDraw_mPixelSeg;
    
    //size_t num_vertices = eset->num_vertices;
    WorldVec *vertices = eset->verts;
    size_t num_edges = eset->num_edges;
    Edge *edges = eset->edges;
   
    for (uint32_t i = 0; i < num_edges; i++) {
        WorldVec p0 = vertices[edges[i].i_v0];
        WorldVec p1 = vertices[edges[i].i_v1];
        Fix64Vec_2D eye0 = worldspace_to_eyespace_2D(p0, v2d);
        Fix64Vec_2D eye1 = worldspace_to_eyespace_2D(p1, v2d);
        uint8_t vis0 = compute_viscode(eye0, v2d);
        uint8_t vis1 = compute_viscode(eye1, v2d);
        zgl_mPixel s0, s1;
        
        if (vis0 & vis1) continue;

        if ( ! (vis0 | vis1)) { // no clipping
            eyespace_to_screen_2D(eye0, &s0, v2d);
            eyespace_to_screen_2D(eye1, &s1, v2d);

            if ( ! (flags & WIREFLAGS_NO_VERTICES)) {
                zglDraw_mPixelDot(g_scr, &v2d->screenbox, &s0, color);
                zglDraw_mPixelDot(g_scr, &v2d->screenbox, &s1, color);
            }
        }
        else { // clipping
            Fix64Vec_2D c0, c1;
            bool accept = SegClip2D(eye0.x, eye0.y, eye1.x, eye1.y,
                                    v2d->worldbox, &c0, &c1);

            if ( ! accept) { // reject
                continue; // this might never occur. TODO
            }

            eyespace_to_screen_2D(c0, &s0, v2d);
            eyespace_to_screen_2D(c1, &s1, v2d);
        }

        fn_drawseg(g_scr, &v2d->screenbox, s0, s1, color);
        
    }
}

static void BlitWorldPointIcon(Window *win, zgl_MipMap *icon, View2D *v2d, WorldVec v) {
    
    Fix64Vec_2D eye = worldspace_to_eyespace_2D(v, v2d);
    //uint8_t vis = compute_viscode(eye, v2d);
    Fix32Vec_2D pixel;
    eyespace_to_screen_2D(eye, &pixel, v2d);
    // .16

    int32_t iconW = 32; // .0
    Fix32 dst_W = screendist_from_worlddist(iconW<<16,
                                              v2d->screenbox.w,
                                              v2d->worldbox.w);
    // .16

    if (dst_W < 1<<16) {
        return;
    }

    zgl_mPixelRect dst_rect = {
        .x = pixel.x - dst_W/2,
        .y = pixel.y - dst_W/2,
        .w = dst_W,
        .h = dst_W,
    };

    zgl_BlitEntireMipMap(g_scr, dst_rect, icon, &win->mrect);
}

void draw_View2D(Window *win) {
    // once-per-frame function
    
    Plane tsf = *(Plane *)(win->data);
    View2D *v2d = &view2D[tsf];

    /* Generate new zoom table.
    static bool generated = true;
    if (!generated) {
        int i = 0;
        for (UFix64 zoom = MIN_ZOOM; zoom <= MAX_ZOOM;) {
            zooms[i++] = zoom;
            num_zooms++;
            printf("%#lX, ", zoom);
            zoom = (zoom<<3)/7;
        }
        printf("\n%i zoom levels\n", num_zooms);
        generated = true;
    }
    */

    v2d->screenbox = win->mrect;
    set_zoom_level(v2d);
    v2d->worldbox.x = (v2d->center.x - v2d->worldbox.w/2);
    v2d->worldbox.y = (v2d->center.y - v2d->worldbox.h/2);

    // Grid background.
    zgl_GrayRect(g_scr,
                 intify(v2d->screenbox.x),
                 intify(v2d->screenbox.y),
                 intify(v2d->screenbox.w),
                 intify(v2d->screenbox.h),
                 0x2F);
    draw_grid_lines(v2d);

    // Draw the Big Box
    draw2D_wireframe(v2d, &view3D_bigbox, 0x2596BE, WIREFLAGS_NO_VERTICES);

    // Draw World Geometry (in either brush, triangulated, or BSP form)
    draw2D_world(v2d);

    // Draw framebrush
    draw2D_wireframe(v2d, &EdgeSet_of(*g_frame), color_brushframe, WIREFLAGS_DOTTED);
        
    // Draw brush selection.
    for (SelectionNode *node = sel_brush.head; node; node = node->next) {
        Brush *brush = (Brush *)node->obj;

        draw2D_wireframe(v2d, &EdgeSet_of(*brush), color_selected_brush, 0);
    }

    // Draw brush selection point-of-reference
    Fix64Seg_2D trans_MPS;
    zgl_mPixelSeg clipped_MPS;

    screenpoint_from_worldpoint(&brushsel_POR, &trans_MPS.pt0, v2d);
    clipped_MPS.pt0.x = (Fix32)trans_MPS.pt0.x;
    clipped_MPS.pt0.y = (Fix32)trans_MPS.pt0.y;
    zglDraw_mPixelDot(g_scr, &v2d->screenbox, &clipped_MPS.pt0, 0xFABCDE);


    ////////////////////////////////////////////////////////////////////////   

    // draw icons for light sources
    for (size_t i_l = 0; i_l < g_world.num_lights; i_l++) {
        Fix32Vec v = g_world.lights[i_l].pos;

        BlitWorldPointIcon(win, MIP_light_icon, v2d, v);
    }

    /////////////////////////////////////////////////////////////////


    WorldVec v0 = {{tf.x, tf.y, tf.z}};
    BlitWorldPointIcon(win, MIP_edcam_icon, v2d, v0);

    // now draw directional arrow
    WorldVec v1 = {{
            tf.x + 32*x_of(tf.dir),
            tf.y + 32*y_of(tf.dir),
            tf.z + 32*z_of(tf.dir)}};

    Fix64Vec_2D eye0, eye1;
    zgl_mPixel s0, s1;
    uint8_t vis0, vis1;
    
    epm_TransformVertex_2D(v2d, v0, &eye0, &vis0, &s0);
    epm_TransformVertex_2D(v2d, v1, &eye1, &vis1, &s1);

    zglDraw_mPixelSeg(g_scr, &v2d->screenbox, s0, s1, 0xFF0000);

    zgl_mPixel s_tmp = s0;
    s0 = s1;
    s1 = s_tmp;

    zgl_mPixel dir;
    dir.x = s1.x - s0.x;
    dir.y = s1.y - s0.y;

    // TODO: rotate by 45 degrees instead of 90, ie. (x \pm y)/\sqrt(2)
    Fix32 tmp;
    tmp = dir.x;
    dir.x = -(dir.y);
    dir.y = +(tmp);

    dir.x >>= 1;
    dir.y >>= 1;
    
    s1.x = dir.x + s0.x;
    s1.y = dir.y + s0.y;

    zglDraw_mPixelSeg(g_scr, &v2d->screenbox, s0, s1, 0xFF0000);
}





/* -------------------------------------------------------------------------- */
// Grid lines

static epm_Result draw_grid_lines(View2D *v2d) {
    /* TODO: A lot of this preparatory computation produces a new result on
       scroll and zoom. Take it out of this function. */
    zgl_mPixelRect screen_box = v2d->screenbox;
    UFix32 gridres_screen;
    
    gridres_screen = screendist_from_worlddist(v2d->gridres_world,
                                               screen_box.w,
                                               v2d->worldbox.w);
    dibassert(gridres_screen > 0);
    
    Fix64Vec_2D base_world;
    zgl_mPixel base_screen;
    base_world.x = round_up_pow2(v2d->worldbox.x, v2d->gridres_world_shift);
    base_world.y = round_up_pow2(v2d->worldbox.y, v2d->gridres_world_shift);
    screenpoint_from_worldpoint_64(&base_world, &base_screen, v2d);
    
    zgl_mPixel offset_s;
    offset_s.x = screendist_from_worlddist(base_world.x - v2d->worldbox.x,
                                           screen_box.w, v2d->worldbox.w);
    offset_s.y = screendist_from_worlddist(base_world.y - v2d->worldbox.y,
                                           screen_box.w, v2d->worldbox.w);
    dibassert(offset_s.x >= 0 && offset_s.y >= 0);

    zgl_mPixelSeg seg; // the current seg to be drawn
    
    // Draw major horizontal grid lines.
    seg.pt0.x = screen_box.x;
    seg.pt0.y = screen_box.y + screen_box.h - offset_s.y;
    seg.pt1.x = screen_box.x + screen_box.w;
    seg.pt1.y = seg.pt0.y;
    zglDraw_mPixelSeg2(g_scr, &screen_box, &seg, world2D_colors.grid_primary_wire);
    for (uint32_t i = 1; i <= v2d->num_hori_lines; i++) {
        seg.pt0.y -= gridres_screen; // start at bottommost line, work up
        seg.pt1.y = seg.pt0.y;
        zglDraw_mPixelSeg2(g_scr,  &screen_box, &seg,
                           world2D_colors.grid_primary_wire);
    }

    // Draw major vertical grid lines.
    seg.pt0.x = screen_box.x + offset_s.x;
    seg.pt0.y = screen_box.y;
    seg.pt1.x = seg.pt0.x;
    seg.pt1.y = screen_box.y + screen_box.h;   
    zglDraw_mPixelSeg2(g_scr, &screen_box, &seg, world2D_colors.grid_primary_wire);
    for (uint32_t i = 1; i <= v2d->num_vert_lines; i++) {
        seg.pt0.x += gridres_screen;
        seg.pt1.x = seg.pt0.x;
        zglDraw_mPixelSeg2(g_scr, &screen_box, &seg,
                           world2D_colors.grid_primary_wire);
    }

    // TODO: minor grid lines

    // Draw origin (axis) lines, which are thicker than others
    Fix64Vec_2D origin_w = {0,0};
    zgl_mPixel origin_s;
    screenpoint_from_worldpoint_64(&origin_w, &origin_s, v2d);
    
    zglDraw_mPixelSeg
        (g_scr, &screen_box,
         (zgl_mPixel){origin_s.x-(1<<16), screen_box.y},
         (zgl_mPixel){origin_s.x-(1<<16), screen_box.y + screen_box.h},
         world2D_colors.grid_axis_edge_wire);
    
    zglDraw_mPixelSeg
        (g_scr, &screen_box,
         (zgl_mPixel){origin_s.x, screen_box.y},
         (zgl_mPixel){origin_s.x, screen_box.y + screen_box.h},
         world2D_colors.grid_axis_center_wire);
    
    zglDraw_mPixelSeg
        (g_scr, &screen_box,
         (zgl_mPixel){origin_s.x+(1<<16), screen_box.y},
         (zgl_mPixel){origin_s.x+(1<<16), screen_box.y + screen_box.h},
         world2D_colors.grid_axis_edge_wire);

    zglDraw_mPixelSeg
        (g_scr, &screen_box,
         (zgl_mPixel){screen_box.x, origin_s.y-(1<<16)},
         (zgl_mPixel){screen_box.x + screen_box.w, origin_s.y-(1<<16)},
         world2D_colors.grid_axis_edge_wire);
    
    zglDraw_mPixelSeg
        (g_scr, &screen_box,
         (zgl_mPixel){screen_box.x, origin_s.y},
         (zgl_mPixel){screen_box.x + screen_box.w, origin_s.y},
         world2D_colors.grid_axis_center_wire);
    
    zglDraw_mPixelSeg
        (g_scr, &screen_box,
         (zgl_mPixel){screen_box.x, origin_s.y+(1<<16)},
         (zgl_mPixel){screen_box.x + screen_box.w, origin_s.y+(1<<16)},
         world2D_colors.grid_axis_edge_wire);

    //temporary, draw grid base point
    zglDraw_mPixelDot(g_scr, &screen_box,
                      &(zgl_mPixel){screen_box.x + offset_s.x,
                                    screen_box.y + screen_box.h - offset_s.y},
                      0x0FF0F0);

    return EPM_SUCCESS;
}

/* -------------------------------------------------------------------------- */
// Changing the state of the 2D view. (zoom level, center point)

void scroll(Window *win, int dx, int dy, Plane tsf) {
    View2D *v2d = &view2D[tsf];
    
    Fix64 new_x = v2d->center.x + wit_from_mPixit(-LSHIFT32(dx, 16),
                                                    fixify(win->rect.w),
                                                    zoom_lut[v2d->i_zoom]);
        
    Fix64 new_y = v2d->center.y - wit_from_mPixit(-LSHIFT32(dy, 16),
                                                    fixify(win->rect.h),
                                                    FIX_MULDIV(zoom_lut[v2d->i_zoom],
                                                               win->rect.h,
                                                               win->rect.w));

    // clamp center to valid world coordinates
    v2d->center.x = MAX(MIN(new_x, FIX32_MAX), FIX32_MIN);
    v2d->center.y = MAX(MIN(new_y, FIX32_MAX), FIX32_MIN);
}

void compute_scroll(Window *win, int dx, int dy, Plane tsf, Fix32 *out_wdx, Fix32 *out_wdy) {
    View2D *v2d = &view2D[tsf];
    
    *out_wdx = (WorldUnit)wit_from_mPixit(-LSHIFT32(dx, 16),
                                          fixify(win->rect.w),
                                          zoom_lut[v2d->i_zoom]);
        
    *out_wdy = (WorldUnit)wit_from_mPixit(-LSHIFT32(dy, 16),
                                          fixify(win->rect.h),
                                          FIX_MULDIV(zoom_lut[v2d->i_zoom],
                                                     win->rect.h,
                                                     win->rect.w));
}

void zoom_level_up(Plane tsf) {
    view2D[tsf].i_zoom = MIN(NUM_ZOOMS-1, view2D[tsf].i_zoom + 1);
    set_zoom_level(view2D + tsf);
}

void zoom_level_down(Plane tsf) {
    view2D[tsf].i_zoom = view2D[tsf].i_zoom == 0 ? 0 : view2D[tsf].i_zoom - 1;
    set_zoom_level(view2D + tsf);
}

void set_zoom_level(View2D *v2d) {
    uint8_t gridres_world_shift;

    // TODO: find min&max x&y

    v2d->worldbox.w = zoom_lut[v2d->i_zoom];
    v2d->worldbox.h = FIX_MULDIV(v2d->worldbox.w,
                                 v2d->screenbox.h,
                                 v2d->screenbox.w);
    
    for (gridres_world_shift = 1; ; gridres_world_shift++) {
        v2d->num_vert_lines = (uint16_t)(v2d->worldbox.w >> gridres_world_shift);
        v2d->num_hori_lines = (uint16_t)(v2d->worldbox.h >> gridres_world_shift);

        if (v2d->num_vert_lines <= 16) {
            break;
        }
            
    }
    
    v2d->gridres_world_shift = gridres_world_shift;
    v2d->gridres_world = 1<<gridres_world_shift;
}



