#include "src/draw/draw3D_BSP.h"

#include "src/draw/window/window.h"
#include "src/draw/draw.h"
#include "src/draw/draw3D.h"
#include "src/world/world.h"

#include "src/draw/text.h"
#include "src/draw/textures.h"
#include "src/draw/colors.h"

#include "src/draw/clip.h"

#define area(a, b, c) ((Fix64)((c).x - (a).x) * (Fix64)((b).y - (a).y) - (Fix64)((c).y - (a).y) * (Fix64)((b).x - (a).x))

bool g_selected_face_only = false;

static bool test_BSPNode
(BSPNode *node, Fix32Vec rayDir, WorldVec rayV0, WorldVec rayV1);
static bool test_BSPFace
(BSPNode *node, Fix32Vec rayDir, WorldVec rayV0, WorldVec rayV1);

WorldVec sect_ring[MAX_SECTS] = {0};
static size_t curr_sect = 0;

typedef struct TypedNode {
    uint8_t iter_type;
    uint8_t side;
    BSPNode *node;
} TypedNode;

static int i_stack = 0;
static TypedNode stack[1024]; // way too big than what's needed

static inline void push(TypedNode tn) {
    stack[++i_stack] = tn;
}

static inline TypedNode pop(void) {
    return stack[i_stack--];
}

static uint8_t sideof_BSPNode
(WorldVec pt,
 BSPNode *node);
extern void draw_BSPNode_wireframe
(Window *win,
 BSPTree *tree,
 BSPNode *node,
 WorldVec campos,
 TransformedVertex *vbuf);
static void draw_BSPNode_recursive
(Window *win,
 BSPTree *tree,
 BSPNode *node,
 WorldVec campos,
 TransformedVertex *vbuf);
static void draw_BSPFace_wireframe
(Window *win,
 BSPNode *node,
 TransformedVertex *vbuf, zgl_Color color);
static void draw_BSPFace
(Window *win,
 BSPNode *node,
 TransformedVertex *vbuf);
extern void draw_face_BSP_visualizer
(Window *win,
 Face *face,
 TransformedFace *tf);
extern void draw_BSPTree
(BSPTree *tree,
 Window *win,
 WorldVec campos,
 TransformedVertex *vbuf);
extern BSPFace *BSPFace_below
(Window *win,
 zgl_Pixel pixel,
 Frustum *fr);
extern bool test_BSPFace
(BSPNode *node,
 Fix32Vec rayDir,
 WorldVec rayV0,
 WorldVec rayV1);

static uint8_t sideof_BSPNode(WorldVec V, BSPNode *node) {
    dibassert(node != NULL);
    
    if (g_settings.proj_mode == PROJ_PERSPECTIVE) {
        return sideof_plane(V, node->splitV, node->splitN);
        /*
        Triangle tri = {
            .v0 = world.bsp->progenitor_vertices[ogface->i_v0],
            .v1 = world.bsp->progenitor_vertices[ogface->i_v1],
            .v2 = world.bsp->progenitor_vertices[ogface->i_v2],
        };
        return sideof_tri(V, tri);
        */
    }
    else if (g_settings.proj_mode == PROJ_ORTHOGRAPHIC) {
        WorldVec N = node->splitN;
        
        // rotate
        Fix64 S = FIX_MUL(x_of(N), tf.hcos) + FIX_MUL(y_of(N), tf.hsin);
        S = (Fix32)(+FIX_MUL(S, tf.vsin) + FIX_MUL(z_of(N), tf.vcos));
        //printf("%x\n", z_of(res));
        // z coordinate of the rotated normal determines which side of plane the
        // "point" (0, 0, -infty) is


        
        
        /*
        Face const *ogface = node->bspface->progenitor_face;
        
        WorldVec v0 = apply_transform(world.bsp->progenitor_vertices[ogface->i_v0]);
        WorldVec v1 = apply_transform(world.bsp->progenitor_vertices[ogface->i_v1]);
        WorldVec v2 = apply_transform(world.bsp->progenitor_vertices[ogface->i_v2]);
        
        Fix64 S =
            (Fix64)x_of(v0) * (Fix64)(y_of(v1) - y_of(v2)) +
            (Fix64)x_of(v1) * (Fix64)(y_of(v2) - y_of(v0)) +
            (Fix64)x_of(v2) * (Fix64)(y_of(v0) - y_of(v1));
        */
        //printf("%lx\n\n", S>>16);


        
        if (S == 0)
            return SIDE_MID;
        else if (S < 0)
            return SIDE_FRONT;
        else
            return SIDE_BACK;
    }
    else {
        dibassert(false);
        return -1;
    }
}

