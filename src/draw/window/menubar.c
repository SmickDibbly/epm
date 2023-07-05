#include "src/draw/draw.h"
#include "src/draw/default_layout.h"
#include "src/draw/text.h"
#include "src/draw/window/window.h"
#include "src/draw/window/window_registry.h"
#include "src/draw/window/window_types.h"
#include "src/input/input.h"

#define MAX_MENUS     24
#define MAX_LABEL_LEN 64

#define GRAY_TOP    72
#define GRAY_BODY   56
#define GRAY_BOTTOM 40

static void do_OnFocusOut_MenuBar(Window *win);
static void do_PointerPress_MenuBar(Window *win, zgl_PointerPressEvent *evt);
static void draw_MenuBar(Window *win);
static void do_KeyPress_MenuBar(Window *win, zgl_KeyPressEvent *evt);
static void do_PointerMotion_MenuBar(Window *win, zgl_PointerMotionEvent *evt);
static void do_OnFocusIn_MenuBar(Window *win);

extern void unselect_menu(void);
extern void select_menu(int sel);

bool active = false;

MenuBar menubar;

WindowNode *const menubar_node = &menubar.node;

void init_menubar(void) {
    MenuBar *mb = &menubar;
       
    strcpy(mb->win.name, "Menubar");
    mb->win.rect.x = menubar_x;
    mb->win.rect.y = menubar_y;
    mb->win.rect.w = menubar_w;
    mb->win.rect.h = menubar_h;
    mb->win.mrect.x = menubar_x<<16;
    mb->win.mrect.y = menubar_y<<16;
    mb->win.mrect.w = menubar_w<<16;
    mb->win.mrect.h = menubar_h<<16;
    mb->win.cursor = ZC_arrow;
    mb->win.focusable = true,
    mb->win.winfncs.draw = draw_MenuBar;
    mb->win.winfncs.onPointerPress = do_PointerPress_MenuBar;
    mb->win.winfncs.onPointerRelease = NULL;
    mb->win.winfncs.onPointerMotion = do_PointerMotion_MenuBar;
    mb->win.winfncs.onPointerEnter = NULL;
    mb->win.winfncs.onPointerLeave = NULL;
    mb->win.winfncs.onFocusOut = do_OnFocusOut_MenuBar;
    mb->win.winfncs.onFocusIn = do_OnFocusIn_MenuBar;
    mb->win.winfncs.onKeyPress = do_KeyPress_MenuBar;
    mb->ddms[0] = dropdownmenus;
    mb->ddms[1] = dropdownmenus+1;
    mb->ddms[2] = dropdownmenus+2;
    mb->ddms[3] = dropdownmenus+3;
    mb->node.win = &menubar.win;
    mb->bg = 0x383838;
    mb->bottom_border = 0x2a2a2a;
    mb->select_bg = 0x484848;
    mb->label_padding = 8;
    mb->num_menus = 4;
    strcpy(mb->menu_labels[0], "File");
    strcpy(mb->menu_labels[1], "Edit");
    strcpy(mb->menu_labels[2], "View");
    strcpy(mb->menu_labels[3], "Options");
    mb->selected_menu = -1;
    
    int tmp = 4;
    for (int i = 0; i < menubar.num_menus; i++) {
        menubar.menu_widths[i] = menubar.label_padding+12*(int)strlen(menubar.menu_labels[i])+menubar.label_padding; // for each letter, 10 + space of 2
        menubar.menu_xs[i] = tmp;
        tmp += menubar.menu_widths[i];

        dropdownmenu_nodes[i]->win->rect.x = menubar.menu_xs[i];
        dropdownmenu_nodes[i]->win->rect.y = 28;
        dropdownmenu_nodes[i]->win->mrect.x = fixify(menubar.menu_xs[i]);
        dropdownmenu_nodes[i]->win->mrect.y = fixify(28);
    }
}


