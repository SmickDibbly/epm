#include "src/misc/epm_includes.h"

#include "src/draw/text.h"
#include "src/draw/window/window.h"
#include "src/misc/stringlist.h"
#include "src/draw/viewport/viewport_internal.h"
#include "src/draw/textrect.h"

typedef struct LogView {
    Window *last_known_win;
    
    int vert_padding;
    int hori_padding;
    zgl_PixelRect usable_rect;

    BMPFontParameters bmpf;

    TextRect tr;
} LogView;

static LogView lv = {.vert_padding = 20, .hori_padding = 20};

static epm_Result epm_InitLogView(void) {
    extern StringList loglist;
    initialize_TextRect(&lv.tr, FC_IBMVGA, NULL, &loglist);
    
    get_font_parameters(FC_IBMVGA, &lv.bmpf);
    lv.vert_padding = 20;
    lv.hori_padding = 20;
    
    return EPM_SUCCESS;
}

static epm_Result epm_TermLogView(void) {
    
    return EPM_SUCCESS;
}

static void onMap_LogView(ViewportInterface *self, Viewport *p_VP) {
    Window *win = &p_VP->VPI_win;

    dibassert(self->i_VPI == VPI_LOGVIEW);

    if (&p_VP->VPI_win != lv.last_known_win) {
        lv.last_known_win = win;
        lv.usable_rect = (zgl_PixelRect)
            {.x = win->rect.x + lv.hori_padding,
             .y = win->rect.y + lv.vert_padding,
             .w = win->rect.w - lv.hori_padding - lv.hori_padding,
             .h = win->rect.h - lv.vert_padding - lv.vert_padding};

        resize_TextRect(&lv.tr, &lv.usable_rect);
    }
}

static void draw_LogView(Window *win) {
    (void)win;
    
    draw_TextRect(g_scr, &lv.tr);
}


#include "src/draw/viewport/viewport_internal.h"
ViewportInterface interface_LogView = {
    .i_VPI = VPI_LOGVIEW,
    .mapped_i_VP = VP_NONE,
    .windata = NULL,
    .onUnmap = NULL,
    .onMap = onMap_LogView,
    .init = epm_InitLogView,
    .term = epm_TermLogView,
    .winfncs = {
        .draw = draw_LogView,
        NULL
    },
    .name = "Log View"
};

