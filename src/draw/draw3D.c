#include "src/draw/draw.h"
#include "src/draw/draw_triangle.h"
#include "src/draw/draw3D.h"
#include "src/draw/text.h"
#include "src/draw/textures.h"
#include "src/draw/colors.h"
#include "src/draw/clip.h"
#include "src/draw/draw3D_BSP.h"

#include "src/world/world.h"
#include "src/world/selection.h"

#include "zigil/zigil_mip.h"

#undef LOG_LABEL
#define LOG_LABEL "DRAW3D"

#define WIREFLAGS_DOTTED 0x01
#define WIREFLAGS_NO_VERTICES 0x02

// TODO: In orthographic project, don't draw sky.

static int8_t const zigzag_i_v[3][11] = {
    {0, -1, -1, -2, -2, -3, -3, -4, -4, -5, -5},
    {1,  0,  2, -1,  3, -2,  4, -3,  5, -4,  6},
    {2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7}
};

zgl_Color Normal_to_Color(WorldVec normal);

FrustumParameters frset_persp = {
    .type = PROJ_PERSPECTIVE,
    .persp_half_fov = DEFAULT_HALF_FOV,
    .D = fixify(1),
    .F = fixify(1),
    .B = fixify(2048),
};

FrustumParameters frset_ortho = {
    .type = PROJ_ORTHOGRAPHIC,
    .ortho_W = fixify(512),
    .D = fixify(0),
    .F = -fixify(1024),
    .B = fixify(1024),
};

static Frustum g_frustum_persp;
static Frustum g_frustum_ortho;
static Frustum *g_p_frustum = &g_frustum_persp;

bool g_farclip = true;

int zglDraw_mPixelSeg_NoOverdraw
(zgl_PixelArray *screen,
 const zgl_mPixelRect *bounds,
 const zgl_mPixel pixel0,
 const zgl_mPixel pixel1,
 const zgl_Color color);

#define area(a, b, c) ((Fix64)((c).x - (a).x) * (Fix64)((b).y - (a).y) - (Fix64)((c).y - (a).y) * (Fix64)((b).x - (a).x))

DrawSettings g_settings = {
    .proj_mode = PROJ_PERSPECTIVE,
    .light_mode = LIGHT_VBRI,
    .wire_mode = WIRE_OFF,
    .wiregeo = WIREGEO_BSP_FACEWISE,
    .BSP_visualizer = false,
};

#define VERTEX_BUFFER_SIZE 10000
static TransformedVertex g_vbuf[VERTEX_BUFFER_SIZE] = {0};

static epm_Result epm_ConstructFrustum(Frustum *fr, FrustumParameters *settings);
extern void epm_UpdateFrustumHeight(Window *win);

extern void epm_TransformVertex
(Window const *win, Frustum *fr,
 WorldVec v,
 TransformedVertex *out_tv);

extern void epm_TransformVertexBatch
(Window const * win, Frustum *fr,
 size_t num_vertices, WorldVec *vertices,
 TransformedVertex *out_vbuf);

static void transform_Model
(Window const *const win, Frustum *const fr, int width, Model *model,
 TransformedVertex * out_vbuf);

extern void clip_trans_triangulate_and_draw_face(Window *win, Face *face, TransformedVertex const *const vbuf);

extern void clip_and_transform(Window const *const win, Fix64Vec V0, Fix64Vec V1, Fix64Vec V2, size_t *out_num_subvs, Fix64Vec *out_subvs, TransformedVertex *out_subvbuf, bool *out_new);

static void draw_bigbox(Window *win);

static void draw_faces
(Window *win, FaceSet *fset, TransformedVertex const *const vbuf);

extern void draw_face(Window *win, Face *face, TransformedFace *tf);

extern void draw_wireframe_face
(Window *win, TransformedWireFace *twf, zgl_Color color);

static void draw_wireframe_faces
(Window *win, FaceSet *fset, zgl_Color color, TransformedVertex const *const vbuf);

static void draw_wireframe_edges
(Window *win, Frustum *fr, EdgeSet *eset, zgl_Color color, TransformedVertex const *const vbuf, uint8_t flags);

static void draw_wireframe_edges_BSP_colors(Window *win, Frustum *fr, EdgeSet *eset, zgl_Color color, TransformedVertex const *const vbuf, uint8_t wireflags);

static void draw_vertices(Window *win, size_t num_vertices, TransformedVertex const *const vbuf);

static uint8_t (*fn_ComputeViscode)(Fix64Vec vertex);
static uint8_t ComputeViscode_ortho(Fix64Vec vertex);
static uint8_t ComputeViscode_persp(Fix64Vec vertex);

static void (*fn_EyespaceToScreen)(Window const *win, TransformedVertex *tv);
static void epm_EyespaceToScreen_ortho(Window const *win, TransformedVertex *tv);
static void epm_EyespaceToScreen_persp(Window const *win, TransformedVertex *tv);

static void epm_BatchEyespaceToScreen(Window const *win, size_t num_tv, TransformedVertex *tv);

static bool (*fn_SegClip3D)
(Fix64 x0, Fix64 y0, Fix64 z0,
 Fix64 x1, Fix64 y1, Fix64 z1,
 Frustum *fr,
 Fix64Vec *out_c0, Fix64Vec *out_c1);

static void (*fn_TriClip3D)
(Fix64Vec const *const v0,
 Fix64Vec const *const v1,
 Fix64Vec const *const v2,
 Frustum *fr,
 Fix64Vec *out_poly, size_t *out_count, bool *out_new);


/* -------------------------------------------------------------------------- */
epm_Result init_Draw3D(void) {
    epm_ConstructFrustum(&g_frustum_persp, &frset_persp);
    epm_ConstructFrustum(&g_frustum_ortho, &frset_ortho);
    epm_SetProjectionMode(PROJ_PERSPECTIVE);
    
    return EPM_SUCCESS;
}

static epm_Result epm_ConstructFrustum(Frustum *fr, FrustumParameters *settings) {
    fr->type = settings->type;

    if (fr->type == PROJ_PERSPECTIVE) {
        fr->D = settings->D;
        if (fr->D == fixify(1))
            fr->W = fixify(1); // might as well be exact in this nice case
        else
            fr->W = (Fix32)FIX_MUL(fr->D, tan18(settings->persp_half_fov));
        fr->B = settings->B;
        fr->F = settings->F;

        fr->xmin = -fr->W;
        fr->xmax = +fr->W - 1;
        fr->zmin = fr->F;
        fr->zmax = fr->B;
    }
    else if (fr->type == PROJ_ORTHOGRAPHIC) {
        fr->D = settings->D;
        fr->W = settings->ortho_W;
        fr->B = settings->B;
        fr->F = settings->F;
        
        fr->xmin = -fr->W;
        fr->xmax = +fr->W - 1;
        fr->zmin = fr->F;
        fr->zmax = fr->B;
    }
    
    return EPM_SUCCESS;
}

void epm_UpdateFrustumHeight(Window *win) {
    g_frustum_persp.H = (Fix32)FIX_MULDIV(g_frustum_persp.W, win->mrect.h, win->mrect.w);
    g_frustum_persp.ymin = -g_frustum_persp.H;
    g_frustum_persp.ymax = +g_frustum_persp.H - 1;

    
    g_frustum_ortho.H = (Fix32)FIX_MULDIV(g_frustum_ortho.W, win->mrect.h, win->mrect.w);
    g_frustum_ortho.ymin = -g_frustum_ortho.H;
    g_frustum_ortho.ymax = +g_frustum_ortho.H - 1;
}


static void draw3D_wireframe_mode(Window *win) {
    if (g_settings.wiregeo == WIREGEO_BRUSH) {
        for (BrushNode *node = g_world.geo_brush->head; node; node = node->next) {
            Brush *brush = node->brush;
            epm_TransformVertexBatch(win, g_p_frustum, brush->num_vertices, brush->vertices, g_vbuf);
            draw_wireframe_edges(win, g_p_frustum, &EdgeSet_of(*brush), color_selected_brush, g_vbuf, 0);
        }
    }
    else if (g_settings.wiregeo == WIREGEO_PREBSP) {
        epm_TransformVertexBatch(win, g_p_frustum,
                                 g_world.geo_prebsp->num_vertices, g_world.geo_prebsp->vertices, g_vbuf);
        draw_wireframe_edges(win, g_p_frustum,
                             &EdgeSet_of(*g_world.geo_prebsp), 0x00FF00, g_vbuf, 0);
        
    }
    else if (g_settings.wiregeo == WIREGEO_BSP_FACEWISE) {
        epm_TransformVertexBatch(win, g_p_frustum,
                                 g_world.geo_bsp->num_vertices, g_world.geo_bsp->vertices, g_vbuf);

        WorldVec BSPpos = {{tf.x, tf.y, tf.z}};
        if (g_settings.proj_mode == PROJ_ORTHOGRAPHIC) {
            x_of(BSPpos) -= (Fix32)FIX_MUL((g_p_frustum->D-g_p_frustum->F), x_of(cam.view_vec));
            y_of(BSPpos) -= (Fix32)FIX_MUL((g_p_frustum->D-g_p_frustum->F), y_of(cam.view_vec));
            z_of(BSPpos) -= (Fix32)FIX_MUL((g_p_frustum->D-g_p_frustum->F), z_of(cam.view_vec));
        }
    
        draw_BSPNode_wireframe(win, g_world.geo_bsp, &g_world.geo_bsp->nodes[0], BSPpos, g_vbuf);
    }
    else if (g_settings.wiregeo == WIREGEO_BSP_EDGEWISE) {
        epm_TransformVertexBatch(win, g_p_frustum,
                                 g_world.geo_bsp->num_vertices, g_world.geo_bsp->vertices, g_vbuf);
        draw_wireframe_edges_BSP_colors(win, g_p_frustum,
                             &EdgeSet_of(*g_world.geo_bsp), 0x00FF00, g_vbuf, 0);
    }
}

