#ifndef WORLD_H
#define WORLD_H

#include "src/world/brush.h"
#include "src/misc/epm_includes.h"
#include "src/world/geometry.h"
#include "src/entity/editor_camera.h"
#include "src/entity/player.h"
#include "src/world/mesh.h"
#include "src/world/bsp/bsp.h"
#include "src/entity/entity.h"
#include "src/world/pregeo.h"

//                        -2^31
#define WORLD_MIN   -2147483648
//                     2^31 - 1
#define WORLD_MAX   +2147483647
//                         2^32
#define WORLD_SIZE  +4294967296

typedef struct Transform {
    WorldUnit x, y, z;
    WorldVec dir;
    Fix32 vcos, vsin, hcos, hsin;
} Transform;

#define BSP_DEFAULT 0
#define BSP_NON_CUTTER 1
#define BSP_NON_CUTTEE 2

#define CSG_UNKNOWN 0
#define CSG_SUBTRACTIVE 1
#define CSG_ADDITIVE 2
#define CSG_SPECIAL 3

#define MAX_STATIC_V (16384)
#define MAX_STATIC_E (16384)
#define MAX_STATIC_F (16384)
#define MAX_STATIC_TEXTURES (64)

#define MAX_STATIC_V (16384)
#define MAX_STATIC_E (16384)
#define MAX_STATIC_F (16384)
#define MAX_STATIC_TEXTURES (64)



typedef struct Light {
    WorldVec pos; // x and y are center, z is foot level
    uint8_t brightness;
    Fix32 inner_radius;
    Fix32 outer_radius;
} Light;

#define WF_LOADED_BRUSHGEO 0x01
#define WF_LOADED_PREBSPGEO 0x02
#define WF_LOADED_BSPGEO 0x04
#define WF_LOADED_ENTITY 0x08
typedef struct epm_World {
    uint32_t worldflags;
    
    BrushGeometry *geo_brush;
    PreGeometry *geo_prebsp;
    BSPTree *geo_bsp;
    epm_EntityNode entity_head;
    
    size_t num_lights;
    Light lights[64];
} epm_World;

extern epm_Result epm_WorldFromMesh(Mesh *mesh);

extern epm_Result read_world(PreGeometry *geo, char *filename);
extern void write_world(PreGeometry const *geo, char const *filename);

extern epm_Result epm_ReadWorldFile(epm_World *world, char const *filename);
extern epm_Result epm_WriteWorldFile(epm_World *world, char const *filename);

extern epm_Result epm_LoadWorld(char *worldname);
extern epm_Result epm_UnloadWorld(void);

// To "build" a world:
// 1) Brush -> Pre (attr: v.pos, f.vtxl) (brushes are unlit)
// 2) Build lighting.
// 3) Compile BSP. (attr: v.pos, f.vtxl, f.vbri)
extern epm_Result epm_Build(void);

extern epm_Result epm_BuildPreBSP(void);
extern epm_Result epm_BuildLighting(void);
extern epm_Result epm_BuildBSP(void);

extern Transform tf;

extern WorldVec directional_light_vector;
extern Fix32 directional_light_intensity;
extern Fix32 ambient_light_intensity;

/*
typedef struct MeshNode MeshNode;
struct MeshNode {
    MeshNode *next;
    Mesh *mesh;
};
extern MeshNode root_mesh;
*/

//extern Fix32Point grid3D[16];
extern EdgeSet view3D_bigbox;
extern EdgeSet view3D_grid;

extern EditorCamera cam;
extern Player player;
extern Mesh E1M1;
extern Mesh teapot;
extern Mesh skybox;

extern epm_World g_world;

extern void epm_ComputeVertexBrightnesses(size_t num_vertices, WorldVec const vertices[], size_t num_lights, Light const lights[], size_t num_faces, Face faces[]);

//extern void epm_ComputeVertexBrightnessesNEW(size_t num_vertices, WorldVec const vertices[], size_t num_lights, Light const lights[], size_t num_faces, Face faces[]);

#endif /* WORLD_H */
