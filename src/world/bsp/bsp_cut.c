#include "src/world/bsp/bsp_internal.h"

//#define NO_REPLACE

/*
  Notation:

  Sets of faces:
  pset - parent subset (that which gets split into a front and back subset)
  fset - front subset
  bset - back subset

*/

static size_t add_vertex_to_tree(Maker_BSPVertex v);

static size_t add_vertex_to_subset
(BSPSubset *set, size_t i_v);
    
static size_t add_edge_to_tree
(Maker_BSPEdge e);

static size_t add_edge_to_subset
(BSPSubset *set, size_t i_e, size_t i_subV0, size_t i_subV1);

static size_t replace_face_in_tree
(size_t i_f, Maker_BSPFace f);

static size_t add_face_to_tree
(Maker_BSPFace f);

/*
static size_t add_face_to_tree0
(size_t i_v0, size_t i_v1, size_t i_v2,
 size_t i_e0, size_t i_e1, size_t i_e2,
 bool from_cut, uint32_t i_gen_face);
*/

static size_t add_face_to_subset
(BSPSubset *set, FaceSubsetEntry subF);

/*
static size_t add_face_to_subset0
(BSPSubset *set, size_t i_f,
 size_t i_subV0, size_t i_subV1, size_t i_subV2,
 size_t i_subE0, size_t i_subE1, size_t i_subE2);
*/

/* Choosing a splitting plane */

// A splitting plane is less desirable if it:
// - creates too many subdivisions;
// - is severely unbalanced;
// - subdivides a non-nearly-degenerate face into a nearly-degenerate face;
// - subdivides a face into a near-zero-area face;


/** Returns TRUE if current_data is optimal and should replace optimal_data */
static void csd_heuristic(bool *early_exit, bool *replace, BSPCutData *current, BSPCutData *optimal) {
    if ((size_t)ABS((int)current->num_front_f - (int)current->num_back_f) + 3*current->num_cut_f
        <
        (size_t)ABS((int)optimal->num_front_f - (int)optimal->num_back_f) + 3*optimal->num_cut_f) {
        *replace = true;
    }
    else {
        *replace = false;
    }

    *early_exit = false;
}

/** Returns TRUE if current_data is optimal and should replace optimal_data */
static void csd_heuristic2(bool *early_exit, bool *replace, BSPCutData *current, BSPCutData *optimal) {
    // num_fset_f - num_bset_f is a measure of imbalance
    
    // type1 + 2*type2f + 2*type2b is the increase in the number of polygons as
    // a result of BSP cuts.
    
    if ((size_t)ABS((int)current->num_fset_f - (int)current->num_bset_f) + 3*(current->num_type1_cut_f + 2*current->num_type2f_cut_f + 2*current->num_type2b_cut_f)
        <
        (size_t)ABS((int)optimal->num_fset_f - (int)optimal->num_bset_f) + 3*(optimal->num_type1_cut_f + 2*optimal->num_type2f_cut_f + 2*optimal->num_type2b_cut_f)) {
        *replace = true;
    }
    else {
        *replace = false;
    }

    *early_exit = false;
}

static void csd_mincut1_balance2(bool *early_exit, bool *replace, BSPCutData *current, BSPCutData *optimal) {
    // TODO: find minimum cut. Among all the splitting planes with the
    // minimum number of cut, choose the most balanced.
    
    *replace = false;
    
    if (current->num_cut_f == optimal->num_cut_f) {
        if (optimal->balance < current->balance) {
            *replace = true;
        }
    }
    else if (current->num_cut_f < optimal->num_cut_f) {
        *replace = true;
    }

    *early_exit = false;
}

static void csd_balance1_mincut2(bool *early_exit, bool *replace, BSPCutData *current, BSPCutData *optimal) {
    *replace = false;
    
    if (ABS(optimal->balance - current->balance) < 0.01) {
        if (current->num_cut_f == optimal->num_cut_f) {
            *replace = true;
        }
    }
    else if (optimal->balance < current->balance) {
        *replace = true;
    }

    *early_exit = false;
}

static void csd_trivial(bool *early_exit, bool *replace, BSPCutData *current, BSPCutData *optimal) {
    (void)current;
    (void)optimal;
    
    *replace = true;
    *early_exit = true;
}

void choose_splitting_plane(BSPSubset *p_subset, BSPCutData *out_data) {
    size_t const num_subV = p_subset->num_subV;
    VtxeSubsetEntry const *subVs = p_subset->subVs;
    size_t const num_subE = p_subset->num_subE;
    EdgeSubsetEntry const *subEs = p_subset->subEs;
    size_t const num_subF = p_subset->num_subF;
    FaceSubsetEntry const *subFs = p_subset->subFs;
    
    BSPCutData data__A;
    BSPCutData data__B;

    dibassert(num_subF > 0);
    data__A.VSRs = zgl_Malloc(num_subV*sizeof(*data__A.VSRs));
    data__A.ESRs = zgl_Malloc(num_subE*sizeof(*data__A.ESRs));
    data__A.FSRs = zgl_Malloc(num_subF*sizeof(*data__A.FSRs));
    
    data__B.VSRs = zgl_Malloc(num_subV*sizeof(*data__B.VSRs));
    data__B.ESRs = zgl_Malloc(num_subE*sizeof(*data__B.ESRs));
    data__B.FSRs = zgl_Malloc(num_subF*sizeof(*data__B.FSRs));

    BSPCutData *optimal_data = &data__A;
    BSPCutData *current_data = &data__B;

    /* In general, we could go through a set of candidates *planes*, not
       faces. Right now the planes are obtained from polygons in the
       world. IDEA: Allow the world designer to suggest, demand, or force
       certain splitting planes, regardless of their relation to the actual
       polygons in the world geometry. */

    bool all_boundaries = true;
    
    for (size_t i_subF = 0; i_subF < num_subF; i_subF++) {
        size_t i_f = subFs[i_subF].i_f;
            
        WorldVec
            planeV = g_gen.vertices[g_gen.faces[g_maker_tree.mbsp_faces[i_f].i_gen_face].i_v[0]],
            planeN = g_gen.faces[g_maker_tree.mbsp_faces[i_f].i_gen_face].normal;

        compute_VSRs(planeV, planeN,
                     num_subV, subVs,
                     current_data->VSRs,
                     &current_data->num_coplanar_v,
                     &current_data->num_front_v,
                     &current_data->num_back_v);
        
        compute_ESRs(num_subE, subEs,
                     current_data->VSRs,
                     current_data->ESRs,
                     &current_data->num_coplanar_e,
                     &current_data->num_front_e,
                     &current_data->num_back_e,
                     &current_data->num_cut_e);

        compute_FSRs(num_subF, subFs,
                     current_data->VSRs,
                     current_data->FSRs,
                     &current_data->num_coplanar_f,
                     &current_data->num_front_f,
                     &current_data->num_back_f,
                     &current_data->num_type1_cut_f,
                     &current_data->num_type2f_cut_f,
                     &current_data->num_type2b_cut_f,
                     &current_data->num_cut_f);

        if (current_data->num_cut_f != 0 ||
            (current_data->num_front_f != 0 && current_data->num_back_f != 0)) {
            all_boundaries = false;
        }
        
        current_data->num_fset_v =
            current_data->num_front_v +
            current_data->num_coplanar_v +
            current_data->num_cut_e;
        current_data->num_fset_e =
            current_data->num_front_e +
            current_data->num_coplanar_e +
            current_data->num_cut_e +
            current_data->num_type1_cut_f +
            2*current_data->num_type2f_cut_f +
            current_data->num_type2b_cut_f;
        current_data->num_fset_f =
            current_data->num_front_f +
            current_data->num_type1_cut_f +
            2*current_data->num_type2f_cut_f +
            current_data->num_type2b_cut_f;

        current_data->num_bset_v =
            current_data->num_back_v +
            current_data->num_coplanar_v +
            current_data->num_cut_e;
        current_data->num_bset_e =
            current_data->num_back_e +
            current_data->num_coplanar_e +
            current_data->num_cut_e +
            current_data->num_type1_cut_f +
            current_data->num_type2f_cut_f +
            2*current_data->num_type2b_cut_f;
        current_data->num_bset_f =
            current_data->num_back_f +
            current_data->num_type1_cut_f +
            current_data->num_type2f_cut_f +
            2*current_data->num_type2b_cut_f;
        
        size_t num_fset_f = current_data->num_fset_f;
        size_t num_bset_f = current_data->num_bset_f;
        
        double balance;
        if (num_fset_f == 0 || num_bset_f == 0) {
            balance = 0;
        }
        else if (num_fset_f <= num_bset_f) {
            balance = (double)num_fset_f / (double)num_bset_f;
        }
        else {
            balance = (double)num_bset_f / (double)num_fset_f;
        }

        current_data->planeV = planeV;
        current_data->planeN = planeN;
        current_data->i_split_f = i_f;
        current_data->balance = balance;

        if (i_subF == 0) { // trivially accept first face as optimal, to seed
                           // the subsequent iterations.
            VtxeSpatialRelation *tmp_VSRs = optimal_data->VSRs;
            EdgeSpatialRelation *tmp_ESRs = optimal_data->ESRs;
            FaceSpatialRelation *tmp_FSRs = optimal_data->FSRs;
            *optimal_data = *current_data;
            optimal_data->VSRs = tmp_VSRs;
            optimal_data->ESRs = tmp_ESRs;
            optimal_data->FSRs = tmp_FSRs;
            
            memcpy(optimal_data->VSRs, current_data->VSRs,
                   num_subV*sizeof(*optimal_data->VSRs));
            memcpy(optimal_data->ESRs, current_data->ESRs,
                   num_subE*sizeof(*optimal_data->ESRs));
            memcpy(optimal_data->FSRs, current_data->FSRs,
                   num_subF*sizeof(*optimal_data->FSRs));
        }

        bool early_exit, replace;
        csd_mincut1_balance2(&early_exit, &replace, current_data, optimal_data);
        //csd_balance1_mincut2(&early_exit, &replace, current_data, optimal_data);
        //csd_heuristic(&early_exit, &replace, current_data, optimal_data);
 
        if (replace) {
            BSPCutData *temp_swap = current_data;
            current_data = optimal_data;
            optimal_data = temp_swap;
        }
        
        if (early_exit) {
            break;
        }

        continue;
    } /* end of split plane candidate loop */

    dibassert(out_data->VSRs == NULL &&
              out_data->ESRs == NULL &&
              out_data->FSRs == NULL);
    *out_data = *optimal_data; // copy all the scalar data
    
    out_data->VSRs = zgl_Malloc(num_subV*sizeof(*out_data->VSRs));
    out_data->ESRs = zgl_Malloc(num_subE*sizeof(*out_data->ESRs));
    out_data->FSRs = zgl_Malloc(num_subF*sizeof(*out_data->FSRs));
    memcpy(out_data->VSRs, optimal_data->VSRs, num_subV*sizeof(*out_data->VSRs));
    memcpy(out_data->ESRs, optimal_data->ESRs, num_subE*sizeof(*out_data->ESRs));
    memcpy(out_data->FSRs, optimal_data->FSRs, num_subF*sizeof(*out_data->FSRs));

    out_data->convex = all_boundaries;
    
    zgl_Free(data__A.VSRs);
    zgl_Free(data__A.ESRs);
    zgl_Free(data__A.FSRs);
    
    zgl_Free(data__B.VSRs);
    zgl_Free(data__B.ESRs);
    zgl_Free(data__B.FSRs);
}


