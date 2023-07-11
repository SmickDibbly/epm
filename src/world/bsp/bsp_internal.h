#ifndef BSP_INTERNAL_H
#define BSP_INTERNAL_H

#define BSP_DEBUG

#include "src/misc/epm_includes.h"
#include "src/world/bsp/bsp.h"
#include "src/world/geometry.h"

extern int num_alloc_nodes;

typedef struct Maker_BSPTree   Maker_BSPTree;
typedef struct Maker_BSPNode   Maker_BSPNode;
typedef struct Maker_BSPVertex Maker_BSPVertex;
typedef struct Maker_BSPEdge   Maker_BSPEdge;
typedef struct Maker_BSPFace   Maker_BSPFace;


struct Maker_BSPNode {
    Maker_BSPNode *parent;
    Maker_BSPNode *front;
    Maker_BSPNode *back;

    size_t num_mbsp_faces;
    size_t i_mbsp_faces; // index to the BSP face itself
    
    WorldVec splitV; // point on splitting plane
    WorldVec splitN; // normal of splitting plane
};

#define MV_FROM_CUT 0x01

#define ME_FROM_CUT 0x01
#define ME_NEW 0x02

#define MF_FROM_CUT 0x01

struct Maker_BSPFace {
    Maker_BSPFace *next_in_node; // pointer to the next face in the list of
                                 // faces coplanar with a BSPNode's splitting
                                 // plane. A similar member doesn't appear in a
                                 // regular BSPFace because the list of coplanar
                                 // faces will be stored contiguously in memory.
    
    uint32_t i_gen_face;
    
    size_t i_v0; // tree->vertices[]
    size_t i_v1; // tree->vertices[]
    size_t i_v2; // tree->vertices[]

    size_t i_e0; // tree->msh_edges[]
    size_t i_e1; // tree->msh_edges[]
    size_t i_e2; // tree->msh_edges[]

    uint8_t flags;
};

struct Maker_BSPEdge {
    size_t i_v0;
    size_t i_v1;

    uint8_t flags;
};

struct Maker_BSPVertex {
    WorldVec vertex;

    uint8_t flags;
};

struct Maker_BSPTree {
    size_t num_cuts;
    // TODO: Other stats.
    
    size_t num_nodes;
    Maker_BSPNode *root;
    
    size_t num_vertices;
    Maker_BSPVertex * const vertices;

    size_t num_msh_edges;
    Maker_BSPEdge * const msh_edges;
    
    size_t num_mbsp_faces;
    Maker_BSPFace * const mbsp_faces;
};

extern void reset_Maker_BSPTree(void);
extern Maker_BSPNode *create_Maker_BSPNode(void);
extern void delete_Maker_BSPNode(Maker_BSPNode *p_node);
extern void add_coplanar_face(Maker_BSPNode *node, size_t i_mbsp_face);

/* -------------------------------------------------------------------------- */
// Subsets

typedef struct VtxeSubsetEntry {
    size_t i_v;

    size_t i_fset_subV;
    size_t i_bset_subV;
} VtxeSubsetEntry;

typedef struct EdgeCutData {
    // tree data
    size_t i_v; // new vertex created at cut point
    size_t i_front_e; // index to Maker_BSPEdge in tree
    size_t i_back_e; // index to Maker_BSPEdge in tree

    // front subset data
    size_t i_fset_subV; // index into front subset vertices
    size_t i_fset_front_subE; // index into front EdgeSubsetEntries

    // back subset data
    size_t i_bset_subV; // index into back subset vertices
    size_t i_bset_back_subE; // index into back EdgeSubsetEntries
} EdgeCutData;

typedef struct EdgeSubsetEntry {
    size_t i_subV0; // subset->i_vertices[]
    size_t i_subV1; // subset->i_vertices[]

    size_t i_e; // tree->msh_edges[]


    /* Data used during the construction of front and back subsets of this
       one. */
    size_t i_fset_subE;
    size_t i_bset_subE;
    
    EdgeCutData *cut_data; // non-NULL if edge has been cut
} EdgeSubsetEntry;

