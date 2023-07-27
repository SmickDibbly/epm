#include "src/world/bsp/bsp_internal.h"

#ifdef BSP_VERBOSITY

void print_BSPCutData(BSPCutData const *data) {
    vbs_printf("       Cut Data      \n"
               "+-------------------+\n"
               " Split Plane Point  | %s\n"
               " Split Plane Normal | %s\n"
               "                    | \n"
               "            Balance | %lf\n"
               "                    | \n"
               "            Convex  | %s\n"
               "                    | \n"
               "         Coplanar V | %zu\n"
               "            Front V | %zu\n"
               "             Back V | %zu\n"
               "                    | \n"
               "         Coplanar E | %zu\n"
               "            Front E | %zu\n"
               "             Back E | %zu\n"
               "              Cut E | %zu\n"
               "                    | \n"
               "         Coplanar F | %zu\n"
               "            Front F | %zu\n"
               "             Back F | %zu\n"
               "       Type 1 Cut F | %zu\n"
               "      Type 2F Cut F | %zu\n"
               "      Type 2B Cut F | %zu\n"
               "              Cut F | %zu\n"
               "                    | \n"
               "         Frontset V | %zu\n"
               "         Frontset E | %zu\n"
               "         Frontset F | %zu\n"
               "                    | \n"
               "          Backset V | %zu\n"
               "          Backset E | %zu\n"
               "          Backset F | %zu\n",
               
               fmt_Fix32Vec(data->planeV),
               fmt_Fix32Vec(data->planeN),
               data->balance,
               data->convex ? "TRUE" : "FALSE",
               
               data->num_coplanar_v,
               data->num_front_v,
               data->num_back_v,

               data->num_coplanar_e,
               data->num_front_e,
               data->num_back_e,
               data->num_cut_e,

               data->num_coplanar_f,
               data->num_front_f,
               data->num_back_f,
               data->num_type1_cut_f,
               data->num_type2f_cut_f,
               data->num_type2b_cut_f,
               data->num_cut_f,

               data->num_fset_v,
               data->num_fset_e,
               data->num_fset_f,
               
               data->num_bset_v,
               data->num_bset_e,
               data->num_bset_f);
}

void print_Vertices(size_t num, WorldVec const *arr) {
    for (size_t i = 0; i < num; i++) {
        vbs_printf("V[%zu] = %s\n", i, fmt_Fix32Vec(arr[i]));
    }
}

void print_Edges(size_t num, Edge const *arr) {
    for (size_t i = 0; i < num; i++) {
        vbs_printf("E[%zu] = (%zu, %zu)\n", i, arr[i].i_v0, arr[i].i_v1);
    }
}

void print_Faces(size_t num, Face const *arr) {
    for (size_t i = 0; i < num; i++) {
        vbs_printf("F[%zu] = (%zu, %zu, %zu)\n", i, arr[i].i_v0, arr[i].i_v1, arr[i].i_v2);
    }
}


void print_Maker_BSPTree_diagram(Maker_BSPNode const *node, uint32_t level) {
    if (node == NULL)
        return;
    
    for (size_t i = 0; i < level; i++)
	    vbs_printf(i == level - 1 ? "|-" : "  ");
    vbs_printf("F[");
    vbs_printf("%zu", node->i_mbsp_faces);

    for (Maker_BSPFace *mbsp_face = &(*(g_maker_tree.mbsp_faces + node->i_mbsp_faces)->next_in_node);
         mbsp_face;
         mbsp_face = mbsp_face->next_in_node) {
        vbs_printf(", %zu", mbsp_face - g_maker_tree.mbsp_faces);
    }


    /*
    for (size_t i_f = 1; i_f < node->num_mbsp_faces; i_f++) {
        vbs_printf(", %zu", node->i_mbsp_faces + i_f);
    }
    */
    vbs_printf("]\n");
	
    print_Maker_BSPTree_diagram(node->front, level + 1);
    print_Maker_BSPTree_diagram( node->back, level + 1);
}

void print_Maker_BSPTree(Maker_BSPTree const *tree) {
    vbs_printf(" Maker BSP Summary \n");
    vbs_printf("+-----------------+\n");
    vbs_printf("# Vertices: %zu\n", tree->num_vertices);
    vbs_printf("# BSPFaces: %zu\n", tree->num_mbsp_faces);
    vbs_printf("    # Cuts: %zu\n", tree->num_cuts);
}

void print_Maker_BSPVertices(size_t num, Maker_BSPVertex const *arr) {
    for (size_t i_v = 0; i_v < num; i_v++) {
        vbs_printf("V[%zu] = %s (flags = %x)\n",
                   i_v,
                   fmt_Fix32Vec(arr[i_v].vertex),
                   arr[i_v].flags);
    }
}

void print_Maker_BSPEdges(size_t num, Maker_BSPEdge const *arr) {
    for (size_t i_edge = 0; i_edge < num; i_edge++) {
        vbs_printf("E[%zu] = V[%zu], V[%zu] (flags = %x)\n",
                   i_edge,
                   arr[i_edge].i_v0,
                   arr[i_edge].i_v1,
                   arr[i_edge].flags);
    }
}

void print_Maker_BSPFaces(size_t num, Maker_BSPFace const *arr) {
    for (size_t i_face = 0; i_face < num; i_face++) {
        vbs_printf("F[%zu] = V[%zu] -> V[%zu] -> V[%zu];  OGF[%u] (flags = %x)\n",
                   i_face,
                   arr[i_face].i_v0,
                   arr[i_face].i_v1,
                   arr[i_face].i_v2,
                   arr[i_face].i_gen_face,
                   arr[i_face].flags);
    }
}

static BSPNode const *root_node;

void print_BSPTree_diagram(BSPNode const *node, uint32_t level) {
    if (level == 0) {
        root_node = node;
    }
    
    if (node == NULL)
        return;
    
    for (size_t i = 0; i < level; i++)
	    vbs_printf(i == level - 1 ? "|-" : "  ");
    vbs_printf("Node[%zu], ", node - root_node);
    vbs_printf("F[");
    vbs_printf("%zu", node->i_first_face + 0);
    for (size_t i_f = 1; i_f < node->num_faces; i_f++) {
        vbs_printf(", %zu", node->i_first_face + i_f);
    }
    vbs_printf("]\n");
	
    print_BSPTree_diagram(node->front, level + 1);
    print_BSPTree_diagram(node->back,  level + 1);
}

void print_BSPTree_summary(BSPTree const *tree) {
    if (tree == NULL) return;
    
    vbs_printf("   BSP Summary   \n");
    vbs_printf("+---------------+\n");
    vbs_printf("# Input Vertices: %zu\n", tree->num_gen_vertices);
    vbs_printf("   # Input Faces: %zu\n", tree->num_gen_faces);
    vbs_printf("      # Vertices: %zu\n", tree->num_vertices);
    vbs_printf("         # Nodes: %zu\n", tree->num_nodes);
    vbs_printf("         # Faces: %zu\n", tree->num_faces);
    vbs_printf("          # Cuts: %zu\n", tree->num_cuts);
}

void print_BSPFaces(size_t num, BSPFace const *bsp_faces, Face const *faces) {
    for (size_t i_bspface = 0; i_bspface < num; i_bspface++) {
        vbs_printf("F[%zu] = V[%zu] -> V[%zu] -> V[%zu];  OGF[%u]\n",
                   i_bspface,
                   faces[i_bspface].i_v0,
                   faces[i_bspface].i_v1,
                   faces[i_bspface].i_v2,
                   bsp_faces[i_bspface].i_gen_face);
    }
}
#endif