typedef struct FaceCutData {
    BSPSubset const *pset;
    BSPSubset *fset;
    BSPSubset *bset;
    
    // Face in tree.
    size_t i_f;
    Maker_BSPFace const *f;

    // Face in parent subset.
    size_t i_pset_subF;
    FaceSubsetEntry const *pset_subF;
    
    VtxeSpatialRelation vsr0;
    VtxeSpatialRelation vsr1;
    VtxeSpatialRelation vsr2;

} FaceCutData;


void cut_type1(FaceCutData *fcd) {
    //    size_t i_pset_subF = fcd->i_pset_subF;
    FaceSubsetEntry const *pset_subF = fcd->pset_subF;

    size_t i_f = pset_subF->i_f;
    dibassert (i_f == fcd->i_f);
    Maker_BSPFace const *f = &g_maker_tree.mbsp_faces[i_f];
    dibassert(f == fcd->f);
    
    /* Get pset sub-indices of face. */
    size_t i_pset_subV0 = pset_subF->i_subV0;
    size_t i_pset_subV1 = pset_subF->i_subV1;
    size_t i_pset_subV2 = pset_subF->i_subV2;
    
    size_t i_pset_subE0 = pset_subF->i_subE0;
    size_t i_pset_subE1 = pset_subF->i_subE1;
    size_t i_pset_subE2 = pset_subF->i_subE2;

    /* The "v" and "e" indices refer to the vertex and edge indices of the face. */
    size_t i_v0 = fcd->pset->subVs[i_pset_subV0].i_v;
    size_t i_v1 = fcd->pset->subVs[i_pset_subV1].i_v;
    size_t i_v2 = fcd->pset->subVs[i_pset_subV2].i_v;
    
    size_t i_e0 = fcd->pset->subEs[i_pset_subE0].i_e;
    size_t i_e1 = fcd->pset->subEs[i_pset_subE1].i_e;
    size_t i_e2 = fcd->pset->subEs[i_pset_subE2].i_e;
    
    dibassert((i_v0 + i_v1 + i_v2 == f->i_v0 + f->i_v1 + f->i_v2) &&
              (i_e0 + i_e1 + i_e2 == f->i_e0 + f->i_e1 + f->i_e2));

    /* pset vertex data  ->  fset & bset vertex data */
    size_t i_fset_subV0 = fcd->pset->subVs[i_pset_subV0].i_fset_subV;
    size_t i_fset_subV1 = fcd->pset->subVs[i_pset_subV1].i_fset_subV;
    size_t i_fset_subV2 = fcd->pset->subVs[i_pset_subV2].i_fset_subV;

    size_t i_fset_subE0 = fcd->pset->subEs[i_pset_subE0].i_fset_subE;
    size_t i_fset_subE1 = fcd->pset->subEs[i_pset_subE1].i_fset_subE;
    size_t i_fset_subE2 = fcd->pset->subEs[i_pset_subE2].i_fset_subE;
    
    size_t i_bset_subV0 = fcd->pset->subVs[i_pset_subV0].i_bset_subV;
    size_t i_bset_subV1 = fcd->pset->subVs[i_pset_subV1].i_bset_subV;
    size_t i_bset_subV2 = fcd->pset->subVs[i_pset_subV2].i_bset_subV;
    
    size_t i_bset_subE0 = fcd->pset->subEs[i_pset_subE0].i_bset_subE;
    size_t i_bset_subE1 = fcd->pset->subEs[i_pset_subE1].i_bset_subE;
    size_t i_bset_subE2 = fcd->pset->subEs[i_pset_subE2].i_bset_subE;

    /* The "u" and "d" indices refer to the vertex and edge indicesthe diagram. TODO: Link it. */
    size_t i_u0, i_u1, i_u2, i_u3;
    size_t i_d0, i_d1, i_d2, i_d3, i_d4;

    size_t i_fset_subU0, i_fset_subU1, /*i_fset_subU2,*/ i_fset_subU3;
    size_t i_fset_subD0, /*i_fset_subD1,*/ /*i_fset_subD2,*/ i_fset_subD3, i_fset_subD4;

    size_t /*i_bset_subU0,*/ i_bset_subU1, i_bset_subU2, i_bset_subU3;
    size_t /*i_bset_subD0,*/ i_bset_subD1, i_bset_subD2, /*i_bset_subD3,*/ i_bset_subD4;


    bool clockwise;
    EdgeCutData cut_data;

    if (fcd->vsr1 == VSR_COPLANAR) { // 1.V2
        dibassert(fcd->pset->subEs[i_pset_subE2].cut_data != NULL);
        cut_data = *fcd->pset->subEs[i_pset_subE2].cut_data;
        
        if (fcd->vsr0 == VSR_FRONT) {
            clockwise = false;

            i_u0 = i_v0;
            i_fset_subU0 = i_fset_subV0;
        
            i_u1 = i_v1;
            i_fset_subU1 = i_fset_subV1;
            i_bset_subU1 = i_bset_subV1;
        
            i_u2 = i_v2;
            i_bset_subU2 = i_bset_subV2;
        
            i_d0 = i_e0;
            i_fset_subD0 = i_fset_subE0;
        
            i_d1 = i_e1;
            i_bset_subD1 = i_bset_subE1;
        }
        else if (fcd->vsr2 == VSR_FRONT) {
            clockwise = true;

            i_u0 = i_v2;
            i_fset_subU0 = i_fset_subV2;
        
            i_u1 = i_v1;
            i_fset_subU1 = i_fset_subV1;
            i_bset_subU1 = i_bset_subV1;
        
            i_u2 = i_v0;
            i_bset_subU2 = i_bset_subV0;
        
            i_d0 = i_e1;
            i_fset_subD0 = i_fset_subE1;
        
            i_d1 = i_e0;
            i_bset_subD1 = i_bset_subE0;
        }
        else {
            dibassert(false);
        }
    }
    else if (fcd->vsr2 == VSR_COPLANAR) {
        dibassert(fcd->pset->subEs[i_pset_subE0].cut_data != NULL);
        cut_data = *fcd->pset->subEs[i_pset_subE0].cut_data;

        if (fcd->vsr1 == VSR_FRONT) { // 1.V0.CCW
            clockwise = false;

            i_u0 = i_v1;
            i_fset_subU0 = i_fset_subV1;
        
            i_u1 = i_v2;
            i_fset_subU1 = i_fset_subV2;
            i_bset_subU1 = i_bset_subV2;
        
            i_u2 = i_v0;
            i_bset_subU2 = i_bset_subV0;
        
            i_d0 = i_e1;
            i_fset_subD0 = i_fset_subE1;
        
            i_d1 = i_e2;
            i_bset_subD1 = i_bset_subE2;
        }
        else if (fcd->vsr0 == VSR_FRONT) { // 1.V0.CW
            clockwise = true;

            i_u0 = i_v0;
            i_fset_subU0 = i_fset_subV0;
        
            i_u1 = i_v2;
            i_fset_subU1 = i_fset_subV2;
            i_bset_subU1 = i_bset_subV2;
        
            i_u2 = i_v1;
            i_bset_subU2 = i_bset_subV1;
        
            i_d0 = i_e2;
            i_fset_subD0 = i_fset_subE2;
        
            i_d1 = i_e1;
            i_bset_subD1 = i_bset_subE1;
        }
        else {
            dibassert(false);
        }
    }
    else if (fcd->vsr0 == VSR_COPLANAR) {
        dibassert(fcd->pset->subEs[i_pset_subE1].cut_data != NULL);
        cut_data = *fcd->pset->subEs[i_pset_subE1].cut_data;
        
        if (fcd->vsr2 == VSR_FRONT) { // 1.V1.CCW
            clockwise = false;
            
            i_u0 = i_v2;
            i_fset_subU0 = i_fset_subV2;
        
            i_u1 = i_v0;
            i_fset_subU1 = i_fset_subV0;
            i_bset_subU1 = i_bset_subV0;
        
            i_u2 = i_v1;
            i_bset_subU2 = i_bset_subV1;
        
            i_d0 = i_e2;
            i_fset_subD0 = i_fset_subE2;
        
            i_d1 = i_e0;
            i_bset_subD1 = i_bset_subE0;
        }
        else if (fcd->vsr1 == VSR_FRONT) { // 1.V1.CW
            clockwise = true;
            
            i_u0 = i_v1;
            i_fset_subU0 = i_fset_subV1;
        
            i_u1 = i_v0;
            i_fset_subU1 = i_fset_subV0;
            i_bset_subU1 = i_bset_subV0;
        
            i_u2 = i_v2;
            i_bset_subU2 = i_bset_subV2;
        
            i_d0 = i_e0;
            i_fset_subD0 = i_fset_subE0;
        
            i_d1 = i_e2;
            i_bset_subD1 = i_bset_subE2;
        }
        else {
            dibassert(false);
        }        
    }
    else {
        dibassert(false);
    }

    i_u3 = cut_data.i_v;
    i_fset_subU3 = cut_data.i_fset_subV;
    i_bset_subU3 = cut_data.i_bset_subV;
    
    i_d2 = cut_data.i_back_e;
    i_bset_subD2 = cut_data.i_bset_back_subE;
        
    i_d3 = cut_data.i_front_e;
    i_fset_subD3 = cut_data.i_fset_front_subE;

    Maker_BSPEdge d4 = {i_u1, i_u3, ME_NEW};
    i_d4 = add_edge_to_tree(d4);
    i_fset_subD4 =
        add_edge_to_subset(fcd->fset, i_d4, i_fset_subU1, i_fset_subU3);
    i_bset_subD4 =
        add_edge_to_subset(fcd->bset, i_d4, i_bset_subU1, i_bset_subU3);

    Maker_BSPFace front_f;
    Maker_BSPFace back_f;
    FaceSubsetEntry fset_subF;
    FaceSubsetEntry bset_subF;
    if (clockwise) {
        front_f.i_v0 = i_u0;
        front_f.i_v1 = i_u3;
        front_f.i_v2 = i_u1;
        front_f.i_e0 = i_d3;
        front_f.i_e1 = i_d4;
        front_f.i_e2 = i_d0;
                        
        back_f.i_v0 = i_u1;
        back_f.i_v1 = i_u3;
        back_f.i_v2 = i_u2;
        back_f.i_e0 = i_d4;
        back_f.i_e1 = i_d2; 
        back_f.i_e2 = i_d1;
                        
        fset_subF.i_subV0 = i_fset_subU0;
        fset_subF.i_subV1 = i_fset_subU3;
        fset_subF.i_subV2 = i_fset_subU1;
        fset_subF.i_subE0 = i_fset_subD3;
        fset_subF.i_subE1 = i_fset_subD4;
        fset_subF.i_subE2 = i_fset_subD0;
                        
        bset_subF.i_subV0 = i_bset_subU1;
        bset_subF.i_subV1 = i_bset_subU3;
        bset_subF.i_subV2 = i_bset_subU2;
        bset_subF.i_subE0 = i_bset_subD4;
        bset_subF.i_subE1 = i_bset_subD2;
        bset_subF.i_subE2 = i_bset_subD1;
    }
    else {
        front_f.i_v0 = i_u0;
        front_f.i_v1 = i_u1;
        front_f.i_v2 = i_u3;
        front_f.i_e0 = i_d0;
        front_f.i_e1 = i_d4;
        front_f.i_e2 = i_d3;
                        
        back_f.i_v0 = i_u3;
        back_f.i_v1 = i_u1;
        back_f.i_v2 = i_u2;
        back_f.i_e0 = i_d4;
        back_f.i_e1 = i_d1; 
        back_f.i_e2 = i_d2;
                        
        fset_subF.i_subV0 = i_fset_subU0;
        fset_subF.i_subV1 = i_fset_subU1;
        fset_subF.i_subV2 = i_fset_subU3;
        fset_subF.i_subE0 = i_fset_subD0;
        fset_subF.i_subE1 = i_fset_subD4;
        fset_subF.i_subE2 = i_fset_subD3;
                        
        bset_subF.i_subV0 = i_bset_subU3;
        bset_subF.i_subV1 = i_bset_subU1;
        bset_subF.i_subV2 = i_bset_subU2;
        bset_subF.i_subE0 = i_bset_subD4;
        bset_subF.i_subE1 = i_bset_subD1;
        bset_subF.i_subE2 = i_bset_subD2;
    }
    
    front_f.flags = f->flags | MF_FROM_CUT;
    back_f.flags  = f->flags | MF_FROM_CUT;
    
    front_f.i_gen_face = f->i_gen_face;
    back_f.i_gen_face  = f->i_gen_face;

    front_f.next_in_node = NULL;
    back_f.next_in_node = NULL;
    
#ifdef NO_REPLACE
    size_t i_front_f = add_face_to_tree(front_f);
#else
    size_t i_front_f = replace_face_in_tree(i_f, front_f);
#endif
    
    size_t i_back_f  = add_face_to_tree(back_f);

    fset_subF.i_f = i_front_f;
    bset_subF.i_f = i_back_f;

    //    vbs_puts("CUT TYPE1 FRONT SUBSET");
    add_face_to_subset(fcd->fset, fset_subF);
    
    //    vbs_puts("CUT TYPE1 BACK SUBSET");
    add_face_to_subset(fcd->bset, bset_subF);
}

