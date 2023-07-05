#include "src/misc/epm_includes.h"
#include "src/draw/window/window.h"
#include "src/draw/text.h"
#include "src/system/dir.h"
#include "src/draw/draw.h"

#define MAX_VISIBLE_ENTRIES 8

typedef struct FileSelect {
    int i_entry; // selected entry; -1 if no selection

    int vert_padding;
    int hori_padding;
    
    size_t num_entries;
    char *entries[MAX_VISIBLE_ENTRIES];
    int entry_max_y[MAX_VISIBLE_ENTRIES];
} FileSelect;

static FileSelect fs = {0};

void update_FileSelect(zgl_DirListing *dl) {
    fs.num_entries = dl->num_entries;
    fs.i_entry = -1;
    fs.vert_padding = 20;
    fs.hori_padding = 20;
    int max_y = 20;
    
    for (size_t i_entry = 0; i_entry < dl->num_entries; i_entry++) {
        fs.entries[i_entry] = dl->entries[i_entry].name;

        fs.entry_max_y[i_entry] = max_y;
        max_y += 18 + 2;
    }
}

static void draw_FileSelect(Window *win) {
    int x = win->rect.x + fs.hori_padding, y = win->rect.y + fs.vert_padding;

    for (int i_entry = 0; i_entry < (int)fs.num_entries; i_entry++) {
        if (i_entry == fs.i_entry) {
            draw_BMPFont_string(g_scr, &win->rect, "*", x-16, y + fs.entry_max_y[i_entry]-20, FC_MONOGRAM2, 0xFFFFFF);
        }
        draw_BMPFont_string(g_scr, &win->rect, fs.entries[i_entry], x, y + fs.entry_max_y[i_entry]-20, FC_MONOGRAM2, 0xFFFFFF);
    }
}

static void do_PointerPress_FileSelect(Window *win, zgl_PointerPressEvent *evt) {
    (void)win, (void)evt;
}

static void do_PointerMotion_FileSelect(Window *win, zgl_PointerMotionEvent *evt) {
    int y = evt->y;
    int origin_y = win->rect.y+fs.vert_padding;
    
    if (y < origin_y + fs.entry_max_y[0]) {
        fs.i_entry = 0;
    }
    else if (y < origin_y + fs.entry_max_y[1]) {
        fs.i_entry = 1;
    }
    else if (y < origin_y + fs.entry_max_y[2]) {
        fs.i_entry = 2;
    }
    else if (y < origin_y + fs.entry_max_y[3]) {
        fs.i_entry = 3;
    }
    else {
        fs.i_entry = -1;
    }
}



#include "src/draw/viewport/viewport_internal.h"
ViewportInterface interface_FileSelect = {
    .i_VPI = VPI_FILESELECT,
    .mapped_i_VP = VP_NONE,
    .windata = NULL,
    .onUnmap = NULL,
    .onMap = NULL,
    .init = NULL,
    .term = NULL,
    .winfncs = {
        .draw = draw_FileSelect,
        .onPointerPress = do_PointerPress_FileSelect,
        .onPointerMotion = do_PointerMotion_FileSelect,
        NULL
    },
    .name = "File Select"
};
