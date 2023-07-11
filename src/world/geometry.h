#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "src/misc/epm_includes.h"
#include "src/draw/draw.h"

#include "src/draw/textures.h"
// The "wit", derived from "World unIT", is the basic unit of distance, much like
// the "tic" is the basic unit of time.

typedef Fix32       WorldUnit;
typedef Fix32Vec    WorldVec;
typedef Fix32Seg    WorldSeg;
typedef Fix32Line   WorldLine;
typedef Fix32Sphere WorldSphere;
typedef Fix32Rect   WorldRect;
typedef Fix32Plane  WorldPlane;

typedef Fix64       WorldUnit64;
typedef Fix64Vec    WorldVec64;
typedef Fix64Seg    WorldSeg64;
typedef Fix64Line   WorldLine64;
typedef Fix64Sphere WorldSphere64;
typedef Fix64Rect   WorldRect64;
typedef Fix64Plane  WorldPlane64;

// P0-P1
#define diff(P0, P1) ((WorldVec){{x_of((P0))-x_of((P1)), y_of((P0))-y_of((P1)), z_of((P0))-z_of((P1))}})

//typedef       Fix32 Wit;
//typedef    Fix32Vec WitPoint;
//typedef    Fix32Seg WitSeg;
//typedef Fix32Sphere WitSphere;
//typedef   Fix32Rect WitRect;
//typedef   Fix32Line WitLine;
//typedef  Fix32Plane WitPlane;

typedef Fix64 Matrix4[4][4];
typedef Fix64 Vector3[3];
typedef Fix64 Vector4[4];

typedef struct Triangle {
    WorldVec v0, v1, v2;
} Triangle;

extern bool is_anticlockwise(struct Fix32Polygon_2D poly);

extern bool clip_seg_to_polygon
(Fix32Seg seg,
 Fix32Polygon_2D poly,
 Fix32Seg *clipped_seg);

extern bool clip_seg_to_rect
(Fix32Seg line,
 Fix32Rect rect,
 Fix32Seg *clipped_line);

extern bool cohen_sutherland
(Fix32 x0, Fix32 y0,
 Fix32 x1, Fix32 y1,
 Fix32 *cx0, Fix32 *cy0,
 Fix32 *cx1, Fix32 *cy1,
 Fix32 xmin, Fix32 xmax,
 Fix32 ymin, Fix32 ymax);

extern UFix32 dist_Euclidean(Fix32Vec u, Fix32Vec v);

extern UFix32 norm_Euclidean(Fix32Vec u);
extern UFix32 norm_L1(Fix32Vec u);
extern UFix32 norm_approx13(Fix32Vec u);
extern UFix32 norm_approx9(Fix32Vec u);
extern UFix32 norm_approx8(Fix32Vec u);

extern UFix32 norm2D_Euclidean(Fix32Vec_2D u);
extern UFix32 norm2D_L1(Fix32Vec_2D u);
extern UFix32 norm2D_Doom(Fix32Vec_2D u);
extern UFix32 norm2D_flipcode(Fix32Vec_2D u);
extern UFix32 norm2D_flipcode2(Fix32Vec_2D u);

extern Fix32Vec midpoint(Fix32Vec pt0, Fix32Vec pt1);


extern Fix64 dot(Fix32Vec u, Fix32Vec v);
extern Fix32 signdist(Fix32Vec pt, Fix32Plane plane);

extern Fix32 dot2D(Fix32Vec_2D u, Fix32Vec_2D v);
extern Fix32 perp2D(Fix32Vec_2D u, Fix32Vec_2D v);
extern Fix32 signdist2D(Fix32Vec_2D pt, Fix32Line_2D line);

extern Fix32Vec_2D closest_point_2D(Fix32Vec_2D point, Fix32Seg_2D seg);
extern bool plane_normal(Fix32Vec *out_N, Fix32Vec *P, Fix32Vec *Q, Fix32Vec *R);
extern Fix64 triangle_area_3D(Fix64Vec P, Fix64Vec Q, Fix64Vec R);

extern Fix32Vec_2D rotate_Fix32Vec_2D(Fix32Vec_2D pt, Ang18 ang);

extern bool point_in_tri(WorldVec pt, WorldVec V0, WorldVec V1, WorldVec V2);

#define SIDE_FRONT 0x01
#define SIDE_MID   0x02
#define SIDE_BACK  0x04
extern uint8_t sideof_tri(WorldVec P, Triangle tri);
extern uint8_t sideof_plane(WorldVec V, WorldVec planeV, WorldVec planeN);

/*
#if defined FIX32_GEOMETRY_AS_MACRO
# if defined FIX32_GEOMETRY_VIA_FIX64
#  define dot(u, v) ((Fix32)(((((Fix64)(u).x) * ((Fix64)(v).x)) + (((Fix64)(u).y) * ((Fix64)(v).y)))>>FIX32_BITS))
#  define perp(u, v) ((Fix32)(((((Fix64)(u).x) * ((Fix64)(v).y)) - (((Fix64)(u).y) * ((Fix64)(v).x)))>>FIX32_BITS))
# else // FIX32_GEOMETRY_VIA_FIX64 UNDEFINED
#  define dot(u, v)  (FIX32_MUL((u).x, (v).x) + FIX32_MUL((u).y, (v).y))
#  define perp(u, v) (FIX32_MUL((u).x, (v).y) - FIX32_MUL((u).y, (v).x))
# endif // FIX32_GEOMETRY_VIA_FIX64
#else // FIX32_GEOMETRY_AS_MACRO UNDEFINED
# if defined FIX32_GEOMETRY_VIA_FIX64
#  define dot(u, v)  _dot_FIX32_via_FIX64((u), (v))
#  define perp(u, v) _perp_FIX32_via_FIX64((u), (v))
# else // FIX32_GEOMETRY_VIA_FIX64 UNDEFINED
#  define  dot(u, v)  dot((u), (v))
#  define perp(u, v) perp((u), (v))
# endif
#endif
*/



