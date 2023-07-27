#include "src/world/brush.h"
#include "src/world/world.h"
#include "src/draw/textures.h"
#include "src/draw/colors.h"

//#define VERBOSITY
#include "zigil/diblib_local/verbosity.h"

#undef LOG_LABEL
#define LOG_LABEL "BRUSH"

#define FP_256 (256<<16)
static WorldVec TEMPLATE_CuboidBrush_vertices[NUM_VERTICES_CUBOID] = {
    [0] = {{     0,      0,      0}},
    [1] = {{FP_256,      0,      0}},
    [2] = {{FP_256, FP_256,      0}},
    [3] = {{     0, FP_256,      0}},
    [4] = {{     0,      0, FP_256}},
    [5] = {{FP_256,      0, FP_256}},
    [6] = {{FP_256, FP_256, FP_256}},
    [7] = {{     0, FP_256, FP_256}},
};

static Edge TEMPLATE_CuboidBrush_edges[NUM_EDGES_CUBOID] = {
    [0] = {0, 1},
    [1] = {1, 2},
    [2] = {2, 3},
    [3] = {3, 0},
    [4] = {7, 6},
    [5] = {6, 5},
    [6] = {5, 4},
    [7] = {4, 7},
    [8] = {0, 4},
    [9] = {5, 1},
    [10] = {2, 6},
    [11] = {7, 3},
};

static size_t TEMPLATE_CuboidBrush_polys_vind[NUM_POLYS_CUBOID][4] = {
    [0] = {0, 1, 2, 3},
    [1] = {7, 6, 5, 4},
    [2] = {1, 0, 4, 5},
    [3] = {3, 2, 6, 7},
    [4] = {0, 3, 7, 4},
    [5] = {2, 1, 5, 6},
};

static Fix32Vec_2D TEMPLATE_CuboidBrush_polys_vtxl[4] = {
    [0] = {          0, (256<<16)-1},
    [1] = {(256<<16)-1, (256<<16)-1},
    [2] = {(256<<16)-1,           0},
    [3] = {          0,           0},
};

static WorldVec const TEMPLATE_CuboidBrush_normal[NUM_POLYS_CUBOID] = {
    [0] = {{ 0x00000000,  0x00000000,  0x00010000}},
    [1] = {{ 0x00000000,  0x00000000, -0x00010000}},
    [2] = {{ 0x00000000,  0x00010000,  0x00000000}},
    [3] = {{ 0x00000000, -0x00010000,  0x00000000}},
    [4] = {{ 0x00010000,  0x00000000,  0x00000000}},
    [5] = {{-0x00010000,  0x00000000,  0x00000000}},
};

static uint8_t TEMPLATE_CuboidBrush_polys_vbri[4] = {255, 255, 255, 255};