void cut_type2f(FaceCutData *fcd) {
    FaceSubsetEntry const *pset_subF = fcd->pset_subF;

    size_t i_f = pset_subF->i_f;
    dibassert (i_f == fcd->i_f);
    Maker_BSPFace const *f = &g_maker_tree.mbsp_faces[i_f];
    dibassert(f == fcd->f);
    
    /* Get pset sub-indices of face. */
    size_t i_pset_V0 = pset_subF->i_subV0;
    size_t i_pset_V1 = pset_subF->i_subV1;
    size_t i_pset_V2 = pset_subF->i_subV2;
    
    size_t i_pset_E0 = pset_subF->i_subE0;
    size_t i_pset_E1 = pset_subF->i_subE1;
    size_t i_pset_E2 = pset_subF->i_subE2;

    /* The "v" and "e" indices refer to the vertex and edge indices of the face. */
    size_t i_v0 = fcd->pset->subVs[i_pset_V0].i_v;
    size_t i_v1 = fcd->pset->subVs[i_pset_V1].i_v;
    size_t i_v2 = fcd->pset->subVs[i_pset_V2].i_v;
    
    size_t i_e0 = fcd->pset->subEs[i_pset_E0].i_e;
    size_t i_e1 = fcd->pset->subEs[i_pset_E1].i_e;
    size_t i_e2 = fcd->pset->subEs[i_pset_E2].i_e;
    
    dibassert((i_v0 + i_v1 + i_v2 == f->i_v0 + f->i_v1 + f->i_v2) &&
              (i_e0 + i_e1 + i_e2 == f->i_e0 + f->i_e1 + f->i_e2));

    /* pset vertex data  ->  fset & bset vertex data */
    size_t i_fset_V0 = fcd->pset->subVs[i_pset_V0].i_fset_subV;
    size_t i_fset_V1 = fcd->pset->subVs[i_pset_V1].i_fset_subV;
    size_t i_fset_V2 = fcd->pset->subVs[i_pset_V2].i_fset_subV;

    size_t i_fset_E0 = fcd->pset->subEs[i_pset_E0].i_fset_subE;
    size_t i_fset_E1 = fcd->pset->subEs[i_pset_E1].i_fset_subE;
    size_t i_fset_E2 = fcd->pset->subEs[i_pset_E2].i_fset_subE;
    
    size_t i_bset_V0 = fcd->pset->subVs[i_pset_V0].i_bset_subV;
    size_t i_bset_V1 = fcd->pset->subVs[i_pset_V1].i_bset_subV;
    size_t i_bset_V2 = fcd->pset->subVs[i_pset_V2].i_bset_subV;
    
    //size_t i_bset_E0 = fcd->pset->subEs[i_pset_E0].i_bset_subE;
    //size_t i_bset_E1 = fcd->pset->subEs[i_pset_E1].i_bset_subE;
    //size_t i_bset_E2 = fcd->pset->subEs[i_pset_E2].i_bset_subE;

    size_t i_u0, i_u1, i_u2, i_u3, i_u4;
    size_t i_d0, i_d1, i_d2, i_d3, i_d4, i_d5, i_d6;

    size_t i_fset_U0, i_fset_U1, i_fset_U2, /*i_fset_U3,*/ i_fset_U4;
    size_t i_fset_D0, i_fset_D1, /*i_fset_D2,*/ /*i_fset_D3,*/ i_fset_D4,
        i_fset_D5, i_fset_D6;

    size_t /*i_bset_U0,*/ /*i_bset_U1,*/ i_bset_U2, i_bset_U3, i_bset_U4;
    size_t /*i_bset_D0,*/ /*i_bset_D1,*/ i_bset_D2, i_bset_D3, /*i_bset_D4,*/
        i_bset_D5/*, i_bset_D6*/;

    EdgeCutData cut_data0;
    EdgeCutData cut_data1;
    if (fcd->vsr2 == VSR_BACK) {
        dibassert(fcd->pset->subEs[i_pset_E1].cut_data != NULL);
        cut_data0 = *fcd->pset->subEs[i_pset_E1].cut_data;
        dibassert(fcd->pset->subEs[i_pset_E2].cut_data != NULL);
        cut_data1 = *fcd->pset->subEs[i_pset_E2].cut_data;
        
        i_u0 = i_v0;
        i_fset_U0 = i_fset_V0;
        
        i_u1 = i_v1;
        i_fset_U1 = i_fset_V1;
        
        i_u3 = i_v2;
        i_bset_U3 = i_bset_V2;
        
        i_d0 = i_e0;
        i_fset_D0 = i_fset_E0;
    }
    else if (fcd->vsr1 == VSR_BACK) {
        dibassert(fcd->pset->subEs[i_pset_E0].cut_data != NULL);
        cut_data0 = *fcd->pset->subEs[i_pset_E0].cut_data;
        dibassert(fcd->pset->subEs[i_pset_E1].cut_data != NULL);
        cut_data1 = *fcd->pset->subEs[i_pset_E1].cut_data;
        
        i_u0 = i_v2;
        i_fset_U0 = i_fset_V2;
        
        i_u1 = i_v0;
        i_fset_U1 = i_fset_V0;
        
        i_u3 = i_v1;
        i_bset_U3 = i_bset_V1;
        
        i_d0 = i_e2;
        i_fset_D0 = i_fset_E2;
    }
    else if (fcd->vsr0 == VSR_BACK) {
        dibassert(fcd->pset->subEs[i_pset_E2].cut_data != NULL);
        cut_data0 = *fcd->pset->subEs[i_pset_E2].cut_data;
        dibassert(fcd->pset->subEs[i_pset_E0].cut_data != NULL);
        cut_data1 = *fcd->pset->subEs[i_pset_E0].cut_data;
        
        i_u0 = i_v1;
        i_fset_U0 = i_fset_V1;
        
        i_u1 = i_v2;
        i_fset_U1 = i_fset_V2;
        
        i_u3 = i_v0;
        i_bset_U3 = i_bset_V0;
        
        i_d0 = i_e1;
        i_fset_D0 = i_fset_E1;
    }
    else {
        dibassert(false);
    }

    i_u2      = cut_data0.i_v;
    i_fset_U2 = cut_data0.i_fset_subV;
    i_bset_U2 = cut_data0.i_bset_subV;
    
    i_u4      = cut_data1.i_v;
    i_fset_U4 = cut_data1.i_fset_subV;
    i_bset_U4 = cut_data1.i_bset_subV;

    i_d1      = cut_data0.i_front_e;
    i_fset_D1 = cut_data0.i_fset_front_subE;
    
    i_d2      = cut_data0.i_back_e;
    i_bset_D2 = cut_data0.i_bset_back_subE;
    
    i_d3      = cut_data1.i_back_e;
    i_bset_D3 = cut_data1.i_bset_back_subE;
    
    i_d4      = cut_data1.i_front_e;
    i_fset_D4 = cut_data1.i_fset_front_subE;

    Maker_BSPEdge d5 = {i_u2, i_u4, ME_NEW};
    i_d5      = add_edge_to_tree(d5);
    i_fset_D5 = add_edge_to_subset(fcd->fset, i_d5, i_fset_U2, i_fset_U4);
    i_bset_D5 = add_edge_to_subset(fcd->bset, i_d5, i_bset_U2, i_bset_U4);

    Maker_BSPEdge d6 = {i_u1, i_u4, ME_NEW};
    i_d6      = add_edge_to_tree(d6);
    i_fset_D6 = add_edge_to_subset(fcd->fset, i_d6, i_fset_U1, i_fset_U4);

    Maker_BSPFace front0_f;
    Maker_BSPFace front1_f;
    Maker_BSPFace back_f;
    FaceSubsetEntry fset0_F;
    FaceSubsetEntry fset1_F;
    FaceSubsetEntry bset_F;

    front0_f.i_v0 = i_u0;
    front0_f.i_v1 = i_u1;
    front0_f.i_v2 = i_u4;
    front0_f.i_e0 = i_d0;
    front0_f.i_e1 = i_d6;
    front0_f.i_e2 = i_d4;

    front1_f.i_v0 = i_u1;
    front1_f.i_v1 = i_u2;
    front1_f.i_v2 = i_u4;
    front1_f.i_e0 = i_d1;
    front1_f.i_e1 = i_d5;
    front1_f.i_e2 = i_d6;

    back_f.i_v0 = i_u2;
    back_f.i_v1 = i_u3;
    back_f.i_v2 = i_u4;
    back_f.i_e0 = i_d2;
    back_f.i_e1 = i_d3; 
    back_f.i_e2 = i_d5;
                        
    fset0_F.i_subV0 = i_fset_U0;
    fset0_F.i_subV1 = i_fset_U1;
    fset0_F.i_subV2 = i_fset_U4;
    fset0_F.i_subE0 = i_fset_D0;
    fset0_F.i_subE1 = i_fset_D6;
    fset0_F.i_subE2 = i_fset_D4;

    fset1_F.i_subV0 = i_fset_U1;
    fset1_F.i_subV1 = i_fset_U2;
    fset1_F.i_subV2 = i_fset_U4;
    fset1_F.i_subE0 = i_fset_D1;
    fset1_F.i_subE1 = i_fset_D5;
    fset1_F.i_subE2 = i_fset_D6;
                        
    bset_F.i_subV0 = i_bset_U2;
    bset_F.i_subV1 = i_bset_U3;
    bset_F.i_subV2 = i_bset_U4;
    bset_F.i_subE0 = i_bset_D2;
    bset_F.i_subE1 = i_bset_D3;
    bset_F.i_subE2 = i_bset_D5;

    front0_f.flags = f->flags | MF_FROM_CUT;
    front1_f.flags = f->flags | MF_FROM_CUT;
    back_f.flags   = f->flags | MF_FROM_CUT;

    front0_f.i_gen_face = f->i_gen_face;
    front1_f.i_gen_face = f->i_gen_face;
    back_f.i_gen_face   = f->i_gen_face;

    front0_f.next_in_node = NULL;
    front1_f.next_in_node = NULL;
    back_f.next_in_node   = NULL;

#ifdef NO_REPLACE
    size_t i_front0_f = add_face_to_tree(front0_f);
#else
    size_t i_front0_f = replace_face_in_tree(i_f, front0_f);
#endif

    size_t i_front1_f = add_face_to_tree(front1_f);
    size_t i_back_f   = add_face_to_tree(back_f);

    fset0_F.i_f = i_front0_f;
    fset1_F.i_f = i_front1_f;
    bset_F.i_f  = i_back_f;

    add_face_to_subset(fcd->fset, fset0_F);
    add_face_to_subset(fcd->fset, fset1_F);
    add_face_to_subset(fcd->bset, bset_F);
}

