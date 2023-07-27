#include "src/world/brush.h"
#include "src/world/world.h"
#include "src/draw/textures.h"

//#define VERBOSITY
#include "zigil/diblib_local/verbosity.h"

#undef LOG_LABEL
#define LOG_LABEL "BRUSH"

Brush *g_frame;

void destroy_brush(Brush *brush) {
    // TODO: Need free internal stuff too.
    zgl_Free(brush->verts);
    zgl_Free(brush->vert_exts);

    zgl_Free(brush->edges);
    //    zgl_Free(brush->edge_exts);

    for (size_t i = 0; i < brush->num_polys; i++) {
        zgl_Free(brush->polys[i].vind);
        zgl_Free(brush->polys[i].vtxl);
        zgl_Free(brush->polys[i].vbri);
    }    
    zgl_Free(brush->polys);
    zgl_Free(brush->poly_exts);
    
    zgl_Free(brush);
}

void unlink_brush(Brush *brush) {
    bool found = false;
    BrushNode *node;
    for (node = g_world.geo_brush->head; node; node = node->next) {
        if (node->brush == brush) {
            found = true;
            break;
        }
    }

    if (found) {
        BrushNode *prev = node->prev;
        BrushNode *next = node->next;
        
        if (prev) {
            prev->next = next;
        }
        if (next) {
            next->prev = prev;
        }

        node->prev = node->next = NULL;

        if (node == g_world.geo_brush->head) {
            g_world.geo_brush->head = next;
        }
        if (node == g_world.geo_brush->tail) {
            g_world.geo_brush->tail = prev;
        }
    }
}

void link_brush(Brush *brush) {
    BrushNode *node;

    if (g_world.geo_brush->head == NULL) {
        dibassert(g_world.geo_brush->tail == NULL);
        node = g_world.geo_brush->head = zgl_Malloc(sizeof(*node));
        node->next = node->prev = NULL;
        g_world.geo_brush->tail = node;
    }
    else {
        dibassert(g_world.geo_brush->tail != NULL);
        dibassert(g_world.geo_brush->tail->next == NULL);
        node = g_world.geo_brush->tail->next = zgl_Malloc(sizeof(*node));
        node->next = NULL;
        node->prev = g_world.geo_brush->tail;
        g_world.geo_brush->tail = node;
    }

    node->brush = brush;
}

Brush *duplicate_Brush(Brush *brush) {
    Brush *dup = zgl_Malloc(sizeof(*dup));

    *dup = *brush; // we only care about flags, BSP, CSG, POR, wirecolor, and nums
    dup->verts            = zgl_Malloc(dup->num_verts*sizeof(*dup->verts));
    memcpy(dup->verts, brush->verts,   dup->num_verts*sizeof(*dup->verts));
    dup->vert_exts        = zgl_Calloc(dup->num_verts, sizeof(*dup->vert_exts));
    // no copy
    
    dup->edges          = zgl_Malloc(dup->num_edges*sizeof(*dup->edges));
    memcpy(dup->edges, brush->edges, dup->num_edges*sizeof(*dup->edges));
    //    dup->edge_exts      = zgl_Calloc(dup->num_edges, sizeof(*dup->edge_exts));
    // no copy
    
    dup->polys          = zgl_Malloc(dup->num_polys*sizeof(*dup->polys));
    memcpy(dup->polys, brush->polys, dup->num_polys*sizeof(*dup->polys));
    dup->poly_exts      = zgl_Calloc(dup->num_polys, sizeof(*dup->poly_exts));

    for (size_t i_poly = 0; i_poly < brush->num_polys; i_poly++) {
        Poly *from = brush->polys + i_poly;
        //BrushPolyExt *fromX = brush->poly_exts + i_poly;

        Poly *to = dup->polys + i_poly;

        to->vind = zgl_Malloc(to->num_v*sizeof(*to->vind));
        memcpy(to->vind, from->vind, to->num_v*sizeof(*to->vind));
        
        to->vtxl = zgl_Malloc(to->num_v*sizeof(*to->vtxl));
        memcpy(to->vtxl, from->vtxl, to->num_v*sizeof(*to->vtxl));
        
        to->vbri = zgl_Malloc(to->num_v*sizeof(*to->vbri));
        memcpy(to->vbri, from->vbri, to->num_v*sizeof(*to->vbri));
    }
    
    return dup;
}


