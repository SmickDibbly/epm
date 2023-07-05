#ifndef MESH_H
#define MESH_H

#include "zigil/zigil.h"
#include "src/misc/epm_includes.h"
#include "src/world/geometry.h"

#define MAX_MESH_VERTICES (16384)
#define MAX_MESH_EDGES    (16384)
#define MAX_MESH_FACES    (16384)

/* A mesh consists of vertices, edges, and faces. */
typedef struct Mesh {
    zgl_Color wirecolor;
    
    Fix32MinMax AABB;

    size_t num_vertices;
    WorldVec *vertices;
    //    WorldVec *vertex_normals;

    size_t num_edges;
    Edge *edges;

    size_t num_faces;
    Face *faces;
} Mesh;

/* A linked mesh consists of vertices, edges, faces, and indcidence relations
   amongst these. */
typedef struct LinkedMesh {
    Fix32MinMax AABB;

    size_t num_vertices;
    LinkedVertex *vertices;
    // for each vertex, a list of edges and faces it belongs to
    
    size_t num_edges;
    LinkedEdge *edges;
    // for each edge, a list of faces it belongs to

    size_t num_faces;
    LinkedFace *faces;
} LinkedMesh;

/* A Model is a mesh together with transformation information. The "true"
   geometry of the Model is the mesh geometry with all the transformations
   applied. */
typedef struct Model {
    Mesh mesh;
    
    Fix32 scale;
    Ang18 ang_h;
    Ang18 ang_v;    
    WorldVec translation;
} Model;

typedef struct EdgeMesh {
    zgl_Color wirecolor;
    
    size_t num_vertices;
    WorldVec *vertices;
    
    size_t num_edges;
    Edge *edges;
} EdgeMesh;

#define FaceSet_of(MESH)                        \
    ((FaceSet){                                 \
        .num_vertices = (MESH).num_vertices,    \
        .vertices = (MESH).vertices,            \
        .num_faces = (MESH).num_faces,          \
        .faces = (MESH).faces                   \
    })

#define EdgeSet_of(MESH)                        \
    ((EdgeSet){                                 \
        .num_vertices = (MESH).num_vertices,    \
        .vertices = (MESH).vertices,            \
        .num_edges = (MESH).num_edges,          \
        .edges = (MESH).edges                   \
    })

extern void print_Mesh(Mesh *mesh);

extern Mesh *read_OBJ(Mesh *p_mesh, char const *filename);
extern Mesh *read_GLTF(Mesh *p_mesh, char const *filename);
extern Mesh *read_DIBJ(Mesh *p_mesh, char const *filename);

extern void epm_ComputeAABB(Mesh *p_mesh);

extern void epm_ComputeFaceNormal(WorldVec vertices[], Face *f);
extern void epm_ComputeFaceBrightness(Face *f);

extern void epm_ComputeFaceNormals(WorldVec vertices[],
                                   size_t num_faces, Face faces[]);
extern void epm_ComputeFaceBrightnesses(size_t num_faces, Face faces[]);

extern epm_Result epm_ComputeEdgesFromFaces
(size_t num_faces, Face *faces,
 size_t *out_num_edges, Edge *(out_edges[]));


extern void find_all_t_junctions(Mesh *mesh);

extern void FillTJunctions
(Mesh *mesh, Face *f0, Face *f1, UFix32 const EdgeThickness);

extern void fix_all_t_junctions(Mesh *mesh);

#endif /* MESH_H */