// TODO: Pit the iterative function against the recursive one in a benchmark
// battle.

#define ITER_1 0
#define ITER_2 1

size_t g_i_acting_root = 0;

void draw_BSPTree
(BSPTree *tree, Window *win, WorldVec campos, TransformedVertex *vbuf) {
    g_rasterflags |= RASTERFLAG_COUNT;
    
    BSPNode *node = &tree->nodes[g_i_acting_root];
    uint8_t iter_type;
    uint8_t side;
    
    side = sideof_BSPNode(campos, node);
    push((TypedNode){ITER_2, side, node});
    node = (side == SIDE_FRONT) ? node->front : node->back;
    iter_type = ITER_1;
    
    while (i_stack >= 0) {
        bool turn_back = (node == NULL);
        
        if (turn_back == false) {
            switch (iter_type) {
            case ITER_1:
                side = sideof_BSPNode(campos, node);
                push((TypedNode){ITER_2, side, node});
                node = (side == SIDE_FRONT) ? node->front : node->back;
                iter_type = ITER_1;
                break;
            case ITER_2:
                draw_BSPFace(win, node, vbuf);
                if (g_all_pixels_drawn) goto End;
                node = (side == SIDE_FRONT) ? node->back : node->front;
                iter_type = ITER_1;
                break;
            }
        }

        if (turn_back == true) {
            TypedNode tn = pop();
            iter_type = tn.iter_type;
            side = tn.side;
            node = tn.node;
        }
    }

 End:
    g_rasterflags &= ~RASTERFLAG_COUNT;
    i_stack = 0;
}


static void draw_BSPNode_recursive(Window *win, BSPTree *tree, BSPNode *node, WorldVec campos, TransformedVertex *vbuf) {
    if (node == NULL) return;
    
    uint8_t side = sideof_BSPNode(campos, node);
    
    switch (side) {
    case SIDE_FRONT:
        draw_BSPNode_recursive( win, tree, node->front, campos, vbuf);
        draw_BSPFace(win, node, vbuf);
        draw_BSPNode_recursive(win, tree, node->back, campos, vbuf);
        break;
    case SIDE_MID: // if MID, doesn't really matter what side we draw first
    case SIDE_BACK:
        draw_BSPNode_recursive(win, tree, node->back, campos, vbuf);
        draw_BSPFace(win, node, vbuf);
        draw_BSPNode_recursive(win, tree, node->front, campos, vbuf);
        break;
    default:
        dibassert(false);
    }
}

static uint16_t g_depth;
static void draw_BSPFace(Window *win, BSPNode *node, TransformedVertex *vbuf) {
    BSPFace *bspface;
    Face *face;
    Face const *gen_face;
    
    for (size_t i_f = node->i_first_face; i_f < node->i_first_face + node->num_faces; i_f++) {
        bspface  = g_world.geo_bsp->bsp_faces + i_f;
        face     = g_world.geo_bsp->faces + i_f;
        gen_face = g_world.geo_bsp->gen_faces + bspface->i_gen_face;
        g_depth  = bspface->depth;

        if (face->flags & FC_DEGEN) {
            // TODO: degenerate triangles should be discarded long before this.
            continue;
        }


        dibassert(gen_face->brushface);
        if (((BrushQuadFace *)gen_face->brushface)->flags & FACEFLAG_SELECTED) {
            face->flags |= FC_SELECTED;
        }
        else {
            face->flags &= ~FC_SELECTED;
            if (g_selected_face_only)
                continue;
        }
        // TODO: This is only for when changing the texture during
        // editing. Handle it more gracefully/elsewhere.
        face->i_tex = gen_face->i_tex;

        
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
            tface.vbri[0] = face->vbri[0];//g_world.geo_bsp->vbris[i_v0];
            tface.vbri[1] = face->vbri[1];//g_world.geo_bsp->vbris[i_v1];
            tface.vbri[2] = face->vbri[2];//g_world.geo_bsp->vbris[i_v2];
            if (g_settings.BSP_visualizer) {
                draw_face_BSP_visualizer(win, face, &tface);
            }
            else {
                draw_face(win, face, &tface);
            }
        }
        else {
            clip_trans_triangulate_and_draw_face(win, face, vbuf);
        }
    }
}