void brush_from_frame(int CSG, int BSP, char *name) {
    static char *default_name = "Untitled Brush";
    
    Brush *brush = duplicate_Brush(g_frame);

    // Some properties we didn't actually want copied.
    brush->flags = 0;
    brush->name = name ? name : default_name;
    brush->BSP = BSP,
    brush->CSG = CSG;

    BrushNode *node;
    if (g_world.geo_brush->head == NULL) {
        dibassert(g_world.geo_brush->tail == NULL);
        node = g_world.geo_brush->head = zgl_Malloc(sizeof(BrushNode));
        node->next = NULL;
        node->prev = NULL;
        g_world.geo_brush->tail = node;
    }
    else {
        dibassert(g_world.geo_brush->tail != NULL);
        node = g_world.geo_brush->tail->next = zgl_Malloc(sizeof(BrushNode));
        node->next = NULL;
        node->prev = g_world.geo_brush->tail;
        g_world.geo_brush->tail = node;
    }

    node->brush = brush;

    print_Brush(brush);

}


#include "src/input/command.h"

static void CMDH_brush_from_frame(int argc, char **argv, char *output_str) {
    if (streq(argv[1], "sub")) {
        brush_from_frame(CSG_SUBTRACTIVE, BSP_DEFAULT, NULL);
    }
    else if (streq(argv[1], "add")) {
        brush_from_frame(CSG_ADDITIVE, BSP_DEFAULT, NULL);
    }
}

epm_Command const CMD_brush_from_frame = {
    .name = "brush_from_frame",
    .argc_min = 2,
    .argc_max = 2,
    .handler = CMDH_brush_from_frame,
};

static Fix32 read_fix_x(char const *str, Fix32 *out_num);

static void CMDH_set_frame_cuboid(int argc, char **argv, char *output_str) {
    WorldVec dv;
    read_fix_x(argv[1], &x_of(dv));
    read_fix_x(argv[2], &y_of(dv));
    read_fix_x(argv[3], &z_of(dv));
    
    set_frame_cuboid(g_frame->verts[0], dv);
}

static void CMDH_set_frame_point(int argc, char **argv, char *output_str) {
    if (argc < 3+1) return;

    WorldUnit x, y, z;
    read_fix_x(argv[1], &x);
    read_fix_x(argv[2], &y);
    read_fix_x(argv[3], &z);

    x_of(g_frame->POR) = x;
    y_of(g_frame->POR) = y;
    z_of(g_frame->POR) = z;
}

epm_Command const CMD_set_frame_point = {
    .name = "set_frame_point",
    .argc_min = 4,
    .argc_max = 4,
    .handler = CMDH_set_frame_point,
};

epm_Command const CMD_set_frame_cuboid = {
    .name = "set_frame_cuboid",
    .argc_min = 2,
    .argc_max = 16,
    .handler = CMDH_set_frame_cuboid,
};







static Fix32 read_fix_x(char const *str, Fix32 *out_num) {
    Fix32 res = 0;
    bool neg = false;
    size_t i = 0;
    
    if (str[i] == '-') {
        neg = true;
        i++;
    }

    bool point = false;
    
    while (true) {
        switch (str[i]) {
        case '.': // if no point in string, is integer literal, so will shift up
                  // by 4 bits at end.
            point = true;
            break;
        case '0':
            res <<= 4;
            res += 0x0;
            break;
        case '1':
            res <<= 4;
            res += 0x1;
            break;
        case '2':
            res <<= 4;
            res += 0x2;
            break;
        case '3':
            res <<= 4;
            res += 0x3;
            break;
        case '4':
            res <<= 4;
            res += 0x4;
            break;
        case '5':
            res <<= 4;
            res += 0x5;
            break;
        case '6':
            res <<= 4;
            res += 0x6;
            break;
        case '7':
            res <<= 4;
            res += 0x7;
            break;
        case '8':
            res <<= 4;
            res += 0x8;
            break;
        case '9':
            res <<= 4;
            res += 0x9;
            break;
        case 'A':
            res <<= 4;
            res += 0xA;
            break;
        case 'B':
            res <<= 4;
            res += 0xB;
            break;
        case 'C':
            res <<= 4;
            res += 0xC;
            break;
        case 'D':
            res <<= 4;
            res += 0xD;
            break;
        case 'E':
            res <<= 4;
            res += 0xE;
            break;
        case 'F':
            res <<= 4;
            res += 0xF;
            break;
        default:
            goto Done;
        }
        i++;
    }

 Done:
    if ( ! point)
        res = res<<4;
        
    if (neg)
        res = -res;

    *out_num = res;

    dibassert(i < INT_MAX);
    return (int)i;
}




