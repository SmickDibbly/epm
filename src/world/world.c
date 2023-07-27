#include "src/misc/epm_includes.h"
#include "src/system/dir.h"
#include "src/system/loop.h"
#include "src/draw/textures.h"
#include "src/world/world.h"
#include "src/world/bsp/bsp.h"
#include "src/world/selection.h"
#include "src/entity/entity.h"

Mesh E1M1;
Mesh teapot;
Mesh deccer_cubes;
Mesh skybox;

EditorCamera const default_cam = {
    .pos = {.v = {fixify(2)+4324, fixify(4)-16, fixify(3)+43}},
    .view_angle_h = 0,
    .view_angle_v = ANG18_PI2,
    .key_angular_motion = false,
    .key_angular_h = 0,
    .key_angular_v = 0,
    .mouse_angular_motion = false,
    .mouse_angular_h = 0,
    .mouse_angular_v = 0,
    .view_vec = {.v = {0, fixify(1), 0}},
    .view_vec_XY = {.x = 0, .y = fixify(1)},
    .key_motion = false,
    .mouse_motion = false, 
    .mouse_motion_vel = {.v = {0, 0, 0}},
    .vel = {.v = {0, 0, 0}},
    .collision_radius = CAMERA_COLLISION_RADIUS,
    .collision_height = CAMERA_COLLISION_HEIGHT,
    .collision_box_reach = CAMERA_COLLISION_BOX_REACH,    
};

EditorCamera cam = default_cam;
epm_EntityNode cam_node = {.entity = &cam, .onTic = onTic_cam};

Player player = {.pos = {.v = {0, 0, 256}}};
epm_EntityNode player_node = {.entity = &player, .onTic = onTic_player};

Transform tf;

BrushGeometry g_geo_brush = {0};
static WorldVec   g_pre_V[MAX_STATIC_V] = {0};
static PreVertExt g_pre_VX[MAX_STATIC_V] = {0};
static Edge       g_pre_E[MAX_STATIC_E] = {0};
//static PreEdgeExt g_pre_EX[MAX_STATIC_E] = {0};
static Face       g_pre_F[MAX_STATIC_F] = {0};
static PreFaceExt g_pre_FX[MAX_STATIC_F] = {0};
PreGeometry g_geo_prebsp = {
    .num_verts = 0,
    .verts = g_pre_V,
    .vert_exts = g_pre_VX,

    .num_edges = 0,
    .edges = g_pre_E,

    .num_faces = 0,
    .faces = g_pre_F,
    .face_exts = g_pre_FX,
};
BSPTree g_geo_bsp = {0};
epm_World g_world = {0};

WorldVec onetime_vels[16] = {0};
int onetime_free = 0;
WorldVec cts_vels[16] = {0};
int cts_free = 0;

WorldVec directional_light_vector = {
    .v = {-fixify(1), 0, 0}
};
Fix32 directional_light_intensity = fixify(1)/2;
Fix32 ambient_light_intensity = fixify(1);

WorldVec bigbox_vertices[16] = {
    [0] = {{INT32_MIN, INT32_MIN, INT32_MIN}},
    [1] = {{INT32_MIN, INT32_MIN, INT32_MAX}},
    [2] = {{INT32_MIN, INT32_MAX, INT32_MAX}},
    [3] = {{INT32_MIN, INT32_MAX, INT32_MIN}},
    [4] = {{INT32_MAX, INT32_MIN, INT32_MIN}},
    [5] = {{INT32_MAX, INT32_MIN, INT32_MAX}},
    [6] = {{INT32_MAX, INT32_MAX, INT32_MAX}},
    [7] = {{INT32_MAX, INT32_MAX, INT32_MIN}},

    [8+0] = {{INT32_MIN, 0, 0}},
    [8+1] = {{INT32_MAX, 0, 0}},
    [8+2] = {{0, INT32_MIN, 0}},
    [8+3] = {{0, INT32_MAX, 0}},
    [8+4] = {{INT32_MIN, INT32_MIN, 0}},
    [8+5] = {{INT32_MIN, INT32_MAX, 0}},
    [8+6] = {{INT32_MAX, INT32_MIN, 0}},
    [8+7] = {{INT32_MAX, INT32_MAX, 0}},
};

