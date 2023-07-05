#include "src/world/bsp/bsp_internal.h"

#define NODESIDE_FRONT 0
#define NODESIDE_BACK 1
#define NODESIDE_ROOT -1

static void bsp_InterpolateAttributes
( WorldVec _V0, WorldVec _V1, WorldVec _V2,
  Fix32Vec_2D TV0, Fix32Vec_2D TV1, Fix32Vec_2D TV2,
  WorldVec _P0, WorldVec _P1, WorldVec _P2,
  Fix32Vec_2D *TP0, Fix32Vec_2D *TP1, Fix32Vec_2D *TP2);

static void bsp_InterpolateAttributes2
(Face const *og_face, Face *final_face);

static void bsp_ConvertMakerBSPNode
(Maker_BSPNode *maker_node, BSPNode *final_parent, int side, uint16_t depth);

static epm_Result bsp_ComputeEdgesFromFaces(BSPTree *tree);

static size_t g_i_node;
static size_t g_i_face;

void finalize_BSPTree(void) {    
    g_p_final_tree->num_gen_vertices = g_gen.num_vertices;
    g_p_final_tree->gen_vertices = g_gen.vertices;
    g_p_final_tree->num_gen_faces = g_gen.num_faces;
    g_p_final_tree->gen_faces = g_gen.faces;
    
    g_p_final_tree->num_vertices = g_maker_tree.num_vertices;
    g_p_final_tree->vertices = zgl_Calloc(g_p_final_tree->num_vertices,
                                          sizeof(WorldVec));
    //    g_p_final_tree->vbris = zgl_Calloc(g_p_final_tree->num_vertices,
    //                                                    sizeof(uint8_t));
    g_p_final_tree->vertex_colors = zgl_Calloc(g_p_final_tree->num_vertices,
                                               sizeof(zgl_Color));

    for (size_t i_v = 0; i_v < g_p_final_tree->num_vertices; i_v++) {
        g_p_final_tree->vertices[i_v] = g_maker_tree.vertices[i_v].vertex;
        g_p_final_tree->vertex_colors[i_v] =
            g_maker_tree.vertices[i_v].flags & MV_FROM_CUT ?
            0xFF0000 :
            0x00FF00;
    }


    g_p_final_tree->num_edges = g_maker_tree.num_msh_edges;
    g_p_final_tree->edges = zgl_Calloc(g_p_final_tree->num_edges, sizeof(Edge));
    g_p_final_tree->edge_colors = zgl_Calloc(g_p_final_tree->num_edges,
                                             sizeof(zgl_Color));

    for (size_t i_e = 0; i_e < g_p_final_tree->num_edges; i_e++) {
        g_p_final_tree->edges[i_e].i_v0 = g_maker_tree.msh_edges[i_e].i_v0;
        g_p_final_tree->edges[i_e].i_v1 = g_maker_tree.msh_edges[i_e].i_v1;
        zgl_Color color = 0;
        if (g_maker_tree.msh_edges[i_e].flags & ME_FROM_CUT) {
            color |= 0xFF0000;
        }
        if (g_maker_tree.msh_edges[i_e].flags & ME_NEW) {
            color |= 0x0000FF;
        }

        if (color == 0) {
            color = 0x00FF00;
        }
        g_p_final_tree->edge_colors[i_e] = color;
    }

    g_p_final_tree->num_nodes     = g_maker_tree.num_nodes;
    g_p_final_tree->num_faces     = g_maker_tree.num_mbsp_faces;
    g_p_final_tree->nodes         = zgl_Calloc(g_p_final_tree->num_nodes,
                                               sizeof(BSPNode));
    g_p_final_tree->faces         = zgl_Calloc(g_p_final_tree->num_faces,
                                               sizeof(Face));
    g_p_final_tree->bsp_faces     = zgl_Calloc(g_p_final_tree->num_faces,
                                               sizeof(BSPFace));
    g_p_final_tree->num_cuts      = g_maker_tree.num_cuts;


    /* Face conversion is done node-by-node, so that faces attached to same node
       are contiguous. */
    g_i_node = 0;
    g_i_face = 0;
    bsp_ConvertMakerBSPNode(g_maker_tree.root, NULL, -1, 0);
    g_i_node = 0;
    g_i_face = 0;

    // TODO: Interpolate instead of really computing.
    /*
    void epm_ComputeVertexBrightnesses(size_t num_vertices, WorldVec const vertices[], size_t num_lights, Light const lights[], uint8_t out_vbri[]);
    epm_ComputeVertexBrightnesses(g_p_final_tree->num_vertices,
                                  g_p_final_tree->vertices,
                                  g_p_final_tree->vbris);
    */
    
    /*
    epm_ComputeEdgesFromFaces(g_p_final_tree->num_faces, g_p_final_tree->faces,
                              &g_p_final_tree->num_edges, &g_p_final_tree->edges);
    */
}