static void draw_icons(Window *win) {
    for (size_t i_l = 0; i_l < g_world.num_lights; i_l++) {
        WorldVec v = g_world.lights[i_l].pos;

        TransformedVertex tv;
        epm_TransformVertex(win, g_p_frustum, v, &tv);

        Fix32Vec_2D pixel = {(Fix32)tv.pixel.XY.x,
                             (Fix32)tv.pixel.XY.y};
        // .16
        
        Fix64 iconZ = tv.pixel.z; // .16

        if (iconZ < 32<<16) continue;
        
        int32_t iconW = 32; // .0
        zgl_PixelRect rect = win->rect; // .0
        
        // By default icons are drawn with "one-texel to one-world-unit"
        Fix32 dst_W =
            (Fix32)FIX_DIV((Fix64)(rect.w * iconW) * (Fix64)g_p_frustum->D,
                             2*FIX_MUL(iconZ, g_p_frustum->W));
        // .16

        if (dst_W < 1<<16) continue;

        zgl_mPixelRect dst_rect = {
            .x = pixel.x - dst_W/2,
            .y = pixel.y - dst_W/2,
            .w = dst_W,
            .h = dst_W,
        };

        zgl_BlitEntireMipMap(g_scr, dst_rect, MIP_light_icon, &win->mrect);
    }
}

static void epm_DrawBillboardIcon(Window *win, zgl_MipMap *icon, WorldVec v) {
    TransformedVertex tv;
    epm_TransformVertex(win, g_p_frustum, v, &tv);

    Fix32Vec_2D pixel = {(Fix32)tv.pixel.XY.x,
                         (Fix32)tv.pixel.XY.y};
    // .16
        
    Fix64 iconZ = tv.pixel.z; // .16

    if (iconZ < 32<<16) return;
        
    int32_t iconW = 32; // .0
    zgl_PixelRect rect = win->rect; // .0
        
    // By default icons are drawn with "two-texel to one-world-unit"
    Fix32 dst_W =
        (Fix32)FIX_DIV((Fix64)(rect.w * iconW) * (Fix64)g_p_frustum->D,
                       2*FIX_MUL(iconZ, g_p_frustum->W));
    // .16

    if (dst_W < 1<<16) return;

    zgl_mPixelRect dst_rect = {
        .x = pixel.x - dst_W/2,
        .y = pixel.y - dst_W/2,
        .w = dst_W,
        .h = dst_W,
    };

    zgl_BlitEntireMipMap(g_scr, dst_rect, icon, &win->mrect);
}

static void draw_BrushSelection(Window *win) {
    // Draw brush selection; always wireframe.
    for (BrushSelectionNode *node = brushsel.head; node; node = node->next) {
        Brush *brush = node->brush;
        epm_TransformVertexBatch(win, g_p_frustum, brush->num_vertices, brush->vertices, g_vbuf);
        draw_wireframe_edges(win, g_p_frustum, &EdgeSet_of(*brush), color_selected_brush, g_vbuf, 0);
    }
    
    // Draw brush selection point-of-reference: TODO: Make a cross shape or
    // larger dot.
    epm_TransformVertex(win, g_p_frustum, brushsel.POR, g_vbuf);
    zgl_mPixel tmp4 = {(Fix32)g_vbuf[0].pixel.XY.x,
                       (Fix32)g_vbuf[0].pixel.XY.y};
    if ( ! g_vbuf[0].vis) {
        zglDraw_mPixelDot(g_scr, &win->mrect, &tmp4, 0xFF0000);
    }
}

static void draw_BrushFrame(Window *win) {
    epm_TransformVertexBatch(win, g_p_frustum, frame->num_vertices, frame->vertices, g_vbuf);
    draw_wireframe_edges(win, g_p_frustum, &EdgeSet_of(*frame), color_brushframe, g_vbuf, WIREFLAGS_DOTTED);
}

static void draw_picked_points(Window *win) {
    epm_TransformVertexBatch(win, g_p_frustum, MAX_SECTS, sect_ring, g_vbuf);
    for (size_t i_sect = 0; i_sect < MAX_SECTS; i_sect++) {
        zgl_mPixel tmp = {(Fix32)g_vbuf[i_sect].pixel.XY.x,
                          (Fix32)g_vbuf[i_sect].pixel.XY.y};
        zglDraw_mPixelDot(g_scr, &win->mrect, &tmp, 0xFFFFFF);
    }    
}

static void draw_t_junctions(Window *win) {
    #define MAX_T_JUNCTIONS 10000
    extern size_t num_tjs;
    extern WorldVec tjs[MAX_T_JUNCTIONS];
    epm_TransformVertexBatch(win, g_p_frustum, num_tjs, tjs, g_vbuf);
    for (size_t i_tj = 0; i_tj < num_tjs; i_tj++) {
        if (g_vbuf[i_tj].vis) {
            continue;
        }
        //        printf("(i_tj %zu)\n", i_tj);
        zgl_mPixel tmp = {(Fix32)g_vbuf[i_tj].pixel.XY.x,
                          (Fix32)g_vbuf[i_tj].pixel.XY.y};
        zglDraw_mPixelDot(g_scr, &win->mrect, &tmp, 0x00FFFF);
    }
}

static void draw_Mesh(Window *win, Mesh *mesh) {
    epm_TransformVertexBatch(win, g_p_frustum, mesh->num_vertices, mesh->vertices, g_vbuf);
    //draw_wireframe_edges(win, g_p_frustum, &EdgeSet_of(*mesh), 0xF0f998, g_vbuf, WIREFLAGS_NO_VERTICES);
    draw_faces(win, &FaceSet_of(*mesh), g_vbuf);
}

static void draw_sky(Window *win) {
    WorldVec skycam = {{fixify(1024), -fixify(1024), 0}};
    WorldVec tf_prev = {{tf.x, tf.y, tf.z}};
    tf.x = x_of(skycam);
    tf.y = y_of(skycam);
    tf.z = z_of(skycam);
    
    epm_TransformVertexBatch(win, g_p_frustum, skybox.num_vertices, skybox.vertices, g_vbuf);

    
    //draw_wireframe_edges(win, g_p_frustum, &EdgeSet_of(skybox), 0xF0f998, g_vbuf, WIREFLAGS_NO_VERTICES);
    g_rasterflags |= RASTERFLAG_SKY;
    draw_faces(win, &FaceSet_of(skybox), g_vbuf);
    g_rasterflags &= ~RASTERFLAG_SKY;

    tf.x = x_of(tf_prev);
    tf.y = y_of(tf_prev);
    tf.z = z_of(tf_prev);
}

void draw3D(Window *win) {
    reset_rasterizer(win);

    epm_UpdateFrustumHeight(win);
   
    /* Always draw bigbox, and always draw it first. */
    bool tmp = g_farclip;
    g_farclip = false;
    draw_bigbox(win);
    g_farclip = tmp;

    if (g_world.loaded) {
        if (g_settings.wire_mode == WIRE_NO_POLY) {
            draw3D_wireframe_mode(win);
        }
        else {
            epm_TransformVertexBatch(win, g_p_frustum,
                                     g_world.geo_bsp->num_vertices, g_world.geo_bsp->vertices, g_vbuf);
            WorldVec BSPpos = {{tf.x, tf.y, tf.z}};
            if (g_settings.proj_mode == PROJ_ORTHOGRAPHIC) {
                x_of(BSPpos) -= (Fix32)FIX_MUL((g_p_frustum->D-g_p_frustum->F), x_of(cam.view_vec));
                y_of(BSPpos) -= (Fix32)FIX_MUL((g_p_frustum->D-g_p_frustum->F), y_of(cam.view_vec));
                z_of(BSPpos) -= (Fix32)FIX_MUL((g_p_frustum->D-g_p_frustum->F), z_of(cam.view_vec));
            }
            draw_BSPTree(g_world.geo_bsp, win, BSPpos, g_vbuf);
        }
    }

    draw_BrushSelection(win);

    draw_BrushFrame(win);
    
    draw_picked_points(win);

    draw_t_junctions(win);
    
    draw_icons(win);

    draw_sky(win);
    
    //draw_Mesh(win, &skybox);
}