Edge bigbox_edges[18] = {
    [0]  = {0, 1},
    [1]  = {1, 2},
    [2]  = {2, 3},
    [3]  = {3, 0},
    [4]  = {4, 5},
    [5]  = {5, 6},
    [6]  = {6, 7},
    [7]  = {7, 4},
    [8]  = {0, 4},
    [9]  = {1, 5},
    [10] = {2, 6},
    [11] = {3, 7},

    [12] = {8+0, 8+1},
    [13] = {8+2, 8+3},
    [14] = {8+4, 8+5},
    [15] = {8+5, 8+7},
    [16] = {8+7, 8+6},
    [17] = {8+6, 8+4},
};

EdgeSet view3D_bigbox = {
    .num_verts = 16,
    .verts = bigbox_vertices,
    .num_edges = 18,
    .edges = bigbox_edges,
    .wirecolor = 0x2596BE
};


WorldVec grid_vertices[24] = {
    {{3*(FIX32_MAX/4), FIX32_MIN, 0}},
    {{3*(FIX32_MAX/4), FIX32_MAX, 0}},
    {{FIX32_MAX/2, FIX32_MIN, 0}},
    {{FIX32_MAX/2, FIX32_MAX, 0}},
    {{FIX32_MAX/4, FIX32_MIN, 0}},
    {{FIX32_MAX/4, FIX32_MAX, 0}},
    {{FIX32_MIN/4, FIX32_MIN, 0}},
    {{FIX32_MIN/4, FIX32_MAX, 0}},
    {{FIX32_MIN/2, FIX32_MIN, 0}},
    {{FIX32_MIN/2, FIX32_MAX, 0}},
    {{3*(FIX32_MIN/4), FIX32_MIN, 0}},
    {{3*(FIX32_MIN/4), FIX32_MAX, 0}},

    {{FIX32_MIN, 3*(FIX32_MAX/4), 0}},
    {{FIX32_MAX, 3*(FIX32_MAX/4), 0}},
    {{FIX32_MIN, FIX32_MAX/2, 0}},
    {{FIX32_MAX, FIX32_MAX/2, 0}},
    {{FIX32_MIN, FIX32_MAX/4, 0}},
    {{FIX32_MAX, FIX32_MAX/4, 0}},
    {{FIX32_MIN, FIX32_MIN/4, 0}},
    {{FIX32_MAX, FIX32_MIN/4, 0}},
    {{FIX32_MIN, FIX32_MIN/2, 0}},
    {{FIX32_MAX, FIX32_MIN/2, 0}},
    {{FIX32_MIN, 3*(FIX32_MIN/4), 0}},
    {{FIX32_MAX, 3*(FIX32_MIN/4), 0}},    
};

Edge grid_edges[12] = {
    [0] =  {0, 1},
    [1] =  {2, 3},
    [2] =  {4, 5},
    [3] =  {6, 7},

    [4] =  {8, 9},
    [5] =  {10, 11},
    [6] =  {12, 13},
    [7] =  {14, 15},

    [8] =  {16, 17},
    [9] =  {18, 19},
    [10] = {20, 21},
    [11] = {22, 23},    
};

EdgeSet view3D_grid = {
    .num_verts = 24,
    .verts = grid_vertices,
    .num_edges = 12,
    .edges = grid_edges,
    .wirecolor = 0x235367
};

epm_Result epm_Tic(void) {
    for (epm_EntityNode *node = g_world.entity_head.next; node; node = node->next) {
        if (node->onTic) node->onTic(node->entity);
    }
    
    return EPM_CONTINUE;
}


