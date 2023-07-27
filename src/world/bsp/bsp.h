#ifndef BSP_H
#define BSP_H

#include "src/misc/epm_includes.h"
#include "src/world/geometry.h"
#include "src/world/mesh.h"
#include "src/world/pregeo.h"

// TODO: Allow the geometry-designer to suggest certain BSP splitting planes.



// "Ext"ra data tacked onto to a geometry elements.
typedef struct BSPVertExt {
    int32_t i_pre_vert; // signed; negative means "not from prebsp geo"
    uint8_t bspflags;
} BSPVertExt;

typedef struct BSPEdgeExt {
    uint32_t i_pre_edge;
    uint8_t bspflags;
} BSPEdgeExt;

typedef struct BSPFaceExt {
    uint32_t i_pre_face;    
    uint16_t bspflags;
    
    uint16_t depth;
} BSPFaceExt;


// A BSP node should only contain pointers and metadata. Store the actual BSP
// face data in an array.
#define NODEFLAG_LEAF 0x01
typedef struct BSPNode BSPNode;
struct BSPNode {
    BSPNode *parent; // TODO: Will this *ever* be used besides in bsp_finalization?
    BSPNode *front;
    BSPNode *back;
    
    WorldVec splitV; // point on splitting plane
    WorldVec splitN; // normal of splitting plane

    size_t num_faces;
    size_t i_first_face;

    uint8_t nodeflags;
};

typedef struct BSPTree {
    size_t num_leaves; // FIXME: My belief of what makes a "leaf" is
                       // inconsistent. I have to imagine the leaf being a "NULL
                       // node".
    size_t num_cuts;
    
    size_t max_node_depth;
    double avg_node_depth;
    double avg_leaf_depth;
    double balance;
    
    size_t num_verts;
    WorldVec *verts;
    BSPVertExt *vert_exts;
    zgl_Color *vert_clrs; // green if og, red if from cut
    
    size_t num_edges;
    Edge *edges;
    zgl_Color *edge_clrs; // green if og, red if from cut, blue if new

    size_t num_faces;
    Face *faces;
    BSPFaceExt *face_exts;
    zgl_Color *face_clrs; // green if og, red if from cut

    size_t num_nodes;
    BSPNode *nodes;

    
    size_t num_pre_verts;
    WorldVec *pre_verts;
    PreVertExt *pre_vert_exts;
    
    size_t num_pre_edges;
    Edge *pre_edges;

    size_t num_pre_faces;
    Face *pre_faces;
    PreFaceExt *pre_face_exts;
} BSPTree;

extern epm_Result create_BSPTree(BSPTree *tree, PreGeometry *pre);

extern void destroy_BSPTree(BSPTree *p_tree);

extern void measure_BSPTree(BSPTree *p_tree);

extern void print_BSPTree(BSPTree const *tree);

#endif /* BSP_H */
