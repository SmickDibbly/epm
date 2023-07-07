#include "src/misc/epm_includes.h"

#include "src/draw/text.h"
#include "src/draw/window/window.h"
#include "src/draw/viewport/viewport_internal.h"
#include "src/system/state.h"
#include "src/world/world.h"
#include "src/entity/editor_camera.h"

#define MAX_TBX_STR_LEN 127
typedef struct Text_Box Text_Box;
struct Text_Box {
    zgl_PixelRect rect;
    char str[MAX_TBX_STR_LEN+1];
};


// timing
static Text_Box TBX_frame;
static Text_Box TBX_tic;
static Text_Box TBX_global_elapsed;
static Text_Box TBX_local_avg_tpf;
static Text_Box TBX_local_avg_fps;

// memory
static Text_Box TBX_mem;

// input
static Text_Box TBX_pointer;
static Text_Box TBX_pointer_rel;
static Text_Box TBX_keypress;
static Text_Box TBX_win_below;
static Text_Box TBX_active_viewport;

// world
static Text_Box TBX_player_pos;
static Text_Box TBX_player_angle_h;
static Text_Box TBX_player_angle_v;


static Text_Box *TBX[] = {
    &TBX_frame,
    &TBX_tic,
    &TBX_global_elapsed,
    &TBX_local_avg_tpf,
    &TBX_local_avg_fps,
    NULL,
    &TBX_mem,
    NULL,
    &TBX_pointer,
    &TBX_pointer_rel,
    &TBX_keypress,
    &TBX_win_below,
    &TBX_active_viewport,
    NULL,
    &TBX_player_pos,
    &TBX_player_angle_h,
    &TBX_player_angle_v,
    NULL,
};

static uint32_t const NUM_TBX = sizeof(TBX)/sizeof(TBX[0]);

static BMPFontCode g_font = FC_IBMVGA;

epm_Result set_textoverlay_font(BMPFontCode font) {
    g_font = font;
    
    BMPFontParameters params;
    get_font_parameters(font, &params);
    int y = 4;
    
    for (size_t i_TBX = 0; i_TBX < NUM_TBX; i_TBX++) {
        if (TBX[i_TBX] == NULL) {
            y += (params.height + params.interline + 2)/2;
            continue;
        }

        TBX[i_TBX]->rect.x = 4;
        TBX[i_TBX]->rect.y = y;
        y += params.height + params.interline + 2;
    }
    
    return EPM_SUCCESS;
}

static epm_Result epm_InitMonitor(void) {
    set_textoverlay_font(FC_IBMVGA);
    
    return EPM_SUCCESS;
}


Fix32 Ang18o_degrees(Ang18 ang) {
    return (Fix32)(360*(((uint64_t)ang)<<16)/(uint64_t)ANG18_2PI);
}

Fix32 Ang18o_degrees_signed(Ang18 ang) {
    if (ang > ANG18_PI) {
        return (Fix32)(-360*(((uint64_t)(ANG18_2PI-ang))<<16)/(uint64_t)ANG18_2PI);
    }
    else {
        return (Fix32)(360*(((uint64_t)ang)<<16)/(uint64_t)ANG18_2PI);
    }   
}


static void draw_Monitor(Window *win) {
    snprintf(TBX_frame.str, MAX_TBX_STR_LEN,
             "Frame: %lu",
             state.loop.frame);
    snprintf(TBX_tic.str, MAX_TBX_STR_LEN,
             "  Tic: %lu",
             state.loop.tic);
    snprintf(TBX_global_elapsed.str, MAX_TBX_STR_LEN,
             "Global Elapsed: %s seconds",
             fmt_fix_d((state.timing.global_elapsed<<16)/1000000000ULL, 16, 4));
    snprintf(TBX_local_avg_tpf.str, MAX_TBX_STR_LEN,
             " Local Avg TPF: %s (ignore)",
             fmt_fix_d(state.timing.local_avg_tpf, 16, 4));
    snprintf(TBX_local_avg_fps.str, MAX_TBX_STR_LEN,
             " Local Avg FPS: %s",
             fmt_fix_d(state.timing.local_avg_fps, 16, 4));


    snprintf(TBX_mem.str, MAX_TBX_STR_LEN,
             "Memory: %li KiB",
             state.sys.mem>>10);
    
    snprintf(TBX_pointer.str, MAX_TBX_STR_LEN,
             "Pointer: (%i, %i)",
             state.input.pointer_x,
             state.input.pointer_y);
    snprintf(TBX_pointer_rel.str, MAX_TBX_STR_LEN,
             "Rel. Pointer: (%i, %i)",
             state.input.pointer_rel_x,
             state.input.pointer_rel_y);
    snprintf(TBX_keypress.str, MAX_TBX_STR_LEN,
             "Last Press: %s",
             LK_strs[state.input.last_press]);
    snprintf(TBX_win_below.str, MAX_TBX_STR_LEN,
             "Window Below: %s",
             state.input.win_below_name);
    snprintf(TBX_active_viewport.str, MAX_TBX_STR_LEN,
             "Active Viewport: %s",
             active_viewport ? active_viewport->wrap_win.name : "(null)");
    
    snprintf(TBX_player_pos.str, MAX_TBX_STR_LEN,
             "Camera Pos: %s",
             fmt_Fix32Vec(cam.pos));
    snprintf(TBX_player_angle_h.str, MAX_TBX_STR_LEN,
             "Camera Hori. Angle: %s deg",
             fmt_fix_d(Ang18o_degrees(cam.view_angle_h), 16, 4));
    snprintf(TBX_player_angle_v.str, MAX_TBX_STR_LEN,
             "Camera Vert. Angle: %s deg",
             fmt_fix_d(Ang18o_degrees_signed(cam.view_angle_v), 16, 4));
    
    for (size_t i_TBX = 0; i_TBX < NUM_TBX; i_TBX++) {
        if (TBX[i_TBX]) {
            draw_BMPFont_string(g_scr, NULL, TBX[i_TBX]->str, win->rect.x + TBX[i_TBX]->rect.x, win->rect.y + TBX[i_TBX]->rect.y, g_font, 0xFFFFFF);    
        }
        
    }
}

static epm_Result epm_TermMonitor(void) {
    
    return EPM_SUCCESS;
}

static void onMap_Monitor(ViewportInterface *self, Viewport *p_VP) {

}


#include "src/draw/viewport/viewport_internal.h"
ViewportInterface interface_Monitor = {
    .i_VPI = VPI_MONITOR,
    .mapped_i_VP = VP_NONE,
    .windata = NULL,
    .onUnmap = NULL,
    .onMap = onMap_Monitor,
    .init = epm_InitMonitor,
    .term = epm_TermMonitor,
    .winfncs = {
        .draw = draw_Monitor,
        NULL
    },
    .name = "Monitor"
};