void cut_type2b(FaceCutData *fcd) {
    FaceSubsetEntry const *pset_subF = fcd->pset_subF;

    size_t i_f = pset_subF->i_f;
    dibassert (i_f == fcd->i_f);
    Maker_BSPFace const *f = &g_maker_tree.mbsp_faces[i_f];
    dibassert(f == fcd->f);
    
    /* Get pset sub-indices of face. */
    size_t i_pset_V0 = pset_subF->i_subV0;
    size_t i_pset_V1 = pset_subF->i_subV1;
    size_t i_pset_V2 = pset_subF->i_subV2;
    
    size_t i_pset_E0 = pset_subF->i_subE0;
    size_t i_pset_E1 = pset_subF->i_subE1;
    size_t i_pset_E2 = pset_subF->i_subE2;

    /* The "v" and "e" indices refer to the vertex and edge indices of the face. */
    size_t i_v0 = fcd->pset->subVs[i_pset_V0].i_v;
    size_t i_v1 = fcd->pset->subVs[i_pset_V1].i_v;
    size_t i_v2 = fcd->pset->subVs[i_pset_V2].i_v;
    
    size_t i_e0 = fcd->pset->subEs[i_pset_E0].i_e;
    size_t i_e1 = fcd->pset->subEs[i_pset_E1].i_e;
    size_t i_e2 = fcd->pset->subEs[i_pset_E2].i_e;
    
    dibassert((i_v0 + i_v1 + i_v2 == f->i_v0 + f->i_v1 + f->i_v2) &&
              (i_e0 + i_e1 + i_e2 == f->i_e0 + f->i_e1 + f->i_e2));

    /* pset vertex data  ->  fset & bset vertex data */
    size_t i_fset_V0 = fcd->pset->subVs[i_pset_V0].i_fset_subV;
    size_t i_fset_V1 = fcd->pset->subVs[i_pset_V1].i_fset_subV;
    size_t i_fset_V2 = fcd->pset->subVs[i_pset_V2].i_fset_subV;

    //size_t i_fset_E0 = fcd->pset->subEs[i_pset_E0].i_fset_subE;
    //size_t i_fset_E1 = fcd->pset->subEs[i_pset_E1].i_fset_subE;
    //size_t i_fset_E2 = fcd->pset->subEs[i_pset_E2].i_fset_subE;
    
    size_t i_bset_V0 = fcd->pset->subVs[i_pset_V0].i_bset_subV;
    size_t i_bset_V1 = fcd->pset->subVs[i_pset_V1].i_bset_subV;
    size_t i_bset_V2 = fcd->pset->subVs[i_pset_V2].i_bset_subV;
    
    size_t i_bset_E0 = fcd->pset->subEs[i_pset_E0].i_bset_subE;
    size_t i_bset_E1 = fcd->pset->subEs[i_pset_E1].i_bset_subE;
    size_t i_bset_E2 = fcd->pset->subEs[i_pset_E2].i_bset_subE;

    size_t i_u0, i_u1, i_u2, i_u3, i_u4;
    size_t i_d0, i_d1, i_d2, i_d3, i_d4, i_d5, i_d6;

    size_t i_fset_U0, i_fset_U1, /*i_fset_U2,*/ /*i_fset_U3,*/ i_fset_U4;
    size_t i_fset_D0, /*i_fset_D1,*/ /*i_fset_D2,*/ /*i_fset_D3,*/ i_fset_D4,
        i_fset_D5/*, i_fset_D6*/;

    size_t /*i_bset_U0,*/ i_bset_U1, i_bset_U2, i_bset_U3, i_bset_U4;
    size_t /*i_bset_D0,*/ i_bset_D1, i_bset_D2, i_bset_D3, /*i_bset_D4,*/
        i_bset_D5, i_bset_D6;


    EdgeCutData cut_data0;
    EdgeCutData cut_data1;
    if (fcd->vsr0 == VSR_FRONT) {
        dibassert(fcd->pset->subEs[i_pset_E0].cut_data != NULL);
        cut_data0 = *fcd->pset->subEs[i_pset_E0].cut_data;
        dibassert(fcd->pset->subEs[i_pset_E2].cut_data != NULL);
        cut_data1 = *fcd->pset->subEs[i_pset_E2].cut_data;
        
        i_u0 = i_v0;
        i_fset_U0 = i_fset_V0;
        
        i_u2 = i_v1;
        i_bset_U2 = i_bset_V1;
        
        i_u3 = i_v2;
        i_bset_U3 = i_bset_V2;
        
        i_d2 = i_e1;
        i_bset_D2 = i_bset_E1;
    }
    else if (fcd->vsr1 == VSR_FRONT) {
        dibassert(fcd->pset->subEs[i_pset_E1].cut_data != NULL);
        cut_data0 = *fcd->pset->subEs[i_pset_E1].cut_data;
        dibassert(fcd->pset->subEs[i_pset_E0].cut_data != NULL);
        cut_data1 = *fcd->pset->subEs[i_pset_E0].cut_data;
        
        i_u0 = i_v1;
        i_fset_U0 = i_fset_V1;
        
        i_u2 = i_v2;
        i_bset_U2 = i_bset_V2;
        
        i_u3 = i_v0;
        i_bset_U3 = i_bset_V0;
        
        i_d2 = i_e2;
        i_bset_D2 = i_bset_E2;
    }
    else if (fcd->vsr2 == VSR_FRONT) {
        dibassert(fcd->pset->subEs[i_pset_E2].cut_data != NULL);
        cut_data0 = *fcd->pset->subEs[i_pset_E2].cut_data;
        dibassert(fcd->pset->subEs[i_pset_E1].cut_data != NULL);
        cut_data1 = *fcd->pset->subEs[i_pset_E1].cut_data;
        
        i_u0 = i_v2;
        i_fset_U0 = i_fset_V2;
        
        i_u2 = i_v0;
        i_bset_U2 = i_bset_V0;
        
        i_u3 = i_v1;
        i_bset_U3 = i_bset_V1;
        
        i_d2 = i_e0;
        i_bset_D2 = i_bset_E0;
    }
    else {
        dibassert(false);
    }

    i_u1      = cut_data0.i_v;
    i_fset_U1 = cut_data0.i_fset_subV;
    i_bset_U1 = cut_data0.i_bset_subV;
    
    i_u4      = cut_data1.i_v;
    i_fset_U4 = cut_data1.i_fset_subV;
    i_bset_U4 = cut_data1.i_bset_subV;

    i_d0      = cut_data0.i_front_e;
    i_fset_D0 = cut_data0.i_fset_front_subE;
    
    i_d1      = cut_data0.i_back_e;
    i_bset_D1 = cut_data0.i_bset_back_subE;
    
    i_d3      = cut_data1.i_back_e;
    i_bset_D3 = cut_data1.i_bset_back_subE;
    
    i_d4      = cut_data1.i_front_e;
    i_fset_D4 = cut_data1.i_fset_front_subE;

    Maker_BSPEdge d5 = {i_u1, i_u4, ME_NEW};
    i_d5      = add_edge_to_tree(d5);
    i_fset_D5 = add_edge_to_subset(fcd->fset, i_d5, i_fset_U1, i_fset_U4);
    i_bset_D5 = add_edge_to_subset(fcd->bset, i_d5, i_bset_U1, i_bset_U4);

    Maker_BSPEdge d6 = {i_u1, i_u3, ME_NEW};
    i_d6      = add_edge_to_tree(d6);
    i_bset_D6 = add_edge_to_subset(fcd->bset, i_d6, i_bset_U1, i_bset_U3);

    Maker_BSPFace front_f;
    Maker_BSPFace back0_f;
    Maker_BSPFace back1_f;
    FaceSubsetEntry fset_F;
    FaceSubsetEntry bset0_F;
    FaceSubsetEntry bset1_F;

    front_f.i_v0 = i_u0;
    front_f.i_v1 = i_u1;
    front_f.i_v2 = i_u4;
    front_f.i_e0 = i_d0;
    front_f.i_e1 = i_d5;
    front_f.i_e2 = i_d4;

    back0_f.i_v0 = i_u1;
    back0_f.i_v1 = i_u3;
    back0_f.i_v2 = i_u4;
    back0_f.i_e0 = i_d6;
    back0_f.i_e1 = i_d3;
    back0_f.i_e2 = i_d5;

    back1_f.i_v0 = i_u2;
    back1_f.i_v1 = i_u3;
    back1_f.i_v2 = i_u1;
    back1_f.i_e0 = i_d2;
    back1_f.i_e1 = i_d6; 
    back1_f.i_e2 = i_d1;
                        
    fset_F.i_subV0 = i_fset_U0;
    fset_F.i_subV1 = i_fset_U1;
    fset_F.i_subV2 = i_fset_U4;
    fset_F.i_subE0 = i_fset_D0;
    fset_F.i_subE1 = i_fset_D5;
    fset_F.i_subE2 = i_fset_D4;

    bset0_F.i_subV0 = i_bset_U1;
    bset0_F.i_subV1 = i_bset_U3;
    bset0_F.i_subV2 = i_bset_U4;
    bset0_F.i_subE0 = i_bset_D6;
    bset0_F.i_subE1 = i_bset_D3;
    bset0_F.i_subE2 = i_bset_D5;
                        
    bset1_F.i_subV0 = i_bset_U2;
    bset1_F.i_subV1 = i_bset_U3;
    bset1_F.i_subV2 = i_bset_U1;
    bset1_F.i_subE0 = i_bset_D2;
    bset1_F.i_subE1 = i_bset_D6;
    bset1_F.i_subE2 = i_bset_D1;

    front_f.flags = f->flags | MF_FROM_CUT;
    back0_f.flags = f->flags | MF_FROM_CUT;
    back1_f.flags = f->flags | MF_FROM_CUT;

    front_f.i_gen_face = f->i_gen_face;
    back0_f.i_gen_face = f->i_gen_face;
    back1_f.i_gen_face = f->i_gen_face;

    front_f.next_in_node = NULL;
    back0_f.next_in_node = NULL;
    back1_f.next_in_node = NULL;

#ifdef NO_REPLACE
    size_t i_front_f = add_face_to_tree(front_f);
#else
    size_t i_front_f = replace_face_in_tree(i_f, front_f);
#endif

    size_t i_back0_f = add_face_to_tree(back0_f);
    size_t i_back1_f = add_face_to_tree(back1_f);

    fset_F.i_f  = i_front_f;
    bset0_F.i_f = i_back0_f;
    bset1_F.i_f = i_back1_f;

    add_face_to_subset(fcd->fset, fset_F);
    add_face_to_subset(fcd->bset, bset0_F);
    add_face_to_subset(fcd->bset, bset1_F);
}