static void do_PointerPress_MenuBar(Window *win, zgl_PointerPressEvent *evt) {
    (void)win;
    active = true;
    int x = evt->x;
    int sel = -1;
    for (int i = 0; i < menubar.num_menus; i++) {
        if (x < menubar.menu_xs[i+1]) {
            sel = i;
            break;
        }
    }

    select_menu(sel);
}

static void do_PointerMotion_MenuBar(Window *win, zgl_PointerMotionEvent *evt) {
    (void)win;

    if (!active) return;
    
    int x = evt->x;
    int sel = -1;
    for (int i = 0; i < menubar.num_menus; i++) {
        if (x < menubar.menu_xs[i+1]) {
            sel = i;
            break;
        }
    }

    select_menu(sel);
}

static void draw_MenuBar(Window *win) {
    zgl_PixelRect rect = win->rect;
    zgl_FillRect(g_scr,
                 rect.x, rect.y,
                 rect.w, 1,
                 0x383838);

    zgl_FillRect(g_scr,
                 rect.x, rect.y + rect.h - 1,
                 rect.w, 1,
                 menubar.bottom_border);

    zgl_FillRect(g_scr,
                 rect.x, rect.y + 1,
                 rect.w, rect.h - 2,
                 menubar.bg);

    int i_menu = menubar.selected_menu;
    if (i_menu != -1) {
        zgl_Pixit x = menubar.menu_xs[i_menu];
        zgl_Pixit w = menubar.menu_widths[i_menu];
	
        zgl_FillRect(g_scr,
                     x, rect.y + 1,
                     w, rect.h - 2,
                     menubar.select_bg);

        zgl_FillRect(g_scr,
                     x, rect.y + rect.h - 2,
                     w, 2,
                     0x884444);
    }

    for (int i = 0; i < menubar.num_menus; i++) {
        draw_BMPFont_string(g_scr, NULL, menubar.menu_labels[i], menubar.menu_xs[i]+menubar.label_padding, 7, FC_MONOGRAM2, 0xDDDDDD);
    }
}

static void do_OnFocusIn_MenuBar(Window *win) {
    (void)win;
}

static void do_OnFocusOut_MenuBar(Window *win) {
    (void)win;
}

extern void select_ddm(DropdownMenu *ddm, int sel);
static void do_KeyPress_MenuBar(Window *win, zgl_KeyPressEvent *evt) {
    (void)win;
    
    switch (evt->lk) {
    case LK_UP:
        epm_SetInputFocus(&menubar.ddms[menubar.selected_menu]->node);
        select_ddm(menubar.ddms[menubar.selected_menu], menubar.ddms[menubar.selected_menu]->selected_menu - 1);
        break;
    case LK_LEFT:
        select_menu(menubar.selected_menu - 1);
        break;
    case LK_DOWN:
        epm_SetInputFocus(&menubar.ddms[menubar.selected_menu]->node);
        select_ddm(menubar.ddms[menubar.selected_menu], menubar.ddms[menubar.selected_menu]->selected_menu + 1);
        break;
    case LK_RIGHT:
        select_menu(menubar.selected_menu + 1);
        break;
    default:
        break;
    }
}

void unselect_menu(void) {
    if (menubar.selected_menu != -1) {
        unlink_WindowNode(dropdownmenu_nodes[menubar.selected_menu]);
    }
    active = false;
    menubar.selected_menu = -1;
}

void select_menu(int sel) {
    sel = (sel + menubar.num_menus) % menubar.num_menus;

    if (sel == menubar.selected_menu) return; // nothing's changed
    
    if (menubar.selected_menu != -1) {
        unlink_WindowNode(dropdownmenu_nodes[menubar.selected_menu]);
    }

    if (sel != -1) {
        link_WindowNode(dropdownmenu_nodes[sel], root_node); // TODO: Link it to menubar. As of 2023-03-21 the window_below function assumes child windows are contained in the rectangle of the parent. This assumption is not true for dropdown menus of the menubar.
    }

    epm_SetInputFocus(dropdownmenu_nodes[sel]);
    menubar.selected_menu = sel;
}