static int32_t const zigzag_i_v[3][11] = {
    {0, -1, -1, -2, -2, -3, -3, -4, -4, -5, -5},
    {1,  0,  2, -1,  3, -2,  4, -3,  5, -4,  6},
    {2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7}
};


static void set_BrushVertExt(BrushVertExt *brush_vX, uint32_t i_pre_vert, uint8_t brushflags) {
    brush_vX->i_pre_vert = i_pre_vert;
    brush_vX->brushflags = brushflags;
}

static void add_BrushVert_to_PreVertExt(PreVertExt *pre_vX, Brush *brush, uint32_t i_v) {
    pre_vX->brush_verts[pre_vX->num_brush_verts].brush = brush;
    pre_vX->brush_verts[pre_vX->num_brush_verts].i_v = i_v;
    pre_vX->num_brush_verts++;
}

static void add_PreFace_to_BrushPolyExt(BrushPolyExt *brush_pX, uint32_t i_pre_face) {
    brush_pX->i_pre_faces[brush_pX->num_pre_faces] = i_pre_face;
    brush_pX->num_pre_faces++;
}

static void set_PreFaceExt(PreFaceExt *pre_fX, Brush *brush, uint32_t i_brush_poly) {
    pre_fX->brush = brush;
    pre_fX->i_brush_poly = i_brush_poly;
    pre_fX->preflags = 0;

    // no touch the num_bsp_faces nor i_bsp_faces[]
}



void triangulate_and_merge_Brush(Brush *brush) {
    PreGeometry *pre = g_world.geo_prebsp;
    size_t i_pre_vs[256] = {0};

    for (size_t i_brush_v = 0; i_brush_v < brush->num_verts; i_brush_v++) {
        WorldVec *brush_v = brush->verts + i_brush_v;
        BrushVertExt *brush_vX = brush->vert_exts + i_brush_v;
        bool new = true;
        
        for (uint32_t i_pre_v = 0; i_pre_v < pre->num_verts; i_pre_v++) {
            WorldVec *pre_v = pre->verts + i_pre_v;
            PreVertExt *pre_vX = pre->vert_exts + i_pre_v;
            if (eq_xyz(*pre_v, *brush_v)) {
                new = false;

                set_BrushVertExt(brush_vX, i_pre_v, 0);
                add_BrushVert_to_PreVertExt(pre_vX, brush, (uint32_t)i_brush_v);
                i_pre_vs[i_brush_v] = i_pre_v;
                break; // VERIFY
            }
        }

        if (new) {
            uint32_t i_pre_v = (uint32_t)(pre->num_verts++);
            WorldVec *pre_v = pre->verts + i_pre_v;
            PreVertExt *pre_vX = pre->vert_exts + i_pre_v;
            *pre_v = *brush_v;
            pre_vX->num_brush_verts = 0;
            set_BrushVertExt(brush_vX, i_pre_v, 0);
            add_BrushVert_to_PreVertExt(pre_vX, brush, (uint32_t)i_brush_v);
            i_pre_vs[i_brush_v] = i_pre_v;
        }
    }

    // TODO: Handle edges similarly to vertices and polys.


    //size_t tri_e_increase = 0;
    size_t tri_f_increase = 0;
    for (size_t i_poly = 0; i_poly < brush->num_polys; i_poly++) {
        // splitting an n-gon creates n-3 *new* edges and n-3 *new* faces
        //tri_e_increase += brush->polys[i_poly].poly.num_vertices - 3;
        tri_f_increase += brush->polys[i_poly].num_v - 3;
    }

    size_t num_tri_f = 0; // accumulator

    for (size_t i_brush_p = 0; i_brush_p < brush->num_polys; i_brush_p++) {
        Poly         *brush_p  = brush->polys + i_brush_p;
        BrushPolyExt *brush_pX = brush->poly_exts + i_brush_p;
        size_t num_v = brush_p->num_v;

        brush_pX->num_pre_faces = 0;
        for (size_t i_subface = 0; i_subface + 2 < num_v; i_subface++) {
            num_tri_f++;
            
            int32_t
                i_brush_v0 = zigzag_i_v[0][i_subface],
                i_brush_v1 = zigzag_i_v[1][i_subface],
                i_brush_v2 = zigzag_i_v[2][i_subface];
            
            i_brush_v0 = i_brush_v0 < 0 ? (int32_t)(i_brush_v0 + num_v) : i_brush_v0;
            i_brush_v1 = i_brush_v1 < 0 ? (int32_t)(i_brush_v1 + num_v) : i_brush_v1;
            
            if (brush->CSG == CSG_ADDITIVE) {
                int32_t tmp = i_brush_v0;
                i_brush_v0 = i_brush_v1;
                i_brush_v1 = tmp;
            }
            
            size_t i_pre_face = pre->num_faces++;
            Face *pre_face = pre->faces + i_pre_face;
            set_PreFaceExt(pre->face_exts + i_pre_face, brush, (uint32_t)i_brush_p);
            add_PreFace_to_BrushPolyExt(brush_pX, (uint32_t)i_pre_face);
            
            pre_face->flags  = brush_p->flags;
            pre_face->normal = brush_p->normal;
            pre_face->fbri   = brush_p->fbri;
            pre_face->i_tex  = brush_p->i_tex;
            pre_face->i_v[0] = i_pre_vs[brush_p->vind[i_brush_v0]];
            pre_face->i_v[1] = i_pre_vs[brush_p->vind[i_brush_v1]];
            pre_face->i_v[2] = i_pre_vs[brush_p->vind[i_brush_v2]];
            pre_face->vtxl[0] = brush_p->vtxl[i_brush_v0];
            pre_face->vtxl[1] = brush_p->vtxl[i_brush_v1];
            pre_face->vtxl[2] = brush_p->vtxl[i_brush_v2];
            pre_face->vbri[0] = brush_p->vbri[i_brush_v0];
            pre_face->vbri[1] = brush_p->vbri[i_brush_v1];
            pre_face->vbri[2] = brush_p->vbri[i_brush_v2];

            if (brush->CSG == CSG_ADDITIVE) {
                pre_face->normal = (WorldVec){{
                        -x_of(pre_face->normal),
                        -y_of(pre_face->normal),
                        -z_of(pre_face->normal)}};
            }

            // TEMP: compute brush poly normal since it may have changed
            epm_ComputeFaceNormal(pre->verts, pre_face);
        }
        dibassert(brush_pX->num_pre_faces == (uint32_t)(num_v - 2));
    }

    dibassert(num_tri_f == brush->num_polys + tri_f_increase);

    epm_ComputeFaceBrightnesses(num_tri_f, pre->faces + pre->num_faces - num_tri_f);
    // TODO: Verify this is the right offset.

    /*
    puts("BEFORE");
    print_Brush(brush);
    puts("AFTER");
    */
}

