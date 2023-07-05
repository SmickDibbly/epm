#include "src/world/bsp/bsp_internal.h"

void initialize_Maker_BSPTree(void) {
    reset_Maker_BSPTree();
    
    // To start off, the BSP vertex array shall be identical to the given vertex
    // array. Additional vertices may be added as the algorithm proceeds.
    g_maker_tree.num_vertices = g_gen.num_vertices;
    for (size_t i_v = 0; i_v < g_gen.num_vertices; i_v++) {
        Maker_BSPVertex *mbsp_vertex = g_maker_tree.vertices + i_v;
        mbsp_vertex->vertex = g_gen.vertices[i_v];
        mbsp_vertex->flags = 0;
    }

    g_maker_tree.num_msh_edges = g_gen.num_edges;
    for (size_t i_e = 0; i_e < g_gen.num_edges; i_e++) {
        Maker_BSPEdge *msh_edge = g_maker_tree.msh_edges + i_e;
        msh_edge->i_v0 = g_gen.edges[i_e].i_v0;
        msh_edge->i_v1 = g_gen.edges[i_e].i_v1;
        msh_edge->flags = 0;
    }
    
    g_maker_tree.num_mbsp_faces = g_gen.num_faces;
    for (uint32_t i_f = 0; i_f < g_gen.num_faces; i_f++) {
        Maker_BSPFace *mbsp_face = g_maker_tree.mbsp_faces + i_f;
        Face const *gen_face = g_gen.faces + i_f;
        mbsp_face->i_v0 = gen_face->i_v[0];
        mbsp_face->i_v1 = gen_face->i_v[1];
        mbsp_face->i_v2 = gen_face->i_v[2];
        mbsp_face->next_in_node = NULL;
        mbsp_face->i_gen_face = i_f;
        mbsp_face->flags = 0;
        
        bool found0 = false, found1 = false, found2 = false;
        for (size_t i_e = 0; i_e < g_gen.num_edges; i_e++) {
            Maker_BSPEdge *msh_edge = g_maker_tree.msh_edges + i_e;
            if (((msh_edge->i_v0 == mbsp_face->i_v0) &&
                 (msh_edge->i_v1 == mbsp_face->i_v1)) ||
                ((msh_edge->i_v1 == mbsp_face->i_v0) &&
                 (msh_edge->i_v0 == mbsp_face->i_v1))) {
                mbsp_face->i_e0 = i_e;
                found0 = true;
            }
            else if (((msh_edge->i_v0 == mbsp_face->i_v1) &&
                      (msh_edge->i_v1 == mbsp_face->i_v2)) ||
                     ((msh_edge->i_v1 == mbsp_face->i_v1) &&
                      (msh_edge->i_v0 == mbsp_face->i_v2))) {
                mbsp_face->i_e1 = i_e;
                found1 = true;
            }
            else if (((msh_edge->i_v0 == mbsp_face->i_v2) &&
                      (msh_edge->i_v1 == mbsp_face->i_v0)) ||
                     ((msh_edge->i_v1 == mbsp_face->i_v2) &&
                      (msh_edge->i_v0 == mbsp_face->i_v0))) {
                mbsp_face->i_e2 = i_e;
                found2 = true;
            }
        }
        dibassert(found0 && found1 && found2);
    }

    g_maker_tree.num_nodes = 0;
    g_maker_tree.root = create_Maker_BSPNode();
    g_maker_tree.num_nodes++;
}