#define BSP_FRONT_MASK 0x3
static void bsp_ConvertMakerBSPNode
(Maker_BSPNode *maker_node, BSPNode *final_parent, int side, uint16_t depth) {
    if (maker_node == NULL)
        return;
    
    BSPNode *final_node = g_p_final_tree->nodes + g_i_node;
    g_i_node++;

    final_node->parent = final_parent;
    if (side == NODESIDE_FRONT) {
        final_parent->front = final_node;
    }
    else if (side == NODESIDE_BACK) {
        final_parent->back = final_node;
    }

    final_node->splitV = maker_node->splitV;
    final_node->splitN = maker_node->splitN;
    final_node->num_faces = maker_node->num_mbsp_faces;
    final_node->i_first_face = g_i_face;
    
    size_t tmp_count = 0;
    for (Maker_BSPFace *mbsp_face = g_maker_tree.mbsp_faces + maker_node->i_mbsp_faces;
         mbsp_face;
         mbsp_face = mbsp_face->next_in_node) {
        Face const *og_face     = g_gen.faces + mbsp_face->i_gen_face;
        Face *final_face        = g_p_final_tree->faces + g_i_face;
        BSPFace *final_bsp_face = g_p_final_tree->bsp_faces + g_i_face;
        g_i_face++;
        tmp_count++;

        if (side == NODESIDE_FRONT) {
            final_bsp_face->bspflags = 0 | BSP_FRONT_MASK;
        }
        else if (side == NODESIDE_BACK) {
            final_bsp_face->bspflags = 0;
        }

        final_bsp_face->i_gen_face = mbsp_face->i_gen_face;
        final_bsp_face->depth = depth;
        
        final_face->i_v[0] = mbsp_face->i_v0;
        final_face->i_v[1] = mbsp_face->i_v1;
        final_face->i_v[2] = mbsp_face->i_v2;
        final_face->flags = 0;
        final_face->normal = og_face->normal;
        final_face->i_tex = og_face->i_tex;
        final_face->fbri = og_face->fbri;
        
        if (mbsp_face->flags & MF_FROM_CUT) {
            bsp_InterpolateAttributes2(og_face, final_face);
        }
        else {
            final_face->vtxl[0] = og_face->vtxl[0];
            final_face->vtxl[1] = og_face->vtxl[1];
            final_face->vtxl[2] = og_face->vtxl[2];
            final_face->vbri[0] = og_face->vbri[0];
            final_face->vbri[1] = og_face->vbri[1];
            final_face->vbri[2] = og_face->vbri[2];
        }
    }
    dibassert(tmp_count == final_node->num_faces);
 
    bsp_ConvertMakerBSPNode(maker_node->front, final_node,
                 NODESIDE_FRONT, depth+1);
    bsp_ConvertMakerBSPNode(maker_node->back, final_node,
                 NODESIDE_BACK, depth+1);
}

