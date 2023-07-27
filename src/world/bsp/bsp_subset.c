#include "src/world/bsp/bsp_internal.h"

static size_t *g_v_inclusion;
static size_t *g_e_inclusion;
static size_t *g_f_inclusion;

void clear_BSPSubset(BSPSubset *set) {
    zgl_Free(set->subVs);
    for (size_t i_subE = 0; i_subE < set->num_occupied_subE; i_subE++) {
        if (set->subEs[i_subE].cut_data) {
            zgl_Free(set->subEs[i_subE].cut_data);
        }
    }
    zgl_Free(set->subEs);
    zgl_Free(set->subFs);
}

void compute_VE_from_F(BSPSubset *p_subset) {
    // Given the face indices for this subset, determine which edges and
    // vertices are present.
    p_subset->num_subV = g_gen.num_verts;
    p_subset->num_subE = g_gen.num_edges;
    
    p_subset->subVs = zgl_Malloc(p_subset->num_subV*sizeof(*p_subset->subVs));
    p_subset->subEs = zgl_Malloc(p_subset->num_subE*sizeof(*p_subset->subEs));

    g_v_inclusion = zgl_Calloc(g_gen.num_verts, sizeof(*g_v_inclusion));
    g_e_inclusion = zgl_Calloc(g_gen.num_edges, sizeof(*g_e_inclusion));
    
    size_t acc_subV = 0;
    size_t acc_subE = 0;
    
    for (size_t i_subF = 0; i_subF < p_subset->num_subF; i_subF++) {
        FaceSubsetEntry *subF = p_subset->subFs + i_subF;
        Maker_BSPFace *mbsp_face = g_maker_tree.mbsp_faces + subF->i_f;

        size_t i_v0 = mbsp_face->i_v0;
        size_t i_v1 = mbsp_face->i_v1;
        size_t i_v2 = mbsp_face->i_v2;
        
        if ( ! g_v_inclusion[i_v0]) { // 0 means "not included"
            g_v_inclusion[i_v0] = acc_subV+1;
            
            p_subset->subVs[acc_subV].i_v = i_v0;            
            acc_subV++;
        }

        if ( ! g_v_inclusion[i_v1]) { // 0 means "not included"
            g_v_inclusion[i_v1] = acc_subV+1;
            
            p_subset->subVs[acc_subV].i_v = i_v1;
            acc_subV++;
        }

        if ( ! g_v_inclusion[i_v2]) { // 0 means "not included"
            g_v_inclusion[i_v2] = acc_subV+1;
            
            p_subset->subVs[acc_subV].i_v = i_v2;
            acc_subV++;
        }

        subF->i_subV0 = g_v_inclusion[i_v0]-1; // 0 means "not included"
        subF->i_subV1 = g_v_inclusion[i_v1]-1; // 0 means "not included"
        subF->i_subV2 = g_v_inclusion[i_v2]-1; // 0 means "not included"
        
        size_t i_e0 = mbsp_face->i_e0;
        size_t i_e1 = mbsp_face->i_e1;
        size_t i_e2 = mbsp_face->i_e2;

        if ( ! g_e_inclusion[i_e0]) { // 0 means "not included"
            g_e_inclusion[i_e0] = acc_subE+1;
            
            p_subset->subEs[acc_subE].i_e = i_e0;
            acc_subE++;
        }

        if ( ! g_e_inclusion[i_e1]) { // 0 means "not included"
            g_e_inclusion[i_e1] = acc_subE+1;
            
            p_subset->subEs[acc_subE].i_e = i_e1;
            acc_subE++;
        }

        if ( ! g_e_inclusion[i_e2]) { // 0 means "not included"
            g_e_inclusion[i_e2] = acc_subE+1; 
            
            p_subset->subEs[acc_subE].i_e = i_e2;
            acc_subE++;
        }

        subF->i_subE0 = g_e_inclusion[i_e0]-1; // 0 means "not included"
        subF->i_subE1 = g_e_inclusion[i_e1]-1; // 0 means "not included"
        subF->i_subE2 = g_e_inclusion[i_e2]-1; // 0 means "not included"
    }
    
    dibassert(acc_subV == p_subset->num_subV);

    //vbs_printf("(acc_subE %zu) (p_subset->num_subE %zu)\n", acc_subE, p_subset->num_subE);
    dibassert(acc_subE == p_subset->num_subE);
  
    // fill in the per-subE subV data
    for (size_t i_subE = 0; i_subE < p_subset->num_subE; i_subE++) {
        EdgeSubsetEntry *subE = p_subset->subEs + i_subE;
        Maker_BSPEdge *edge = g_maker_tree.msh_edges + subE->i_e;

        size_t i_v0 = edge->i_v0;
        size_t i_v1 = edge->i_v1;

        dibassert(g_v_inclusion[i_v0] > 0);
        dibassert(g_v_inclusion[i_v1] > 0);
        subE->i_subV0 = g_v_inclusion[i_v0]-1; // 0 means "not included"
        subE->i_subV1 = g_v_inclusion[i_v1]-1; // 0 means "not included"

        subE->cut_data = NULL;
    }

    p_subset->num_occupied_subV = p_subset->num_subV;
    p_subset->num_occupied_subE = p_subset->num_subE;

    zgl_Free(g_v_inclusion);
    zgl_Free(g_e_inclusion);
}

void initialize_first_BSPSubset(BSPSubset *p_subset) {
    p_subset->num_subF = g_gen.num_faces;
    p_subset->subFs = zgl_Malloc(p_subset->num_subF*sizeof(*p_subset->subFs));
    g_f_inclusion = zgl_Calloc(g_gen.num_faces, sizeof(*g_f_inclusion));
        
    for (size_t i_subF = 0; i_subF < p_subset->num_subF; i_subF++) {
        p_subset->subFs[i_subF].i_f = i_subF;
        g_f_inclusion[i_subF] = i_subF+1; // 0 means "not included"
    }

    p_subset->num_occupied_subF = p_subset->num_subF;
    
    compute_VE_from_F(p_subset);

    p_subset->mbsp_node = g_maker_tree.root;
    p_subset->mbsp_node->parent = NULL;

    p_subset->nonempty = true;

    zgl_Free(g_f_inclusion);
}