void draw_BSPNode_wireframe(Window *win, BSPTree *tree, BSPNode *node, WorldVec campos, TransformedVertex *vbuf) {
    if (node == NULL) {
        return;
    }

    uint8_t side = sideof_BSPNode(campos, node);
    
    switch (side) {
    case SIDE_FRONT:
        draw_BSPNode_wireframe(win, tree, node->back, campos, vbuf);
        draw_BSPFace_wireframe(win, node, vbuf, color_brush);
        draw_BSPNode_wireframe(win, tree, node->front, campos, vbuf);
        break;
    case SIDE_MID: // if MID, doesn't really matter what side we draw first
    case SIDE_BACK:
        draw_BSPNode_wireframe(win, tree, node->front, campos, vbuf);
        draw_BSPFace_wireframe(win, node, vbuf, color_brush);
        draw_BSPNode_wireframe(win, tree, node->back, campos, vbuf);
        break;
    default:
        dibassert(false);
    }

    return;
}

static void draw_BSPFace_wireframe
(Window *win,
 BSPNode *node,
 TransformedVertex *vbuf, zgl_Color color) {
    BSPFace *bspface;
    Face *face;
    //    Face const *gen_face;
    
    for (size_t i_f = node->i_first_face; i_f < node->i_first_face + node->num_faces; i_f++) {
        bspface = g_world.geo_bsp->bsp_faces + i_f;
        face    = g_world.geo_bsp->faces + i_f;
        //        gen_face = g_world.geo_bsp->gen_faces + bspface->i_gen_face;
        g_depth = bspface->depth;
    
        if (face->flags & FC_DEGEN) {
            return;
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
        else {
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
                                        FC_MONOGRAM2, 0xFFFFFF);
                }
            }
        }
    }
}