static void draw_bigbox(Window *win) {
    // an underlay of the world limits.
    epm_TransformVertexBatch(win, g_p_frustum,
                       view3D_bigbox.num_vertices, view3D_bigbox.vertices, g_vbuf);
    draw_wireframe_edges(win, g_p_frustum, &view3D_bigbox, 0x2596BE, g_vbuf, WIREFLAGS_NO_VERTICES);

    epm_TransformVertexBatch(win, g_p_frustum,
                       view3D_grid.num_vertices, view3D_grid.vertices, g_vbuf);
    draw_wireframe_edges(win, g_p_frustum, &view3D_grid, 0x235367, g_vbuf, WIREFLAGS_NO_VERTICES);
}


static void epm_EyespaceToScreen_ortho(Window const *win, TransformedVertex *tv) {
    Fix64Vec eye = tv->eye;    
    tv->vis = ComputeViscode_ortho(eye);

    DepthPixel pix;
    pix.z = z_of(eye);
    pix.XY.x = win->mrect.x + (FIX_MUL((1L<<16) - FIX_DIV(x_of(eye), g_frustum_ortho.W), win->mrect.w)>>1);
    pix.XY.y = win->mrect.y + (FIX_MUL((1L<<16) - FIX_DIV(y_of(eye), g_frustum_ortho.H), win->mrect.h)>>1);
    tv->pixel = pix;
}

static void epm_EyespaceToScreen_persp(Window const *win, TransformedVertex *tv) {
    Fix64Vec eye = tv->eye;
    tv->vis = ComputeViscode_persp(eye);

    DepthPixel pix;
    pix.z = z_of(eye);
    if ( ! (tv->vis & VIS_NEAR)) { // TODO: Clip to front plane.
        Fix64 x = FIX_MULDIV(g_frustum_persp.D, x_of(eye), z_of(eye));
        Fix64 y = FIX_MULDIV(g_frustum_persp.D, y_of(eye), z_of(eye));
        pix.XY.x = win->mrect.x + (FIX_MUL((1L<<16)-x, win->mrect.w)>>1);
        pix.XY.y = win->mrect.y + (FIX_MUL((1L<<16)-y, win->mrect.h)>>1);
    }
    tv->pixel = pix;

    /*
    if (z_of(eye) != 0) {
        tv->zinv = (1LL<<48)/z_of(eye);
    }
    */
}

static void epm_BatchEyespaceToScreen(Window const *win, size_t num_tv, TransformedVertex *tvs) {
    for (size_t i_tv = 0; i_tv < num_tv; i_tv++) {
        fn_EyespaceToScreen(win, tvs + i_tv);
    }
}

static uint8_t ComputeViscode_ortho(Fix64Vec vertex) {
    uint8_t viscode = 0;   
    if (x_of(vertex) < g_frustum_ortho.xmin) viscode |= VIS_RIGHT;
    if (x_of(vertex) > g_frustum_ortho.xmax) viscode |= VIS_LEFT;
    if (y_of(vertex) < g_frustum_ortho.ymin) viscode |= VIS_BELOW;
    if (y_of(vertex) > g_frustum_ortho.ymax) viscode |= VIS_ABOVE;
    if (z_of(vertex) < g_frustum_ortho.zmin) viscode |= VIS_NEAR;
    if (z_of(vertex) > g_frustum_ortho.zmax) viscode |= VIS_FAR;
    
    return viscode;
}

static uint8_t ComputeViscode_persp(Fix64Vec vertex) {
    uint8_t viscode = 0;
    if (x_of(vertex) < -z_of(vertex)) viscode |= VIS_RIGHT;
    if (x_of(vertex) > +z_of(vertex)) viscode |= VIS_LEFT;
    if (y_of(vertex) < -z_of(vertex)) viscode |= VIS_BELOW;
    if (y_of(vertex) > +z_of(vertex)) viscode |= VIS_ABOVE;
    if (z_of(vertex) < g_frustum_persp.zmin) viscode |= VIS_NEAR;
    if (g_farclip && z_of(vertex) > g_frustum_persp.zmax) viscode |= VIS_FAR;

    return viscode;
}

static inline Fix64Vec worldspace_to_eyespace(WorldVec in) {
    Fix64Vec out;
    
    // 1) translate
    Fix64
        tmp_x = (Fix64)x_of(in) - (Fix64)tf.x,
        tmp_y = (Fix64)y_of(in) - (Fix64)tf.y,
        tmp_z = (Fix64)z_of(in) - (Fix64)tf.z;
        
    // 2) rotate
    Fix64 tmp =  FIX_MUL(tmp_x, tf.hcos) + FIX_MUL(tmp_y, tf.hsin);
    x_of(out)  = (-FIX_MUL(tmp_x, tf.hsin) + FIX_MUL(tmp_y, tf.hcos));
    y_of(out)  = (-FIX_MUL(tmp, tf.vcos)   + FIX_MUL(tmp_z, tf.vsin));
    z_of(out)  = (+FIX_MUL(tmp, tf.vsin)   + FIX_MUL(tmp_z, tf.vcos));

    if (g_p_frustum->type == PROJ_PERSPECTIVE) {
        // 3) Scale x and y to normalized perspective frustum. The orthographic
        // frustum is *not* scaled this way since it is a cuboid, which is
        // a simple enough shape already.
        x_of(out) = (LSHIFT64(x_of(out), 16)) / (Fix64)g_frustum_persp.W;
        y_of(out) = (LSHIFT64(y_of(out), 16)) / (Fix64)g_frustum_persp.H;
    }
    
    return out;
}

void epm_TransformVertex
(Window const *win, Frustum *fr, WorldVec v, TransformedVertex *out_tv) {
    out_tv->eye = worldspace_to_eyespace(v);
    fn_EyespaceToScreen(win, out_tv);
}


void epm_TransformVertexBatch
(Window const *win, Frustum *fr,
 size_t num_vertices, WorldVec *v,
 TransformedVertex *tv) {
    // TODO: Benchmark this naive attempt at loop-unrolling.
    size_t rem = num_vertices & (8-1);
    
    WorldVec *vmax = v + num_vertices - rem;
    while (v < vmax) {
        tv->eye = worldspace_to_eyespace(*v);
        fn_EyespaceToScreen(win, tv);
        v++, tv++;
        
        tv->eye = worldspace_to_eyespace(*v);
        fn_EyespaceToScreen(win, tv);
        v++, tv++;
        
        tv->eye = worldspace_to_eyespace(*v);
        fn_EyespaceToScreen(win, tv);
        v++, tv++;
        
        tv->eye = worldspace_to_eyespace(*v);
        fn_EyespaceToScreen(win, tv);
        v++, tv++;
        
        tv->eye = worldspace_to_eyespace(*v);
        fn_EyespaceToScreen(win, tv);
        v++, tv++;
        
        tv->eye = worldspace_to_eyespace(*v);
        fn_EyespaceToScreen(win, tv);
        v++, tv++;
        
        tv->eye = worldspace_to_eyespace(*v);
        fn_EyespaceToScreen(win, tv);
        v++, tv++;
        
        tv->eye = worldspace_to_eyespace(*v);
        fn_EyespaceToScreen(win, tv);
        v++, tv++;
    }

    vmax += rem;
    while (v < vmax) {
        tv->eye = worldspace_to_eyespace(*v);
        fn_EyespaceToScreen(win, tv);
        v++, tv++;
    }
}



