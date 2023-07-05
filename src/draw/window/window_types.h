#ifndef WINDOW_TYPES_H
#define WINDOW_TYPES_H

#include "src/draw/window/window.h"

#define MAX_MENUS     24
#define MAX_LABEL_LEN 64

typedef struct MenuBar MenuBar;
typedef struct DropdownMenu DropdownMenu;
typedef void (*DropdownMenuAction_fnc)(void);

struct DropdownMenu {
    Window win;
    WindowNode node;

    MenuBar *menubar;

    int scr_x;
    int scr_y;
    int width;
    zgl_Color bg;
    zgl_Color select_bg;
    int label_padding;
    int num_menus;
    char menu_labels[MAX_MENUS][MAX_LABEL_LEN];
    int menu_ys[MAX_MENUS];
    DropdownMenuAction_fnc actions[MAX_MENUS];
    int selected_menu;
};

extern DropdownMenu dropdownmenus[4];

struct MenuBar {
    Window win;
    WindowNode node;

    DropdownMenu *ddms[MAX_MENUS];
    
    int scr_x;
    int scr_y;
    zgl_Color bg;
    zgl_Color bottom_border;
    zgl_Color select_bg;
    int label_padding;
    int num_menus;
    char menu_labels[MAX_MENUS][MAX_LABEL_LEN];
    zgl_Pixit menu_widths[MAX_MENUS];
    zgl_Pixit menu_xs[MAX_MENUS];
    int selected_menu;
};

extern MenuBar menubar;

#endif /* WINDOW_TYPES_H */
