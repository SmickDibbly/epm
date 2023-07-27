#ifndef ELEMENTS_H
#define ELEMENTS_H

#include "src/misc/epm_includes.h"
#include "src/world/geometry.h"

#define FC_DEGEN 0x01

typedef struct Vertex Vertex;
typedef struct Edge Edge;

typedef struct IndexedVertex IndexedVertex;
typedef struct IndexedEdge IndexedEdge;
typedef struct IndexedFace IndexedFace;

typedef struct NonIndexedVertex NonIndexedVertex;
typedef struct NonIndexedEdge NonIndexedEdge;
typedef struct NonIndexedFace NonIndexedFace;

typedef struct LinkedVertex LinkedVertex;
typedef struct LinkedEdge LinkedEdge;
typedef struct LinkedFace LinkedFace;

typedef struct Poly3 Poly3;
typedef struct Poly4 Poly4;
typedef struct Poly Poly;

typedef struct Poly3 Face;
typedef struct Poly3 Tri;
typedef struct Poly4 QuadFace;
typedef struct Poly4 Quad;

typedef uint32_t VP_Index;
typedef uint32_t E_Index;
typedef uint32_t F_Index;
typedef uint32_t VT_Index;
typedef uint32_t VN_Index;

typedef struct EditorPolyData {
    uint8_t edflags;
    void *brushpoly; // back pointer to close the Brush-Tri-BSP cycle    
} EditorPolyData;

struct Poly {
    // vertex attributes
    size_t num_v;
    size_t *vind;
    Fix32Vec_2D *vtxl;
    uint8_t *vbri;
    
    // face attributes
    WorldVec normal;
    size_t i_tex;
    uint8_t fbri;
    
    uint8_t flags;
};

struct Poly3 {
    // vertex attributes
    size_t i_v[3];
    Fix32Vec_2D vtxl[3];
    uint8_t vbri[3];
    
    // face attributes
    WorldVec normal;
    size_t i_tex;
    uint8_t fbri;
    
    uint8_t flags;
};

struct Poly4 {
    // vertex attributes
    size_t i_v[4];
    Fix32Vec_2D vtxl[4];
    uint8_t vbri[4];
    
    // face attributes
    WorldVec normal;
    size_t i_tex;
    uint8_t fbri;
    
    uint8_t flags;
};

#define FC_SELECTED 0x02
#define FC_SKY 0x04
struct NonIndexedFace {
    void *brushpoly;
    
    WorldVec v0, v1, v2;
    Fix32Vec_2D vt0, vt1, vt2;
    WorldVec normal;
    
    size_t i_tex;
    
    uint8_t flags;
    uint8_t brightness;
};

struct IndexedFace {
    void *brushpoly;
    
    uint32_t i_v0, i_v1, i_v2;
    uint32_t i_vt0, i_vt1, i_vt2;
    uint32_t i_fn;

    size_t i_tex;
    
    uint8_t brightness;
    uint8_t flags;
};

struct Vertex {
    size_t i_v;
};

struct Edge {
    size_t i_v0, i_v1;
};

typedef struct EdgeSet {
    size_t num_verts;
    WorldVec *verts;
    
    size_t num_edges;
    Edge *edges;

    zgl_Color wirecolor;
} EdgeSet;

#define EdgeSet_of(STRUCT)                      \
    ((EdgeSet){                                 \
        .num_verts = (STRUCT).num_verts,        \
        .verts = (STRUCT).verts,                \
        .num_edges = (STRUCT).num_edges,        \
        .edges = (STRUCT).edges                 \
    })

typedef struct FaceSet {
    size_t num_verts;
    WorldVec *verts;
    
    size_t num_faces;
    Face *faces;

    zgl_Color wirecolor;
} FaceSet;

#define FaceSet_of(STRUCT)                      \
    ((FaceSet){                                 \
        .num_verts = (STRUCT).num_verts,        \
        .verts = (STRUCT).verts,                \
        .num_faces = (STRUCT).num_faces,        \
        .faces = (STRUCT).faces                 \
    })

/* Linked Elements */

struct LinkedVertex {
    size_t i_v;
    
    size_t *i_linked_edges;
    size_t *i_linked_faces;
};

struct LinkedEdge {
    size_t i_v0, i_v1;
    
    size_t *i_linked_faces;
};

struct LinkedFace {
    size_t i_v0, i_v1, i_v2;
};


extern void epm_ComputeFaceNormal(WorldVec vertices[], Face *f);
extern void epm_ComputeFaceNormals(WorldVec vertices[], size_t num_faces, Face faces[]);

extern void epm_ComputeFaceBrightness(Face *face);
extern void epm_ComputeFaceBrightnesses(size_t num_faces, Face faces[]);

extern size_t epm_CountEdgesFromPolys(size_t num_polys, Poly polys[]);
extern epm_Result epm_ComputeEdgesFromPolys
(size_t num_polys, Poly polys[],
 size_t *out_num_edges, Edge *(out_edges[]));

extern size_t epm_CountEdgesFromFaces(size_t num_faces, Face faces[]);
extern epm_Result epm_ComputeEdgesFromFaces
(size_t num_faces, Face faces[],
 size_t *out_num_edges, Edge *(out_edges[]));


/*
extern void epm_ComputeVertexBrightnessesNEW
(size_t num_vertices, WorldVec const vertices[],
 size_t num_lights, Light const lights[],
 size_t num_faces, Face faces[]);

extern void epm_ComputeVertexBrightness
(WorldVec vertex,
 size_t num_lights, Light const lights[],
 uint8_t *vbri);
extern void epm_ComputeVertexBrightnesses
(size_t num_vertices, WorldVec const vertices[],
 size_t num_lights, Light const lights[],
 size_t num_faces, Face faces[]);
*/

#endif
