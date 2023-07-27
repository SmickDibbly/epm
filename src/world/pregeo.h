#ifndef PREGEO_H
#define PREGEO_H

#include "src/misc/epm_includes.h"
#include "src/world/brush.h"

typedef struct BrushVertIndex {
    Brush *brush;
    uint32_t i_v;
} BrushVertIndex;

typedef struct PreVertExt {
    uint32_t i_bsp_vert;
    
    uint32_t num_brush_verts;
    BrushVertIndex brush_verts[8];

    uint8_t preflags;
} PreVertExt;

typedef struct PreFaceExt {
    uint32_t num_bsp_faces;
    uint32_t i_bsp_faces[8];
    
    Brush *brush;
    uint32_t i_brush_poly;
    
    uint8_t preflags;
} PreFaceExt;

typedef struct PreGeometry {
    size_t num_verts;
    WorldVec *verts;
    PreVertExt *vert_exts;
    
    size_t num_edges;
    Edge *edges;
    //PreEdgeExt *const edge_exts;

    size_t num_faces;
    Face *faces;
    PreFaceExt *face_exts;
} PreGeometry;
epm_Result reset_PreGeometry(void);

#endif