static void transform_Model
(Window const *const win, Frustum *const fr, int width, Model *model,
 TransformedVertex * out_vbuf) {
    size_t num_vertices = model->mesh.num_vertices;
    WorldVec *vertices = model->mesh.vertices;
    WorldVec translation = model->translation;
    
    Fix64
        vcos = tf.vcos,
        vsin = tf.vsin,
        hcos = tf.hcos,
        hsin = tf.hsin;
    Fix64
        eye_x = tf.x,
        eye_y = tf.y,
        eye_z = tf.z;

    Fix32 W = fr->W;
    Fix32 H = fr->H = (Fix32)FIX_MULDIV(W, win->mrect.h, win->mrect.w);
    fr->ymin = -fr->H;
    fr->ymax = +fr->H - 1;
    
    for (size_t i_v = 0; i_v < num_vertices; i_v++) {
        Fix64Vec eye;

        // TODO: 1) Self-relative rotation.
        
        // 2) Self-relative & Cam-relative translation.
        Fix64 tmp_x, tmp_y, tmp_z;
        
        tmp_x = x_of(((Fix32Vec *)vertices)[i_v]) - eye_x + x_of(translation);
        tmp_y = y_of(((Fix32Vec *)vertices)[i_v]) - eye_y + y_of(translation);
        tmp_z = z_of(((Fix32Vec *)vertices)[i_v]) - eye_z + z_of(translation);
        
        // 3) Cam-relative rotation.
        Fix64 tmp = FIX_MUL(tmp_x, hcos) + FIX_MUL(tmp_y, hsin);
        x_of(eye)  = -FIX_MUL(tmp_x, hsin) + FIX_MUL(tmp_y, hcos);
        y_of(eye)  = -FIX_MUL(tmp, vcos)   + FIX_MUL(tmp_z, vsin);
        z_of(eye)  = +FIX_MUL(tmp, vsin)   + FIX_MUL(tmp_z, vcos);

        // 4) Projection.
        TransformedVertex *tv = out_vbuf + i_v;
        if (fr->type == PROJ_PERSPECTIVE) {
            x_of(eye) = (LSHIFT64(x_of(eye), 16))/W;
            y_of(eye) = (LSHIFT64(y_of(eye), 16))/H;   
        }

        tv->eye = eye;
        fn_EyespaceToScreen(win, tv);
    }
}


void draw_wireframe_face
(Window *win, TransformedWireFace *twf, zgl_Color color) {
    if (area(twf->pixel[0].XY, twf->pixel[1].XY, twf->pixel[2].XY) <= 0) {
        return;
    }
    
    Fix64Vec_2D scr_v0 = twf->pixel[0].XY;
    Fix64Vec_2D scr_v1 = twf->pixel[1].XY;
    Fix64Vec_2D scr_v2 = twf->pixel[2].XY;
    
#define FixPoint2D_64to32(POINT) ((Fix32Vec_2D){(Fix32)(POINT).x, (Fix32)(POINT).y})
#define as_mPixelSeg(P1, P2) ((zgl_mPixelSeg){{(P1).x, (P1).y}, {(P2).x, (P2).y}})
    if ( ! (twf->vis[0] | twf->vis[1] | twf->vis[2])) { // no clipping
        zglDraw_mPixelSeg2(g_scr, &win->mrect,
                           &as_mPixelSeg(FixPoint2D_64to32(scr_v0),
                                         FixPoint2D_64to32(scr_v1)), color);
        zglDraw_mPixelSeg2(g_scr, &win->mrect,
                           &as_mPixelSeg(FixPoint2D_64to32(scr_v1),
                                         FixPoint2D_64to32(scr_v2)), color);
        zglDraw_mPixelSeg2(g_scr, &win->mrect,
                           &as_mPixelSeg(FixPoint2D_64to32(scr_v2),
                                         FixPoint2D_64to32(scr_v0)), color);
    }
    else { // clipping
        Fix64Vec_2D subvs[32];
        bool new[32];
        size_t num_subvs;
        TriClip2D(&scr_v0, &scr_v1, &scr_v2,
                  win->mrect.x, win->mrect.x + win->mrect.w - 1,
                  win->mrect.y, win->mrect.y + win->mrect.h - 1,
                  subvs, &num_subvs, new);

        for (size_t i_subv = 0; i_subv < num_subvs; i_subv++) {
            zgl_mPixel tmp0;
            zgl_mPixel tmp1;
            tmp0.x = (Fix32)subvs[i_subv].x;
            tmp0.y = (Fix32)subvs[i_subv].y;

            tmp1.x = (Fix32)subvs[(i_subv+1) % num_subvs].x;
            tmp1.y = (Fix32)subvs[(i_subv+1) % num_subvs].y;

            zglDraw_mPixelSeg(g_scr, &win->mrect, tmp0, tmp1, color);
        }
    }
}

void draw_face(Window *win, Face *face, TransformedFace *tf) {    
    DepthPixel
        scr_V0 = tf->tv[0].pixel,
        scr_V1 = tf->tv[1].pixel,
        scr_V2 = tf->tv[2].pixel;
    
    if (area(scr_V0.XY, scr_V1.XY, scr_V2.XY) <= 0) {
        return;
    }

    if (g_settings.wire_mode == WIRE_OVERLAY) {
        zglDraw_mPixelSeg_NoOverdraw(g_scr, &win->mrect, 
                                     FixPoint2D_64to32(scr_V0.XY),
                                     FixPoint2D_64to32(scr_V1.XY),
                                     0xFFFFFF);
        zglDraw_mPixelSeg_NoOverdraw(g_scr, &win->mrect, 
                                     FixPoint2D_64to32(scr_V1.XY),
                                     FixPoint2D_64to32(scr_V2.XY),
                                     0xFFFFFF);
        zglDraw_mPixelSeg_NoOverdraw(g_scr, &win->mrect, 
                                     FixPoint2D_64to32(scr_V2.XY),
                                     FixPoint2D_64to32(scr_V0.XY),
                                     0xFFFFFF);

    }
    
    if ( ! (tf->tv[0].vis | tf->tv[1].vis | tf->tv[2].vis)) { // no clipping needed
        ScreenTri screen_tri = {
            .vpxl = {&scr_V0, &scr_V1, &scr_V2},
            .vtxl = {&face->vtxl[0], &face->vtxl[1], &face->vtxl[2]},
            .vbri = {tf->vbri[0], tf->vbri[1], tf->vbri[2]},
            .fbri = face->fbri,
            .ftex = textures[face->i_tex].pixarr,
            .shft = textures[face->i_tex].log2_wh,
            .fclr = 0};

        if (streq(textures[face->i_tex].name, "sky") && ! (g_rasterflags & RASTERFLAG_SKY)) {
            extern bool g_skymask;
            g_skymask = true;
        }
            
        
        if (face->flags & FC_SELECTED) {
            g_rasterflags |= RASTERFLAG_HILIT;
        }
        draw_Triangle(win, &screen_tri);
        g_rasterflags &= ~RASTERFLAG_HILIT;

        extern bool g_skymask;
        g_skymask = false;
    }
    else { // 2D clipping needed
        epm_Log(LT_WARN, "2D clipping phase triggered.");
        
        // 2023-05-13: This doesn't appear to ever be reached, which suggests
        // the earlier 3D clipping to frustum is highly accurate.
        Fix64Vec_2D subvs[32];
        bool new[32];
        size_t num_subvs;
        TriClip2D(&scr_V0.XY, &scr_V1.XY, &scr_V2.XY,
                  win->mrect.x, win->mrect.x + win->mrect.w - 1,
                  win->mrect.y, win->mrect.y + win->mrect.h - 1,
                  subvs, &num_subvs, new);

        // need signed
        ptrdiff_t const i_0[] = {0, -1, -1, -2, -2, -3, -3, -4, -4, -5, -5};
        ptrdiff_t const i_1[] = {1,  0,  2, -1,  3, -2,  4, -3,  5, -4,  6};
        ptrdiff_t const i_2[] = {2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7};
        
        #define area(a, b, c) ((Fix64)((c).x - (a).x) * (Fix64)((b).y - (a).y) - (Fix64)((c).y - (a).y) * (Fix64)((b).x - (a).x))

        Fix64 area = ABS(area(scr_V0.XY, scr_V1.XY, scr_V2.XY)>>16);
        Fix64 B0, B1, B2;

        dibassert(area != 0);

        // number of subdivided triangles is 2 less than number of vertices
        for (size_t i_subface = 0; i_subface + 2 < num_subvs; i_subface++) {
            size_t
                i_subv0 = (num_subvs + i_0[i_subface]) % num_subvs,
                i_subv1 = (num_subvs + i_1[i_subface]) % num_subvs,
                i_subv2 = (num_subvs + i_2[i_subface]) % num_subvs;
            
            zgl_mPixel subTV0, subTV1, subTV2;
            DepthPixel subV0, subV1, subV2;
            subV0.XY = subvs[i_subv0];
            subV1.XY = subvs[i_subv1];
            subV2.XY = subvs[i_subv2];
            
            ScreenTri subtri;
            subtri.ftex = textures[face->i_tex].pixarr;
            subtri.shft = textures[face->i_tex].log2_wh;
            subtri.fbri = face->fbri;
            
            Fix64 Zinv;
            Fix64
                Z0 = scr_V0.z,
                Z1 = scr_V1.z,
                Z2 = scr_V2.z;
            Fix32Vec_2D
                TV0 = face->vtxl[0],
                TV1 = face->vtxl[1],
                TV2 = face->vtxl[2];
            uint64_t
                vbri0 = face->vbri[0],//tf->vbri[0],//bsp.vertex_intensities[face->i_v[0]],
                vbri1 = face->vbri[1],//tf->vbri[1],//bsp.vertex_intensities[face->i_v[1]],
                vbri2 = face->vbri[2];//tf->vbri[2];//bsp.vertex_intensities[face->i_v[2]];

            vbri0 <<= 16;
            vbri1 <<= 16;
            vbri2 <<= 16;
            
            // depth & texel 0
            B0 = ABS(area(subV0.XY, scr_V1.XY, scr_V2.XY))/area;
            B1 = ABS(area(subV0.XY, scr_V0.XY, scr_V2.XY))/area;
            B2 = (1<<16) - B0 - B1;
            Zinv = ((B0<<16)/Z0 + (B1<<16)/Z1 + (B2<<16)/Z2);
            subV0.z  = ((B0+B1+B2)<<16)/Zinv;
            subTV0.x = (Fix32)((B0*FIX_DIV(TV0.x, Z0) +
                                  B1*FIX_DIV(TV1.x, Z1) +
                                  B2*FIX_DIV(TV2.x, Z2))/Zinv);
            subTV0.y = (Fix32)((B0*FIX_DIV(TV0.y, Z0) +
                                  B1*FIX_DIV(TV1.y, Z1) +
                                  B2*FIX_DIV(TV2.y, Z2))/Zinv);
            subtri.vbri[0] = (uint8_t)((B0*FIX_DIV(vbri0, Z0) +
                                        B1*FIX_DIV(vbri1, Z1) +
                                        B2*FIX_DIV(vbri2, Z2))/Zinv);

            // depth & texel 1
            B0 = ABS(area(subV1.XY, scr_V1.XY, scr_V2.XY))/area;
            B1 = ABS(area(subV1.XY, scr_V0.XY, scr_V2.XY))/area;
            B2 = (1<<16) - B0 - B1;
            Zinv = ((B0<<16)/Z0 + (B1<<16)/Z1 + (B2<<16)/Z2);
            subV1.z  = ((B0+B1+B2)<<16)/Zinv;
            subTV1.x = (Fix32)((B0*FIX_DIV(TV0.x, Z0) +
                                  B1*FIX_DIV(TV1.x, Z1) +
                                  B2*FIX_DIV(TV2.x, Z2))/Zinv);
            subTV1.y = (Fix32)((B0*FIX_DIV(TV0.y, Z0) +
                                  B1*FIX_DIV(TV1.y, Z1) +
                                  B2*FIX_DIV(TV2.y, Z2))/Zinv);
            subtri.vbri[1] = (uint8_t)((B0*FIX_DIV(vbri0, Z0) +
                                        B1*FIX_DIV(vbri1, Z1) +
                                        B2*FIX_DIV(vbri2, Z2))/Zinv);
            // depth & texel 2
            B0 = ABS(area(subV2.XY, scr_V1.XY, scr_V2.XY))/area;
            B1 = ABS(area(subV2.XY, scr_V0.XY, scr_V2.XY))/area;
            B2 = (1<<16) - B0 - B1;
            Zinv = ((B0<<16)/Z0 + (B1<<16)/Z1 + (B2<<16)/Z2);
            subV2.z  = ((B0+B1+B2)<<16)/Zinv;
            subTV2.x = (Fix32)((B0*FIX_DIV(TV0.x, Z0) +
                                  B1*FIX_DIV(TV1.x, Z1) +
                                  B2*FIX_DIV(TV2.x, Z2))/Zinv);
            subTV2.y = (Fix32)((B0*FIX_DIV(TV0.y, Z0) +
                                  B1*FIX_DIV(TV1.y, Z1) +
                                  B2*FIX_DIV(TV2.y, Z2))/Zinv);
            subtri.vbri[2] = (uint8_t)((B0*FIX_DIV(vbri0, Z0) +
                                        B1*FIX_DIV(vbri1, Z1) +
                                        B2*FIX_DIV(vbri2, Z2))/Zinv);
            
            subtri.vtxl[0] = &subTV0;
            subtri.vtxl[1] = &subTV1;
            subtri.vtxl[2] = &subTV2;
            subtri.vpxl[0] = &subV0;
            subtri.vpxl[1] = &subV1;
            subtri.vpxl[2] = &subV2;

            draw_Triangle(win, &subtri);
        }
    }
}


