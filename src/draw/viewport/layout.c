#include "src/draw/viewport/viewport.h"
#include "src/draw/viewport/viewport_internal.h"

#include "src/draw/window/window_registry.h"
#include "src/input/input.h"

// The active preference is used during a viewport layout change to decide which
// viewport gets the active status when the currently active viewport becomes
// hidden.
//static VPCode active_prefer;




/* Covering[i][j] is:
     0 if i doesn't cover j and j doesn't cover i
     1 if i covers j
     2 if j covers i

     By convention, we declare that a viewport doesn't cover itself and isn't
     covered by itself.
 */
/*
#define NEITHER 0
#define COVERS 1
#define COVERED_BY 2
static int covering[NUM_VP][NUM_VP] = {
    [VP_NONE] = {0, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    [VP_TL]   = {1, 0, 0, 0, 0, 2, 0, 2, 0, 2},
    [VP_TR]   = {1, 0, 0, 0, 0, 0, 2, 2, 0, 2},
    [VP_BR]   = {1, 0, 0, 0, 0, 0, 2, 0, 2, 2},
    [VP_BL]   = {1, 0, 0, 0, 0, 2, 0, 0, 2, 2},
    [VP_L]    = {1, 1, 0, 0, 1, 0, 0, 0, 0, 2},
    [VP_R]    = {1, 0, 1, 1, 0, 0, 0, 0, 0, 2},
    [VP_T]    = {1, 1, 1, 0, 0, 0, 0, 0, 0, 2},
    [VP_B]    = {1, 0, 0, 1, 1, 0, 0, 0, 0, 2},
    [VP_FULL] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
};
*/

VPLayoutCode i_curr_VPL;

ViewportLayout const viewport_layouts[NUM_VPL] = {
    [VPL_QUAD]   = {
        .VP_vec = {-1, 0, 1, 2, 3, -1, -1, -1, -1, -1},
        .VP_cycle_len = 4, .VP_cycle = {VP_TL, VP_TR, VP_BR, VP_BL}},
    [VPL_TRI_L]  = {
        .VP_vec = {-1, -1, 1, 2, -1, 0, -1, -1, -1, -1},
        .VP_cycle_len = 3, .VP_cycle = {VP_L, VP_TR, VP_BR}},
    [VPL_TRI_R]  = {
        .VP_vec = {-1, 0, -1, -1, 2, -1, 1, -1, -1, -1},
        .VP_cycle_len = 3, .VP_cycle = {VP_TL, VP_R, VP_BL}},
    [VPL_TRI_T]  = {
        .VP_vec = {-1, -1, -1, 1, 2, -1, -1, 0, -1, -1},
        .VP_cycle_len = 3, .VP_cycle = {VP_T, VP_BR, VP_BL}},
    [VPL_TRI_B]  = {
        .VP_vec = {-1, 0, 1, -1, -1, -1, -1, -1, 2, -1},
        .VP_cycle_len = 3, .VP_cycle = {VP_TL, VP_TR, VP_B}},
    [VPL_DUO_LR] = {
        .VP_vec = {-1, -1, -1, -1, -1, 0, 1, -1, -1, -1},
        .VP_cycle_len = 2, .VP_cycle = {VP_L, VP_R}},
    [VPL_DUO_TB] = {
        .VP_vec = {-1, -1, -1, -1, -1, -1, -1, 0, 1, -1},
        .VP_cycle_len = 2, .VP_cycle = {VP_T, VP_B}},
    [VPL_MONO]   = {
        .VP_vec = {-1, -1, -1, -1, -1, -1, -1, -1, -1, 0},
        .VP_cycle_len = 1, .VP_cycle = {VP_FULL}},
};
// this also acts as a cycle for cycling through the different layouts


#define STAY_HIDDEN 0
#define HIDE 1
#define SHOW 2
#define STAY_SHOWN 3

void epm_InitVPLayout(VPLayoutCode i_VPL) {
    //    void unmap_all_viewports(void);
    //    unmap_all_viewports();

    i_curr_VPL = i_VPL;
    
    for (size_t i_VP_cycle = 0; i_VP_cycle < viewport_layouts[i_VPL].VP_cycle_len; i_VP_cycle++) {
        link_WindowNode(viewport_wrap_nodes[viewport_layouts[i_VPL].VP_cycle[i_VP_cycle]], view_container_node);
    }

    
}

void epm_SetVPLayout(VPLayoutCode i_new_VPL) {
    ViewportLayout const *old_layout;
    ViewportLayout const *new_layout;
    old_layout = &viewport_layouts[i_curr_VPL];
    new_layout = &viewport_layouts[i_new_VPL];
    i_curr_VPL = i_new_VPL;
    
    // TODO: Preservation type:
    //    1) Preserve mapping when a viewport stays shown during transition.
    //    2) Preserve all mappings per-layout.

    // TODO: If active_viewport is non-NULL before, it should remain non-NULL
    // after. Decide which viewport gets active status. If the active viewport
    // stays shown, just let it keep the focus.

    // TODO: Unmap viewports that become hidden?
    
    int delta[NUM_VP] = {0};
    for (size_t i_VP = 0; i_VP < NUM_VP; i_VP++) {
        // for each viewport, record whether it will be hidden, shown, unchanged (hidden or shown) by the layout change

        if (old_layout->VP_vec[i_VP] != -1)
            delta[i_VP] += 1;

        if (new_layout->VP_vec[i_VP] != -1)
            delta[i_VP] += 2;

        /* temp[i]        meaning
             0           NOT IN old layout, NOT IN new layout  (STAY HIDDEN)
             1               IN old layout, NOT IN new layout  (HIDE)
             2           NOT IN old layout,     IN new layout  (SHOW)
             3               IN old layout,     IN new layout  (STAY SHOWN)
         */

        extern WindowNode *input_focus;
        if (delta[i_VP] == HIDE) {
            unlink_WindowNode(viewport_wrap_nodes[i_VP]);
            if (input_focus == viewport_VPI_nodes[i_VP]) {
                epm_UnsetInputFocus();
                // TODO: Instead of Unsetting, set the input focus to whatever
                // viewport is replacing this one.
                dibassert(!input_focus);
            }
            // unmap the now-hidden viewport's interface
            epm_SetVPInterface(i_VP, VPI_NONE);
        }
        
        if (delta[i_VP] == SHOW) {
            link_WindowNode(viewport_wrap_nodes[i_VP], view_container_node);
        }
            
    }

    epm_SetInputFocus(&viewports[viewport_layouts[i_new_VPL].VP_cycle[0]].VPI_node);
}

VPLayoutCode epm_PrevVPLayout(void) {
    VPLayoutCode i_VPL = (i_curr_VPL + NUM_VPL - 1) % NUM_VPL;
    epm_SetVPLayout(i_VPL);
    
    return i_VPL;
}

VPLayoutCode epm_NextVPLayout(void) {
    VPLayoutCode i_VPL = (i_curr_VPL + 1) % NUM_VPL;
    epm_SetVPLayout(i_VPL);
    
    return i_VPL;
}
