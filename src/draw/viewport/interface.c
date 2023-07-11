#include "src/draw/viewport/viewport.h"
#include "src/draw/viewport/viewport_internal.h"

#include "src/interface/interface_registry.h"

//#define VERBOSITY
#include "zigil/diblib_local/verbosity.h"

ViewportInterface default_vpi = {
    .i_VPI = VPI_NONE,
    .mapped_i_VP  = VP_NONE,

    .windata = NULL,
    .onMap   = NULL,
    .onUnmap = NULL,
    .winfncs = {NULL},
    .name = "Default Interface"
};

ViewportInterface * const viewport_interfaces[NUM_VPI] = {
    [VPI_NONE]        = &default_vpi,
    [VPI_MONITOR]     = &interface_Monitor,
    [VPI_WORLD3D]     = &interface_World3D,
    [VPI_WORLDTOP]    = &interface_WorldTop,
    [VPI_WORLDSIDE]   = &interface_WorldSide,
    [VPI_WORLDFRONT]  = &interface_WorldFront,
    [VPI_FILESELECT]  = &interface_FileSelect,
    [VPI_LOGVIEW]     = &interface_LogView,
    [VPI_CONSOLE]     = &interface_Console,
    [VPI_TEXVIEW]     = &interface_TexView,
    [VPI_BSPVIEW]     = &interface_BSPView,
};

VPInterfaceCode interface_cycle[NUM_VPI] = {
    VPI_NONE,
    VPI_MONITOR,
    VPI_WORLD3D,
    VPI_WORLDTOP,
    VPI_WORLDSIDE,
    VPI_WORLDFRONT,
    VPI_FILESELECT,
    VPI_LOGVIEW,
    VPI_CONSOLE,
    VPI_TEXVIEW,
    VPI_BSPVIEW,
};

static void unmap(Viewport *p_VP) {
    dibassert(p_VP != NULL);
    
    ViewportInterface *p_VPI = p_VP->mapped_p_VPI;
    
    if (p_VPI) {
        // disconnect from interface
        if (p_VPI->onUnmap) p_VPI->onUnmap(p_VPI, p_VP);
        p_VPI->mapped_i_VP = VP_NONE;

        // disconnect from viewport
        p_VP->VPI_win.data    = default_vpi.windata;
        p_VP->VPI_win.winfncs = default_vpi.winfncs;
        p_VP->mapped_p_VPI    = &default_vpi;
    }
}

static void map(Viewport *p_VP, ViewportInterface *p_VPI) {
    // arguments assumed non-NULL
    dibassert(p_VP->mapped_p_VPI == &default_vpi ||
              p_VP->mapped_p_VPI == NULL);
    dibassert(p_VPI->mapped_i_VP == VP_NONE);

    // connect to viewport
    p_VP->mapped_p_VPI = p_VPI;
    p_VP->VPI_win.data = p_VPI->windata;
    p_VP->VPI_win.winfncs = p_VPI->winfncs;

    // connect to interface
    p_VPI->mapped_i_VP = p_VP->i_VP;
    if (p_VPI->onMap) p_VPI->onMap(p_VPI, p_VP);
}

void unmap_all_viewports(void) {
    for (size_t i_VP = 0; i_VP < NUM_VP; i_VP++) {
        unmap(viewports + i_VP);
    }
}

static void set_vpi2(Viewport *p_VP, ViewportInterface *p_VPI) {
    if (p_VP == NULL) return;

    // unmap current interface
    if (p_VP->mapped_p_VPI != &default_vpi) {
        //printf("Unmapping %s\n", p_VP->name);
        unmap(p_VP);
    }
    
    // map new interface
    if (p_VPI) {
        // unmap new interface if mapped elsewhere
        if (p_VPI->mapped_i_VP != VP_NONE) {
            unmap(&viewports[p_VPI->mapped_i_VP]);
        }
        vbs_printf("Mapping interface \"%s\" to viewport \"%s\"\n", p_VPI->name, p_VP->name);
        map(p_VP, p_VPI);
    }

}