#define area2(a, b, c) ((Fix64)(x_of(c) - x_of(a)) * (Fix64)(y_of(b) - y_of(a)) - (Fix64)(y_of(c) - y_of(a)) * (Fix64)(x_of(b) - x_of(a)))


static void draw_wireframe_edges_BSP_colors(Window *win, Frustum *fr, EdgeSet *eset, zgl_Color color, TransformedVertex const *const vbuf, uint8_t wireflags) {
    int (*fn_drawseg)(zgl_PixelArray *, const zgl_mPixelRect *,
                      const zgl_mPixel, const zgl_mPixel, const zgl_Color);
    fn_drawseg =
        wireflags & WIREFLAGS_DOTTED ?
        zglDraw_mPixelSeg_Dotted :
        zglDraw_mPixelSeg;

    size_t num_edges = eset->num_edges;
    Edge *edges = eset->edges;

    for (size_t i_edge = 0; i_edge < num_edges; i_edge++) {
        color = g_world.geo_bsp->edge_colors[i_edge];
        size_t i_v0 = edges[i_edge].i_v0;
        size_t i_v1 = edges[i_edge].i_v1;

        uint8_t vis0, vis1;
        Fix64Vec p0, p1;
        Fix64Vec c0, c1;
        
        p0 = vbuf[i_v0].eye;
        p1 = vbuf[i_v1].eye;

        vis0 = vbuf[i_v0].vis;
        vis1 = vbuf[i_v1].vis;
        

        // TODO: This rejects some lines that need to be clipped!
        // 2023-05-19: I don't remember when I wrote previous line, or if its
        // still true.
        if (vis0 & vis1) { // trivial reject
            continue;
        }

        if ( ! (vis0 | vis1)) {
            fn_drawseg(g_scr, &win->mrect,
                       (zgl_mPixel){(Fix32)vbuf[i_v0].pixel.XY.x, (Fix32)vbuf[i_v0].pixel.XY.y},
                       (zgl_mPixel){(Fix32)vbuf[i_v1].pixel.XY.x, (Fix32)vbuf[i_v1].pixel.XY.y},
                       color);            
        }
        else {
            bool accept = fn_SegClip3D(x_of(p0), y_of(p0), z_of(p0),
                                       x_of(p1), y_of(p1), z_of(p1),
                                       fr, &c0, &c1);
            
            if ( ! accept) { // reject
                continue; // this might never occur. TODO
            }

            TransformedVertex tv0;
            TransformedVertex tv1;

            tv0.eye = c0;
            tv1.eye = c1;

            fn_EyespaceToScreen(win, &tv0);
            fn_EyespaceToScreen(win, &tv1);
            fn_drawseg(g_scr, &win->mrect,
                       (zgl_mPixel){(Fix32)tv0.pixel.XY.x, (Fix32)tv0.pixel.XY.y},
                       (zgl_mPixel){(Fix32)tv1.pixel.XY.x, (Fix32)tv1.pixel.XY.y},
                       color);
        }
    }

    if ( ! (wireflags & WIREFLAGS_NO_VERTICES)) {
        size_t num_v = eset->num_vertices;
        for (size_t i_v = 0; i_v < num_v; i_v++) {
            if (vbuf[i_v].vis) continue;

            color = g_world.geo_bsp->vertex_colors[i_v];            
            zglDraw_mPixelDot(g_scr, &win->mrect, &(zgl_mPixel){(Fix32)vbuf[i_v].pixel.XY.x, (Fix32)vbuf[i_v].pixel.XY.y}, color);
        }    
    }
}





