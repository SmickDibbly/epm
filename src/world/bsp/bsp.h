#ifndef BSP_H
#define BSP_H

#include "src/misc/epm_includes.h"
#include "src/world/geometry.h"
#include "src/world/mesh.h"

// TODO: Allow the geometry-designer to suggest certain BSP splitting planes.

// Extra data tacked onto to a struct Face.
typedef struct BSPFace2 {
    Face face;
    Face const *gen_face;
    size_t i_gen_face;

    uint16_t bspflags;
    uint16_t depth;
} BSPFace2;

typedef struct BSPFace {
    uint32_t i_gen_face;    
    uint16_t bspflags;
    uint16_t depth;
} BSPFace;

typedef struct BSPNode2 BSPNode2;
struct BSPNode2 {
    BSPNode2 *parent; // TODO: Will this *ever* be used?
    BSPNode2 *front;
    BSPNode2 *back;
    
    WorldVec splitV; // point on splitting plane
    WorldVec splitN; // normal of splitting plane

    uint32_t num_bsp_faces;
    uint32_t i_bsp_faces;
};

// A BSP node should only contain pointers and metadata. Store the actual BSP
// face data in an array.
typedef struct BSPNode BSPNode;
struct BSPNode {
    BSPNode *parent; // TODO: Will this *ever* be used besides in bsp_finalization?
    BSPNode *front;
    BSPNode *back;
    
    WorldVec splitV; // point on splitting plane
    WorldVec splitN; // normal of splitting plane

    size_t num_faces;
    size_t i_first_face;
};

typedef struct BSPTree2 {
    uint32_t num_leaves;
    uint32_t num_cuts;
    
    uint32_t max_node_depth;
    double avg_node_depth;
    double avg_leaf_depth;
    double balance;
    
    uint32_t num_nodes;
    BSPNode *nodes;

    uint32_t num_vertices;
    WorldVec *vertices;
    uint8_t *vbris;
    
    uint32_t num_edges;
    Edge *edges;
    
    uint32_t num_faces;
    Face *faces;
    BSPFace *bsp_faces;

    /* The "progenitor" (or "gen" for short) face structure that the BSP Tree is
       based on is kept on hand. Much of the data about the subdivided BSP faces
       can be inherited from the progenitor, such as normal vector,
       ambient/directional brightness, and texture (but not u,v mapping or
       vertices) */
    uint32_t num_gen_vertices;
    WorldVec const *gen_vertices;
    uint8_t const *gen_vertex_intensities;
    
    uint32_t num_gen_faces;
    Face const *gen_faces; // pointer to originally-given face array.
} BSPTree2;



typedef struct BSPTree {
    size_t num_leaves;
    size_t num_cuts;
    
    size_t max_node_depth;
    double avg_node_depth;
    double avg_leaf_depth;
    double balance;
    
    size_t num_nodes;
    BSPNode *nodes;
    
    size_t num_vertices;
    WorldVec *vertices;
    //uint8_t *vbris;
    zgl_Color *vertex_colors; // green if og, red if from cut

    size_t num_edges;
    Edge *edges;
    zgl_Color *edge_colors; // green if og, red if from cut, blue if new

    size_t num_faces;
    Face *faces;
    BSPFace *bsp_faces;
    zgl_Color *face_colors; // green if og, red if from cut

    /* The "progenitor" (or "gen" for short) face structure that the BSP Tree is
       based on is kept on hand. Much of the data about the subdivided BSP faces
       can be inherited from the progenitor, such as normal vector,
       ambient/directional brightness, and texture (but not u,v mapping or
       vertices) */
    size_t num_gen_vertices;
    WorldVec const *gen_vertices;
    uint8_t const *gen_vertex_intensities;
    
    size_t num_gen_faces;
    Face const *gen_faces; // pointer to originally-given face array.
} BSPTree;

extern epm_Result create_BSPTree_v0(BSPTree *tree, int bspsel,
                                 size_t num_vertices, WorldVec const *vertices,
                                 size_t num_faces, Face const *faces);

extern epm_Result create_BSPTree
(BSPTree *tree, int bspsel,
 size_t num_vertices, WorldVec const *vertices,
 size_t num_edges, Edge const *edges,
 size_t num_faces, Face const *faces);

extern void destroy_BSPTree(BSPTree *p_tree);

extern void measure_BSPTree(BSPTree *p_tree);

#endif /* BSP_H */