VPInterfaceCode prev_vpi(VPCode i_VP) {
    if (! viewports[i_VP].mapped_p_VPI) {
        set_vpi2(viewports + i_VP, viewport_interfaces[VPI_NONE]);
    }
    
    VPInterfaceCode i_VPI = viewports[i_VP].mapped_p_VPI->i_VPI;
    i_VPI = ((i_VPI + NUM_VPI) - 1) % NUM_VPI;
    set_vpi2(viewports + i_VP, viewport_interfaces[i_VPI]);

    return i_VPI;
}
VPInterfaceCode next_vpi(VPCode i_VP) {
    if (! viewports[i_VP].mapped_p_VPI) {
        set_vpi2(viewports + i_VP, viewport_interfaces[VPI_NONE]);
    }
    
    VPInterfaceCode i_VPI = viewports[i_VP].mapped_p_VPI->i_VPI;
    i_VPI = (i_VPI + 1) % NUM_VPI;
    set_vpi2(viewports + i_VP, viewport_interfaces[i_VPI]);

    return i_VPI;
}





void epm_SetVPInterface(VPCode i_VP, VPInterfaceCode i_VPI) {
    if (i_VP == VP_NONE) return;
    
    set_vpi2(viewports + i_VP, viewport_interfaces[i_VPI]);
}

VPInterfaceCode epm_PrevVPInterface(void) {
    if ( ! active_viewport) return VPI_NONE;

    if ( ! active_viewport->mapped_p_VPI) {
        set_vpi2(active_viewport, viewport_interfaces[VPI_NONE]);
    }
    
    VPInterfaceCode i_VPI = active_viewport->mapped_p_VPI->i_VPI;
    while (viewport_interfaces[i_VPI]->mapped_i_VP != VP_NONE)
        i_VPI = ((i_VPI + NUM_VPI) - 1) % NUM_VPI;
    set_vpi2(active_viewport, viewport_interfaces[i_VPI]);

    return i_VPI;
}
VPInterfaceCode epm_NextVPInterface(void) {
    if ( ! active_viewport) return VPI_NONE;
    
    if ( ! active_viewport->mapped_p_VPI) {
        set_vpi2(active_viewport, viewport_interfaces[VPI_NONE]);
    }
    
    VPInterfaceCode i_VPI = active_viewport->mapped_p_VPI->i_VPI;

    // find next *unmapped* interface
    while (viewport_interfaces[i_VPI]->mapped_i_VP != VP_NONE) 
        i_VPI = (i_VPI + 1) % NUM_VPI;
    
    set_vpi2(active_viewport, viewport_interfaces[i_VPI]);
    
    return i_VPI;
}


epm_Result epm_InitInterface(void) {
    _epm_Log("INIT.INTERFACE", LT_INFO, "Initializing viewport interfaces.");
    
    for (size_t i_VPI = VPI_NONE + 1; i_VPI < NUM_VPI; i_VPI++) {
        if (viewport_interfaces[i_VPI]->init)
            viewport_interfaces[i_VPI]->init();
    }

    _epm_Log("INIT.INTERFACE", LT_INFO, "Setting initial viewport layout.");
    epm_InitVPLayout(VPL_TRI_R);

    _epm_Log("INIT.INTERFACE", LT_INFO, "Mapping initial viewport interfaces.");
    epm_SetVPInterface(VP_TL,   VPI_WORLD3D);
    epm_SetVPInterface(VP_TR,   VPI_NONE);
    epm_SetVPInterface(VP_BR,   VPI_NONE);
    epm_SetVPInterface(VP_BL,   VPI_LOGVIEW);
    epm_SetVPInterface(VP_L,    VPI_NONE);
    epm_SetVPInterface(VP_R,    VPI_CONSOLE);
    epm_SetVPInterface(VP_T,    VPI_NONE);
    epm_SetVPInterface(VP_B,    VPI_NONE);
    epm_SetVPInterface(VP_FULL, VPI_NONE);

    return EPM_SUCCESS;
}
    
epm_Result epm_TermInterface(void) {
    for (size_t i_VPI = VPI_NONE + 1; i_VPI < NUM_VPI; i_VPI++) {
        if (viewport_interfaces[i_VPI]->term)
            viewport_interfaces[i_VPI]->term();
    }
    
    return EPM_SUCCESS;
}
