#include "zigil/zigil_mem.h"
#include "src/misc/mathematics.h"

#include "src/world/bsp/bsp_internal.h"
#include "src/world/bsp/bsp.h"
#include "src/world/world.h"

// In this project, the style-conforming identifier for an array is plural, such
// as things[] instead of thing[] (or *things instead of *thing in pointer
// form). The style-conforming identifier for an index into an array things[]
// (or *things) is i_thing.
//
// In the declaration of struct FaceSubset below, *i_faces is an array whose
// values are themselves indices into the array faces[]. Thus in *i_faces,
// the "i_face" part represents the values being indices, and the "s" represents
// the fact that it is an array.
//
// The style-conforming notation for an index into *i_faces is therefore
// i_i_face. In general, the number of "i_" prefixes represents how many layers
// deep the indices go. There should, however, be no reason for more than two
// levels.


// When a triangle is split during the BSP construction process, 1 or 2 new
// vertices are created, and 2 or 3 triangles are created. Any new vertex is on
// the line joining 2 vertices of the split triangle, and those 2 vertices are
// stored as "edge-parents" of the new vertex. Any new triangle also has the
// split triangle stored as a "face-parent". New vertices and new triangles also
// have the "progenitor face" and "progenitor" vertices, since triangles may be
// split multiple times during recursion.

// TODO: The leaves correspond to a convex partition of the world. Compute this
// partition: Starting from a leaf, how to find a minimum set of splitting
// planes that bound the convex subregion.

// TODO: Splitting a face entails splitting one or two edges. When this happens,
// we will split the edges for each of their incident faces as well. For the
// face that is being split, we proceed as normal. For any other incident faces,
// we merely subdivide the face. TODO: What happens when one of the "any other
// incident faces" is also going to be split by the same splitting plane?

// IDEA: Record the "spatial relation" of each bspface in the maker_bspface itself?

// IDEA: Each BSP node corresponds to a splitting plane, and that splitting plane is determined by a polygon in the world geometry. I should consider storing not just that one polygon in the node, but all coplanar polygons as well.

// TODO: Be consistent about usage of "split" and "cut". Pick one and stick with
// it.

#undef LOG_LABEL
#define LOG_LABEL "BSP"

void destroy_BSPTree(BSPTree *p_tree) {
    //    zgl_Free(p_tree->vbris);
    zgl_Free(p_tree->vertices);
    zgl_Free(p_tree->edges);
    zgl_Free(p_tree->nodes);
    zgl_Free(p_tree->faces);
    zgl_Free(p_tree->bsp_faces);

    p_tree->nodes = NULL;
    //    p_tree->vbris = NULL;
    p_tree->vertices = NULL;
    p_tree->edges = NULL;
    p_tree->faces = NULL;
    p_tree->bsp_faces = NULL;
}


static void measure_BSPTree_recursion(BSPNode *node, int node_depth);
static int g_max_node_depth;
static int g_num_leaves;
static int g_total_node_depth;
static int g_total_leaf_depth;
static int g_num_faces;
#include <math.h>
void measure_BSPTree(BSPTree *p_tree) {
    if (p_tree == NULL) return;

    g_max_node_depth = 0;
    g_num_leaves = 0;
    g_total_node_depth = 0;
    g_total_leaf_depth = 0;
    g_num_faces = 0;

    measure_BSPTree_recursion(&p_tree->nodes[0], 0);
    
    p_tree->max_node_depth = g_max_node_depth;
    p_tree->num_leaves = g_num_leaves;
    p_tree->avg_node_depth = (double)g_total_node_depth / (double)p_tree->num_nodes;
    p_tree->avg_leaf_depth = (double)g_total_leaf_depth / (double)g_num_leaves;
    p_tree->balance = p_tree->avg_leaf_depth / log2((double)p_tree->num_nodes);
    
    snprintf(bigbuf, BIGBUF_LEN,
             "      BSP Profile      \n"
             "+---------------------+\n"
             "# Progenitor Vertices | %zu\n"
             "           # Vertices | %zu\n"
             "                      |    \n"
             "   # Progenitor Faces | %zu\n"
             "              # Faces | %zu\n"
             "               # Cuts | %zu\n"
             "                      |    \n"
             "              # Nodes | %zu\n"
             "             # Leaves | %zu\n"
             "            Max Depth | %zu\n"
             "       Avg Node Depth | %lf\n"
             "       Avg Leaf Depth | %lf\n"
             "              Balance | %lf\n",
             p_tree->num_gen_vertices,
             p_tree->num_vertices,
             p_tree->num_gen_faces,
             p_tree->num_faces,
             p_tree->num_cuts,
             p_tree->num_nodes,
             p_tree->num_leaves,
             p_tree->max_node_depth,
             p_tree->avg_node_depth,
             p_tree->avg_leaf_depth,
             p_tree->balance);

    puts(bigbuf);

    vbs_printf("(Expected %zu) (Actual %i)\n", p_tree->num_faces, g_num_faces);
    dibassert((int)p_tree->num_faces == g_num_faces);
}

static void measure_BSPTree_recursion(BSPNode *node, int depth) {
    if (node == NULL) return;

    //    dibassert(node->bsp_faces->depth == depth);
    g_max_node_depth = MAX(depth, g_max_node_depth);
    g_total_node_depth += depth;
    g_num_faces += (int)node->num_faces;
    if (node->front == NULL && node->back == NULL) {
        g_num_leaves++;
        g_total_leaf_depth += depth;
    }

    measure_BSPTree_recursion(node->front, depth+1);
    measure_BSPTree_recursion(node->back,  depth+1);
}

/*
void add_incidence_node(Maker_SharedEdge *msh_edge, Maker_BSPFace *mbsp_face) {
    if (msh_edge->num_incidence_nodes == 0) {
        dibassert(msh_edge->head_node == NULL && msh_edge->tail_node == NULL);
        msh_edge->head_node = zgl_Malloc(sizeof(*msh_edge->head_node));
        msh_edge->head_node->next = NULL;
        msh_edge->head_node->mbsp_face = mbsp_face;
        msh_edge->tail_node = msh_edge->head_node;
        msh_edge->num_incidence_nodes++;
    }
    else {
        dibassert(msh_edge->tail_node != NULL);
        msh_edge->tail_node->next = zgl_Malloc(sizeof(*msh_edge->tail_node->next));
        msh_edge->tail_node->next->next = NULL;
        msh_edge->tail_node->next->mbsp_face = mbsp_face;
        msh_edge->tail_node = msh_edge->tail_node->next;
        msh_edge->num_incidence_nodes++;
    }
}
*/

static void CMDH_measure_BSPTree(int argc, char **argv, char *output_str) {
    measure_BSPTree(g_world.geo_bsp);
    
    strcpy(output_str, bigbuf);
}

#include "src/input/command.h"
epm_Command const CMD_measure_BSPTree = {
    .name = "measure_BSPTree",
    .argc_min = 1,
    .argc_max = 1,
    .handler = CMDH_measure_BSPTree,
};