epm_Result epm_InitWorld(void) {
    g_world.worldflags = 0;
    g_world.geo_brush = &g_geo_brush;
    g_world.geo_prebsp = &g_geo_prebsp;
    g_world.geo_bsp = &g_geo_bsp;

    g_world.entity_head.next = &cam_node;
    cam_node.next = &player_node;
    player_node.next = NULL;
    
    g_frame = create_cuboid_brush
        ((WorldVec){{-fixify(128), -fixify(128), -fixify(128)}},
         (WorldVec){{ fixify(256),  fixify(256),  fixify(256)}});
    g_world.worldflags |= WF_LOADED_BRUSHGEO;
    
    extern void init_OBJ_reader(void);
    init_OBJ_reader();
    read_OBJ(&skybox, "skybox");

    
    //read_OBJ(&teapot, "african_head");
    //read_OBJ(&teapot, "DOOM_E1M1");
    //read_GLTF(&teapot, "SM_Deccer_Cubes");
    //epm_WorldFromMesh(&teapot);
    

    /*
    find_all_t_junctions(&(Mesh){
            .num_vertices = g_world.geo_bsp->num_vertices,
            .vertices = g_world.geo_bsp->vertices,
            .num_edges = g_world.geo_bsp->num_edges,
            .edges = g_world.geo_bsp->edges,
            .num_faces = g_world.geo_bsp->num_faces,
            .faces = g_world.geo_bsp->faces,
        });
    */
    //fix_all_t_junctions((Mesh){});

    //read_OBJ(&teapot, "DOOM_E1M1");
    //read_OBJ(&skybox, "skybox");        
    //epm_LoadWorld("brush_world2");

    return EPM_SUCCESS;
}

epm_Result epm_TermWorld(void) {
    BrushSel_clear();
    BrushPolySel_clear();
    BrushVertSel_clear();
    
    epm_UnloadWorld();

    destroy_brush(g_frame);
    
    return EPM_SUCCESS;
}

epm_Result epm_LoadWorld(char *worldname) {
    BrushSel_clear();
    BrushPolySel_clear();
    BrushVertSel_clear();
    
    if (g_world.worldflags) epm_UnloadWorld();
    g_world.worldflags = 0;
    
    if (EPM_SUCCESS != epm_ReadWorldFile(&g_world, worldname)) {
        epm_UnloadWorld();
        return EPM_FAILURE;
    }
    g_world.worldflags |= WF_LOADED_BRUSHGEO;

    epm_Build();

    // Prepare entities. TODO: Don't hardcode this obviously.
    g_world.entity_head.next = &cam_node;
    cam_node.next = &player_node;
    player_node.next = NULL;
    g_world.worldflags |= WF_LOADED_ENTITY;

    BrushSel_clear();
    BrushPolySel_clear();
    BrushVertSel_clear();

    return EPM_SUCCESS;
}

epm_Result epm_WorldFromMesh(Mesh *mesh) {
    BrushSel_clear();
    BrushPolySel_clear();
    BrushVertSel_clear();
    
    if (g_world.worldflags) epm_UnloadWorld();
    
    g_world.geo_prebsp->num_verts = mesh->num_verts;
    memcpy(g_world.geo_prebsp->verts, mesh->verts, mesh->num_verts*sizeof(WorldVec));

    g_world.geo_prebsp->num_edges = mesh->num_edges;
    memcpy(g_world.geo_prebsp->edges, mesh->edges, mesh->num_edges*sizeof(Edge));

    g_world.geo_prebsp->num_faces = mesh->num_faces;
    memcpy(g_world.geo_prebsp->faces, mesh->faces, mesh->num_faces*sizeof(Face));

    g_world.worldflags |= WF_LOADED_PREBSPGEO;

    epm_BuildLighting();
    epm_BuildBSP();
    
    // temporary
    g_world.entity_head.next = &cam_node;
    cam_node.next = &player_node;
    player_node.next = NULL;
    g_world.worldflags |= WF_LOADED_ENTITY;

    return EPM_SUCCESS;
}

epm_Result epm_UnloadWorld(void) {
    destroy_BSPTree(g_world.geo_bsp);
    g_world.worldflags &= ~WF_LOADED_BSPGEO;
    
    reset_PreGeometry();
    g_world.worldflags &= ~WF_LOADED_PREBSPGEO;

    reset_BrushGeometry();
    g_world.worldflags &= ~WF_LOADED_PREBSPGEO;
    
    return EPM_SUCCESS;
}