void cut_vertices(BSPSubset const *pset, BSPSubset *fset, BSPSubset *bset, VtxeSpatialRelation *VSRs) {
    // Sort each pre-vertex into front or back subset.

    // IDEA: A coplanar vertex doesn't necessarily appear as the vertex on both a
    // front and a back face, does it?
    fset->num_occupied_subV = 0;
    bset->num_occupied_subV = 0;
    for (size_t i_pset_subV = 0; i_pset_subV < pset->num_subV; i_pset_subV++) {
        size_t i_v = pset->subVs[i_pset_subV].i_v;
        switch (VSRs[i_pset_subV]) {
        case VSR_COPLANAR: {
            //if (fset->nonempty) {
                size_t i_fset_subV = add_vertex_to_subset(fset, i_v);
                pset->subVs[i_pset_subV].i_fset_subV = i_fset_subV;
                //}
                
                //if (bset->nonempty) {
                size_t i_bset_subV = add_vertex_to_subset(bset, i_v);
                pset->subVs[i_pset_subV].i_bset_subV = i_bset_subV;    
                //}
        }
            break;
        case VSR_FRONT: {
            //dibassert(fset->nonempty);
            size_t i_fset_subV = add_vertex_to_subset(fset, i_v);
            pset->subVs[i_pset_subV].i_fset_subV = i_fset_subV;
        }
            break;
        case VSR_BACK: {
            //dibassert(bset->nonempty);
            size_t i_bset_subV = add_vertex_to_subset(bset, i_v);
            pset->subVs[i_pset_subV].i_bset_subV = i_bset_subV;
        }
            break;
        }
    }
}

