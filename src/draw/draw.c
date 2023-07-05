#include "zigil/zigil.h"

#include "src/misc/epm_includes.h"
#include "src/ntsc/crt_core.h"

#include "src/draw/default_layout.h"
#include "src/draw/textoverlay.h"
#include "src/draw/textures.h"
#include "src/draw/colors.h"

#include "src/draw/window/window.h"
#include "src/draw/window/window_registry.h"

#include "src/draw/viewport/viewport.h"
#include "src/draw/viewport/viewport_internal.h"

#include "src/draw/draw.h"

// weird includes
#include "src/system/loop.h"
#include "src/system/config_reader.h"

#undef LOG_LABEL
#define LOG_LABEL "DRAW"

#ifdef TWO_FRAMEBUFFER
static zgl_PixelArray *video_out;
static zgl_PixelArray *__scrA;
static zgl_PixelArray *__scrB;
static zgl_PixelArray *g_scr_prev = NULL;
#endif

zgl_PixelArray *g_scr;
zgl_Color *g_scr_pixels;
zgl_Pixit g_scr_w;
zgl_Pixit g_scr_h;

static Window root_window = {
    .name = "Root Window",
    .dragged = false,
    .last_ptr_x = 0,
    .last_ptr_y = 0,
    .cursor = ZC_arrow,
    .winfncs = {
        .draw = NULL,
        .onPointerPress = NULL,
        .onPointerRelease = NULL,
        .onPointerMotion = NULL,
        .onPointerEnter = NULL,
        .onPointerLeave = NULL,
    }
};
static WindowNode _root_node = {.win = &root_window, NULL};
WindowNode *const root_node = &_root_node;

bool show_textoverlay = false;

extern void init_sidebar(void);
extern void init_menubar(void);
extern void init_dropdownmenu(void);
extern void init_viewports(void);
extern epm_Result epm_InitText(void);
extern epm_Result epm_TermText(void);
extern void epm_InitTextOverlay(void);



epm_Result epm_InitDraw(void) {
#ifdef TWO_FRAMEBUFFER
    video_out = &fb_info.fb;
    __scrA = zgl_CreatePixelArray(video_out->w, video_out->h);
    __scrB = zgl_CreatePixelArray(video_out->w, video_out->h);
    g_scr = __scrA;
#else
    g_scr = &fb_info.fb;
#endif
    
    g_scr_pixels = g_scr->pixels;
    g_scr_w = g_scr->w;
    g_scr_h = g_scr->h;
    
    if (EPM_SUCCESS != epm_InitText()) {
        return EPM_FAILURE;
    }
    epm_InitTextOverlay();
    epm_Result init_Draw3D(void);
    init_Draw3D();

    
    read_color_config();
    
    configure_default_layout();

    _epm_Log("INIT.DRAW", LT_INFO, "Initializing windows.");
    
    init_menubar();
    init_sidebar();
    init_dropdownmenu();
    extern void load_icons(void);
    load_icons();
    

    _epm_Log("INIT.DRAW", LT_INFO, "Initializing viewports.");
    init_viewports();
    

    _epm_Log("INIT.DRAW", LT_INFO, "Linking windows.");
    root_window.rect.x = root_window.rect.y = 0;
    root_window.rect.w = g_scr->w;
    root_window.rect.h = g_scr->h;
    link_WindowNode(sidebar_node, root_node);
    link_WindowNode(view_container_node, root_node);
    link_WindowNode(menubar_node, root_node);
    for (VPCode i_VP = 1; i_VP < NUM_VP; i_VP++) {
        link_WindowNode(viewport_bar_nodes[i_VP], viewport_wrap_nodes[i_VP]);
        link_WindowNode(viewport_VPI_nodes[i_VP], viewport_wrap_nodes[i_VP]);
    }

#ifdef VERBOSITY
    print_WindowTree(root_node);
#endif
    
    _epm_Log("INIT.DRAW", LT_INFO, "Preloading textures.");
    // Loading textures. TODO: This can be done on an as-needed basis upon
    // loading a world.
    size_t tmp;
    if (EPM_FAILURE == get_texture_by_name("null", &tmp)) {
        return EPM_FAILURE;
    }
    /*
    if (EPM_FAILURE == get_texture_by_name("grass256", &tmp)) {
        return EPM_FAILURE;
    }
    if (EPM_FAILURE == get_texture_by_name("dirt09_256", &tmp)) {
        return EPM_FAILURE;
    }
    if (EPM_FAILURE == get_texture_by_name("rock256", &tmp)) {
        return EPM_FAILURE;
    }
    if (EPM_FAILURE == get_texture_by_name("brick02_128", &tmp)) {
        return EPM_FAILURE;
    }
    if (EPM_FAILURE == get_texture_by_name("squarepave", &tmp)) {
        return EPM_FAILURE;
    }
    if (EPM_FAILURE == get_texture_by_name("quaker", &tmp)) {
        return EPM_FAILURE;
    }
    if (EPM_FAILURE == get_texture_by_name("sky", &tmp)) {
        return EPM_FAILURE;
    }
    */
    
    zgl_PixelArray *tmp_pixarr = zgl_ReadBMP("../assets/light_icon.bmp");
    MIP_light_icon = zgl_MipifyPixelArray(tmp_pixarr);
    zgl_DestroyPixelArray(tmp_pixarr);

    tmp_pixarr = zgl_ReadBMP("../assets/editorcam.bmp");
    MIP_edcam_icon = zgl_MipifyPixelArray(tmp_pixarr);
    zgl_DestroyPixelArray(tmp_pixarr);
    
    return EPM_SUCCESS;
}

epm_Result epm_TermDraw(void) {
    extern void unload_icons(void);
    unload_icons();

    zgl_DestroyMipMap(MIP_light_icon);
    
    for (size_t i_tex = 0; i_tex < g_num_textures; i_tex++) {
        unload_Texture(textures + i_tex);
    }

    epm_TermText();
    
    return EPM_SUCCESS;
}

epm_Result epm_Render(void) {
#ifdef TWO_FRAMEBUFFER
    g_scr = (g_scr == __scrA) ? __scrB : __scrA;
    g_scr_prev = (g_scr == __scrA) ? __scrB : __scrA;
    
    g_scr_pixels = g_scr->pixels;
    g_scr_w = g_scr->w;
    g_scr_h = g_scr->h;
#endif
    
    zgl_ZeroEntire(g_scr);
   
    draw_WindowTree(root_node);
    
    if (show_textoverlay) {
        epm_DrawTextOverlay();
    }

#ifdef TWO_FRAMEBUFFER
    /* Do something with the combination of current and previous frame. */
    for (size_t i_pixel = 0; i_pixel < g_scr->w*g_scr->h; i_pixel++) {
        zgl_Color
            color1 = g_scr->pixels[i_pixel],
            color2 = g_scr_prev->pixels[i_pixel];

        video_out->pixels[i_pixel] =
            ((0xFF)-(((ABS((int32_t)r_of(color1) - (int32_t)r_of(color2)))>>1)))<<16 |
            ((0xFF)-(((ABS((int32_t)g_of(color1) - (int32_t)g_of(color2)))>>1)))<<8  |
            ((0xFF)-(((ABS((int32_t)b_of(color1) - (int32_t)b_of(color2)))>>1)))<<0;
    }
    
    /* TEMP */
    //memcpy(video_out->pixels, g_scr->pixels, g_scr->w*g_scr->h*sizeof(zgl_Color));
#endif
    
    zgl_VideoUpdate();

    return EPM_CONTINUE;
}