/**
   Use edge-based culling and clipping.
*/
static void draw_wireframe_edges(Window *win, Frustum *fr, EdgeSet *eset, zgl_Color color, TransformedVertex const *const vbuf, uint8_t wireflags) {
    int (*fn_drawseg)(zgl_PixelArray *, const zgl_mPixelRect *,
                      const zgl_mPixel, const zgl_mPixel, const zgl_Color);
    fn_drawseg =
        wireflags & WIREFLAGS_DOTTED ?
        zglDraw_mPixelSeg_Dotted :
        zglDraw_mPixelSeg;

    size_t num_edges = eset->num_edges;
    Edge *edges = eset->edges;

    for (size_t i_edge = 0; i_edge < num_edges; i_edge++) {
        size_t i_v0 = edges[i_edge].i_v0;
        size_t i_v1 = edges[i_edge].i_v1;

        uint8_t vis0, vis1;
        Fix64Vec p0, p1;
        Fix64Vec c0, c1;
        
        p0 = vbuf[i_v0].eye;
        p1 = vbuf[i_v1].eye;

        vis0 = vbuf[i_v0].vis;
        vis1 = vbuf[i_v1].vis;
        

        // TODO: This rejects some lines that need to be clipped!
        // 2023-05-19: I don't remember when I wrote previous line, or if its
        // still true.
        if (vis0 & vis1) { // trivial reject
            continue;
        }

        if ( ! (vis0 | vis1)) {
            fn_drawseg(g_scr, &win->mrect,
                       (zgl_mPixel){(Fix32)vbuf[i_v0].pixel.XY.x, (Fix32)vbuf[i_v0].pixel.XY.y},
                       (zgl_mPixel){(Fix32)vbuf[i_v1].pixel.XY.x, (Fix32)vbuf[i_v1].pixel.XY.y},
                       color);            
        }
        else {
            bool accept = fn_SegClip3D(x_of(p0), y_of(p0), z_of(p0),
                                       x_of(p1), y_of(p1), z_of(p1),
                                       fr, &c0, &c1);
            
            if ( ! accept) { // reject
                continue; // this might never occur. TODO
            }

            TransformedVertex tv0;
            TransformedVertex tv1;

            tv0.eye = c0;
            tv1.eye = c1;

            fn_EyespaceToScreen(win, &tv0);
            fn_EyespaceToScreen(win, &tv1);
            fn_drawseg(g_scr, &win->mrect,
                       (zgl_mPixel){(Fix32)tv0.pixel.XY.x, (Fix32)tv0.pixel.XY.y},
                       (zgl_mPixel){(Fix32)tv1.pixel.XY.x, (Fix32)tv1.pixel.XY.y},
                       color);
        }
    }

    if ( ! (wireflags & WIREFLAGS_NO_VERTICES)) {
        size_t num_v = eset->num_vertices;
        for (size_t i_v = 0; i_v < num_v; i_v++) {
            if (vbuf[i_v].vis) continue;
            zglDraw_mPixelDot(g_scr, &win->mrect, &(zgl_mPixel){(Fix32)vbuf[i_v].pixel.XY.x, (Fix32)vbuf[i_v].pixel.XY.y}, color);
        }
    }
}

/**
   Draw mesh as wireframe, but use face-based culling and clipping. Visualizes
   triangle subdivisions.
*/
static void draw_wireframe_faces(Window *win, FaceSet *fset, zgl_Color color, TransformedVertex const *const vbuf) {
    size_t num_faces = fset->num_faces;
    Face *faces = fset->faces;

    for (size_t i_face = 0; i_face < num_faces; i_face++) {
        Face *face = faces + i_face;

        if (face->flags & FC_DEGEN) {
            continue;
        }
        
        size_t
            i_v0 = face->i_v[0],
            i_v1 = face->i_v[1],
            i_v2 = face->i_v[2];

        // grab data from vertex buffer
        TransformedWireFace twface;
        twface.vis[0] = vbuf[i_v0].vis;
        twface.vis[1] = vbuf[i_v1].vis;
        twface.vis[2] = vbuf[i_v2].vis;
            
        if (twface.vis[0] & twface.vis[1] & twface.vis[2]) { // trivial reject
            continue;
        }
        
        if ( ! ((twface.vis[0]) | (twface.vis[1]) | (twface.vis[2]))) {
            twface.pixel[0] = vbuf[i_v0].pixel;
            twface.pixel[1] = vbuf[i_v1].pixel;
            twface.pixel[2] = vbuf[i_v2].pixel;
            twface.color = color;

            // vertex number labels if selected
            if (face->flags & FC_SELECTED) {
                draw_BMPFont_string
                    (g_scr, NULL, "0",
                     (Fix32)(twface.pixel[0].XY.x>>16),
                     (Fix32)(twface.pixel[0].XY.y>>16), FC_MONOGRAM1, 0xFFFFFF);
                draw_BMPFont_string
                    (g_scr, NULL, "1",
                     (Fix32)(twface.pixel[1].XY.x>>16),
                     (Fix32)(twface.pixel[1].XY.y>>16), FC_MONOGRAM1, 0xFFFFFF);
                draw_BMPFont_string
                    (g_scr, NULL, "2",
                     (Fix32)(twface.pixel[2].XY.x>>16),
                     (Fix32)(twface.pixel[2].XY.y>>16), FC_MONOGRAM1, 0xFFFFFF);
            }
            
            draw_wireframe_face(win, &twface, color);
        }
        else { // 3D clipping
            size_t num_vertices;
            Fix64Vec poly[32];
            TransformedVertex tpoly[32];
            bool new[32];
            clip_and_transform(win, vbuf[i_v0].eye, vbuf[i_v1].eye, vbuf[i_v2].eye,
                               &num_vertices, poly, tpoly, new);

            // zig-zag subdivision
            ptrdiff_t const i_0[]  = { 0,-1,-1,-2,-2,-3,-3,-4,-4,-5,-5};
            ptrdiff_t const i_1[]  = { 1, 0, 2,-1, 3,-2, 4,-3, 5,-4, 6};
            ptrdiff_t const i_2[]  = { 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7};

            size_t i_subv0, i_subv1, i_subv2;
            TransformedWireFace twf;
            if (num_vertices == 0) {
                continue;
            }
            else if (num_vertices == 3) {
                twf = (TransformedWireFace){
                    {tpoly[0].pixel, tpoly[1].pixel, tpoly[2].pixel},
                    {tpoly[0].vis,   tpoly[1].vis,   tpoly[2].vis},
                    color,
                };
                
                draw_wireframe_face(win, &twf, color);
            }
            else { // num_vertices >= 4
                for (size_t i_subface = 0; i_subface + 2 + 1 < num_vertices; i_subface++) {
                    i_subv0 = (num_vertices + i_0[i_subface]) % num_vertices;
                    i_subv1 = (num_vertices + i_1[i_subface]) % num_vertices;
                    i_subv2 = (num_vertices + i_2[i_subface]) % num_vertices;

                    twf = (TransformedWireFace){
                        {tpoly[i_subv0].pixel, tpoly[i_subv1].pixel, tpoly[i_subv2].pixel},
                        {tpoly[i_subv0].vis,   tpoly[i_subv1].vis,   tpoly[i_subv2].vis},
                        color,
                    };
                
                    draw_wireframe_face(win, &twf, color);
                }
                
                // handle last one separately for correct wire coloring.
                i_subv0 = (num_vertices + i_0[num_vertices-3]) % num_vertices;
                i_subv1 = (num_vertices + i_1[num_vertices-3]) % num_vertices;
                i_subv2 = (num_vertices + i_2[num_vertices-3]) % num_vertices;
                if (num_vertices & 1) {
                    twf = (TransformedWireFace){
                        {tpoly[i_subv0].pixel, tpoly[i_subv1].pixel, tpoly[i_subv2].pixel},
                        {tpoly[i_subv0].vis,   tpoly[i_subv1].vis,   tpoly[i_subv2].vis},
                        color,
                    };
                }
                else {
                    twf = (TransformedWireFace){
                        {tpoly[i_subv0].pixel, tpoly[i_subv1].pixel, tpoly[i_subv2].pixel},
                        {tpoly[i_subv0].vis,   tpoly[i_subv1].vis,   tpoly[i_subv2].vis},
                        color,
                    };
                }
                
                draw_wireframe_face(win, &twf, color);
            }

            if (face->flags & FC_SELECTED) {
                char num[3+1];
                for (size_t i_vertex = 0; i_vertex < num_vertices; i_vertex++) {
                    sprintf(num, "%zu", i_vertex);
                    draw_BMPFont_string(g_scr, NULL, num,
                                        (Fix32)(tpoly[i_vertex].pixel.XY.x>>16),
                                        (Fix32)(tpoly[i_vertex].pixel.XY.y>>16),
                                        FC_MONOGRAM1, 0xFFFFFF);
                }
            }
        }
    }
    
    return;
}

/* 1) Clip triangle V0->V1->V2 to to frustum; yields a polygon.

   2) The resulting polygon's vertices are already in eyespace. We copy them
   directly into a TransformedVertex eye member, then do EyespaceToScreen. */
void clip_and_transform(Window const *const win,
                        Fix64Vec V0, Fix64Vec V1, Fix64Vec V2,
                        size_t *out_num_subvs, Fix64Vec *out_subvs,
                        TransformedVertex *out_subvbuf, bool *out_new) {
    size_t num_subvs;
    fn_TriClip3D(&V0, &V1, &V2, g_p_frustum, out_subvs, &num_subvs, out_new);
    
    Fix64Vec eye;
    for (size_t i_subv = 0; i_subv < num_subvs; i_subv++) {
        eye = out_subvs[i_subv];

        TransformedVertex *tv = out_subvbuf + i_subv;
        tv->eye = eye;
        fn_EyespaceToScreen(win, tv);
    }
    
    *out_num_subvs = num_subvs;
}

