#include "src/draw/window/window_types.h"
#include "src/draw/window/window.h"
#include "src/draw/default_layout.h"
#include "src/draw/text.h"
#include "src/draw/viewport/viewport.h"
#include "src/system/dir.h"

#define GRAY_BODY 56
#define VERT_PAD  4
#define HORI_PAD  8

static void do_KeyPress_DropdownMenu(Window *win, zgl_KeyPressEvent *evt);
static void do_PointerMotion_DropdownMenu(Window *win, zgl_PointerMotionEvent *evt);
static void do_PointerPress_DropdownMenu(Window *win, zgl_PointerPressEvent *evt);
static void draw_DropdownMenu(Window *win);
static void do_OnFocusOut_DropdownMenu(Window *win);

extern void select_ddm(DropdownMenu *ddm, int sel);
extern void unselect_menu(void);
extern void select_menu(int sel, WindowNode *from);

/* Menu entry actions */
extern void close(void);
extern void world_menu(void);

DropdownMenu dropdownmenus[4] = {
    [0] = {
        .win = {
            .name = "File Menu",
            .rect = {0},
            .mrect = {0},
            .cursor = ZC_arrow,
            .data = &dropdownmenus[0],
            .focusable = true,
            .winfncs = {
                .draw = draw_DropdownMenu,
                .onPointerPress = do_PointerPress_DropdownMenu,
                .onPointerRelease = NULL,
                .onPointerMotion = do_PointerMotion_DropdownMenu,
                .onPointerEnter = NULL,
                .onPointerLeave = NULL,
                .onKeyPress = do_KeyPress_DropdownMenu,
                .onFocusOut = do_OnFocusOut_DropdownMenu,
            },
        },
        .menubar = &menubar,
        .node = {&dropdownmenus[0].win},
        .bg = 0x383838,
        .select_bg = 0x484848,
        .num_menus = 4,
        .menu_labels = {[0]="New...", [1]="Save...", [2]="Load...", [3]="Quit"},
        .actions = {[0] = NULL, [1] = NULL, [2] = world_menu, [3] = close},
        .selected_menu = -1,
    },

    [1] = {
        .win = {
            .name = "Edit Menu",
            .rect = {0},
            .mrect = {0},
            .cursor = ZC_arrow,
            .data = &dropdownmenus[1],
            .focusable = true,
            .winfncs = {
                .draw = draw_DropdownMenu,
                .onPointerPress = do_PointerPress_DropdownMenu,
                .onPointerRelease = NULL,
                .onPointerMotion = do_PointerMotion_DropdownMenu,
                .onPointerEnter = NULL,
                .onPointerLeave = NULL,
                .onKeyPress = do_KeyPress_DropdownMenu,
                .onFocusOut = do_OnFocusOut_DropdownMenu,
            },
        },
        .menubar = &menubar,
        .node = {&dropdownmenus[1].win},
        .bg = 0x383838,
        .select_bg = 0x484848,
        .num_menus = 2,
        .menu_labels = {[0]="Copy", [1]="Paste"},
        .actions = {NULL},
        .selected_menu = -1,
    },

    [2] = {
        .win = {
            .name = "View Menu",
            .rect = {0},
            .mrect = {0},
            .cursor = ZC_arrow,
            .data = &dropdownmenus[2],
            .focusable = true,
            .winfncs = {
                .draw = draw_DropdownMenu,
                .onPointerPress = do_PointerPress_DropdownMenu,
                .onPointerRelease = NULL,
                .onPointerMotion = do_PointerMotion_DropdownMenu,
                .onPointerEnter = NULL,
                .onPointerLeave = NULL,
                .onKeyPress = do_KeyPress_DropdownMenu,
                .onFocusOut = do_OnFocusOut_DropdownMenu,
            },
        },
        .menubar = &menubar,
        .node = {&dropdownmenus[2].win},
        .bg = 0x383838,
        .select_bg = 0x484848,
        .num_menus = 3,
        .menu_labels = {[0]="3D", [1]="Wireframe", [2]="Lighting"},
        .actions = {NULL},
        .selected_menu = -1,
    },
    
    [3] = {
        .win = {
            .name = "Options Menu",
            .rect = {0},
            .mrect = {0},
            .cursor = ZC_arrow,
            .data = &dropdownmenus[3],
            .focusable = true,
            .winfncs = {
                .draw = draw_DropdownMenu,
                .onPointerPress = do_PointerPress_DropdownMenu,
                .onPointerRelease = NULL,
                .onPointerMotion = do_PointerMotion_DropdownMenu,
                .onPointerEnter = NULL,
                .onPointerLeave = NULL,
                .onKeyPress = do_KeyPress_DropdownMenu,
                .onFocusOut = do_OnFocusOut_DropdownMenu,
            },
        },
        .menubar = &menubar,
        .node = {&dropdownmenus[3].win},
        .bg = 0x383838,
        .select_bg = 0x484848,
        .num_menus = 1,
        .menu_labels = {[0]="Nothing"},
        .actions = {NULL},
        .selected_menu = -1,
    },
};

WindowNode *const dropdownmenu_nodes[4] = {
    &dropdownmenus[0].node,
    &dropdownmenus[1].node,
    &dropdownmenus[2].node,
    &dropdownmenus[3].node,
};

void close(void) {
    zgl_PushEvent(&(zgl_Event){.type = EC_CloseRequest});
}

extern void update_FileSelect(zgl_DirListing *dl);
void world_menu(void) {
    zgl_DirListing dl;
    zgl_GetDirListing(&dl, DIR_WORLD);
    // TODO: How best to communicate this directory listing to the FileSelect interface?
    update_FileSelect(&dl);
    epm_SetVPInterface(VP_TL, VPI_FILESELECT);
}