void cut_edges(BSPSubset const *pset, BSPSubset *fset, BSPSubset *bset, VtxeSpatialRelation *VSRs, EdgeSpatialRelation *ESRs, WorldVec const *const splitV, WorldVec const *const splitN) {
    // Sort and/or cut pre-edges. Cutting involves creating new vertex and
    // sorting it appropriately.
    fset->num_occupied_subE = 0;
    bset->num_occupied_subE = 0;
    for (size_t i_pset_subE = 0; i_pset_subE < pset->num_subE; i_pset_subE++) {
        EdgeSubsetEntry *pset_subE = &pset->subEs[i_pset_subE];
                
        size_t i_e = pset_subE->i_e;

        size_t i_pset_subV0 = pset_subE->i_subV0;
        size_t i_pset_subV1 = pset_subE->i_subV1;
        
        size_t i_fset_subV0 = pset->subVs[pset_subE->i_subV0].i_fset_subV;
        size_t i_fset_subV1 = pset->subVs[pset_subE->i_subV1].i_fset_subV;
        
        size_t i_bset_subV0 = pset->subVs[pset_subE->i_subV0].i_bset_subV;
        size_t i_bset_subV1 = pset->subVs[pset_subE->i_subV1].i_bset_subV;
        
        VtxeSpatialRelation vsr0 = VSRs[i_pset_subV0];
        VtxeSpatialRelation vsr1 = VSRs[i_pset_subV1];
        
        Maker_BSPEdge *msh_edge = &g_maker_tree.msh_edges[i_e];

        switch (ESRs[i_pset_subE]) {
        case ESR_COPLANAR: {
            //            if (fset->nonempty) {
                size_t i_fset_subE =
                    add_edge_to_subset(fset, i_e, i_fset_subV0, i_fset_subV1);
                pset_subE->i_fset_subE = i_fset_subE;
                //            }
            
                //            if (bset->nonempty) {
                size_t i_bset_subE =
                    add_edge_to_subset(bset, i_e, i_bset_subV0, i_bset_subV1);
                pset_subE->i_bset_subE = i_bset_subE;
                //            }
        }
            break;
        case ESR_FRONT: {
            //dibassert(fset->nonempty);
            size_t i_fset_subE =
                add_edge_to_subset(fset, i_e, i_fset_subV0, i_fset_subV1);
            pset_subE->i_fset_subE = i_fset_subE;
        }
            break;
        case ESR_BACK: {
            //dibassert(bset->nonempty);
            size_t i_bset_subE =
                add_edge_to_subset(bset, i_e, i_bset_subV0, i_bset_subV1);
            pset_subE->i_bset_subE = i_bset_subE;
        }
            break;
        case ESR_CUT: {
            //            dibassert(fset->nonempty);
            //            dibassert(bset->nonempty);
            pset_subE->cut_data = zgl_Malloc(sizeof(*pset_subE->cut_data));

            size_t i_v0 = msh_edge->i_v0;
            size_t i_v1 = msh_edge->i_v1;
            
            // Make new vertex.
            Maker_BSPVertex v_new;

            find_intersection(g_maker_tree.vertices[i_v0].vertex,
                              g_maker_tree.vertices[i_v1].vertex,
                              *splitV, *splitN, &v_new.vertex);

            // TODO: Assert that the new vertex is sufficiently close to being
            // coplanar.
            dibassert(BSP_sideof(v_new.vertex, *splitV, *splitN) == SIDE_MID);
            
            // Store  new vertex...
            v_new.flags = 0 | MV_FROM_CUT;
            
            // ... in tree
            size_t i_v_new =
                add_vertex_to_tree(v_new);
            pset_subE->cut_data->i_v = i_v_new;
            
            // ... in front subset
            size_t i_fset_subV_new;
                i_fset_subV_new =
                    add_vertex_to_subset(fset, i_v_new);
                pset_subE->cut_data->i_fset_subV = i_fset_subV_new;
            
            // ... in back subset
            size_t i_bset_subV_new;
                i_bset_subV_new =
                    add_vertex_to_subset(bset, i_v_new);
                pset_subE->cut_data->i_bset_subV = i_bset_subV_new;
                
            // Make new edges.
            Maker_BSPEdge front_e;
            Maker_BSPEdge back_e;
            EdgeSubsetEntry fset_front_subE;
            EdgeSubsetEntry bset_back_subE;
            if (vsr0 == VSR_FRONT) {
                dibassert(vsr1 == VSR_BACK);
                front_e.i_v0 = i_v0;
                front_e.i_v1 = i_v_new;
                front_e.flags = msh_edge->flags | ME_FROM_CUT;
                back_e.i_v0 = i_v_new;
                back_e.i_v1 = i_v1;
                back_e.flags = msh_edge->flags | ME_FROM_CUT;

                fset_front_subE.i_subV0 = i_fset_subV0;
                fset_front_subE.i_subV1 = i_fset_subV_new;
                bset_back_subE.i_subV0  = i_bset_subV_new;    
                
                bset_back_subE.i_subV1  = i_bset_subV1;
            }
            else if (vsr0 == VSR_BACK) {
                dibassert(vsr1 == VSR_FRONT);
                front_e.i_v0 = i_v1;
                front_e.i_v1 = i_v_new;
                back_e.i_v0 = i_v_new;
                back_e.i_v1 = i_v0;


                fset_front_subE.i_subV0 = i_fset_subV1;
                fset_front_subE.i_subV1 = i_fset_subV_new;
                bset_back_subE.i_subV0  = i_bset_subV_new;
                bset_back_subE.i_subV1  = i_bset_subV0;
            }
            else {
                dibassert(false);
            }
                
            // Store new edges...
            front_e.flags = msh_edge->flags | ME_FROM_CUT;
            back_e.flags = msh_edge->flags | ME_FROM_CUT;
            
            // ... in tree
            size_t i_front_e =
                add_edge_to_tree(front_e);
            size_t i_back_e =
                add_edge_to_tree(back_e);

            // ... in front subset
            size_t i_fset_front_subE;
                i_fset_front_subE =
                    add_edge_to_subset(fset, i_front_e,
                                       fset_front_subE.i_subV0,
                                       fset_front_subE.i_subV1);
            
            // ... in back subset
            size_t i_bset_back_subE;
                i_bset_back_subE = 
                    add_edge_to_subset(bset, i_back_e,
                                       bset_back_subE.i_subV0,
                                       bset_back_subE.i_subV1);
            
            // ... in original edge cut_data
            pset_subE->cut_data->i_front_e = i_front_e;
            pset_subE->cut_data->i_back_e = i_back_e;
            pset_subE->cut_data->i_fset_front_subE = i_fset_front_subE;
            pset_subE->cut_data->i_bset_back_subE = i_bset_back_subE;
        }
            break;
        }
    }

}