typedef struct FaceSubsetEntry {
    size_t i_subV0; // subset->i_vertices[]
    size_t i_subV1; // subset->i_vertices[]
    size_t i_subV2; // subset->i_vertices[]

    size_t i_subE0; // subset->subE[]
    size_t i_subE1; // subset->subE[]
    size_t i_subE2; // subset->subE[]

    size_t i_f; // tree->mbsp_faces[]
} FaceSubsetEntry;

// IDEA: Use a hash table for subsets?

typedef struct BSPSubset {
    Maker_BSPNode *mbsp_node;

    bool nonempty;
    
    size_t num_subV;
    size_t num_occupied_subV;
    VtxeSubsetEntry *subVs;

    size_t num_subE;
    size_t num_occupied_subE;
    EdgeSubsetEntry *subEs;

    size_t num_subF;
    size_t num_occupied_subF;
    FaceSubsetEntry *subFs; // tree->mbsp_faces[]
} BSPSubset;

extern void clear_BSPSubset(BSPSubset *set);
extern void complete_BSPSubset(BSPSubset *p_subset);
extern void initialize_first_BSPSubset(BSPSubset *p_subset);

/* -------------------------------------------------------------------------- */
// Splitting

#define SIDE_FRONT 0x01
#define SIDE_MID   0x02
#define SIDE_BACK  0x04

typedef enum VtxeSpatialRelation {
    VSR_COPLANAR = SIDE_MID,
    VSR_FRONT    = SIDE_FRONT,
    VSR_BACK     = SIDE_BACK,
} VtxeSpatialRelation;

typedef enum EdgeSpatialRelation {
    ESR_COPLANAR = 0x01,
    ESR_FRONT    = 0x02,
    ESR_BACK     = 0x04,
    ESR_CUT      = 0x08,
} EdgeSpatialRelation;

typedef enum FaceSpatialRelation {
    FSR_COPLANAR   = 0x01,
    FSR_FRONT      = 0x02,
    FSR_BACK       = 0x04,
    FSR_CUT_TYPE1  = 0x08,
    FSR_CUT_TYPE2F = 0x10,
    FSR_CUT_TYPE2B = 0x20,
} FaceSpatialRelation;

typedef struct BSPCutData {
    WorldVec planeV;
    WorldVec planeN;    
    size_t i_split_f;

    VtxeSpatialRelation *VSRs;
    EdgeSpatialRelation *ESRs;
    FaceSpatialRelation *FSRs;

    double balance;

    bool convex;
    
    // Current subset VSR numbers
    size_t num_coplanar_v;
    size_t num_front_v;
    size_t num_back_v;

    // Current subset ESR numbers
    size_t num_coplanar_e;
    size_t num_front_e;
    size_t num_back_e;
    size_t num_cut_e;

    // Current subset FSR numbers
    size_t num_coplanar_f;
    size_t num_front_f;
    size_t num_back_f;
    size_t num_type1_cut_f;
    size_t num_type2f_cut_f;
    size_t num_type2b_cut_f;
    size_t num_cut_f; // = num_type1_cut_f + num_type2f_cut_f + num_type2b_cut_f

    // Front subset expected numbers
    size_t num_fset_v;  
    size_t num_fset_e;  
    size_t num_fset_f;
    
    // Back subset expected numbers
    size_t num_bset_v;
    size_t num_bset_e;
    size_t num_bset_f;
} BSPCutData;

extern void compute_VSRs
(WorldVec planeV, WorldVec planeN,
 size_t num_subV, VtxeSubsetEntry const *subV,
 VtxeSpatialRelation *out_VSRs,
 size_t *out_num_coplanar_v,
 size_t *out_num_front_v,
 size_t *out_num_back_v);

