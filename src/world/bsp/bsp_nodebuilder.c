#include "src/world/bsp/bsp_internal.h"
#include "src/world/bsp/bsp.h"

#include "zigil/zigil_time.h"

#undef LOG_LABEL
#define LOG_LABEL "BSP"

int num_alloc_nodes = 0;

ProgenitorGeometry g_gen;
BSPTree *g_p_final_tree;

static void prune_subset(BSPSubset *set);
static void clear_BSPCutData(BSPCutData *data);
static bool assert_geometry_uniqueness(void);
static void BSP_recursion(BSPSubset subfaces);
static void check_no_unused_faces(BSPTree *tree);

epm_Result create_BSPTree(BSPTree *out_tree, int ignore,
                          size_t in_num_gen_vertices, WorldVec const *in_gen_vertices,
                          size_t in_num_gen_edges, Edge const *in_gen_edges,
                          size_t in_num_gen_faces, Face const *in_gen_faces) {

    utime_t start = zgl_ClockQuery();
    epm_Log(LT_INFO, "Creating BSP tree.");

    /* ------------------------------------------------------------------*/
    // Make genesis data accessible.
    
    g_gen.num_vertices = in_num_gen_vertices;
    g_gen.vertices = in_gen_vertices;

    g_gen.num_edges = in_num_gen_edges;
    g_gen.edges = in_gen_edges;

    g_gen.num_faces = in_num_gen_faces;
    g_gen.faces = in_gen_faces;
    
    vbs_putchar('\n');
    vbs_printf("+----------------------------+\n"
               "| Starting the BSP algorithm |\n"
               "+----------------------------+\n");
    vbs_printf("Given: %zu vertices\n"
               "       %zu edges\n"
               "       %zu faces\n",
               g_gen.num_vertices,
               g_gen.num_edges,
               g_gen.num_faces);
    vbs_putchar('\n');
    vbs_printf(" Given Vertices\n"
               "+--------------+\n");
    print_Vertices(g_gen.num_vertices, g_gen.vertices);
    vbs_putchar('\n');
    
    vbs_printf(" Given Edges\n"
               "+-----------+\n");
    print_Edges(g_gen.num_edges, g_gen.edges);
    vbs_putchar('\n');
    
    vbs_printf(" Given Faces\n"
               "+-----------+\n");
    print_Faces(g_gen.num_faces, g_gen.faces);
    vbs_putchar('\n');

    //dibassert(assert_geometry_uniqueness());

    /* ------------------------------------------------------------------*/
    // Initialization.
    //     gen_vertices --> g_maker_tree.vertices
    //     gen_edges    --> g_maker_tree.msh_edges
    //     gen_faces    --> g_maker_tree.mbsp_faces

    initialize_Maker_BSPTree();
   
    /* These prints can be used to ensure that the maker tree in its initial
       state is correct. */
    vbs_printf("Initial Maker Tree: %zu vertices\n"
               "                    %zu edges\n"
               "                    %zu faces\n",
               g_maker_tree.num_vertices,
               g_maker_tree.num_msh_edges,
               g_maker_tree.num_mbsp_faces);
    vbs_putchar('\n');
    vbs_printf(" Initial Maker Tree Vertices\n"
               "+---------------------------+\n");
    print_Maker_BSPVertices(g_maker_tree.num_vertices, g_maker_tree.vertices);
    vbs_putchar('\n');
    vbs_printf(" Initial Maker Tree Edges\n"
               "+------------------------+\n");
    print_Maker_BSPEdges(g_maker_tree.num_msh_edges, g_maker_tree.msh_edges);
    vbs_putchar('\n');
    vbs_printf(" Initial Maker Tree Faces\n"
               "+------------------------+\n");
    print_Maker_BSPFaces(g_maker_tree.num_mbsp_faces, g_maker_tree.mbsp_faces);
    vbs_putchar('\n');

    // TODO: Assert accuracy.
    
    /* ------------------------------------------------------------------*/
    // Initialize the first subset.
    
    BSPSubset subset;
    initialize_first_BSPSubset(&subset);

    // TODO: Print the first subset, assert accuracy.
    
    /* ------------------------------------------------------------------*/
    // Do the recursion.
    
    vbs_putchar('\n');
    vbs_printf("BSP Root Recursion Step Begin\n");
    
    BSP_recursion(subset);

    vbs_printf("Maker Tree: %zu vertices\n"
               "            %zu edges\n"
               "            %zu faces\n",
               g_maker_tree.num_vertices,
               g_maker_tree.num_msh_edges,
               g_maker_tree.num_mbsp_faces);
    
    print_Maker_BSPTree(&g_maker_tree);
    vbs_putchar('\n');
    
    vbs_printf(" Maker Tree Vertices\n"
               "+-------------------+\n");
    print_Maker_BSPVertices(g_maker_tree.num_vertices, g_maker_tree.vertices);
    vbs_putchar('\n');
    
    vbs_printf(" Maker Tree Edges\n"
               "+----------------+\n");
    print_Maker_BSPEdges(g_maker_tree.num_msh_edges, g_maker_tree.msh_edges);
    vbs_putchar('\n');
    
    vbs_printf(" Maker Tree Faces\n"
               "+----------------+\n");
    print_Maker_BSPFaces(g_maker_tree.num_mbsp_faces, g_maker_tree.mbsp_faces);
    vbs_putchar('\n');
    
    vbs_printf(" Maker Tree\n"
               "+----------+\n");
    print_Maker_BSPTree_diagram(g_maker_tree.root, 0);
    vbs_putchar('\n');

    // TODO: Do some assertions.

    /* ------------------------------------------------------------------*/
    // Finalization.
    
    g_p_final_tree = out_tree;
    finalize_BSPTree();
    
    print_BSPTree(g_p_final_tree);
    vbs_putchar('\n');
    
    vbs_printf(" Final BSP Vertices\n"
               "+------------------+\n");
    print_Vertices(g_p_final_tree->num_vertices, g_p_final_tree->vertices);
    vbs_putchar('\n');
    vbs_printf(" Final BSP Edges\n"
               "+---------------+\n");
    print_Edges(g_p_final_tree->num_edges, g_p_final_tree->edges);
    vbs_putchar('\n');    
    vbs_printf(" Final BSP Faces\n"
               "+---------------+\n");
    print_BSPFaces(g_p_final_tree->num_faces, g_p_final_tree->bsp_faces, g_p_final_tree->faces);
    vbs_putchar('\n');

    vbs_printf(" Final BSP Tree Diagram\n"
               "+----------------------+\n");
    print_BSPTree_diagram(&g_p_final_tree->nodes[0], 0);
    vbs_putchar('\n');

    measure_BSPTree(g_p_final_tree);

    check_no_unused_faces(g_p_final_tree);
        
    // TODO: More assertions.

    
    /* Cleanup */
    reset_Maker_BSPTree();
    vbs_putchar('\n');

    utime_t end = zgl_ClockQuery();
    epm_Log(LT_INFO, "BSP Compilation Time: %s seconds", fmt_fix_d(((end-start)<<16)/1000, 16, 4));
    
    return EPM_SUCCESS;
}