void cut_faces(BSPSubset const *pset, BSPSubset *fset, BSPSubset *bset, VtxeSpatialRelation *VSRs, FaceSpatialRelation *FSRs) {
    fset->num_occupied_subF = 0;
    bset->num_occupied_subF = 0;
    
    //vbs_puts("FACECUT BEGIN");
    
    for (size_t i_pset_subF = 0; i_pset_subF < pset->num_subF; i_pset_subF++) {
        FaceSubsetEntry *pset_subF = &pset->subFs[i_pset_subF];
        size_t i_f = pset_subF->i_f;
        size_t i_pset_subV0 = pset_subF->i_subV0;
        size_t i_pset_subV1 = pset_subF->i_subV1;
        size_t i_pset_subV2 = pset_subF->i_subV2;
        size_t i_pset_subE0 = pset_subF->i_subE0;
        size_t i_pset_subE1 = pset_subF->i_subE1;
        size_t i_pset_subE2 = pset_subF->i_subE2;
        
        VtxeSpatialRelation vsr0 = VSRs[i_pset_subV0];
        VtxeSpatialRelation vsr1 = VSRs[i_pset_subV1];
        VtxeSpatialRelation vsr2 = VSRs[i_pset_subV2];
        
        Maker_BSPFace const *f = &g_maker_tree.mbsp_faces[i_f];
        
        FaceCutData fcd;
        fcd.pset = pset;
        fcd.fset = fset;
        fcd.bset = bset;
        fcd.i_f = i_f;
        fcd.f = f;
        fcd.i_pset_subF = i_pset_subF;
        fcd.pset_subF = pset_subF;
        fcd.vsr0 = vsr0;
        fcd.vsr1 = vsr1;
        fcd.vsr2 = vsr2;

        switch (FSRs[i_pset_subF]) {
        case FSR_COPLANAR:
            break;
        case FSR_FRONT: {
            //vbs_printf("FRONT F\n");
            dibassert(fset->nonempty);
            FaceSubsetEntry fset_subF;
            fset_subF.i_f = i_f;
            fset_subF.i_subV0 = pset->subVs[i_pset_subV0].i_fset_subV;
            fset_subF.i_subV1 = pset->subVs[i_pset_subV1].i_fset_subV;
            fset_subF.i_subV2 = pset->subVs[i_pset_subV2].i_fset_subV;
            fset_subF.i_subE0 = pset->subEs[i_pset_subE0].i_fset_subE;
            fset_subF.i_subE1 = pset->subEs[i_pset_subE1].i_fset_subE;
            fset_subF.i_subE2 = pset->subEs[i_pset_subE2].i_fset_subE;
            add_face_to_subset(fset, fset_subF);
        }
            break;
        case FSR_BACK: {
            //vbs_printf("BACK F\n");
            dibassert(bset->nonempty);
            FaceSubsetEntry bset_subF;
            bset_subF.i_f = i_f;
            bset_subF.i_subV0 = pset->subVs[i_pset_subV0].i_bset_subV;
            bset_subF.i_subV1 = pset->subVs[i_pset_subV1].i_bset_subV;
            bset_subF.i_subV2 = pset->subVs[i_pset_subV2].i_bset_subV;
            bset_subF.i_subE0 = pset->subEs[i_pset_subE0].i_bset_subE;
            bset_subF.i_subE1 = pset->subEs[i_pset_subE1].i_bset_subE;
            bset_subF.i_subE2 = pset->subEs[i_pset_subE2].i_bset_subE;
            add_face_to_subset(bset, bset_subF);
        }
            break;
        case FSR_CUT_TYPE1:
            //vbs_printf("CUT TYPE1 F\n");
            cut_type1(&fcd);
            break;
        case FSR_CUT_TYPE2F:
            //vbs_printf("CUT TYPE2F F\n");
            cut_type2f(&fcd);
            break;
        case FSR_CUT_TYPE2B:
            //vbs_printf("CUT TYPE2B F\n");
            cut_type2b(&fcd);
            break;
        }
    }
}


void cut(WorldVec const *const splitV, WorldVec const *const splitN, BSPSubset const *pset, BSPCutData const *data, BSPSubset *fset, BSPSubset *bset) {    
    cut_vertices(pset, fset, bset, data->VSRs);
    cut_edges(pset, fset, bset, data->VSRs, data->ESRs, splitV, splitN);
    cut_faces(pset, fset, bset, data->VSRs, data->FSRs);

    dibassert(fset->num_occupied_subV == fset->num_subV);
    dibassert(fset->num_occupied_subE == fset->num_subE);
    dibassert(fset->num_occupied_subF == fset->num_subF);
    
    dibassert(bset->num_occupied_subV == bset->num_subV);
    dibassert(bset->num_occupied_subE == bset->num_subE);
    dibassert(bset->num_occupied_subF == bset->num_subF);
}


void compute_VSRs
(WorldVec planeV, WorldVec planeN,
 size_t num_subV, VtxeSubsetEntry const *subVs,
 VtxeSpatialRelation *out_VSRs,
 size_t *out_num_coplanar_v,
 size_t *out_num_front_v,
 size_t *out_num_back_v) {
    uint8_t side;
    size_t num_coplanar_v = 0;
    size_t num_front_v = 0;
    size_t num_back_v = 0;
    for(size_t i_subV = 0; i_subV < num_subV; i_subV++) {
        side = BSP_sideof(g_maker_tree.vertices[subVs[i_subV].i_v].vertex,
                          planeV, planeN);
        VtxeSpatialRelation vsr;
        
        switch (side) {
        case SIDE_MID:
            vsr = VSR_COPLANAR;
            num_coplanar_v++;
            break;
        case SIDE_FRONT:
            vsr = VSR_FRONT;
            num_front_v++;
            break;
        case SIDE_BACK:
            vsr = VSR_BACK;
            num_back_v++;
            break;
        default:
            dibassert(false);
        }

        out_VSRs[i_subV] = vsr;
    }

    *out_num_coplanar_v = num_coplanar_v;
    *out_num_front_v = num_front_v;
    *out_num_back_v = num_back_v;
}

void compute_ESRs
(size_t num_subE, EdgeSubsetEntry const *subEs,
 VtxeSpatialRelation const *VSRs,
 EdgeSpatialRelation *out_ESRs,
 size_t *out_num_coplanar_e,
 size_t *out_num_front_e,
 size_t *out_num_back_e,
 size_t *out_num_cut_e) {
    size_t num_coplanar_e = 0;
    size_t num_front_e = 0;
    size_t num_back_e = 0;
    size_t num_cut_e = 0;
    
    for(size_t i_subE = 0; i_subE < num_subE; i_subE++) {
        VtxeSpatialRelation vsr0 = VSRs[subEs[i_subE].i_subV0];
        VtxeSpatialRelation vsr1 = VSRs[subEs[i_subE].i_subV1];

        EdgeSpatialRelation esr;
        if ((vsr0&vsr1)&VSR_COPLANAR) {
            esr = ESR_COPLANAR;
            num_coplanar_e++;
        }
        else if ( ! ((vsr0|vsr1)&VSR_BACK)) {
            // either both front, or one front and one coplanar
            esr = ESR_FRONT;
            num_front_e++;
        }
        else if ( ! ((vsr0|vsr1)&VSR_FRONT)) {
            // either both back, or one back and one coplanar
            esr = ESR_BACK;
            num_back_e++;
        }
        else if ((vsr0 == VSR_FRONT && vsr1 == VSR_BACK) ||
                 (vsr0 == VSR_BACK && vsr1 == VSR_FRONT)) {
            esr = ESR_CUT;
            num_cut_e++;
        }
        else {
            dibassert(false);
        }

        out_ESRs[i_subE] = esr;
    }

    *out_num_coplanar_e = num_coplanar_e;
    *out_num_front_e = num_front_e;
    *out_num_back_e = num_back_e;
    *out_num_cut_e = num_cut_e;
}