static Poly TEMPLATE_CuboidBrush_polys[NUM_POLYS_CUBOID] = {
    [0] = {
        .flags = 0,
            
        .normal = TEMPLATE_CuboidBrush_normal[0],
        .i_tex = 0,
        .fbri = 255,

        .num_v = 4,
        .vind = TEMPLATE_CuboidBrush_polys_vind[0],
        .vtxl = TEMPLATE_CuboidBrush_polys_vtxl,
        .vbri = TEMPLATE_CuboidBrush_polys_vbri,
    },

    [1] = {
        .flags = 0,
            
        .normal = TEMPLATE_CuboidBrush_normal[1],
        .i_tex = 0,
        .fbri = 255,

        .num_v = 4,
        .vind = TEMPLATE_CuboidBrush_polys_vind[1],
        .vtxl = TEMPLATE_CuboidBrush_polys_vtxl,
        .vbri = TEMPLATE_CuboidBrush_polys_vbri,
    },

    [2] = {
        .flags = 0,
            
        .normal = TEMPLATE_CuboidBrush_normal[2],
        .i_tex = 0,
        .fbri = 255,

        .num_v = 4,
        .vind = TEMPLATE_CuboidBrush_polys_vind[2],
        .vtxl = TEMPLATE_CuboidBrush_polys_vtxl,
        .vbri = TEMPLATE_CuboidBrush_polys_vbri,

    },

    [3] = {
        .flags = 0,
            
        .normal = TEMPLATE_CuboidBrush_normal[3],
        .i_tex = 0,
        .fbri = 255,

        .num_v = 4,
        .vind = TEMPLATE_CuboidBrush_polys_vind[3],
        .vtxl = TEMPLATE_CuboidBrush_polys_vtxl,
        .vbri = TEMPLATE_CuboidBrush_polys_vbri,
    },

    [4] = {
        .flags = 0,
            
        .normal = TEMPLATE_CuboidBrush_normal[4],
        .i_tex = 0,
        .fbri = 255,

        .num_v = 4,
        .vind = TEMPLATE_CuboidBrush_polys_vind[4],
        .vtxl = TEMPLATE_CuboidBrush_polys_vtxl,
        .vbri = TEMPLATE_CuboidBrush_polys_vbri,
    },

    [5] = {
        .flags = 0,
            
        .normal = TEMPLATE_CuboidBrush_normal[5],
        .i_tex = 0,
        .fbri = 255,

        .num_v = 4,
        .vind = TEMPLATE_CuboidBrush_polys_vind[5],
        .vtxl = TEMPLATE_CuboidBrush_polys_vtxl,
        .vbri = TEMPLATE_CuboidBrush_polys_vbri,

    },
};

static Brush const TEMPLATE_CuboidBrush = {
    .flags = 0,
    .name = NULL,
    .BSP = -1,
    .CSG = -1,
    .POR = {{0,0,0}},
    .wirecolor = 0xFFFFFF,

    .num_verts = NUM_VERTICES_CUBOID,
    .verts = TEMPLATE_CuboidBrush_vertices,
    .num_edges = NUM_EDGES_CUBOID,
    .edges = TEMPLATE_CuboidBrush_edges,
    .num_polys = NUM_POLYS_CUBOID,
    .polys = TEMPLATE_CuboidBrush_polys,
};