static void BSP_recursion(BSPSubset pset) {    
    // +----------------------------+
    // | Choosing a Splitting Plane |
    // +----------------------------+

    // Also involves computing some splitting-relevant data that is reused in
    // next step.
    
    BSPCutData cut_data = {0};
    vbs_printf("Choosing a split plane from %zu faces...\n", pset.num_subF);
    
    choose_splitting_plane(&pset, &cut_data);
    
    print_BSPCutData(&cut_data);
    
    dibassert(cut_data.num_fset_f + cut_data.num_bset_f >=
              pset.num_subF - cut_data.num_coplanar_f);
    
    // +-----------------+
    // | Fill in BSPNode |
    // +-----------------+

    vbs_printf("Constructing BSP node...\n");
    
    WorldVec const *p_splitV, *p_splitN;
    {
        // Plane data
        Face const *og_face = &g_gen.faces[g_maker_tree.mbsp_faces[cut_data.i_split_f].i_gen_face];
        p_splitV = &g_gen.vertices[og_face->i_v[0]];
        p_splitN = &og_face->normal;
        pset.mbsp_node->splitV = *p_splitV;
        pset.mbsp_node->splitN = *p_splitN;

        // Store coplanar geometry in node.
        pset.mbsp_node->num_mbsp_faces = 0;
        for (size_t i_pset_subF = 0; i_pset_subF < pset.num_subF; i_pset_subF++) {
            if (cut_data.FSRs[i_pset_subF] == FSR_COPLANAR) {
                add_coplanar_face(pset.mbsp_node, pset.subFs[i_pset_subF].i_f);
            }
        }

        dibassert(pset.mbsp_node->num_mbsp_faces == cut_data.num_coplanar_f);
        
        pset.mbsp_node->front = NULL;
        pset.mbsp_node->back = NULL;
        // parent filled by caller of this function
    }
    
    if (pset.num_subF == cut_data.num_coplanar_f) {
        vbs_printf("Pre-leaf node.\n");
        // TODO: Assert number of all other faces is 0.
        
        clear_BSPSubset(&pset);
        clear_BSPCutData(&cut_data);
        
        return;
    }

    
    // +------------------------------+
    // | Fill in frontset and backset |
    // +------------------------------+

    vbs_printf("Preparing front and back subsets...\n");

    BSPSubset fset = {0}, bset = {0};

    fset.num_subV = cut_data.num_fset_v;
    fset.num_subE = cut_data.num_fset_e;
    fset.num_subF = cut_data.num_fset_f;
    if (fset.num_subV > 0) {
        fset.subVs = zgl_Malloc(fset.num_subV*sizeof(*fset.subVs));     
    }
    if (fset.num_subE > 0) {
        fset.subEs = zgl_Malloc(fset.num_subE*sizeof(*fset.subEs));
    }
    if (fset.num_subF > 0) {
        // front node is created and has parent set now, but the rest is filled
        // during front recursive step
        
        pset.mbsp_node->front = create_Maker_BSPNode();
        g_maker_tree.num_nodes++;

        fset.mbsp_node = pset.mbsp_node->front;
        fset.mbsp_node->parent = pset.mbsp_node;

        fset.nonempty = true;
                
        fset.subFs = zgl_Malloc(fset.num_subF*sizeof(*fset.subFs));
    }
    else {
        fset.nonempty = false;
    }    

    bset.num_subV = cut_data.num_bset_v;
    bset.num_subE = cut_data.num_bset_e;
    bset.num_subF = cut_data.num_bset_f;
    if (bset.num_subV > 0) {
        bset.subVs = zgl_Malloc(bset.num_subV*sizeof(*bset.subVs));     
    }
    if (bset.num_subE > 0) {
        bset.subEs = zgl_Malloc(bset.num_subE*sizeof(*bset.subEs));    
    }
    if (bset.num_subF > 0) {
        // back node is created and has parent set now, but the rest is filled
        // during back recursive step
        
        pset.mbsp_node->back = create_Maker_BSPNode();
        g_maker_tree.num_nodes++;

        bset.mbsp_node = pset.mbsp_node->back;
        bset.mbsp_node->parent = pset.mbsp_node;

        bset.nonempty = true;
        
        bset.subFs = zgl_Malloc(bset.num_subF*sizeof(*bset.subFs));
    }
    else {
        bset.nonempty = false;
    }
    
    vbs_printf("Performing the BSP cut...\n");
    
    cut(p_splitV, p_splitN, &pset, &cut_data, &fset, &bset);

    // These are no longer needed.
    clear_BSPSubset(&pset);
    clear_BSPCutData(&cut_data);
    
    vbs_printf("Front pre-prune: %zu vertices\n"
               "                 %zu edges\n"
               "                 %zu faces\n",
               fset.num_subV,
               fset.num_subE,
               fset.num_subF);

    vbs_printf("Back pre-prune: %zu vertices\n"
               "                %zu edges\n"
               "                %zu faces\n",
               bset.num_subV,
               bset.num_subE,
               bset.num_subF);

    vbs_printf("Pruning subsets...\n");
    
    prune_subset(&fset);
    prune_subset(&bset);

    vbs_printf("Front: %zu vertices\n"
               "       %zu edges\n"
               "       %zu faces\n",
               fset.num_subV,
               fset.num_subE,
               fset.num_subF);

    vbs_printf("Back: %zu vertices\n"
               "      %zu edges\n"
               "      %zu faces\n",
               bset.num_subV,
               bset.num_subE,
               bset.num_subF);
    
    if (fset.num_subF > 0) {
        vbs_putchar('\n');
        vbs_printf("Front Recursion Step Begin\n");
        BSP_recursion(fset);
    }
    else {
        zgl_Free(fset.subVs);
        zgl_Free(fset.subEs);
    }
    
    if (bset.num_subF > 0) {
        vbs_putchar('\n');
        vbs_printf("Back Recursion Step Begin\n");
        BSP_recursion(bset);
    }
    else {
        zgl_Free(bset.subVs);
        zgl_Free(bset.subEs);
    }
}