epm_Result triangulate_world(void) {
    g_world.geo_prebsp->num_verts = 0;
    g_world.geo_prebsp->num_edges = 0;
    g_world.geo_prebsp->num_faces = 0;
    
    for (BrushNode *node = g_world.geo_brush->head; node; node = node->next) {
        triangulate_and_merge_Brush(node->brush);
    }
    
    // TODO: The goal is to avoid calling the entire edge-from-face construction
    // by carefully preserving what edges are already known.
    Edge *tmp_edges;
    epm_ComputeEdgesFromFaces(g_world.geo_prebsp->num_faces, g_world.geo_prebsp->faces,
                              &g_world.geo_prebsp->num_edges, &tmp_edges);
    memcpy(g_world.geo_prebsp->edges, tmp_edges, g_world.geo_prebsp->num_edges*sizeof(Edge));
    zgl_Free(tmp_edges);

    //extern void print_StaticGeometry(void);
    //print_StaticGeometry();
    
    return EPM_SUCCESS;
}


#include <inttypes.h>
static void print_BrushMesh(Brush *brush) {
    for (size_t i_v = 0; i_v < brush->num_verts; i_v++) {
        printf("    v %s %s %s\n",
                fmt_fix_x(x_of(brush->verts[i_v]), 16),
                fmt_fix_x(y_of(brush->verts[i_v]), 16),
                fmt_fix_x(z_of(brush->verts[i_v]), 16));
    }

    for (size_t i_e = 0; i_e < brush->num_edges; i_e++) {
        printf("    e %zu %zu\n",
                brush->edges[i_e].i_v0,
                brush->edges[i_e].i_v1);
    }
    
    for (size_t i = 0; i < brush->num_polys; i++) {
        Poly *poly = brush->polys + i;
        printf("    begin poly\n");
        printf("        poly_flags %"PRIu8"\n", poly->flags);
        printf("        normal %s %s %s\n",
                fmt_fix_x(x_of(poly->normal), 16),
                fmt_fix_x(y_of(poly->normal), 16),
                fmt_fix_x(z_of(poly->normal), 16));
        printf("        i_tex %zu\n", poly->i_tex);
        printf("        fbri %"PRIu8"\n", poly->fbri);
        
        printf("        num_v %zu\n", poly->num_v);
        printf("        vind");
        for (size_t j = 0; j < poly->num_v; j++) {
            printf(" %zu", poly->vind[j]);
        }
        printf("\n");
        printf("        vtxl");
        for (size_t j = 0; j < poly->num_v; j++) {
            printf(" %s %s",
                    fmt_fix_x(poly->vtxl[j].x, 16),
                    fmt_fix_x(poly->vtxl[j].y, 16));
        }
        printf("\n");
        printf("        vbri");
        for (size_t j = 0; j < poly->num_v; j++) {
            printf(" %"PRIu8, poly->vbri[j]);
        }
        printf("\n");
        printf("    end poly\n");
    }
}