void draw_face_BSP_visualizer(Window *win, Face *face, TransformedFace *tf) {
    uint8_t saved_rasterflags = g_rasterflags;
    g_rasterflags &= ~RASTERFLAG_TEXEL;
    g_rasterflags &= ~RASTERFLAG_VBRI;
    g_rasterflags &= ~RASTERFLAG_FBRI;
    
    DepthPixel
        scr_V0 = tf->tv[0].pixel,
        scr_V1 = tf->tv[1].pixel,
        scr_V2 = tf->tv[2].pixel;
    
    if (area(scr_V0.XY, scr_V1.XY, scr_V2.XY) <= 0) {
        goto End;
    }

    extern zgl_Color Normal_to_Color(WorldVec normal);
    
    if ( ! (tf->tv[0].vis | tf->tv[1].vis | tf->tv[2].vis)) { // no clipping needed
        ScreenTri screen_tri = {
            .vpxl = {&scr_V0, &scr_V1, &scr_V2},
            .vtxl = {&face->vtxl[0], &face->vtxl[1], &face->vtxl[2]},
            .vbri = {tf->vbri[0], tf->vbri[1], tf->vbri[2]},
            .fbri = face->fbri,
            .ftex = textures[face->i_tex].pixarr,
            .fclr = Normal_to_Color(face->normal)}; //GRAY(g_depth*8)};

        if (face->flags & FC_SELECTED) {
            screen_tri.fbri = 255;
        }

        draw_Triangle(win, &screen_tri);
    }
    else { // clipping needed
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
            subtri.fbri = face->fbri;
            
            Fix64 Z_denom;
            Fix64
                Z0 = scr_V0.z,
                Z1 = scr_V1.z,
                Z2 = scr_V2.z;
            Fix32Vec_2D
                TV0 = face->vtxl[0],
                TV1 = face->vtxl[1],
                TV2 = face->vtxl[2];

            // depth & texel 0
            B0 = ABS(area(subV0.XY, scr_V1.XY, scr_V2.XY))/area;
            B1 = ABS(area(subV0.XY, scr_V0.XY, scr_V2.XY))/area;
            B2 = (1<<16) - B0 - B1;
            Z_denom = ((B0<<16)/Z0 + (B1<<16)/Z1 + (B2<<16)/Z2);
            subV0.z  = ((B0+B1+B2)<<16)/Z_denom;
            subTV0.x = (Fix32)((B0*FIX_DIV(TV0.x, Z0) +
                                  B1*FIX_DIV(TV1.x, Z1) +
                                  B2*FIX_DIV(TV2.x, Z2))/Z_denom);
            subTV0.y = (Fix32)((B0*FIX_DIV(TV0.y, Z0) +
                                  B1*FIX_DIV(TV1.y, Z1) +
                                  B2*FIX_DIV(TV2.y, Z2))/Z_denom);

            // depth & texel 1
            B0 = ABS(area(subV1.XY, scr_V1.XY, scr_V2.XY))/area;
            B1 = ABS(area(subV1.XY, scr_V0.XY, scr_V2.XY))/area;
            B2 = (1<<16) - B0 - B1;
            Z_denom = ((B0<<16)/Z0 + (B1<<16)/Z1 + (B2<<16)/Z2);
            subV1.z  = ((B0+B1+B2)<<16)/Z_denom;
            subTV1.x = (Fix32)((B0*FIX_DIV(TV0.x, Z0) +
                                  B1*FIX_DIV(TV1.x, Z1) +
                                  B2*FIX_DIV(TV2.x, Z2))/Z_denom);
            subTV1.y = (Fix32)((B0*FIX_DIV(TV0.y, Z0) +
                                  B1*FIX_DIV(TV1.y, Z1) +
                                  B2*FIX_DIV(TV2.y, Z2))/Z_denom);

            // depth & texel 2
            B0 = ABS(area(subV2.XY, scr_V1.XY, scr_V2.XY))/area;
            B1 = ABS(area(subV2.XY, scr_V0.XY, scr_V2.XY))/area;
            B2 = (1<<16) - B0 - B1;
            Z_denom = ((B0<<16)/Z0 + (B1<<16)/Z1 + (B2<<16)/Z2);
            subV2.z  = ((B0+B1+B2)<<16)/Z_denom;
            subTV2.x = (Fix32)((B0*FIX_DIV(TV0.x, Z0) +
                                  B1*FIX_DIV(TV1.x, Z1) +
                                  B2*FIX_DIV(TV2.x, Z2))/Z_denom);
            subTV2.y = (Fix32)((B0*FIX_DIV(TV0.y, Z0) +
                                  B1*FIX_DIV(TV1.y, Z1) +
                                  B2*FIX_DIV(TV2.y, Z2))/Z_denom);

            subtri.vtxl[0] = &subTV0;
            subtri.vtxl[1] = &subTV1;
            subtri.vtxl[2] = &subTV2;
            subtri.vpxl[0] = &subV0;
            subtri.vpxl[1] = &subV1;
            subtri.vpxl[2] = &subV2;

            subtri.fclr = Normal_to_Color(face->normal);//GRAY(g_depth*8);
            draw_Triangle(win, &subtri);
        }
    }

 End:
    g_rasterflags = saved_rasterflags;
}

static Frustum *g_fr;
static Window *g_win;
static WorldVec g_vertex;
static zgl_mPixel g_click_pt;
extern void epm_TransformVertex
(Window const *win, Frustum *fr,
 WorldVec v,
 TransformedVertex *out_tv);