static void bsp_InterpolateAttributes2
(Face const *og_face, Face *final_face) {
    Fix64Vec V[3], P[3];

    x_of(V[0]) = x_of(g_gen.vertices[og_face->i_v[0]]);
    y_of(V[0]) = y_of(g_gen.vertices[og_face->i_v[0]]);
    z_of(V[0]) = z_of(g_gen.vertices[og_face->i_v[0]]);
    x_of(V[1]) = x_of(g_gen.vertices[og_face->i_v[1]]);
    y_of(V[1]) = y_of(g_gen.vertices[og_face->i_v[1]]);
    z_of(V[1]) = z_of(g_gen.vertices[og_face->i_v[1]]);
    x_of(V[2]) = x_of(g_gen.vertices[og_face->i_v[2]]);
    y_of(V[2]) = y_of(g_gen.vertices[og_face->i_v[2]]);
    z_of(V[2]) = z_of(g_gen.vertices[og_face->i_v[2]]);
    x_of(P[0]) = x_of(g_p_final_tree->vertices[final_face->i_v[0]]);
    y_of(P[0]) = y_of(g_p_final_tree->vertices[final_face->i_v[0]]);
    z_of(P[0]) = z_of(g_p_final_tree->vertices[final_face->i_v[0]]);
    x_of(P[1]) = x_of(g_p_final_tree->vertices[final_face->i_v[1]]);
    y_of(P[1]) = y_of(g_p_final_tree->vertices[final_face->i_v[1]]);
    z_of(P[1]) = z_of(g_p_final_tree->vertices[final_face->i_v[1]]);
    x_of(P[2]) = x_of(g_p_final_tree->vertices[final_face->i_v[2]]);
    y_of(P[2]) = y_of(g_p_final_tree->vertices[final_face->i_v[2]]);
    z_of(P[2]) = z_of(g_p_final_tree->vertices[final_face->i_v[2]]);
    
    Fix64 area = triangle_area_3D(V[0], V[1], V[2]);
    dibassert(area != 0);
    Fix32Vec_2D
        vtxl0 = og_face->vtxl[0],
        vtxl1 = og_face->vtxl[1],
        vtxl2 = og_face->vtxl[2];
    int64_t
        vbri0 = og_face->vbri[0],
        vbri1 = og_face->vbri[1],
        vbri2 = og_face->vbri[2],
        tmp_vbri;
    for (uint8_t i = 0; i < 3; i++) {
        Fix64 B0 = (triangle_area_3D(P[i], V[1], V[2])<<16)/area;
        Fix64 B1 = (triangle_area_3D(P[i], V[0], V[2])<<16)/area;
        Fix64 B2 = (1<<16) - B0 - B1;
        final_face->vtxl[i].x = (Fix32)((B0*vtxl0.x + B1*vtxl1.x + B2*vtxl2.x)>>16);
        final_face->vtxl[i].y = (Fix32)((B0*vtxl0.y + B1*vtxl1.y + B2*vtxl2.y)>>16);
        tmp_vbri = (B0*vbri0 + B1*vbri1 + B2*vbri2)>>16;
        if (tmp_vbri < 0) // sometimes -1 but seemingly never -2 or lower
            final_face->vbri[i] = 0;
        else if (tmp_vbri > 255)
            final_face->vbri[i] = 255;
        else
            final_face->vbri[i] = (uint8_t)tmp_vbri;
    }
}

static void bsp_InterpolateAttributes
(WorldVec _V0, WorldVec _V1, WorldVec _V2,
 Fix32Vec_2D TV0, Fix32Vec_2D TV1, Fix32Vec_2D TV2,
 WorldVec _P0, WorldVec _P1, WorldVec _P2,
 Fix32Vec_2D *out_TP0, Fix32Vec_2D *out_TP1, Fix32Vec_2D *out_TP2) {

    Fix64Vec V0, V1, V2, P0, P1, P2;

    x_of(V0) = x_of(_V0);
    y_of(V0) = y_of(_V0);
    z_of(V0) = z_of(_V0);
    x_of(V1) = x_of(_V1);
    y_of(V1) = y_of(_V1);
    z_of(V1) = z_of(_V1);
    x_of(V2) = x_of(_V2);
    y_of(V2) = y_of(_V2);
    z_of(V2) = z_of(_V2);
    x_of(P0) = x_of(_P0);
    y_of(P0) = y_of(_P0);
    z_of(P0) = z_of(_P0);
    x_of(P1) = x_of(_P1);
    y_of(P1) = y_of(_P1);
    z_of(P1) = z_of(_P1);
    x_of(P2) = x_of(_P2);
    y_of(P2) = y_of(_P2);
    z_of(P2) = z_of(_P2);
    
    Fix64 area = triangle_area_3D(V0, V1, V2);
    dibassert(area != 0);

    Fix64 B0, B1, B2;
        
    // texel 0
    B0 = (triangle_area_3D(P0, V1, V2)<<16)/area;
    B1 = (triangle_area_3D(P0, V0, V2)<<16)/area;
    B2 = (1<<16) - B0 - B1;
    out_TP0->x = (Fix32)((B0*TV0.x + B1*TV1.x + B2*TV2.x)>>16);
    out_TP0->y = (Fix32)((B0*TV0.y + B1*TV1.y + B2*TV2.y)>>16);

    // texel 1
    B0 = (triangle_area_3D(P1, V1, V2)<<16)/area;
    B1 = (triangle_area_3D(P1, V0, V2)<<16)/area;
    B2 = (1<<16) - B0 - B1;
    out_TP1->x = (Fix32)((B0*TV0.x + B1*TV1.x + B2*TV2.x)>>16);
    out_TP1->y = (Fix32)((B0*TV0.y + B1*TV1.y + B2*TV2.y)>>16);
                
    // texel 2
    B0 = (triangle_area_3D(P2, V1, V2)<<16)/area;
    B1 = (triangle_area_3D(P2, V0, V2)<<16)/area;
    B2 = (1<<16) - B0 - B1;
    out_TP2->x = (Fix32)((B0*TV0.x + B1*TV1.x + B2*TV2.x)>>16);
    out_TP2->y = (Fix32)((B0*TV0.y + B1*TV1.y + B2*TV2.y)>>16);
}