void print_Brush(Brush *brush) {
    puts("begin Brush");
    printf("  flags %x\n", brush->flags);
    printf("  BSP default\n");
    printf("  CSG %s\n", brush->CSG == CSG_SUBTRACTIVE ? "sub" : "add");
    printf("  POR %s %s %s\n",
            fmt_fix_x(x_of(brush->POR), 16),
            fmt_fix_x(y_of(brush->POR), 16),
            fmt_fix_x(z_of(brush->POR), 16));
    printf("  begin geo\n");
    print_BrushMesh(brush);
    printf("  end geo\n");
    printf("end Brush\n");
}





/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

// Brush face selection

#include "src/world/selection.h"

static bool brush_is_selected(void *obj) {
    Brush *brush = ((Brush *)obj);
    
    return (bool)(brush->flags & BRUSHFLAG_SELECTED);
}

static void brush_select(void *obj) {
    Brush *brush = ((Brush *)obj);
    
    brush->flags |= BRUSHFLAG_SELECTED;
}

static void brush_unselect(void *obj) {
    Brush *brush = ((Brush *)obj);
    
    brush->flags &= ~BRUSHFLAG_SELECTED;
}

Selection sel_brush = {
    .fn_is_selected = brush_is_selected,
    .fn_select = brush_select,
    .fn_unselect = brush_unselect,
    .num_selected = 0,
    .head = NULL,
    .tail = NULL,
    .nodes = {{0}}
};

void BrushSel_del(Brush *brush) {    
    sel_del(&sel_brush, brush);
}

void BrushSel_add(Brush *brush) {
    sel_add(&sel_brush, brush);
    
    brushsel_POR = brush->POR;
}

void BrushSel_toggle(Brush *brush) {
    sel_toggle(&sel_brush, brush);
}

void BrushSel_clear(void) {
    sel_clear(&sel_brush);
}

////////////////////////////////////////////////////////////////////////////////

typedef struct BrushVertEntry {
    Brush *brush;
    uint32_t i_v;
} BrushVertEntry;

static bool brushvert_is_selected(void *obj) {
    Brush *brush = ((BrushVertEntry *)obj)->brush;
    uint32_t i_v = ((BrushVertEntry *)obj)->i_v;
    
    return (bool)(brush->vert_exts[i_v].brushflags & VERTFLAG_SELECTED);
}

static void brushvert_select(void *obj) {
    Brush *brush = ((BrushVertEntry *)obj)->brush;
    uint32_t i_v = ((BrushVertEntry *)obj)->i_v;
    
    brush->vert_exts[i_v].brushflags |= VERTFLAG_SELECTED;
}

static void brushvert_unselect(void *obj) {
    Brush *brush = ((BrushVertEntry *)obj)->brush;
    uint32_t i_v = ((BrushVertEntry *)obj)->i_v;
    
    brush->vert_exts[i_v].brushflags &= ~VERTFLAG_SELECTED;
}

Selection sel_brushvert = {
    .fn_is_selected = brushvert_is_selected,
    .fn_select = brushvert_select,
    .fn_unselect = brushvert_unselect,
    .num_selected = 0,
    .head = NULL,
    .tail = NULL,
    .nodes = {{0}}
};

WorldVec brushsel_POR = {{0}};

