#ifndef DRAW3D_H
#define DRAW3D_H

#include "src/misc/epm_includes.h"
#include "zigil/zigil.h"
#include "src/draw/draw_triangle.h"
//#include "src/draw/window/window.h"

typedef struct TransformedVertex {
    Fix64Vec eye; // vertex in viewspacae
    DepthPixel pixel;
    uint8_t vis;
} TransformedVertex;

typedef struct TransformedFace {
    TransformedVertex tv[3];
    uint8_t vbri[3];
} TransformedFace;

typedef struct TransformedWireFace {
    DepthPixel pixel[3];
    uint8_t vis[3];
    zgl_Color color;
} TransformedWireFace;

typedef struct DrawSettings {
    int proj_mode;
    int light_mode;
    int wire_mode;
    int wiregeo;
    bool BSP_visualizer;
    bool normal_visualizer;
} DrawSettings;

extern DrawSettings g_settings;

typedef struct Frustum {
    int type;
    
    // world coordinates (WC)
    Fix32 W; // depends only on FOV and d
    Fix32 H;

    Fix32Vec VRP; // view reference point
    Fix32Vec VPN; // view-plane normal
    Fix32Vec VUP; // view up vector
    Fix32    VPD; // view-plane distance

    // View Volume Specification
    // viewing-reference coordinates (VRC)
    Fix32Vec PRP; // projection reference point
    Fix32 u_min, u_max, v_min, v_max;
    Fix32 F, B; // front and back distance
    Fix32 D; // distance from projection point to projection plane

    // 3D Viewport Specification
    Fix32 xmin, xmax, ymin, ymax, zmin, zmax;

    // normalized projection coordinates (NPC)
} Frustum;


extern int32_t BSPVertex_below(Window *win, zgl_Pixel pixel);
extern void epm_SelectBrushesByVertex(Window *win, zgl_Pixel pixel);    
extern void select_face(Window *win, zgl_Pixel pixel);
extern void select_one_face(Window *win, zgl_Pixel pixel);
extern void select_brush(Window *win, zgl_Pixel pixel);
extern void select_vert(Window *win, zgl_Pixel pixel);
extern void draw3D(Window *win);
//extern Fix64Vec worldspace_to_eyespace(WitPoint in);
extern void clip_trans_triangulate_and_draw_face(Window *win, Face *face, TransformedVertex const *const vbuf);
extern void clip_and_transform(Window const *const win, Fix64Vec V0, Fix64Vec V1, Fix64Vec V2, size_t *out_num_subvs, Fix64Vec *out_subvs, TransformedVertex *out_subvbuf, bool *out_new);
extern void epm_DrawFace(Window *win, Face *face, TransformedFace *tf);
extern void epm_DrawWireframeFace
(Window *win, TransformedWireFace *twf, zgl_Color color);

/* -------------------------------------------------------------------------- */

// Visibility codes of a point in space, with respect to a cuboid viewing
// frustum. Can be stored in uint8_t. A viscode of 0 means the point is inside
// the frustum.
#define VIS_NEAR   0x01
#define VIS_FAR    0x02
#define VIS_LEFT   0x04
#define VIS_RIGHT  0x08
#define VIS_ABOVE  0x10
#define VIS_BELOW  0x20

#define DEFAULT_HALF_FOV (ANG18_PI4)
#define DEFAULT_DISTANCE_FROM_CAMERA (fixify32(8))

/* A minimal set of parameters to completely determine a viewing frustum. */
typedef struct FrustumParameters {
    int type;
    Ang18 persp_half_fov;
    Fix32 ortho_W;
    
    Fix32 D, F, B;
} FrustumParameters;

extern FrustumParameters frset_persp;
extern FrustumParameters frset_ortho;

/* -------------------------------------------------------------------------- */
// Render Settings

extern zgl_Color (*fn_set_fclr)(Face *);

#define WIRE_OFF     0
#define WIRE_NO_POLY 1
#define WIRE_OVERLAY 2
#define NUM_WIRE     3
extern void epm_SetWireMode(int mode);
extern void epm_ToggleWireMode(void);

#define WIREGEO_BRUSH        0
#define WIREGEO_PREBSP       1
#define WIREGEO_BSP_EDGEWISE 2
#define WIREGEO_BSP_FACEWISE 3
#define NUM_WIREGEO          4
extern void epm_SetWireGeometry(int wiregeo);

#define LIGHT_OFF                 0
#define LIGHT_FBRI                1
#define LIGHT_VBRI                2
#define LIGHT_BOTH                3
#define NUM_LIGHT                 4
extern void epm_SetLightMode(int mode);
extern void epm_ToggleLightMode(void);

#define PROJ_PERSPECTIVE  0
#define PROJ_ORTHOGRAPHIC 1
#define NUM_PROJ          2
extern void epm_SetProjectionMode(int mode);
extern void epm_ToggleProjectionMode(void);

extern void epm_SetBSPVisualizerMode(bool state);
extern void epm_ToggleBSPVisualizerMode(void);
extern void epm_SetNormalVisualizerMode(bool state);
extern void epm_ToggleNormalVisualizerMode(void);
    
#endif /* DRAW3D_H */
