#include "src/world/bsp/bsp_internal.h"

#define MAX_STATIC_BSP_V (16384)
#define MAX_STATIC_BSP_E (16384)
#define MAX_STATIC_BSP_F (16384)

//static WorldVec g_static_V[MAX_STATIC_BSP_V] = {0};
//static Maker_BSPEdge g_static_E[MAX_STATIC_BSP_E] = {0};
//static Maker_BSPFace g_static_F[MAX_STATIC_BSP_F] = {0};

Maker_BSPTree g_maker_tree = {
    .num_cuts = 0,
    .num_nodes = 0,
    .root = NULL,
    .num_vertices = 0,
    .vertices = (Maker_BSPVertex [MAX_STATIC_BSP_V]){0},//g_static_V,
    .num_msh_edges = 0,
    .msh_edges = (Maker_BSPEdge [MAX_STATIC_BSP_E]){{0}},//g_static_E,
    .num_mbsp_faces = 0,
    .mbsp_faces = (Maker_BSPFace [MAX_STATIC_BSP_F]){{0}},//g_static_F,
};

void reset_Maker_BSPTree(void) {
    if (g_maker_tree.root != NULL) {
        delete_Maker_BSPNode(g_maker_tree.root);
    }

    g_maker_tree.num_cuts = 0;
    g_maker_tree.num_nodes = 0;
    g_maker_tree.root = NULL;
    g_maker_tree.num_vertices = 0;
    memset(g_maker_tree.vertices, 0, MAX_STATIC_BSP_V);
    g_maker_tree.num_msh_edges = 0;
    memset(g_maker_tree.msh_edges, 0, MAX_STATIC_BSP_E);
    g_maker_tree.num_mbsp_faces = 0;
    memset(g_maker_tree.mbsp_faces, 0, MAX_STATIC_BSP_F);
}

Maker_BSPNode *create_Maker_BSPNode(void) {
    num_alloc_nodes++;

    Maker_BSPNode *node = zgl_Malloc(sizeof(*node));

    node->parent = NULL;
    node->front = NULL;
    node->back = NULL;
    node->i_mbsp_faces = 0;
    node->num_mbsp_faces = 0;
    
    return node;
}

void add_coplanar_face(Maker_BSPNode *node, size_t i_mbsp_face) {
    if (node->num_mbsp_faces == 0) {
        node->i_mbsp_faces = i_mbsp_face;
        
        Maker_BSPFace *face = g_maker_tree.mbsp_faces + node->i_mbsp_faces;
        face->next_in_node = NULL;
    }
    else {
        Maker_BSPFace *face = g_maker_tree.mbsp_faces + node->i_mbsp_faces;
        while (face->next_in_node != NULL) {
            face = face->next_in_node;
        }
        face->next_in_node = g_maker_tree.mbsp_faces + i_mbsp_face;
        face->next_in_node->next_in_node = NULL;
    }

    node->num_mbsp_faces++;
}

void delete_Maker_BSPNode(Maker_BSPNode *p_node) {
    num_alloc_nodes--;
    
    if (p_node->front != NULL) {
        delete_Maker_BSPNode(p_node->front);
    }

    if (p_node->back != NULL) {
        delete_Maker_BSPNode(p_node->back);
    }

    zgl_Free(p_node);
    p_node = NULL;
}