static bool assert_geometry_uniqueness(void) {
    bool unique;

    // vertices
    unique = true;
    for (size_t i_v = 0; i_v < g_gen.num_vertices; i_v++) {
        for (size_t j_v = 0; j_v < g_gen.num_vertices; j_v++) {
            if (i_v == j_v) continue;

            if ((x_of(g_gen.vertices[i_v]) ==
                 x_of(g_gen.vertices[j_v])) &&
                (y_of(g_gen.vertices[i_v]) ==
                 y_of(g_gen.vertices[j_v])) &&
                (z_of(g_gen.vertices[i_v]) ==
                 z_of(g_gen.vertices[j_v]))) {
                unique = false;
                break;
            }
        }
    }

    if ( ! unique) return false;

    // edges
    unique = true;
    for (size_t i_e = 0; i_e < g_gen.num_vertices; i_e++) {
        for (size_t j_e = 0; j_e < g_gen.num_vertices; j_e++) {
            if (i_e == j_e) continue;

            if ((g_gen.edges[i_e].i_v0 == g_gen.edges[j_e].i_v0 &&
                 g_gen.edges[i_e].i_v1 == g_gen.edges[j_e].i_v1) ||
                (g_gen.edges[i_e].i_v0 == g_gen.edges[j_e].i_v1 &&
                 g_gen.edges[i_e].i_v1 == g_gen.edges[j_e].i_v0)) {
                unique = false;
                break;
            }
        }
    }

    if ( ! unique) return false;

    return true;
}