/*
extern int normalize
(Fix32Vec vec,
 Fix32Vec *normalized);
extern int normalize_secret_float
(Fix32Vec vec,
 Fix32Vec *normalized);
*/

/*
extern int set_length
(Fix32Vec vec,
 Fix32 length,
 Fix32Vec *normalized);
extern int set_length_secret_flaot
(Fix32Vec vec,
 Fix32 length,
 Fix32Vec *normalized);
*/

/*
extern Fix32 x_intercept
(Fix32Seg seg);

extern Fix32 y_intercept
(Fix32Seg seg);

extern Fix32 z_intercept
(Fix32Seg seg);
*/

/*
extern Fix32LineExact seg_to_line_exact
(Fix32Seg seg,
 Fix32LineExact *line);
*/

/*
extern Fix32Line seg_to_line
(Fix32Seg const *seg,
 Fix32Line *line);
*/

/*
extern Fix32Vec midpoint
(Fix32Vec pt0,
 Fix32Vec pt1,
 Fix32Vec *mid);
*/

/*
extern int32_t is_front
(Fix32Vec pt,
 Fix32Seg seg);
*/

/*
extern Fix32Vec compute_line_normal
(Fix32Seg seg,
 Fix32Vec *normal);

extern Fix32 compute_line_constant
(Fix32Vec origin,
 Fix32Vec normal,
 Fix32 *constant);
*/

/*
extern Fix32Vec closest_point
(Fix32Vec point, 
 Fix32Seg seg);
*/


/* Intersection functions */

extern bool overlap_Rect_2D(const Fix32Rect_2D *r1, const Fix32Rect_2D *r2);
extern bool overlap_Rect(const Fix32Rect *r1, const Fix32Rect *r2);
extern void Mat4Mul(Vector4 *out_vec, Matrix4 mat, Vector4 vec);

#define FC_DEGEN 0x01

typedef struct Vertex Vertex;
typedef struct Edge Edge;
typedef struct Face Face;
typedef struct QuadFace QuadFace;

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

typedef uint32_t VP_Index;
typedef uint32_t E_Index;
typedef uint32_t F_Index;
typedef uint32_t VT_Index;
typedef uint32_t VN_Index;

typedef struct EditorPolyData {
    uint8_t edflags;
    void *brushface; // back pointer to close the Brush-Tri-BSP cycle    
} EditorPolyData;

struct Poly {
    // authoring data
    void *brushface;

    // vertex attributes
    size_t num_v;
    size_t *i_v;
    Fix32Vec_2D vtxl;
    uint8_t vbri;
    
    // face attributes
    WorldVec normal;
    size_t i_tex;
    uint8_t fbri;
    
    uint8_t flags;
};

struct Poly3 {
    uint8_t polyflags;
    
    size_t i_v0, i_v1, i_v2;
    
    WorldVec normal;
    uint8_t brightness;
    
    size_t i_tex;
    zgl_mPixel tx0, tx1, tx2;

    EditorPolyData ed;
};

struct Poly4 {
    uint8_t polyflags;
    
    size_t i_v0, i_v1, i_v2, i_v3;
    
    WorldVec normal;
    uint8_t brightness;

    size_t i_tex;
    zgl_mPixel tx0, tx1, tx2, tx3;

    EditorPolyData ed;
};

#define FC_SELECTED 0x02
#define FC_SKY 0x04
struct Face {
    // authoring data
    void *brushface;

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

struct NonIndexedFace {
    void *brushface;
    
    WorldVec v0, v1, v2;
    Fix32Vec_2D vt0, vt1, vt2;
    WorldVec normal;
    
    size_t i_tex;
    
    uint8_t flags;
    uint8_t brightness;
};

struct IndexedFace {
    void *brushface;
    
    uint32_t i_v0, i_v1, i_v2;
    uint32_t i_vt0, i_vt1, i_vt2;
    uint32_t i_fn;

    size_t i_tex;
    
    uint8_t brightness;
    uint8_t flags;
};

struct QuadFace {
    size_t i_v0, i_v1, i_v2, i_v3;
    
    uint8_t flags;
    WorldVec normal;
    uint8_t brightness;
    size_t i_tex;

    Fix32Vec_2D tv0, tv1, tv2, tv3;
    void *brushface;
};

struct Vertex {
    size_t i_v;
};

struct Edge {
    size_t i_v0, i_v1;
};

typedef struct EdgeSet {
    size_t num_vertices;
    WorldVec *vertices;
    
    size_t num_edges;
    Edge *edges;

    zgl_Color wirecolor;
} EdgeSet;

typedef struct FaceSet {
    size_t num_vertices;
    WorldVec *vertices;
    
    size_t num_faces;
    Face *faces;

    zgl_Color wirecolor;
} FaceSet;

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

#endif /* GEOMETRY_H */
