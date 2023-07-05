#ifndef VP_H
#define VP_H

#include "src/draw/viewport/viewport.h"

typedef struct ViewbarButton ViewbarButton;
typedef struct ViewbarSpec ViewbarSpec;
typedef struct ViewportInterface ViewportInterface;
typedef struct Viewport Viewport;

typedef void (*fn_onMap)(ViewportInterface *self, Viewport *vp);
typedef void (*fn_onUnmap)(ViewportInterface *self, Viewport *vp);
typedef epm_Result (*fn_init)(void);
typedef epm_Result (*fn_term)(void);

typedef void (*fn_onClickButton)(ViewbarButton *self, Viewport *vp);
typedef void (*fn_updateButton)(ViewbarButton *self, Viewport *vp);
struct ViewbarButton {
    fn_onClickButton onClickButton;
    fn_updateButton updateButton;
    zgl_Pixel icon_pos;

    zgl_Pixel offset; // internal use only
};

struct ViewbarSpec {
    zgl_Pixel icon;
    
    uint8_t num_subbtns;
    ViewbarButton subbtns[8];
    uint8_t num_relbtns;
    ViewbarButton relbtns[8];
};

struct ViewportInterface {
    VPInterfaceCode i_VPI; // assert p_VPI == viewport_interfaces[p_VPI->i_VPI]
    VPCode mapped_i_VP;
   
    void *windata;
    fn_onMap onMap;
    fn_onUnmap onUnmap;
    fn_init init;
    fn_term term;
    WindowFunctions winfncs;
    
    char name[64];
};

struct Viewport {
    VPCode i_VP; // assert p_VP == &viewports[p_VP->i_VP]
    
    Window wrap_win; // container for the bar and interface
    WindowNode wrap_node;
    
    Window bar_win; // viewbar window
    WindowNode bar_node;

    Window VPI_win; // the window that is passed to the interface
    WindowNode VPI_node;
    ViewportInterface *mapped_p_VPI;

    char name[64];
};

typedef struct ViewportLayout {
    int VP_vec[NUM_VP]; // VP_vec[i_VP] is -1 if i_VP is not in VP_cycle,
                        // otherwise it equals the index in VP_cycle
    size_t VP_cycle_len;
    VPCode VP_cycle[4];

    char name[64];
} ViewportLayout;

extern Viewport viewports[NUM_VP];
extern Viewport *active_viewport;
extern ViewportLayout const viewport_layouts[NUM_VPL];
extern ViewportInterface * const viewport_interfaces[NUM_VPI];

#endif /* VP_H */