extern void compute_ESRs
(size_t num_subE, EdgeSubsetEntry const *subE,
 VtxeSpatialRelation const *VSRs,
 EdgeSpatialRelation *out_ESRs,
 size_t *out_num_coplanar_e,
 size_t *out_num_front_e,
 size_t *out_num_back_e,
 size_t *out_num_cut_e);

extern void compute_FSRs
(size_t num_subF, FaceSubsetEntry const *subF,
 VtxeSpatialRelation const *VSRs,
 FaceSpatialRelation *out_FSRs,
 size_t *out_num_coplanar_f,
 size_t *out_num_front_f,
 size_t *out_num_back_f,
 size_t *out_num_type1_cut_f,
 size_t *out_num_type2f_cut_f,
 size_t *out_num_type2b_cut_f,
 size_t *out_num_cut_f);

extern void choose_splitting_plane(BSPSubset *p_subset, BSPCutData *data);

extern void cut(WorldVec const *const splitV, WorldVec const *const splitN, BSPSubset const *subset, BSPCutData const *data, BSPSubset *front, BSPSubset *back);


/* -------------------------------------------------------------------------- */
// Printing

//#define BSP_VERBOSITY

#ifdef BSP_VERBOSITY
#  define VERBOSITY
extern void print_Vertices(size_t num, WorldVec const *arr);
extern void print_Edges(size_t num, Edge const *arr);
extern void print_Faces(size_t num, Face const *arr);
extern void print_Maker_BSPTree_diagram(Maker_BSPNode const *node, uint32_t level);
extern void print_Maker_BSPTree(Maker_BSPTree const *tree);
extern void print_Maker_BSPVertices(size_t num, Maker_BSPVertex const *arr);
extern void print_Maker_BSPEdges(size_t num, Maker_BSPEdge const *arr);
extern void print_Maker_BSPFaces(size_t num, Maker_BSPFace const *arr);
extern void print_BSPTree_diagram(BSPNode const *node, uint32_t level);
extern void print_BSPTree(BSPTree const *tree);
extern void print_BSPFaces(size_t num, BSPFace const *bsp_faces, Face const *faces);
extern void print_BSPCutData(BSPCutData const *data);
#else
#  define print_Vertices(num, arr) (void)0
#  define print_Edges(num, arr) (void)0
#  define print_Faces(num, arr) (void)0
#  define print_Maker_BSPTree_diagram(node, level) (void)0
#  define print_Maker_BSPTree(tree) (void)0
#  define print_Maker_BSPVertices(num, arr) (void)0
#  define print_Maker_BSPEdges(num, arr) (void)0
#  define print_Maker_BSPFaces(num, arr) (void)0
#  define print_BSPTree_diagram(node, level) (void)0
#  define print_BSPTree(tree) (void)0
#  define print_BSPFaces(num, bspfaces, faces) (void)0
#  define print_BSPCutData(data) (void)0
#endif

#include "zigil/diblib_local/verbosity.h"


/* -------------------------------------------------------------------------- */
// Global data

typedef struct ProgenitorGeometry {
    size_t num_vertices;
    WorldVec const *vertices;
    uint8_t const *vbris;
    
    size_t num_edges;
    Edge const *edges;

    size_t num_faces;
    Face const *faces;
} ProgenitorGeometry;

extern ProgenitorGeometry g_gen;
extern Maker_BSPTree g_maker_tree;
extern BSPTree *g_p_final_tree;

/* -------------------------------------------------------------------------- */
// Geometry

extern bool find_intersection
(WorldVec lineV0, WorldVec lineV1,
 WorldVec planeV, WorldVec planeN,
 WorldVec *out_sect);

extern uint8_t BSP_sideof(WorldVec V, WorldVec planeV, WorldVec planeN);

/* -------------------------------------------------------------------------- */
// initialization

extern void initialize_Maker_BSPTree(void);

/* -------------------------------------------------------------------------- */
// Finalization

extern void finalize_BSPTree(void);

#endif /* BSP_INTERNAL_H */
