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
size_t curr_sect = 0;

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

extern void draw_BSPTree
(BSPTree *tree,
 Window *win,
 WorldVec campos,
 TransformedVertex *vbuf);
static void draw_BSPTreeRecursive
(Window *win,
 BSPTree *tree,
 BSPNode *node,
 WorldVec campos,
 TransformedVertex *vbuf);
static void draw_BSPNodeFaces
(Window *win,
 BSPNode *node,
 TransformedVertex *vbuf);



extern void draw_BSPTreeWireframe
(Window *win,
 BSPTree *tree,
 BSPNode *node,
 WorldVec campos,
 TransformedVertex *vbuf);
static void draw_BSPNodeFacesWireframe
(Window *win,
 BSPNode *node,
 TransformedVertex *vbuf, zgl_Color color);

extern BSPFaceExt *BSPFace_below
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
    }
    else if (g_settings.proj_mode == PROJ_ORTHOGRAPHIC) {
        WorldVec N = node->splitN;
        
        // rotate
        Fix64 S = FIX_MUL(x_of(N), tf.hcos) + FIX_MUL(y_of(N), tf.hsin);
        S = (Fix32)(+FIX_MUL(S, tf.vsin) + FIX_MUL(z_of(N), tf.vcos));
        
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
                draw_BSPNodeFaces(win, node, vbuf);
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


static void draw_BSPTreeRecursive(Window *win, BSPTree *tree, BSPNode *node, WorldVec campos, TransformedVertex *vbuf) {
    if (node == NULL) return;
    
    uint8_t side = sideof_BSPNode(campos, node);
    
    switch (side) {
    case SIDE_FRONT:
        draw_BSPTreeRecursive( win, tree, node->front, campos, vbuf);
        draw_BSPNodeFaces(win, node, vbuf);
        draw_BSPTreeRecursive(win, tree, node->back, campos, vbuf);
        break;
    case SIDE_MID: // if MID, doesn't really matter what side we draw first
    case SIDE_BACK:
        draw_BSPTreeRecursive(win, tree, node->back, campos, vbuf);
        draw_BSPNodeFaces(win, node, vbuf);
        draw_BSPTreeRecursive(win, tree, node->front, campos, vbuf);
        break;
    default:
        dibassert(false);
    }
}

static uint16_t g_depth;
static void draw_BSPNodeFaces(Window *win, BSPNode *node, TransformedVertex *vbuf) {
    Face *bsp_f;
    BSPFaceExt *bsp_fX;
    
    for (size_t i_bsp_f = node->i_first_face; i_bsp_f < node->i_first_face + node->num_faces; i_bsp_f++) {
        bsp_f    = g_world.geo_bsp->faces     + i_bsp_f;
        bsp_fX   = g_world.geo_bsp->face_exts + i_bsp_f;
        g_depth  = bsp_fX->depth;
        
        if (bsp_f->flags & FC_DEGEN) {
            // TODO: degenerate triangles should be discarded long before this.
            continue;
        }

        /*
        if (bsp_f->flags & FC_SELECTED) {
            puts("SELECTED");
        }
        */  
        
        /*
        Face *pre_f;
        PreFaceExt *pre_fX;
        size_t i_pre_f = bsp_fX->i_pre_face;
        pre_f     = g_world.geo_prebsp->faces + i_pre_f;
        pre_fX    = g_world.geo_prebsp->face_exts + i_pre_f;

        if (pre_fX->brush->poly_exts[pre_fX->i_brush_poly].brushflags & FACEFLAG_SELECTED) {
            bsp_f->flags |= FC_SELECTED;
        }
        else {
            bsp_f->flags &= ~FC_SELECTED;
            if (g_selected_face_only)
                continue;
        }
        
        bsp_f->i_tex = pre_f->i_tex;
        */
        
        size_t
            i_v0 = bsp_f->i_v[0],
            i_v1 = bsp_f->i_v[1],
            i_v2 = bsp_f->i_v[2];
    
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
            tface.vbri[0] = bsp_f->vbri[0];//g_world.geo_bsp->vbris[i_v0];
            tface.vbri[1] = bsp_f->vbri[1];//g_world.geo_bsp->vbris[i_v1];
            tface.vbri[2] = bsp_f->vbri[2];//g_world.geo_bsp->vbris[i_v2];
            
            epm_DrawFace(win, bsp_f, &tface);
        }
        else {
            clip_trans_triangulate_and_draw_face(win, bsp_f, vbuf);
        }
    }
}

void draw_BSPTreeWireframe(Window *win, BSPTree *tree, BSPNode *node, WorldVec campos, TransformedVertex *vbuf) {
    if (node == NULL) {
        return;
    }

    uint8_t side = sideof_BSPNode(campos, node);
    
    switch (side) {
    case SIDE_FRONT:
        draw_BSPTreeWireframe(win, tree, node->back, campos, vbuf);
        draw_BSPNodeFacesWireframe(win, node, vbuf, color_brush);
        draw_BSPTreeWireframe(win, tree, node->front, campos, vbuf);
        break;
    case SIDE_MID: // if MID, doesn't really matter what side we draw first
    case SIDE_BACK:
        draw_BSPTreeWireframe(win, tree, node->front, campos, vbuf);
        draw_BSPNodeFacesWireframe(win, node, vbuf, color_brush);
        draw_BSPTreeWireframe(win, tree, node->back, campos, vbuf);
        break;
    default:
        dibassert(false);
    }

    return;
}

static void draw_BSPNodeFacesWireframe
(Window *win,
 BSPNode *node,
 TransformedVertex *vbuf, zgl_Color color) {
    BSPFaceExt *bspface;
    Face *face;
    //    Face const *gen_face;
    
    for (size_t i_f = node->i_first_face; i_f < node->i_first_face + node->num_faces; i_f++) {
        bspface = g_world.geo_bsp->face_exts + i_f;
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
            
            epm_DrawWireframeFace(win, &twface, color);
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
                
                epm_DrawWireframeFace(win, &twf, color);
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
                
                    epm_DrawWireframeFace(win, &twf, color);
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
                
                epm_DrawWireframeFace(win, &twf, color);
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

zgl_Color set_fclr_bsp_depth(Face *face) {
    return GRAY(g_depth*8);
}

BSPNode *g_sect_node;
BSPFaceExt *g_sect_bspface;
WorldVec g_sect_point;

BSPFaceExt *BSPFace_below(Window *win, zgl_Pixel pixel, Frustum *fr) {
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
        BSPFaceExt *bspface = g_world.geo_bsp->face_exts + i_f;
        Face *f = g_world.geo_bsp->faces + i_f;

        WorldVec
            triV0 = g_world.geo_bsp->verts[f->i_v[0]],
            triV1 = g_world.geo_bsp->verts[f->i_v[1]],
            triV2 = g_world.geo_bsp->verts[f->i_v[2]];

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
