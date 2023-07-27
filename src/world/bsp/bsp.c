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

static BSPTree const initial_BSPTree = {
    .num_leaves = 0,
    .num_cuts = 0,
    
    .max_node_depth = 0,
    .avg_node_depth = 0.0,
    .avg_leaf_depth = 0.0,
    .balance        = 0.0,
    
    .num_verts = 0,
    .verts = NULL,
    .vert_exts = NULL,
    .vert_clrs = NULL,
    
    .num_edges = 0,
    .edges = NULL,
    .edge_clrs = NULL,

    .num_faces = 0,
    .faces = NULL,
    .face_exts = NULL,
    .face_clrs = NULL,

    .num_nodes = 0,
    .nodes = NULL,

    
    .num_pre_verts = 0,
    .pre_verts = NULL,
    .pre_vert_exts = NULL,
    
    .num_pre_edges = 0,
    .pre_edges = NULL,

    .num_pre_faces = 0,
    .pre_faces = NULL,
    .pre_face_exts = NULL,
};

void destroy_BSPTree(BSPTree *p_tree) {
    zgl_Free(p_tree->verts);
    zgl_Free(p_tree->vert_exts);
    zgl_Free(p_tree->vert_clrs);

    zgl_Free(p_tree->edges);
    zgl_Free(p_tree->edge_clrs);

    zgl_Free(p_tree->faces);
    zgl_Free(p_tree->face_exts);
    zgl_Free(p_tree->face_clrs);
    
    zgl_Free(p_tree->nodes);
    
    *p_tree = initial_BSPTree;
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
             "    BSP Profile   \n"
             "+----------------+\n"
             "# Input Vertices | %zu\n"
             "      # Vertices | %zu\n"
             "                 |    \n"
             "   # Input Edges | %zu\n"
             "         # Edges | %zu\n"
             "                 |    \n"
             "   # Input Faces | %zu\n"
             "         # Faces | %zu\n"
             "          # Cuts | %zu\n"
             "                 |    \n"
             "         # Nodes | %zu\n"
             "        # Leaves | %zu\n"
             "       Max Depth | %zu\n"
             "  Avg Node Depth | %lf\n"
             "  Avg Leaf Depth | %lf\n"
             "         Balance | %lf\n",
             p_tree->num_pre_verts,
             p_tree->num_verts,
             p_tree->num_pre_edges,
             p_tree->num_edges,
             p_tree->num_pre_faces,
             p_tree->num_faces,
             p_tree->num_cuts,
             p_tree->num_nodes,
             p_tree->num_leaves,
             p_tree->max_node_depth,
             p_tree->avg_node_depth,
             p_tree->avg_leaf_depth,
             p_tree->balance);

    puts(bigbuf);

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


void sprint_BSPTree(BSPTree const *tree) {
    if (tree == NULL) return;
    
    snprintf(bigbuf, BIGBUF_LEN,
             "    BSP Profile   \n"
             "+----------------+\n"
             "# Input Vertices | %zu\n"
             "      # Vertices | %zu\n"
             "                 |    \n"
             "   # Input Edges | %zu\n"
             "         # Edges | %zu\n"
             "                 |    \n"
             "   # Input Faces | %zu\n"
             "         # Faces | %zu\n"
             "          # Cuts | %zu\n"
             "                 |    \n"
             "         # Nodes | %zu\n"
             "        # Leaves | %zu\n"
             "       Max Depth | %zu\n"
             "  Avg Node Depth | %lf\n"
             "  Avg Leaf Depth | %lf\n"
             "         Balance | %lf\n",
             tree->num_pre_verts,
             tree->num_verts,
             tree->num_pre_edges,
             tree->num_edges,
             tree->num_pre_faces,
             tree->num_faces,
             tree->num_cuts,
             tree->num_nodes,
             tree->num_leaves,
             tree->max_node_depth,
             tree->avg_node_depth,
             tree->avg_leaf_depth,
             tree->balance);

    puts(bigbuf);
}

void print_BSPTree(BSPTree const *tree) {
    if (tree == NULL) return;
    
    snprintf(bigbuf, BIGBUF_LEN,
             "    BSP Profile   \n"
             "+----------------+\n"
             "# Input Vertices | %zu\n"
             "      # Vertices | %zu\n"
             "                 |    \n"
             "   # Input Edges | %zu\n"
             "         # Edges | %zu\n"
             "                 |    \n"
             "   # Input Faces | %zu\n"
             "         # Faces | %zu\n"
             "          # Cuts | %zu\n"
             "                 |    \n"
             "         # Nodes | %zu\n"
             "        # Leaves | %zu\n"
             "       Max Depth | %zu\n"
             "  Avg Node Depth | %lf\n"
             "  Avg Leaf Depth | %lf\n"
             "         Balance | %lf\n",
             tree->num_pre_verts,
             tree->num_verts,
             tree->num_pre_edges,
             tree->num_edges,
             tree->num_pre_faces,
             tree->num_faces,
             tree->num_cuts,
             tree->num_nodes,
             tree->num_leaves,
             tree->max_node_depth,
             tree->avg_node_depth,
             tree->avg_leaf_depth,
             tree->balance);

    puts(bigbuf);
}