#define VERTEX_PICKING_DIST fixify(12)
bool test_face_vertices(BSPNode *node) {
    WorldVec v;
    TransformedVertex tv;
    size_t i_f = 0;
    Face const *gen_face = g_world.geo_bsp->gen_faces + (g_world.geo_bsp->bsp_faces + node->i_first_face + i_f)->i_gen_face;
        
    v = g_world.geo_prebsp->vertices[gen_face->i_v[0]];
    epm_TransformVertex(g_win, g_fr, v, &tv);
    if ((tv.vis == 0) &&
        (ABS(tv.pixel.XY.x - g_click_pt.x) < VERTEX_PICKING_DIST &&
         ABS(tv.pixel.XY.y - g_click_pt.y) < VERTEX_PICKING_DIST)) {
        g_vertex = v;
        return true;
    }

    
    v = g_world.geo_prebsp->vertices[gen_face->i_v[1]];
    epm_TransformVertex(g_win, g_fr, v, &tv);
    if ((tv.vis == 0) &&
        (ABS(tv.pixel.XY.x - g_click_pt.x) < VERTEX_PICKING_DIST &&
         ABS(tv.pixel.XY.y - g_click_pt.y) < VERTEX_PICKING_DIST)) {
        g_vertex = v;
        return true;
    }

    
    v = g_world.geo_prebsp->vertices[gen_face->i_v[2]];
    epm_TransformVertex(g_win, g_fr, v, &tv);
    if ((tv.vis == 0) &&
        (ABS(tv.pixel.XY.x - g_click_pt.x) < VERTEX_PICKING_DIST &&
         ABS(tv.pixel.XY.y - g_click_pt.y) < VERTEX_PICKING_DIST)) {
        g_vertex = v;
        return true;
    }
    
    return false;
}

bool test_node_vertices(BSPNode *node) {
    if (node == NULL) return false;
    
    uint8_t side = sideof_BSPNode((WorldVec){{tf.x, tf.y, tf.z}}, node);

    switch (side) {
    case SIDE_FRONT:
        if (test_node_vertices(node->front)) return true;
        if (test_face_vertices(node))        return true;
        if (test_node_vertices(node->back))  return true;
        break;
    case SIDE_MID: // if MID, doesn't really matter what side we do first
    case SIDE_BACK:
        if (test_node_vertices(node->back))  return true;
        if (test_face_vertices(node))        return true;
        if (test_node_vertices(node->front)) return true;
        break;
    default:
        dibassert(false);
    }

    return false;
}

WorldVec *vertex_below(Window *win, zgl_Pixel pixel, Frustum *fr) {
    // traverse BSP tree front-to-back, testing all three vertices of each face until one is found within a box
    g_click_pt = mpixel_from_pixel(pixel);
    g_win = win;
    g_fr = fr;
    static WorldVec res;
    
    if (test_node_vertices(&g_world.geo_bsp->nodes[0])) {
        puts("Found!");
        sect_ring[curr_sect] = g_vertex;
        curr_sect = (curr_sect+1) & (MAX_SECTS-1);

        res = g_vertex;
        return &res;
    }

    puts("Not found!");
    return NULL;
}



BSPNode *g_sect_node;
BSPFace *g_sect_bspface;
WorldVec g_sect_point;