////////////////////////////////////////////////////////////////////////////////

typedef struct BrushPolyEntry {
    Brush *brush;
    uint32_t i_p;
} BrushPolyEntry;

static bool brushpoly_is_selected(void *obj) {
    Brush *brush = ((BrushPolyEntry *)obj)->brush;
    uint32_t i_p = ((BrushPolyEntry *)obj)->i_p;

    BrushPolyExt *brush_pX = brush->poly_exts + i_p;
    
    return (bool)(brush_pX->brushflags & FACEFLAG_SELECTED);
}

static void brushpoly_select(void *obj) {
    Brush *brush = ((BrushPolyEntry *)obj)->brush;
    uint32_t i_p = ((BrushPolyEntry *)obj)->i_p;
    
    BrushPolyExt *brush_pX = brush->poly_exts + i_p;
    brush_pX->brushflags |= FACEFLAG_SELECTED;

    // propagate to pre-faces
    for (size_t i_i_pre_f = 0; i_i_pre_f < brush_pX->num_pre_faces; i_i_pre_f++) {
        int32_t i_pre_f = brush_pX->i_pre_faces[i_i_pre_f];

        PreFaceExt *pre_fX = g_world.geo_prebsp->face_exts + i_pre_f;
        pre_fX->preflags |= FACEFLAG_SELECTED;

        // propagate to bsp faces
        for (size_t i_i_bsp_f = 0; i_i_bsp_f < pre_fX->num_bsp_faces; i_i_bsp_f++) {
            int32_t i_bsp_f = pre_fX->i_bsp_faces[i_i_bsp_f];
            
            BSPFaceExt *bsp_fX = g_world.geo_bsp->face_exts + i_bsp_f;
            bsp_fX->bspflags |= FACEFLAG_SELECTED;
            
            Face *bsp_f = g_world.geo_bsp->faces + i_bsp_f;
            bsp_f->flags |= FC_SELECTED;


        }
    }
}

static void brushpoly_unselect(void *obj) {
    Brush *brush = ((BrushPolyEntry *)obj)->brush;
    uint32_t i_p = ((BrushPolyEntry *)obj)->i_p;
    
    BrushPolyExt *brush_pX = brush->poly_exts + i_p;
    brush_pX->brushflags &= ~FACEFLAG_SELECTED;

    // propagate to pre-faces
    for (size_t i_i_pre_f = 0; i_i_pre_f < brush_pX->num_pre_faces; i_i_pre_f++) {
        int32_t i_pre_f = brush_pX->i_pre_faces[i_i_pre_f];
        
        PreFaceExt *pre_fX = g_world.geo_prebsp->face_exts + i_pre_f;
        pre_fX->preflags &= ~FACEFLAG_SELECTED;

        // propagate to bsp faces
        for (size_t i_i_bsp_f = 0; i_i_bsp_f < pre_fX->num_bsp_faces; i_i_bsp_f++) {
            int32_t i_bsp_f = pre_fX->i_bsp_faces[i_i_bsp_f];
            
            BSPFaceExt *bsp_fX = g_world.geo_bsp->face_exts + i_bsp_f;
            bsp_fX->bspflags &= ~FACEFLAG_SELECTED;

            Face *bsp_f = g_world.geo_bsp->faces + i_bsp_f;
            bsp_f->flags &= ~FC_SELECTED;
        }
    }
}

Selection sel_brushpoly = {
    .fn_is_selected = brushpoly_is_selected,
    .fn_select = brushpoly_select,
    .fn_unselect = brushpoly_unselect,
    .num_selected = 0,
    .head = NULL,
    .tail = NULL,
    .nodes = {{0}}
};

// Some behind the scenes BrushPoly selection magic.
void BrushPolySel_del(Brush *brush, uint32_t i_p) {
    BrushPolyEntry entry;
    entry.brush = brush;
    entry.i_p = i_p;
    
    void *res = sel_del(&sel_brushpoly, &entry);
    if (res) zgl_Free(res);
}

void BrushPolySel_add(Brush *brush, uint32_t i_p) {
    BrushPolyEntry *entry = zgl_Malloc(sizeof(*entry));
    entry->brush = brush;
    entry->i_p = i_p;

    sel_add(&sel_brushpoly, entry);
}

