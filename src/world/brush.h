#ifndef BRUSH_H
#define BRUSH_H

#include "src/misc/epm_includes.h"
#include "src/world/geometry.h"
#include "src/world/elements.h"
#include "zigil/zigil.h"
#include "src/draw/draw.h"

/* -------------------------------------------------------------------------- */
// Brush faces

// __________________       ______________________       _______________
// |  BrushVertExt  |       |  PreVertExt        |       |  BSPVertExt |
// | .i_pre_vert----------->| .i_bsp_vert--------------->|             |
// |                |<--------.brush_verts[].i_v |<--------.i_pre_vert |
// |________________|       |____________________|       |_____________|

// __________________       __________________       _______________
// |  BrushEdgeExt  |       |  PreEdgeExt    |       |  BSPEdgeExt |
// |               -------->|               -------->|             |
// |                |<--------               |<--------            |
// |________________|       |________________|       |_____________|

// __________________       __________________       _______________
// |  BrushPolyExt  |       |  PreFaceExt    |       |  BSPFaceExt |
// | .i_pre_faces[]-------->| .i_bsp_faces[]-------->|             |
// |                |<--------.i_brush_poly  |<--------.i_pre_face |
// |________________|       |________________|       |_____________|



typedef struct BrushPolyExt BrushPolyExt;
typedef struct BrushEdgeExt BrushEdgeExt;
typedef struct BrushVertExt BrushVertExt;

#define VERTFLAG_SELECTED 0x01
struct BrushVertExt {
    uint32_t i_pre_vert;
    uint8_t brushflags;
};

struct BrushEdgeExt {
    uint32_t i_pre_edge;
    uint8_t brushflags;
};

#define FACEFLAG_SELECTED 0x01
struct BrushPolyExt {
    uint32_t num_pre_faces;
    uint32_t i_pre_faces[8];
    uint8_t brushflags;
};

typedef struct BrushPoly4 {
    uint32_t brushflags;

    Poly4 poly4;

    Face *subface0, *subface1;
} BrushPoly4;

typedef struct BrushPoly3 {
    uint32_t brushflags;

    Poly3 poly3;

    Face *subface;
} BrushPoly3;


/* -------------------------------------------------------------------------- */
// Generic Brush Container

#define BRUSHFLAG_SELECTED 0x01
typedef struct Brush {
    uint8_t flags;

    char *name;
    
    int BSP;
    int CSG;
    WorldVec POR;
    zgl_Color wirecolor;

    size_t num_verts;
    WorldVec *verts;
    BrushVertExt *vert_exts;
    
    size_t num_edges;
    Edge *edges;
    //    BrushEdgeExt *edge_exts;
    
    size_t num_polys;
    Poly *polys;
    BrushPolyExt *poly_exts;
} Brush;


/* -------------------------------------------------------------------------- */
// Specific Brush Geometry

#define NUM_VERTICES_CUBOID 8
#define NUM_EDGES_CUBOID    12
#define NUM_POLYS_CUBOID    6
#define NUM_QUADS_CUBOID    6
#define NUM_TRIS_CUBOID     12

extern Brush *create_cuboid_brush(WorldVec v, WorldVec dv);
extern void destroy_brush(Brush *brush);

/* -------------------------------------------------------------------------- */
// The Brush Frame

#define BRUSHFLAG_SELECTED 0x01
typedef struct FrameBrush {
    uint8_t flags;

    char *name;
    
    WorldVec POR;
    //    zgl_Color wirecolor;

    size_t num_verts;
    WorldVec *verts;
    WorldVec *prebsp_vertex;

    size_t num_edges;
    Edge *edges;
    
    size_t num_polys;
    Poly *polys;
    BrushPolyExt *poly_exts;
} FrameBrush;

extern Brush *set_frame_cuboid(WorldVec v, WorldVec dv);
extern Brush *set_frame_cylinder(WorldVec v, WorldUnit radius, size_t num_sides);
extern Brush *set_frame_pyramid(WorldVec v, WorldUnit base, WorldUnit height);

extern Brush *g_frame;

/* -------------------------------------------------------------------------- */
// The linked list of all brushes.

typedef struct BrushNode BrushNode;
struct BrushNode {
    BrushNode *next;
    BrushNode *prev;
    
    Brush *brush;
};
typedef struct BrushGeometry { //geometry as given by a level designer.
    BrushNode *head;
    BrushNode *tail;
} BrushGeometry;

extern void unlink_brush(Brush *brush);
extern void link_brush(Brush *brush);
extern epm_Result reset_BrushGeometry(void);
extern epm_Result triangulate_world(void);

extern void print_Brush(Brush *brush);

#endif /* BRUSH_H */
