#include <stdio.h>
#include <inttypes.h>

#include "zigil/zigil.h"

#include "src/misc/epm_includes.h"
#include "src/system/state.h"

#include "src/draw/text.h"

// weird includes
#include "src/draw/viewport/viewport_internal.h"
#include "src/world/world.h"
#include "src/entity/editor_camera.h"

#define MAX_TBX_STR_LEN 127
typedef struct Text_Box Text_Box;
struct Text_Box {
    zgl_PixelRect rect;
    char str[MAX_TBX_STR_LEN+1];
};

#define NUM_TBX 15
static Text_Box TBX[NUM_TBX];

// timing
static Text_Box *TBX_frame   = &TBX[0];
static Text_Box *TBX_tic     = &TBX[1];
static Text_Box *TBX_global_avg_tpf = &TBX[2];
static Text_Box *TBX_local_avg_tpf = &TBX[3];
static Text_Box *TBX_global_avg_fps = &TBX[4];
static Text_Box *TBX_local_avg_fps = &TBX[5];

// memory
static Text_Box *TBX_mem        = &TBX[6];

// input
static Text_Box *TBX_pointer  = &TBX[7];
static Text_Box *TBX_pointer_rel = &TBX[8];
static Text_Box *TBX_keypress = &TBX[9];
static Text_Box *TBX_win_below = &TBX[10];
static Text_Box *TBX_active_viewport = &TBX[11];

// world
static Text_Box *TBX_player_pos = &TBX[12];
static Text_Box *TBX_player_angle_h = &TBX[13];
static Text_Box *TBX_player_angle_v = &TBX[14];

static BMPFontCode g_font = FC_IBMVGA;

epm_Result set_textoverlay_font(BMPFontCode font) {
    g_font = font;
    
    BMPFontParameters params;
    get_font_parameters(font, &params);

    int y = 4;
    
    for (int i = 0; i <= 5; i++) {
        TBX[i].rect.x = 4;
        TBX[i].rect.y = y;
        y += params.height + params.interline + 2;
    }
    y += (params.height + params.interline + 2)/2;

    TBX[6].rect.x = 4;
    TBX[6].rect.y = y;
    y += params.height + params.interline + 2;

    y += (params.height + params.interline + 2)/2;

    TBX[8].rect.x = 4;
    TBX[8].rect.y = y;
    y += params.height + params.interline + 2;

    TBX[9].rect.x = 4;
    TBX[9].rect.y = y;
    y += params.height + params.interline + 2;

    TBX[10].rect.x = 4;
    TBX[10].rect.y = y;
    y += params.height + params.interline + 2;

    TBX[11].rect.x = 4;
    TBX[11].rect.y = y;
    y += params.height + params.interline + 2;

    y += (params.height + params.interline + 2)/2;

    TBX[12].rect.x = 4;
    TBX[12].rect.y = y;
    y += params.height + params.interline + 2;

    TBX[13].rect.x = 4;
    TBX[13].rect.y = y;
    y += params.height + params.interline + 2;

    TBX[14].rect.x = 4;
    TBX[14].rect.y = y;
    y += params.height + params.interline + 2;
    
    return EPM_SUCCESS;    
}

epm_Result epm_InitTextOverlay(void) {
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

 
epm_Result epm_DrawTextOverlay(void) {
    snprintf(TBX_frame->str, MAX_TBX_STR_LEN,
             "Frame: %lu",
             state.loop.frame);
    snprintf(TBX_tic->str, MAX_TBX_STR_LEN,
             "  Tic: %lu",
             state.loop.tic);
    snprintf(TBX_global_avg_tpf->str, MAX_TBX_STR_LEN,
             "Global Avg TPF: %s",
             fmt_fix_d(state.timing.global_avg_tpf, 16, 4));
    snprintf(TBX_local_avg_tpf->str, MAX_TBX_STR_LEN,
             " Local Avg TPF: %s (ignore)",
             fmt_fix_d(state.timing.local_avg_tpf, 16, 4));
    snprintf(TBX_global_avg_fps->str, MAX_TBX_STR_LEN,
             "Global Avg FPS: %s",
             fmt_fix_d(state.timing.global_avg_fps, 16, 4));
    snprintf(TBX_local_avg_fps->str, MAX_TBX_STR_LEN,
             " Local Avg FPS: %s",
             fmt_fix_d(state.timing.local_avg_fps, 16, 4));


    snprintf(TBX_mem->str, MAX_TBX_STR_LEN,
             "Memory: %li KiB",
             state.sys.mem>>10);
    
    snprintf(TBX_pointer->str, MAX_TBX_STR_LEN,
             "Pointer: (%i, %i)",
             state.input.pointer_x,
             state.input.pointer_y);
    snprintf(TBX_pointer_rel->str, MAX_TBX_STR_LEN,
             "Rel. Pointer: (%i, %i)",
             state.input.pointer_rel_x,
             state.input.pointer_rel_y);
    snprintf(TBX_keypress->str, MAX_TBX_STR_LEN,
             "Last Press: %s",
             LK_strs[state.input.last_press]);
    snprintf(TBX_win_below->str, MAX_TBX_STR_LEN,
             "Window Below: %s",
             state.input.win_below_name);
    snprintf(TBX_active_viewport->str, MAX_TBX_STR_LEN,
             "Active Viewport: %s",
             active_viewport ? active_viewport->wrap_win.name : "(null)");
    
    snprintf(TBX_player_pos->str, MAX_TBX_STR_LEN,
             "Camera Pos: %s",
             fmt_Fix32Vec(cam.pos));
    snprintf(TBX_player_angle_h->str, MAX_TBX_STR_LEN,
             "Camera Hori. Angle: %s deg",
             fmt_fix_d(Ang18o_degrees(cam.view_angle_h), 16, 4));
    snprintf(TBX_player_angle_v->str, MAX_TBX_STR_LEN,
             "Camera Vert. Angle: %s deg",
             fmt_fix_d(Ang18o_degrees_signed(cam.view_angle_v), 16, 4));
    
    for (size_t i_tbx = 0; i_tbx < NUM_TBX; i_tbx++) {
        draw_BMPFont_string(g_scr, NULL, TBX[i_tbx].str, TBX[i_tbx].rect.x, TBX[i_tbx].rect.y, g_font, 0xFFFFFF);
    }

    /*
    char tmp_buf[256] = "00000000";
    if (modifier_states & ZGL_CTRL_MASK)          tmp_buf[0] = '1';
    if (modifier_states & ZGL_ALT_MASK)           tmp_buf[1] = '1';
    if (modifier_states & ZGL_SHIFT_MASK)         tmp_buf[2] = '1';
    if (mouse_states & ZGL_MOUSELEFT_MASK)        tmp_buf[3] = '1';
    if (mouse_states & ZGL_MOUSEMID_MASK)         tmp_buf[4] = '1';
    if (mouse_states & ZGL_MOUSERIGHT_MASK)       tmp_buf[5] = '1';
    if (mouse_states & ZGL_MOUSEWHEELUP_MASK)     tmp_buf[6] = '1';
    if (mouse_states & ZGL_MOUSEWHEELDOWN_MASK)   tmp_buf[7] = '1';
    draw_BMPFont_string(scr_p, NULL, tmp_buf, 600, 400, g_font, 0xFFFFFF);
    */
    
    return 0;
}