void compute_FSRs
(size_t num_subF, FaceSubsetEntry const *subFs,
 VtxeSpatialRelation const *VSRs,
 FaceSpatialRelation *out_FSRs,
 size_t *out_num_coplanar_f,
 size_t *out_num_front_f,
 size_t *out_num_back_f,
 size_t *out_num_type1_cut_f,
 size_t *out_num_type2f_cut_f,
 size_t *out_num_type2b_cut_f,
 size_t *out_num_cut_f) {
    size_t num_coplanar_f = 0;
    size_t num_front_f = 0;
    size_t num_back_f = 0;
    size_t num_type1_cut_f = 0;
    size_t num_type2f_cut_f = 0;
    size_t num_type2b_cut_f = 0;
    size_t num_cut_f = 0;
    
    for(size_t i_subF = 0; i_subF < num_subF; i_subF++) {
        VtxeSpatialRelation vsr0 = VSRs[subFs[i_subF].i_subV0];
        VtxeSpatialRelation vsr1 = VSRs[subFs[i_subF].i_subV1];
        VtxeSpatialRelation vsr2 = VSRs[subFs[i_subF].i_subV2];
        
        FaceSpatialRelation fsr;
        if ((vsr0&vsr1&vsr2)&VSR_COPLANAR) {
            // every point the same relation, and that relation is MID
            fsr = FSR_COPLANAR;
            num_coplanar_f++;
        } // now at least one non-MID
        else if ( ! ((vsr0|vsr1|vsr2)&VSR_BACK)) {
            // one or two points MID, the other FRONT
            // equiv., all three are not BACK
            fsr = FSR_FRONT;
            num_front_f++;
        } // now at least one BACK
        else if ( ! ((vsr0|vsr1|vsr2)&VSR_FRONT)) {
            // one or two points MID, the other BACK
            // equiv., all three are not FRONT
            fsr = FSR_BACK;
            num_back_f++;
        } // now at least one FRONT and at least one BACK
        else if ((vsr0|vsr1|vsr2)&VSR_COPLANAR) {
            fsr = FSR_CUT_TYPE1;
            num_type1_cut_f++;
            num_cut_f++;
        } // now either two FRONT and one BACK, or two BACK and one FRONT 
        else if ((vsr0&VSR_FRONT)+(vsr1&VSR_FRONT)+(vsr2&VSR_FRONT) > VSR_FRONT) {
            fsr = FSR_CUT_TYPE2F;
            num_type2f_cut_f++;
            num_cut_f++;
        }
        else if ((vsr0&VSR_BACK)+(vsr1&VSR_BACK)+(vsr2&VSR_BACK) > VSR_BACK) {
            fsr = FSR_CUT_TYPE2B;
            num_type2b_cut_f++;
            num_cut_f++;
        }
        else {
            dibassert(false);
        }

        out_FSRs[i_subF] = fsr;
    }

    dibassert(num_cut_f == num_type1_cut_f + num_type2f_cut_f + num_type2b_cut_f);

    *out_num_coplanar_f   = num_coplanar_f;
    *out_num_front_f      = num_front_f;
    *out_num_back_f       = num_back_f;
    *out_num_type1_cut_f  = num_type1_cut_f;
    *out_num_type2f_cut_f = num_type2f_cut_f;
    *out_num_type2b_cut_f = num_type2b_cut_f;
    *out_num_cut_f        = num_cut_f;
}

static size_t add_vertex_to_tree(Maker_BSPVertex v) {
    size_t i_v = g_maker_tree.num_vertices++;
    g_maker_tree.vertices[i_v] = v;
    
    return i_v;
}

static size_t add_vertex_to_subset(BSPSubset *set, size_t i_v) {
    VtxeSubsetEntry new_subV;

    new_subV.i_v = i_v;
    
    size_t i_set_subV = set->num_occupied_subV++;    
    set->subVs[i_set_subV] = new_subV;

    return i_set_subV;
}

static size_t add_edge_to_tree
(Maker_BSPEdge e) {
    size_t i_e = g_maker_tree.num_msh_edges++;
    g_maker_tree.msh_edges[i_e] = e;

    return i_e;
}

static size_t add_edge_to_subset
(BSPSubset *set, size_t i_e, size_t i_subV0, size_t i_subV1) {
    EdgeSubsetEntry new_subE;

    dibassert(set->subVs[i_subV0].i_v == g_maker_tree.msh_edges[i_e].i_v0);
    dibassert(set->subVs[i_subV1].i_v == g_maker_tree.msh_edges[i_e].i_v1);
    
    new_subE.i_e = i_e;
    new_subE.i_subV0 = i_subV0;
    new_subE.i_subV1 = i_subV1;
    new_subE.cut_data = NULL;

    size_t i_set_subE = set->num_occupied_subE++;
    set->subEs[i_set_subE] = new_subE;

    return i_set_subE;
}

static size_t add_face_to_tree(Maker_BSPFace f) {
    size_t i_f = g_maker_tree.num_mbsp_faces++;
    g_maker_tree.mbsp_faces[i_f] = f;

    return i_f;
}

static size_t replace_face_in_tree(size_t i_f, Maker_BSPFace f) {
    g_maker_tree.mbsp_faces[i_f] = f;

    return i_f;
}
    
static size_t add_face_to_tree0
(size_t i_v0, size_t i_v1, size_t i_v2,
 size_t i_e0, size_t i_e1, size_t i_e2,
 bool from_cut, uint32_t i_gen_face) {
    Maker_BSPFace f = {0};
    f.i_v0 = i_v0;
    f.i_v1 = i_v1;
    f.i_v2 = i_v2;
    f.i_e0 = i_e0;
    f.i_e1 = i_e1;
    f.i_e2 = i_e2;
    f.flags |= MF_FROM_CUT;
    f.i_gen_face = i_gen_face;
    f.next_in_node = NULL;
    
    size_t i_f = g_maker_tree.num_mbsp_faces++;
    g_maker_tree.mbsp_faces[i_f] = f;

    return i_f;
}



static size_t add_face_to_subset
(BSPSubset *set, FaceSubsetEntry subF) {
    Maker_BSPFace f = g_maker_tree.mbsp_faces[subF.i_f];

    dibassert(set->subVs[subF.i_subV0].i_v +
              set->subVs[subF.i_subV1].i_v +
              set->subVs[subF.i_subV2].i_v ==
              f.i_v0 +
              f.i_v1 +
              f.i_v2);

    dibassert(set->subEs[subF.i_subE0].i_e +
              set->subEs[subF.i_subE1].i_e +
              set->subEs[subF.i_subE2].i_e ==
              f.i_e0 +
              f.i_e1 +
              f.i_e2);

    size_t i_set_subF = set->num_occupied_subF++;
    set->subFs[i_set_subF] = subF;

    return i_set_subF;
    
}





/*
bool is_convex(BSPSubset *set) {
    // Determine if every face has all other faces on one side or coplanar.
    WorldVec planeV, planeN;
    
    for (size_t i_subF = 0; i_subF < set->num_subF; i_subF++) {
        size_t i_f = set->subFs[i_subF].i_f;
        planeV = g_gen.vertices[g_gen.faces[g_maker_tree.mbsp_faces[i_f].i_gen_face].i_v0];
        planeN = g_gen.faces[g_maker_tree.mbsp_faces[i_f].i_gen_face].normal;
        
        for (size_t j_subF = i_subF + 1; j_subF < set->num_subF; j_subF++) {

            
            uint8_t side = BSP_sideof(g_maker_tree.vertices[subVs[i_subV].i_v].vertex,
                                      planeV, planeN);
            VtxeSpatialRelation vsr;
        
            switch (side) {
            case SIDE_MID:
                vsr = VSR_COPLANAR;
                break;
            case SIDE_FRONT:
                vsr = VSR_FRONT;
                break;
            case SIDE_BACK:
                vsr = VSR_BACK;
                break;
            default:
                dibassert(false);
            }
        }
    }
}
*/