void init_dropdownmenu(void) {
    for (int i_menu = 0; i_menu < 4; i_menu++) {
        int max_width = 0;
        int tmp = 0;

        for (int i_label = 0; i_label < dropdownmenus[i_menu].num_menus; i_label++) {
            tmp = 12*(int)strlen(dropdownmenus[i_menu].menu_labels[i_label]);
            if (max_width < tmp) max_width = tmp;
        }

        tmp = 28 + 1;
        
        for (int i_label = 0; i_label < dropdownmenus[i_menu].num_menus; i_label++) {
            dropdownmenus[i_menu].menu_ys[i_label] = tmp;
            tmp += VERT_PAD + 18 + VERT_PAD;
        }

        dropdownmenus[i_menu].menu_ys[dropdownmenus[i_menu].num_menus] = tmp;
        dropdownmenus[i_menu].win.rect.w = HORI_PAD + max_width + HORI_PAD;
        dropdownmenus[i_menu].win.rect.h = (VERT_PAD+18+VERT_PAD)*dropdownmenus[i_menu].num_menus + 1 + 1;
        dropdownmenus[i_menu].win.mrect.w = fixify(dropdownmenus[i_menu].win.rect.w);
        dropdownmenus[i_menu].win.mrect.h = fixify(dropdownmenus[i_menu].win.rect.h);
    }
}

static void do_PointerPress_DropdownMenu(Window *win, zgl_PointerPressEvent *evt) {
    DropdownMenu *ddm = (DropdownMenu *)win->data;

    int y = evt->y;
    int sel = -1;
    
    for (int i = 0; i < ddm->num_menus + 1; i++) {
        if (y < ddm->menu_ys[i+1]) {
            sel = i;
            break;
        }
    }

    select_ddm(ddm, sel);

    if (ddm->selected_menu != -1 &&
        ddm->actions[ddm->selected_menu]) {
        ddm->actions[ddm->selected_menu]();
    }
}

static void draw_DropdownMenu(Window *win) {
    DropdownMenu *ddm = (DropdownMenu *)win->data;
    zgl_PixelRect rect = win->rect;

    zgl_FillRect(g_scr,
                 rect.x, rect.y,
                 rect.w, 1,
                 0x999999);
    zgl_FillRect(g_scr,
                 rect.x, rect.y+rect.h-1,
                 rect.w, 1,
                 0x999999);
    zgl_FillRect(g_scr,
                 rect.x, rect.y+1,
                 1, rect.h-2,
                 0x999999);
    zgl_FillRect(g_scr,
                 rect.x + rect.w - 1, rect.y + 1,
                 1, rect.h-2,
                 0x999999);
    
    for (int i = 0; i < ddm->num_menus; i++) {
        if (ddm->selected_menu == i) {
            zgl_FillRect(g_scr,
                         rect.x+1,
                         ddm->menu_ys[ddm->selected_menu],
                         rect.w-2,
                         VERT_PAD + 18 + VERT_PAD,
                         0x444444);
            draw_BMPFont_string(g_scr, NULL,
                                ddm->menu_labels[i],
                                rect.x + HORI_PAD,
                                ddm->menu_ys[i] + VERT_PAD,
                                FC_MONOGRAM2, 0xFFFFFF);
            continue;
        }
        zgl_FillRect(g_scr,
                     rect.x + 1,
                     ddm->menu_ys[i],
                     rect.w-2,
                     VERT_PAD + 18 + VERT_PAD,
                     0x282828);
        
        draw_BMPFont_string(g_scr, NULL, ddm->menu_labels[i],
                            rect.x + HORI_PAD, ddm->menu_ys[i] + VERT_PAD, FC_MONOGRAM2, 0xFFFFFF);
    }
}

static void do_PointerMotion_DropdownMenu(Window *win, zgl_PointerMotionEvent *evt) {
    DropdownMenu *ddm = (DropdownMenu *)win->data;
    int y = evt->y;

    int sel = -1;
    
    for (int i = 0; i < ddm->num_menus + 1; i++) {
        if (y < ddm->menu_ys[i+1]) {
            sel = i;
            break;
        }
    }

    select_ddm(ddm, sel);
}

static void do_OnFocusOut_DropdownMenu(Window *win) {
    (void)win;
    
    unselect_menu();
}

static void do_KeyPress_DropdownMenu(Window *win, zgl_KeyPressEvent *evt) {
    DropdownMenu *ddm = (DropdownMenu *)win->data;
    int i_menu = ddm->selected_menu;
    int i_menubar_menu = ddm->menubar->selected_menu;
    
    switch (evt->lk) {
    case LK_UP:
        select_ddm(ddm, i_menu - 1);
        break;
    case LK_LEFT:
        select_menu(i_menubar_menu - 1, &ddm->node);
        break;
    case LK_DOWN:
        select_ddm(ddm, i_menu + 1);
        break;
    case LK_RIGHT:
        select_menu(i_menubar_menu + 1, &ddm->node);
        break;
    case LK_RET:
        if (ddm->selected_menu != -1 &&
            ddm->actions[i_menu]) {
            ddm->actions[i_menu]();
        }
        break;
    default:
        break;
    }
}

extern void pass_input_focus(WindowNode *to, WindowNode *from);
void select_ddm(DropdownMenu *ddm, int sel) {
    sel = (sel + ddm->num_menus) % ddm->num_menus;

    if (sel == ddm->selected_menu) return; // nothing's changed
    
    ddm->selected_menu = sel;
}