BSPFace *BSPFace_below(Window *win, zgl_Pixel pixel, Frustum *fr) {
    zgl_mPixel scr_pt = mpixel_from_pixel(pixel);
    Fix32Vec rayDir;
    Fix64 tmp;
    WorldVec rayV0;
    WorldVec rayV1;
    
    if (g_settings.proj_mode == PROJ_PERSPECTIVE) {
        // TODO: Diagram. The ray starts at the world transform point and points
        // in a direction based on screen position.
        Fix64Vec tip;
        
        x_of(tip) = (1<<16) - 2*FIX_DIV(scr_pt.x - win->mrect.x, win->mrect.w);
        y_of(tip) = (1<<16) - 2*FIX_DIV(scr_pt.y - win->mrect.y, win->mrect.h);
        z_of(tip) = fr->D;
    
        x_of(tip) = FIX_MUL(x_of(tip), fr->W);
        y_of(tip) = FIX_MUL(y_of(tip), fr->H);
    
        tmp = -FIX_MUL(y_of(tip), tf.vcos) + FIX_MUL(z_of(tip), tf.vsin);
        x_of(rayDir) = (Fix32)(-FIX_MUL(x_of(tip), tf.hsin) + FIX_MUL(tmp, tf.hcos));
        y_of(rayDir) = (Fix32)(+FIX_MUL(x_of(tip), tf.hcos) + FIX_MUL(tmp, tf.hsin));
        z_of(rayDir) = (Fix32)(+FIX_MUL(y_of(tip), tf.vsin) + FIX_MUL(z_of(tip), tf.vcos));
    
        x_of(rayV0) = tf.x;
        y_of(rayV0) = tf.y;
        z_of(rayV0) = tf.z;
    
        x_of(rayV1) = x_of(rayV0) + x_of(rayDir);
        y_of(rayV1) = y_of(rayV0) + y_of(rayDir);
        z_of(rayV1) = z_of(rayV0) + z_of(rayDir);
    }
    else {
        // TODO: Diagram. The ray starts at a point on the the zmin face of the
        // frustrum based on screen position, and points the same direction as
        // the world transform.
        rayDir = tf.dir;

        Fix64Vec root;
        x_of(root) = (1<<16) - 2*FIX_DIV(scr_pt.x - win->mrect.x, win->mrect.w);
        y_of(root) = (1<<16) - 2*FIX_DIV(scr_pt.y - win->mrect.y, win->mrect.h);
        z_of(root) = fr->zmin;

        x_of(root) = FIX_MUL(x_of(root), fr->W);
        y_of(root) = FIX_MUL(y_of(root), fr->H);

        tmp =         -FIX_MUL(y_of(root), tf.vcos) + FIX_MUL(z_of(root), tf.vsin);
        x_of(rayV0) = (Fix32)(-FIX_MUL(x_of(root), tf.hsin) + FIX_MUL(tmp, tf.hcos));
        y_of(rayV0) = (Fix32)(+FIX_MUL(x_of(root), tf.hcos) + FIX_MUL(tmp, tf.hsin));
        z_of(rayV0) = (Fix32)(+FIX_MUL(y_of(root), tf.vsin) + FIX_MUL(z_of(root), tf.vcos));

        x_of(rayV0) += tf.x;
        y_of(rayV0) += tf.y;
        z_of(rayV0) += tf.z;
        
        x_of(rayV1) = x_of(rayV0) + x_of(rayDir);
        y_of(rayV1) = y_of(rayV0) + y_of(rayDir);
        z_of(rayV1) = z_of(rayV0) + z_of(rayDir);
    }
    
    g_sect_node = NULL;
    if (test_BSPNode(&g_world.geo_bsp->nodes[0], rayDir, rayV0, rayV1)) {
        sect_ring[curr_sect] = g_sect_point;
        curr_sect = (curr_sect + 1) & (MAX_SECTS - 1);
        
        return g_sect_bspface;
    }
    
    return NULL;
}


static bool test_BSPNode
(BSPNode *node, Fix32Vec rayDir, WorldVec rayV0, WorldVec rayV1) {
    if (node == NULL) return false;

    uint8_t side = sideof_BSPNode((WorldVec){{tf.x, tf.y, tf.z}}, node);

    switch (side) {
    case SIDE_FRONT:
        if (test_BSPNode(node->front, rayDir, rayV0, rayV1)) return true;
        if (test_BSPFace(node, rayDir, rayV0, rayV1))        return true;
        if (test_BSPNode(node->back, rayDir, rayV0, rayV1))  return true;
        break;
    case SIDE_MID: // if MID, doesn't really matter what side we do first
    case SIDE_BACK:
        if (test_BSPNode(node->back, rayDir, rayV0, rayV1))  return true;
        if (test_BSPFace(node, rayDir, rayV0, rayV1))        return true;
        if (test_BSPNode(node->front, rayDir, rayV0, rayV1)) return true;
        break;
    default:
        dibassert(false);
    }

    return false;
}