void epm_DrawTriangleZigZag
(Window *win,
 size_t num_v, Fix64Vec v[], TransformedVertex tv[], Fix32Vec_2D tx[], uint8_t vbri[],
 WorldVec normal, size_t i_tex, uint8_t fbri, uint8_t flags,
 bool new[]) {
    // zig-zag triangulation (aka "triangle strip"???)
    //    int8_t const i_0[] = {0, -1, -1, -2, -2, -3, -3, -4, -4, -5, -5};
    //    int8_t const i_1[] = {1,  0,  2, -1,  3, -2,  4, -3,  5, -4,  6};
    //    int8_t const i_2[] = {2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7};
    
    for (uint8_t i_tri = 0; i_tri + 2 < (uint8_t)num_v; i_tri++) {
        int8_t
            i_v0 = zigzag_i_v[0][i_tri],
            i_v1 = zigzag_i_v[1][i_tri],
            i_v2 = zigzag_i_v[2][i_tri];

        i_v0 = i_v0 < 0 ? i_v0 + (int8_t)num_v : i_v0;
        i_v1 = i_v1 < 0 ? i_v1 + (int8_t)num_v : i_v1;
        // no need for i_v2 = 
                
        // construct subface structure
        Face subface;
        subface.i_v[0] = i_v0;
        subface.i_v[1] = i_v1;
        subface.i_v[2] = i_v2;
        subface.flags = flags; // TODO: Possible to get degenerate subface?
        subface.normal = normal;
        subface.fbri = fbri;
        subface.i_tex = i_tex;
                
        // grab data from vertex buffers.
        TransformedFace subtface;
        subtface.tv[0] = tv[i_v0];
        subtface.tv[1] = tv[i_v1];
        subtface.tv[2] = tv[i_v2];
   
        subface.vtxl[0] = tx[i_v0];
        subtface.vbri[0] = vbri[i_v0];

        subface.vtxl[1] = tx[i_v1];
        subtface.vbri[1] = vbri[i_v1];

        subface.vtxl[2] = tx[i_v2];
        subtface.vbri[2] = vbri[i_v2];
        
        if (g_settings.BSP_visualizer){
            draw_face_BSP_visualizer(win, &subface, &subtface);
        }
        else {
            draw_face(win, &subface, &subtface);
        }
    }
}

void clip_trans_triangulate_and_draw_face(Window *win, Face *face, TransformedVertex const *const vbuf) {
    // If face is partially outside frustum, we do 3D clipping &
    // subdivision against all frustum planes. An alternative would be
    // to only 3D clip & subdivide against near plane, then do 2D
    // clipping in screen space.

    Fix64Vec V0 = vbuf[face->i_v[0]].eye;
    Fix64Vec V1 = vbuf[face->i_v[1]].eye;
    Fix64Vec V2 = vbuf[face->i_v[2]].eye;
    
    size_t num_v;
    Fix64Vec v[32];
    TransformedVertex tv[32];
    Fix32Vec_2D tx[32];
    uint8_t vbri[32];
    bool new[32];
    clip_and_transform(win, V0, V1, V2, &num_v, v, tv, new);

    /* Interpolate VA */
    // TODO: Avoid computing preexisting vertices. This is not for speed,
    // but for avoiding precision loss.
    Fix64 area = triangle_area_3D(V0, V1, V2);
    dibassert(area != 0);
    Fix32Vec_2D
        vtxl0 = face->vtxl[0],
        vtxl1 = face->vtxl[1],
        vtxl2 = face->vtxl[2];        
    int64_t
        vbri0 = face->vbri[0],
        vbri1 = face->vbri[1],
        vbri2 = face->vbri[2];
    for (size_t i_v = 0; i_v < num_v; i_v++) {
        Fix64 B0 = (triangle_area_3D(v[i_v], V1, V2)<<16)/area;
        Fix64 B1 = (triangle_area_3D(v[i_v], V0, V2)<<16)/area;
        Fix64 B2 = (1<<16) - B0 - B1;
        tx[i_v].x = (Fix32)((B0*vtxl0.x + B1*vtxl1.x + B2*vtxl2.x)>>16);
        tx[i_v].y = (Fix32)((B0*vtxl0.y + B1*vtxl1.y + B2*vtxl2.y)>>16);
        /*
        if ((B0*vbri0 + B1*vbri1 + B2*vbri2)>>16 < 0)
            vbri[i_v] = 0;
        else if (((B0*vbri0 + B1*vbri1 + B2*vbri2)>>16) > 255)
            vbri[i_v] = 255;
        else
        */
            vbri[i_v] = (uint8_t)((B0*vbri0 + B1*vbri1 + B2*vbri2)>>16);
    }

    epm_DrawTriangleZigZag(win,
                           num_v, v, tv, tx, vbri,
                           face->normal, face->i_tex, face->fbri, face->flags,
                           new);
    
    return;
}

static void draw_vertices(Window *win, size_t num_vertices, TransformedVertex const *const vbuf) {
    for (size_t i_v = 0; i_v < num_vertices; i_v++) {
        zgl_mPixel p = {(Fix32)vbuf[i_v].pixel.XY.x, (Fix32)vbuf[i_v].pixel.XY.y};
        uint8_t vis0 = vbuf[i_v].vis;
        
        // TODO: This rejects some lines that need to be clipped!
        if (vis0) { // trivial reject
            continue;
        }
        else {
            zglDraw_mPixelDot(g_scr, &win->mrect, &p, 0xFF00FF);
        }

    }
}

static void draw_faces(Window *win, FaceSet *fset, TransformedVertex const *const vbuf) {
    size_t num_faces = fset->num_faces;
    Face *faces = fset->faces;
    
    for (size_t i_face = 0; i_face < num_faces; i_face++) {
        Face *face = faces + i_face;
        
        if (face->flags & FC_DEGEN) {
            continue;
        }

        size_t
            i_v0 = face->i_v[0],
            i_v1 = face->i_v[1],
            i_v2 = face->i_v[2];

        // grab data from vertex buffer
        TransformedFace tface;
        tface.tv[0].vis = vbuf[i_v0].vis;
        tface.tv[1].vis = vbuf[i_v1].vis;
        tface.tv[2].vis = vbuf[i_v2].vis;

        if (tface.tv[0].vis & tface.tv[1].vis & tface.tv[2].vis) {
            // trivial reject
            continue;
        }

        if ( ! ((tface.tv[0].vis) | (tface.tv[1].vis) | (tface.tv[2].vis))) {
            tface.tv[0] = vbuf[i_v0];
            tface.tv[1] = vbuf[i_v1];
            tface.tv[2] = vbuf[i_v2];
            tface.vbri[0] = 255;
            tface.vbri[1] = 255;
            tface.vbri[2] = 255;
            draw_face(win, face, &tface);
        }
        else {
            clip_trans_triangulate_and_draw_face(win, face, vbuf);
        } 
    }
    
    return;    
}


/* -------------------------------------------------------------------------- */

#define roundup_pow2_32(v)                      \
    v--;                                        \
    v |= v >> 1;                                \
    v |= v >> 2;                                \
    v |= v >> 4;                                \
    v |= v >> 8;                                \
    v |= v >> 16;                               \
    v++                                         \

#define roundup_pow2_64(v)                      \
    v--;                                        \
    v |= v >> 1;                                \
    v |= v >> 2;                                \
    v |= v >> 4;                                \
    v |= v >> 8;                                \
    v |= v >> 16;                               \
    v |= v >> 32;                               \
    v++                                         \




void select_vertex(Window *win, zgl_Pixel pixel) {
    vertex_below(win, pixel, g_p_frustum);
}

void select_one_face(Window *win, zgl_Pixel pixel) {
    BSPFace *bspface = BSPFace_below(win, pixel, g_p_frustum);
    if ( ! g_world.geo_bsp || ! bspface) return;
    Face const *gen_face = g_world.geo_bsp->gen_faces + bspface->i_gen_face;
    if (bspface) {
        bool selected =
            ((BrushQuadFace *)gen_face->brushface)->flags & FACEFLAG_SELECTED;
        sel_clear(&sel_face);
        clear_brush_selection(&brushsel);
        if ( ! selected) {
            sel_toggle_face(gen_face->brushface);
        }
    }
}

void select_face(Window *win, zgl_Pixel pixel) {
    BSPFace *bspface = BSPFace_below(win, pixel, g_p_frustum);
    if (bspface) {
        uint32_t i_gen_face = bspface->i_gen_face;
        sel_toggle_face(g_world.geo_bsp->gen_faces[i_gen_face].brushface);
    }
}