Brush *create_cuboid_brush(WorldVec v, WorldVec dv) {
    WorldUnit x = x_of(v);
    WorldUnit y = y_of(v);
    WorldUnit z = z_of(v);
    WorldUnit dx = x_of(dv);
    WorldUnit dy = y_of(dv);
    WorldUnit dz = z_of(dv);
    
    Brush *brush = zgl_Malloc(sizeof(*brush));

    *brush = (Brush){
        .flags = TEMPLATE_CuboidBrush.flags,
        .name = TEMPLATE_CuboidBrush.name,
        .BSP = TEMPLATE_CuboidBrush.BSP,
        .CSG = TEMPLATE_CuboidBrush.CSG,
        .POR = v,
        .wirecolor = TEMPLATE_CuboidBrush.wirecolor,
        .num_verts = TEMPLATE_CuboidBrush.num_verts,
        .num_edges = TEMPLATE_CuboidBrush.num_edges,
        .num_polys = TEMPLATE_CuboidBrush.num_polys
    };

    brush->verts     = zgl_Calloc(brush->num_verts, sizeof(*brush->verts));
    brush->vert_exts = zgl_Calloc(brush->num_verts, sizeof(*brush->vert_exts));
    WorldVec *vertices = brush->verts;
    vertices[0] = (WorldVec){{x   , y   , z   }};
    vertices[1] = (WorldVec){{x+dx, y   , z   }};
    vertices[2] = (WorldVec){{x+dx, y+dy, z   }};
    vertices[3] = (WorldVec){{x   , y+dy, z   }};
    vertices[4] = (WorldVec){{x   , y   , z+dz}};
    vertices[5] = (WorldVec){{x+dx, y   , z+dz}};
    vertices[6] = (WorldVec){{x+dx, y+dy, z+dz}};
    vertices[7] = (WorldVec){{x   , y+dy, z+dz}};

    brush->edges     = zgl_Calloc(brush->num_edges, sizeof(*brush->edges));
    //    brush->edge_exts = zgl_Calloc(brush->num_edges, sizeof(*brush->edge_exts));
    Edge *edges = brush->edges;
    edges[0]  = TEMPLATE_CuboidBrush_edges[0];
    edges[1]  = TEMPLATE_CuboidBrush_edges[1];
    edges[2]  = TEMPLATE_CuboidBrush_edges[2];
    edges[3]  = TEMPLATE_CuboidBrush_edges[3];
    edges[4]  = TEMPLATE_CuboidBrush_edges[4];
    edges[5]  = TEMPLATE_CuboidBrush_edges[5];
    edges[6]  = TEMPLATE_CuboidBrush_edges[6];
    edges[7]  = TEMPLATE_CuboidBrush_edges[7];
    edges[8]  = TEMPLATE_CuboidBrush_edges[8];
    edges[9]  = TEMPLATE_CuboidBrush_edges[9];
    edges[10] = TEMPLATE_CuboidBrush_edges[10];
    edges[11] = TEMPLATE_CuboidBrush_edges[11];
    
    
    brush->polys     = zgl_Calloc(brush->num_polys, sizeof(*brush->polys));
    brush->poly_exts = zgl_Calloc(brush->num_polys, sizeof(*brush->poly_exts));

    for (size_t i = 0; i < brush->num_polys; i++) {        
        Poly *poly = brush->polys + i;
        poly->flags = TEMPLATE_CuboidBrush_polys[i].flags;

        poly->normal = TEMPLATE_CuboidBrush_polys[i].normal;
        poly->i_tex = TEMPLATE_CuboidBrush_polys[i].i_tex;
        poly->fbri = TEMPLATE_CuboidBrush_polys[i].fbri;

        poly->num_v = TEMPLATE_CuboidBrush_polys[i].num_v;
        poly->vind = zgl_Malloc(poly->num_v*sizeof(*poly->vind));
        poly->vtxl = zgl_Malloc(poly->num_v*sizeof(*poly->vtxl));
        poly->vbri = zgl_Malloc(poly->num_v*sizeof(*poly->vbri));

        poly->vind[0] = TEMPLATE_CuboidBrush_polys_vind[i][0];
        poly->vind[1] = TEMPLATE_CuboidBrush_polys_vind[i][1];
        poly->vind[2] = TEMPLATE_CuboidBrush_polys_vind[i][2];
        poly->vind[3] = TEMPLATE_CuboidBrush_polys_vind[i][3];
        poly->vtxl[0] = TEMPLATE_CuboidBrush_polys_vtxl[0];
        poly->vtxl[1] = TEMPLATE_CuboidBrush_polys_vtxl[1];
        poly->vtxl[2] = TEMPLATE_CuboidBrush_polys_vtxl[2];
        poly->vtxl[3] = TEMPLATE_CuboidBrush_polys_vtxl[3];
        poly->vbri[0] = TEMPLATE_CuboidBrush_polys_vbri[0];
        poly->vbri[1] = TEMPLATE_CuboidBrush_polys_vbri[1];
        poly->vbri[2] = TEMPLATE_CuboidBrush_polys_vbri[2];
        poly->vbri[3] = TEMPLATE_CuboidBrush_polys_vbri[3];
    }

       
    return brush;
}


#include "src/world/selection.h"

Brush *set_frame_cuboid(WorldVec v, WorldVec dv) {
    BrushSel_clear();
    BrushSel_del(g_frame); // TODO: double check if needed
    
    destroy_brush(g_frame);
    g_frame = NULL;
    
    g_frame = create_cuboid_brush(v, dv);

    return g_frame;
}