epm_Result reset_BrushGeometry(void) {
    if (g_geo_brush.head == NULL) {
        dibassert(g_geo_brush.tail == NULL);
        return EPM_SUCCESS;
    }
    
    BrushNode *node_next;
    for (BrushNode *node = g_geo_brush.head; node; node = node_next) {
         node_next = node->next;

         dibassert(node->brush);
         destroy_brush(node->brush);
         zgl_Free(node);
    }
 
    g_geo_brush.head = NULL;
    g_geo_brush.tail = NULL;
    
    return EPM_SUCCESS;
}

epm_Result reset_PreGeometry(void) {
    g_geo_prebsp.num_verts = 0;
    g_geo_prebsp.num_edges = 0;
    g_geo_prebsp.num_faces = 0;
    
    return EPM_SUCCESS;    
}


#include "src/input/command.h"

static void CMDH_worldfromobj(int argc, char **argv, char *output_str) {
    Mesh mesh;
    read_OBJ(&mesh, argv[1]);
    epm_WorldFromMesh(&mesh);
}

epm_Command const CMD_worldfromobj = {
    .name = "worldfromobj",
    .argc_min = 2,
    .argc_max = 2,
    .handler = CMDH_worldfromobj,
};

static void CMDH_loadworld(int argc, char **argv, char *output_str) {
    (void)output_str;

    epm_LoadWorld(argv[1]);
}

epm_Command const CMD_loadworld = {
    .name = "loadworld",
    .argc_min = 2,
    .argc_max = 2,
    .handler = CMDH_loadworld,
};

static void CMDH_unloadworld(int argc, char **argv, char *output_str) {
    (void)argc, (void)argv, (void)output_str;
    
    epm_UnloadWorld();
}

epm_Command const CMD_unloadworld = {
    .name = "unloadworld",
    .argc_min = 1,
    .argc_max = 1,
    .handler = CMDH_unloadworld,
};


epm_Result epm_SaveWorld(char const *filename) {
    epm_WriteWorldFile(&g_world, filename);

    return EPM_SUCCESS;
}

static void CMDH_saveworld(int argc, char **argv, char *output_str) {
    (void)argc, (void)argv, (void)output_str;

    if (argc == 1)
        epm_SaveWorld("tmp"); // TODO: Save over current file.
    else
        epm_SaveWorld(argv[1]); // TODO: Save over current file.
}

epm_Command const CMD_saveworld = {
    .name = "saveworld",
    .argc_min = 1,
    .argc_max = 2,
    .handler = CMDH_saveworld,
};


epm_Result epm_BuildPreBSP(void) {
    if (EPM_SUCCESS != triangulate_world()) {
        epm_UnloadWorld();
        return EPM_FAILURE;
    }
    g_world.worldflags |= WF_LOADED_PREBSPGEO;
    
    return EPM_SUCCESS;
}

epm_Result epm_BuildLighting(void) {
    g_world.lights[0] = (Light){.pos = (WorldVec){{fixify(256), fixify(256), 0}},
                                .brightness = 128,
                                .inner_radius = fixify(1),
                                .outer_radius = fixify(512)};
    g_world.lights[1] = (Light){.pos = (WorldVec){{-fixify(256), -fixify(256), 0}},
                                .brightness = 96,
                                .inner_radius = fixify(1),
                                .outer_radius = fixify(512)};
    g_world.num_lights = 2;

    epm_ComputeVertexBrightnesses(g_geo_prebsp.num_verts, g_geo_prebsp.verts,
                                  g_world.num_lights, g_world.lights,
                                  g_geo_prebsp.num_faces, g_geo_prebsp.faces);

    return EPM_SUCCESS;
    return EPM_SUCCESS;
}

epm_Result epm_BuildBSP(void) {
    if (EPM_SUCCESS !=
        create_BSPTree(g_world.geo_bsp, g_world.geo_prebsp)) {
        epm_UnloadWorld();
        return EPM_FAILURE;
    }
    g_world.worldflags |= WF_LOADED_BSPGEO;

    extern void update_BSPView(void);
    update_BSPView();
    
    return EPM_SUCCESS;
}