void BrushPolySel_toggle(Brush *brush, uint32_t i_p) {
    BrushPolyEntry entry;
    entry.brush = brush;
    entry.i_p = i_p;
    
    if (brushpoly_is_selected(&entry)) {
        BrushPolySel_del(brush, i_p);
    }
    else {
        BrushPolySel_add(brush, i_p);
    }
}


// FIXME: Heckin memleaks!
void BrushPolySel_clear(void) {
    for (SelectionNode *node = sel_brushpoly.head; node; node = node->next) {
        BrushPolyEntry entry = *(BrushPolyEntry *)node->obj;
        BrushPolySel_del(entry.brush, entry.i_p);
    }

    sel_clear(&sel_brushpoly);
}

// Operations on brushpoly selection.

void apply_texture(char const *texname) {
    int32_t i_tex = epm_TextureIndexFromName(texname);
    
    if (i_tex != EPM_FAILURE) {
        for (SelectionNode *node = sel_brushpoly.head; node; node = node->next) {
            Brush *brush = ((BrushPolyEntry *)node->obj)->brush;
            uint32_t i_p = ((BrushPolyEntry *)node->obj)->i_p;

            Poly *brush_p = brush->polys + i_p;
            brush_p->i_tex = i_tex;

            BrushPolyExt *brush_pX = brush->poly_exts + i_p;

            // propagate to pre-faces
            for (size_t i_i_pre_f = 0; i_i_pre_f < brush_pX->num_pre_faces; i_i_pre_f++) {
                int32_t i_pre_f = brush_pX->i_pre_faces[i_i_pre_f];
                
                Face *pre_f = g_world.geo_prebsp->faces + i_pre_f;
                pre_f->i_tex = i_tex;
                
                PreFaceExt *pre_fX = g_world.geo_prebsp->face_exts + i_pre_f;

                // propagate to bsp faces
                for (size_t i_i_bsp_f = 0; i_i_bsp_f < pre_fX->num_bsp_faces; i_i_bsp_f++) {
                    int32_t i_bsp_f = pre_fX->i_bsp_faces[i_i_bsp_f];
                    
                    Face *bsp_f = g_world.geo_bsp->faces + i_bsp_f;
                    bsp_f->i_tex = i_tex;
                }
            }
        }
    }
}


#include "src/input/command.h"

static void CMDH_apply_texture(int argc, char **argv, char *output_str) {
    apply_texture(argv[1]);
}

epm_Command const CMD_apply_texture = {
    .name = "apply_texture",
    .argc_min = 2,
    .argc_max = 2,
    .handler = CMDH_apply_texture,
};


void select_all_brush_faces(void) {
    for (SelectionNode *node = sel_brushpoly.head; node; node = node->next) {
        Brush *brush = ((BrushPolyEntry *)node->obj)->brush;
        for (uint32_t i_p = 0; i_p < brush->num_polys; i_p++) {
            BrushPolySel_add(brush, i_p);
        }
    }
}

static void CMDH_select_all_brush_faces(int argc, char **argv, char *output_str) {
    for (SelectionNode *node = sel_brushpoly.head; node; node = node->next) {
        Brush *brush = ((BrushPolyEntry *)node->obj)->brush;
        for (uint32_t i_p = 0; i_p < brush->num_polys; i_p++) {
            BrushPolySel_add(brush, i_p);
        }
    }
}

epm_Command const CMD_select_all_brush_faces = {
    .name = "select_all_brush_faces",
    .argc_min = 1,
    .argc_max = 1,
    .handler = CMDH_select_all_brush_faces,
};







///////////////////////////////////


void BrushVertSel_del(Brush *brush, uint32_t i_v) {
    BrushVertEntry entry;
    entry.brush = brush;
    entry.i_v = i_v;
    
    void *res = sel_del(&sel_brushvert, &entry);
    if (res) zgl_Free(res);
}

void BrushVertSel_add(Brush *brush, uint32_t i_v) {
    BrushVertEntry *entry = zgl_Malloc(sizeof(*entry));
    entry->brush = brush;
    entry->i_v = i_v;

    sel_add(&sel_brushvert, entry);
}

void BrushVertSel_toggle(Brush *brush, uint32_t i_v) {
    BrushVertEntry entry;
    entry.brush = brush;
    entry.i_v = i_v;
    
    if (brushvert_is_selected(&entry)) {
        BrushVertSel_del(brush, i_v);
    }
    else {
        BrushVertSel_add(brush, i_v);
    }
}

void BrushVertSel_clear(void) {
    sel_clear(&sel_brushvert);
}