static bool test_BSPFace
(BSPNode *node, Fix32Vec rayDir, WorldVec rayV0, WorldVec rayV1) {
    WorldVec planeV = node->splitV;
    WorldVec planeN = node->splitN;

    Fix64 denom = dot(rayDir, planeN);

    WorldVec ray_to_plane;
    x_of(ray_to_plane) = x_of(planeV) - x_of(rayV0);
    y_of(ray_to_plane) = y_of(planeV) - y_of(rayV0);
    z_of(ray_to_plane) = z_of(planeV) - z_of(rayV0);
    Fix64 numer = dot(ray_to_plane, planeN);

    //printf("N = %lf\nD = %lf\n", FIX_dbl(N), FIX_dbl(D));

    /*
    if (numer >= 0) { // reject
        //printf("face %zu: N >= 0; viewpoint behind or coincident with one-sided plane\n", i_face);
        return false;
    }
    */
    
    if (denom == 0) { // reject
        //printf("face %zu: D = 0; view line parallel\n", i_face);
        return false;
    }
    
    Fix64 T = FIX_DIV(numer, denom);

    if (T <= 1<<16) { // reject
        //printf("face %zu: t<=1; intersection point not visible\n", i_face);
        return false;
    }

    WorldVec intersection;
    x_of(intersection) = x_of(rayV0) + (Fix32)FIX_MUL(T, x_of(rayDir));
    y_of(intersection) = y_of(rayV0) + (Fix32)FIX_MUL(T, y_of(rayDir));
    z_of(intersection) = z_of(rayV0) + (Fix32)FIX_MUL(T, z_of(rayDir));
    
    //printf("face %zu: Distance: %lf\n", i_face,
    //       FIX_dbl(dist));

    for (size_t i_f = node->i_first_face; i_f < node->i_first_face + node->num_faces; i_f++) {
        BSPFace *bspface = g_world.geo_bsp->bsp_faces + i_f;
        Face *f = g_world.geo_bsp->faces + i_f;

        WorldVec
            triV0 = g_world.geo_bsp->vertices[f->i_v[0]],
            triV1 = g_world.geo_bsp->vertices[f->i_v[1]],
            triV2 = g_world.geo_bsp->vertices[f->i_v[2]];

        /* IDEA: I could be clever and simply compare normals. Since all faces
           in this loop are coplanar, we only care about which of two possible
           directions the normal points in. */
        if (sideof_plane(rayV0, triV0, f->normal) == SIDE_BACK) {
            continue;
        }

        bool hit = point_in_tri(intersection, triV0, triV1, triV2);

        if (hit) {
            g_sect_point = intersection;
            g_sect_bspface = bspface;
            g_sect_node = node;
            return true;
        } 
    }
        
    return false;



    //////////////////////////////////////////////////////////////////////////

    /*
    for (size_t i_f = 0; i_f < node->num_bsp_faces; i_f++) {
        BSPFace *bspface = node->bsp_faces + i_f;
        Face *f = &bspface->face;

        WorldVec
            triV0 = world.bsp->vertices[f->i_v0],
            triV1 = world.bsp->vertices[f->i_v1],
            triV2 = world.bsp->vertices[f->i_v2];

        Fix64 denom = dot(rayDir, f->normal);

        WorldVec ray_to_plane;
        x_of(ray_to_plane) = x_of(triV0) - x_of(rayV0);
        y_of(ray_to_plane) = y_of(triV0) - y_of(rayV0);
        z_of(ray_to_plane) = z_of(triV0) - z_of(rayV0);
        Fix64 numer = dot(ray_to_plane, f->normal);

        //printf("N = %lf\nD = %lf\n", FIX_dbl(N), FIX_dbl(D));

        if (numer >= 0) { // reject
            //printf("face %zu: N >= 0; viewpoint behind or coincident with one-sided plane\n", i_face);
            continue;
        }
    
        if (denom == 0) { // reject
            //printf("face %zu: D = 0; view line parallel\n", i_face);
            continue;
        }
    
        Fix64 T = FIX_DIV(numer, denom);

        if (T <= 1<<16) { // reject
            //printf("face %zu: t<=1; intersection point not visible\n", i_face);
            continue;
        }

        WorldVec intersection;
        x_of(intersection) = x_of(rayV0) + (Fix32)FIX_MUL(T, x_of(rayDir));
        y_of(intersection) = y_of(rayV0) + (Fix32)FIX_MUL(T, y_of(rayDir));
        z_of(intersection) = z_of(rayV0) + (Fix32)FIX_MUL(T, z_of(rayDir));
    
        //printf("face %zu: Distance: %lf\n", i_face,
        //       FIX_dbl(dist));


        bool hit = point_in_tri(intersection, triV0, triV1, triV2);

        if (hit) {
            g_sect_point = intersection;
            g_sect_bspface = bspface;
            g_sect_node = node;
            return true;
        } 
    }


    return false;
    */
}