/* Make no change to brush geometry, but process the world from that point. */
epm_Result epm_Build(void) {
    BrushSel_clear();
    BrushPolySel_clear();
    BrushVertSel_clear();
    
    // Unload only BSP and PreBSP
    destroy_BSPTree(g_world.geo_bsp);
    g_world.worldflags &= ~WF_LOADED_BSPGEO;
    reset_PreGeometry();
    g_world.worldflags &= ~WF_LOADED_PREBSPGEO;

    epm_BuildPreBSP();
    epm_BuildLighting();
    epm_BuildBSP();

    return EPM_SUCCESS;
}

void print_StaticGeometry(void) {
    PreGeometry *geo = g_world.geo_prebsp;

    for (size_t i_v = 0; i_v < geo->num_verts; i_v++) {
        printf("v %s %s %s\n",
               fmt_fix_x(x_of(geo->verts[i_v]), 16),
               fmt_fix_x(y_of(geo->verts[i_v]), 16),
               fmt_fix_x(z_of(geo->verts[i_v]), 16));
    }

    for (size_t i_e = 0; i_e < geo->num_edges; i_e++) {
        printf("e %zu %zu\n", geo->edges[i_e].i_v0, geo->edges[i_e].i_v1);
    }

    for (size_t i_f = 0; i_f < geo->num_faces; i_f++) {
        printf("f %zu %zu %zu\n",
               geo->faces[i_f].i_v[0],
               geo->faces[i_f].i_v[1],
               geo->faces[i_f].i_v[2]);
    }
}

static void CMDH_rebuild(int argc, char **argv, char *output_str) {
    /*
    if ( ! g_world.loaded) {
        sprintf(output_str, "Can't rebuild an empty world.");
        return;
    }
    */

    epm_Build();
}

epm_Command const CMD_rebuild = {
    .name = "rebuild",
    .argc_min = 1,
    .argc_max = 1,
    .handler = CMDH_rebuild,
};


void epm_ComputeVertexBrightnessesNEW(size_t num_vertices, WorldVec const vertices[], size_t num_lights, Light const lights[], size_t num_faces, Face faces[]) {
    for (size_t i_f = 0; i_f < num_faces; i_f++) {
        Face *face = faces + i_f;
        WorldVec fnormal = face->normal;

        for (size_t i_i_v = 0; i_i_v < 3; i_i_v++) {
            WorldVec v = vertices[face->i_v[i_i_v]];
            Fix64 vI = 0;

            for (size_t i_l = 0; i_l < num_lights; i_l++) {
                Light const *light = lights + i_l;
                WorldVec l = light->pos; 
                int16_t lI = light->brightness; // .0
                Fix32 r = light->inner_radius; // .16
                Fix32 R = light->outer_radius; // .16

                // linear attentuation
                Fix64 D = norm_Euclidean(diff(l, v))/*>>8*/; // .16
                /*
                  Fix64 D2 = ((Fix64)x_of(diff)*(Fix64)x_of(diff) +
                  (Fix64)y_of(diff)*(Fix64)y_of(diff) +
                  (Fix64)z_of(diff)*(Fix64)z_of(diff))>>18;
                */
                //            printf("L = %lu\n", lI);
                //            printf("r = %s\n", fmt_fix_d(r, 16, 4));
                //            printf("R = %s\n", fmt_fix_d(R, 16, 4));
                //            printf("D = %s\n", fmt_fix_d(D, 16, 4));
            
                // Suppose we have D = 256 Then 1/D = 1/256. So if L=255 then L/D = 0. Thus we should have a scalar c so that D = R gives cI/R = 1/256. ie. c = I*256/R,
            
                Fix64 vI_inc;
                if (D <= r) {
                    vI_inc = lI;
                }
                else if (R <= D) {
                    vI_inc = 0;
                }
                else {
                    //inc = (lbri*FIX_MUL(r,R))/(256*D);
                    vI_inc = (lI*(D-R))/(r-R);
                    //inc = (lbri<<16)/(D);
                }

                WorldVec dir = diff(l, v);
                while (x_of(dir) > (1<<16) ||
                       y_of(dir) > (1<<16) ||
                       z_of(dir) > (1<<16)) {
                    x_of(dir) >>= 1;
                    y_of(dir) >>= 1;
                    z_of(dir) >>= 1;
                }
                UFix32 norm = norm_Euclidean(dir);
                x_of(dir) = (Fix32)FIX_DIV(x_of(dir), norm);
                y_of(dir) = (Fix32)FIX_DIV(y_of(dir), norm);
                z_of(dir) = (Fix32)FIX_DIV(z_of(dir), norm);
                
                Fix64 prod = dot(fnormal, dir); // [-1, 1] (.16)
                vI_inc = ABS(prod * vI_inc)>>16;

                //printf("inc = %lu\n\n", vI_inc);
                dibassert(vI_inc <= 255);
                vI = MIN(vI + vI_inc, 255);
            }

            face->vbri[i_i_v] = (uint8_t)vI;
        }
    }
}