void select_brush(Window *win, zgl_Pixel pixel) {
    BSPFace *bspface = BSPFace_below(win, pixel, g_p_frustum);
    if (bspface) {
        toggle_selected_brush(&brushsel, g_world.geo_prebsp->progenitor_brush[bspface->i_gen_face]);
    }
}




void epm_SetLightMode(int mode) {
    if (mode < 0 || 3 < mode) return;
    
    g_settings.light_mode = mode;
    
    if ( mode == LIGHT_OFF) {
        g_rasterflags &= ~RASTERFLAG_VBRI;
        g_rasterflags &= ~RASTERFLAG_FBRI;
    }
    else if (mode == LIGHT_VBRI) {
        g_rasterflags |=  RASTERFLAG_VBRI;
        g_rasterflags &= ~RASTERFLAG_FBRI;
    }
    else if (mode == LIGHT_FBRI) {
        g_rasterflags &= ~RASTERFLAG_VBRI;
        g_rasterflags |=  RASTERFLAG_FBRI;
    }
    else if (mode == LIGHT_BOTH) {
        g_rasterflags |=  RASTERFLAG_VBRI;
        g_rasterflags |=  RASTERFLAG_FBRI;
    }
}

void epm_ToggleLightMode(void) {
    static int prev_mode = LIGHT_VBRI;

    if (g_settings.light_mode != LIGHT_OFF) {
        prev_mode = g_settings.light_mode;
        epm_SetLightMode(LIGHT_OFF);
    }
    else { // restore saved mode
        epm_SetLightMode(prev_mode);
        prev_mode = LIGHT_OFF;
    }
}


void epm_SetWireMode(int mode) {
    if (mode < 0 || 2 < mode) return;
    
    g_settings.wire_mode = mode;
}

void epm_ToggleWireMode(void) {
    static int prev_mode = WIRE_NO_POLY;

    if (g_settings.wire_mode != WIRE_OFF) {
        prev_mode = g_settings.wire_mode;
        epm_SetWireMode(WIRE_OFF);
    }
    else { // restore saved mode
        epm_SetWireMode(prev_mode);
        prev_mode = WIRE_OFF;
    }
}

void epm_SetWireGeometry(int wiregeo) {
    if (wiregeo < 0 || 3 < wiregeo) return;

    g_settings.wiregeo = wiregeo;
}


void epm_SetProjectionMode(int mode) {
    if (mode != PROJ_ORTHOGRAPHIC && mode != PROJ_PERSPECTIVE) return;

    g_settings.proj_mode = mode;

    if (mode == PROJ_PERSPECTIVE) {
        g_rasterflags &= ~RASTERFLAG_ORTHO;
        
        g_p_frustum = &g_frustum_persp;
        fn_SegClip3D = SegClip3D_Persp;
        fn_TriClip3D = TriClip3D_Persp;
        fn_ComputeViscode = ComputeViscode_persp;
        fn_EyespaceToScreen = epm_EyespaceToScreen_persp;
    }
    else if (mode == PROJ_ORTHOGRAPHIC) {
        g_rasterflags |= RASTERFLAG_ORTHO;
        
        g_p_frustum = &g_frustum_ortho;
        fn_SegClip3D = SegClip3D_Ortho;
        fn_TriClip3D = TriClip3D_Ortho;
        fn_ComputeViscode = ComputeViscode_ortho;
        fn_EyespaceToScreen = epm_EyespaceToScreen_ortho;
    }
    else {
        dibassert(false);
    }

}

void epm_ToggleProjectionMode(void) {
    if (g_settings.proj_mode == PROJ_ORTHOGRAPHIC) {
        epm_SetProjectionMode(PROJ_PERSPECTIVE);
    }
    else {
        epm_SetProjectionMode(PROJ_ORTHOGRAPHIC);
    }
}

zgl_Color Normal_to_Color(WorldVec normal) {
    Fix64 x, y, z;
    
    // 2) rotate
    Fix64 tmp =  FIX_MUL(x_of(normal), tf.hcos) + FIX_MUL(y_of(normal), tf.hsin);
    x  = (-FIX_MUL(x_of(normal), tf.hsin) + FIX_MUL(y_of(normal), tf.hcos));
    y  = (-FIX_MUL(tmp, tf.vcos)   + FIX_MUL(z_of(normal), tf.vsin));
    z  = (+FIX_MUL(tmp, tf.vsin)   + FIX_MUL(z_of(normal), tf.vcos));

    
    return
        (((((x + (1<<16))*255)>>17)&0xFF)<<16) |
        (((((y + (1<<16))*255)>>17)&0xFF)<<8) |
        (((((z + (1<<16))*255)>>17)&0xFF)<<0);
}







/////////////////////////////////////////////////////////////////////////////

static int seg_low_bresenham_no_overdraw
(zgl_PixelArray *screen,
 const zgl_Pixit x0,
 const zgl_Pixit y0,
 const zgl_Pixit x1,
 const zgl_Pixit y1,
 const zgl_Color color) {
    dibassert(x1 < (zgl_Pixit)screen->w
             && x0 < (zgl_Pixit)screen->w
             && y1 < (zgl_Pixit)screen->h
             && y0 < (zgl_Pixit)screen->h);
    dibassert(x0 <= x1);
    
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    int32_t yi = 1;

    dibassert(dx >= 0);
    
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }

    int32_t D = (dy << 1) - dx;
    
    zgl_Color *pixels = (zgl_Color*)screen->pixels;
    int32_t x = x0, y = y0;
    
    for ( ; x <= x1; x++) {
        if ( ! (pixels[y*screen->w + x] & (1LL<<31)))
            pixels[y*screen->w + x] = color | (1LL<<31);

        if (D > 0) {
            y = y + yi;
            D = D + (LSHIFT32((dy - dx), 1));
        }
        else {
            D = D + (dy << 1);
        }
    }

    return 0;
}

static int seg_high_bresenham_no_overdraw
(zgl_PixelArray *screen,
 const zgl_Pixit x0,
 const zgl_Pixit y0,
 const zgl_Pixit x1,
 const zgl_Pixit y1,
 const zgl_Color color) {
    dibassert(x1 < (zgl_Pixit)screen->w
             && x0 < (zgl_Pixit)screen->w
             && y1 < (zgl_Pixit)screen->h
             && y0 < (zgl_Pixit)screen->h);
    dibassert(y0 <= y1);
    
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    int32_t xi = 1;
        
    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }

    int32_t D = (dx << 1) - dy;
    
    zgl_Color *pixels = (zgl_Color*)screen->pixels;
    
    int32_t x = x0, y = y0;
    for ( ; y <= y1; y++) {
        if ( ! (pixels[y*screen->w + x] & (1LL<<31)))
            pixels[y*screen->w + x] = color | (1LL<<31);
	
        if (D > 0) {
            x = x + xi;
            D = D + (LSHIFT32((dx - dy), 1));
        }
        else {
            D = D + (dx << 1);
        }
    }

    return 0;
}

int zglDraw_mPixelSeg_NoOverdraw
(zgl_PixelArray *screen,
 const zgl_mPixelRect *bounds,
 const zgl_mPixel pixel0,
 const zgl_mPixel pixel1,
 const zgl_Color color) {
    zgl_mPixelSeg clipped_seg;
    if ( ! zgl_ClipSegToRect((zgl_mPixelSeg){pixel0, pixel1}, *bounds, &clipped_seg)) {
        return 1; //nothing to draw, seg is entirely outside boundary
    }
    
    zgl_PixelSeg pix_seg = pixelseg_from_mpixelseg(clipped_seg);
    
    if (ABS((int64_t)pix_seg.pt1.y - (int64_t)pix_seg.pt0.y)
        <
        ABS((int64_t)pix_seg.pt1.x - (int64_t)pix_seg.pt0.x)) {
        if (pix_seg.pt0.x > pix_seg.pt1.x) {
            seg_low_bresenham_no_overdraw(screen,
                                          pix_seg.pt1.x,
                                          pix_seg.pt1.y,
                                          pix_seg.pt0.x,
                                          pix_seg.pt0.y,
                                          color);
        }
        else {
            seg_low_bresenham_no_overdraw(screen,
                                          pix_seg.pt0.x,
                                          pix_seg.pt0.y,
                                          pix_seg.pt1.x,
                                          pix_seg.pt1.y,
                                          color);
        }
    }
    else {
        if (pix_seg.pt0.y > pix_seg.pt1.y) {
            seg_high_bresenham_no_overdraw(screen,
                                           pix_seg.pt1.x,
                                           pix_seg.pt1.y,
                                           pix_seg.pt0.x,
                                           pix_seg.pt0.y,
                                           color);
        }
        else {
            seg_high_bresenham_no_overdraw(screen,
                                           pix_seg.pt0.x,
                                           pix_seg.pt0.y,
                                           pix_seg.pt1.x,
                                           pix_seg.pt1.y,
                                           color);
        }
    }
    
    return 0;
}