static void clear_BSPCutData(BSPCutData *data) {
    zgl_Free(data->VSRs);
    zgl_Free(data->ESRs);
    zgl_Free(data->FSRs);
}



static void prune_subset(BSPSubset *set) {
    VtxeSubsetEntry *copy_subVs = zgl_Malloc(set->num_subV*sizeof(*copy_subVs));
    EdgeSubsetEntry *copy_subEs = zgl_Malloc(set->num_subE*sizeof(*copy_subEs));

    memcpy(copy_subVs, set->subVs, set->num_subV*sizeof(*copy_subVs));
    memcpy(copy_subEs, set->subEs, set->num_subE*sizeof(*copy_subEs));
    
    bool *subV_inclusion = zgl_Calloc(set->num_subV, sizeof(*subV_inclusion));
    bool *subE_inclusion = zgl_Calloc(set->num_subE, sizeof(*subE_inclusion));

    //    printf("(set->num_subV %zu)\n", set->num_subV);
    //    printf("(set->num_subE %zu)\n\n", set->num_subE);
    for (size_t i_subF = 0; i_subF < set->num_subF; i_subF++) {
        FaceSubsetEntry subF = set->subFs[i_subF];
        //        printf("(subF.i_subV0 %zu)\n", subF.i_subV0);
        //        printf("(subF.i_subV1 %zu)\n", subF.i_subV1);
        //        printf("(subF.i_subV2 %zu)\n", subF.i_subV2);
        //printf("(subF.i_subE0 %zu)\n", subF.i_subE0);
        //printf("(subF.i_subE1 %zu)\n", subF.i_subE1);
        //printf("(subF.i_subE2 %zu)\n\n", subF.i_subE2);
        subV_inclusion[subF.i_subV0] = true;
        subV_inclusion[subF.i_subV1] = true;
        subV_inclusion[subF.i_subV2] = true;
        subE_inclusion[subF.i_subE0] = true;
        subE_inclusion[subF.i_subE1] = true;
        subE_inclusion[subF.i_subE2] = true;
    }

    size_t *old_to_new_v = zgl_Malloc(set->num_subV*sizeof(*old_to_new_v));
    size_t *old_to_new_e = zgl_Malloc(set->num_subE*sizeof(*old_to_new_e));
    
    size_t i_prn_subV = 0;
    for (size_t i_subV = 0; i_subV < set->num_subV; i_subV++) {
        if (subV_inclusion[i_subV] == true) {
            set->subVs[i_prn_subV].i_v = copy_subVs[i_subV].i_v;
            
            old_to_new_v[i_subV] = i_prn_subV;

            i_prn_subV++;
        }
    }
    set->num_subV = i_prn_subV;
        
    size_t i_prn_subE = 0;
    for (size_t i_subE = 0; i_subE < set->num_subE; i_subE++) {
        if (subE_inclusion[i_subE] == true) {
            EdgeSubsetEntry *subE_old = &copy_subEs[i_subE];
            EdgeSubsetEntry *subE_new = &set->subEs[i_prn_subE];
            subE_new->i_e = subE_old->i_e;
            subE_new->i_subV0 = old_to_new_v[subE_old->i_subV0];
            subE_new->i_subV1 = old_to_new_v[subE_old->i_subV1];
            subE_new->i_fset_subE = subE_old->i_fset_subE;
            subE_new->i_bset_subE = subE_old->i_bset_subE;
            subE_new->cut_data = subE_old->cut_data;
                
            old_to_new_e[i_subE] = i_prn_subE;
            
            i_prn_subE++;
        }
    }
    set->num_subE = i_prn_subE;
    

    for (size_t i_subF = 0; i_subF < set->num_subF; i_subF++) {
        FaceSubsetEntry *subF = &set->subFs[i_subF];   
        subF->i_subV0 = old_to_new_v[subF->i_subV0];
        subF->i_subV1 = old_to_new_v[subF->i_subV1];
        subF->i_subV2 = old_to_new_v[subF->i_subV2];

        subF->i_subE0 = old_to_new_e[subF->i_subE0];
        subF->i_subE1 = old_to_new_e[subF->i_subE1];
        subF->i_subE2 = old_to_new_e[subF->i_subE2];
    }

    zgl_Free(copy_subVs);
    zgl_Free(copy_subEs);
    zgl_Free(subV_inclusion);
    zgl_Free(subE_inclusion);
    zgl_Free(old_to_new_v);
    zgl_Free(old_to_new_e);
}


static void check_no_unused_faces(BSPTree *tree) {
    size_t *face_vec = zgl_Calloc(tree->num_faces, sizeof(*face_vec));

    for (size_t i_node = 0; i_node < tree->num_nodes; i_node++) {
        BSPNode *node = tree->nodes + i_node;
        for (size_t i_face = 0; i_face < node->num_faces; i_face++) {
            size_t face_tree_index = node->i_first_face + i_face;
            face_vec[face_tree_index]++;
        }
    }

    for (size_t i_face = 0; i_face < tree->num_faces; i_face++) {
        dibassert(face_vec[i_face] == 1);
    }

    zgl_Free(face_vec);
}