void epm_ComputeVertexBrightness
(WorldVec v,
 size_t num_lights, Light const lights[],
 uint8_t *vbri) {
    Fix64 vI = 0;

    for (size_t i_l = 0; i_l < num_lights; i_l++) {
        Light const *light = lights + i_l;
        WorldVec l = light->pos; 
        int16_t lI = light->brightness; // .0
        Fix32 r = light->inner_radius; // .16
        Fix32 R = light->outer_radius; // .16

        Fix64 D = norm_Euclidean(diff(l, v))/*>>8*/; // .16

        Fix64 vI_inc;
        if (D <= r) {
            vI_inc = lI;
        }
        else if (R <= D) {
            vI_inc = 0;
        }
        else {
            vI_inc = (lI*(D-R))/(r-R);
        }

        dibassert(vI_inc <= 255);
        vI = MIN(vI + vI_inc, 255);
    }
    *vbri = (uint8_t)(vI);
}

void epm_ComputeVertexBrightnesses(size_t num_vertices, WorldVec const vertices[], size_t num_lights, Light const lights[], size_t num_faces, Face faces[]) {
    uint8_t *tmp_vbri = zgl_Malloc(num_vertices*sizeof(*tmp_vbri));
    
    for (size_t i_v = 0; i_v < num_vertices; i_v++) {
        WorldVec v = vertices[i_v];
        Fix64 vI = 0;

        for (size_t i_l = 0; i_l < num_lights; i_l++) {
            Light const *light = lights + i_l;
            WorldVec l = light->pos; 
            int16_t lI = light->brightness; // .0
            Fix32 r = light->inner_radius; // .16
            Fix32 R = light->outer_radius; // .16

            // linear attentuation
            Fix64 D = norm_Euclidean(diff(l, v))/*>>8*/; // .16
            /*
              Fix64 D2 = ((Fix64)x_of(diff)*(Fix64)x_of(diff) +
              (Fix64)y_of(diff)*(Fix64)y_of(diff) +
              (Fix64)z_of(diff)*(Fix64)z_of(diff))>>18;
            */
            // printf("L = %lu\n", lI);
            // printf("r = %s\n", fmt_fix_d(r, 16, 4));
            // printf("R = %s\n", fmt_fix_d(R, 16, 4));
            // printf("D = %s\n", fmt_fix_d(D, 16, 4));
            
            // Suppose we have D = 256 Then 1/D = 1/256. So if L=255 then L/D = 0. Thus we should have a scalar c so that D = R gives cI/R = 1/256. ie. c = I*256/R,

            Fix64 vI_inc;
            if (D <= r) {
                vI_inc = lI;
            }
            else if (R <= D) {
                vI_inc = 0;
            }
            else {
                //inc = (lbri*FIX_MUL(r,R))/(256*D);
                vI_inc = (lI*(D-R))/(r-R);
                //inc = (lbri<<16)/(D);
            }

            //printf("(i_l %zu) (vI_inc %lu)\n\n", i_l, vI_inc);
            dibassert(vI_inc <= 255);
            vI = MIN(vI + vI_inc, 255);
        }
        tmp_vbri[i_v] = (uint8_t)(vI);
    }

    for (size_t i_f = 0; i_f < num_faces; i_f++) {
        Face *face = faces + i_f;
        for (size_t i_i_v = 0; i_i_v < 3; i_i_v++) {
            face->vbri[i_i_v] = tmp_vbri[face->i_v[i_i_v]];
        }
    }

    zgl_Free(tmp_vbri);
}
