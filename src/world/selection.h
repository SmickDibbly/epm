#ifndef SELECTION_H
#define SELECTION_H

#include "src/misc/epm_includes.h"
#include "src/world/world.h"

#define STATIC_STORAGE_LIMIT 64

void select_all_brush_faces(void);

/*
typedef struct BrushSelectionNode BrushSelectionNode;
struct BrushSelectionNode {
    BrushSelectionNode *next;
    BrushSelectionNode *prev;

    Brush *brush;
};
typedef struct BrushSelection {
    WorldVec POR;
    size_t num_selected;
    BrushSelectionNode *head;
    BrushSelectionNode *tail;
    BrushSelectionNode nodes[STATIC_STORAGE_LIMIT];
} BrushSelection;

extern void del_selected_brush(BrushSelection *sel, Brush *brush);
extern void add_selected_brush(BrushSelection *sel, Brush *brush);
extern void toggle_selected_brush(BrushSelection *sel, Brush *brush);
extern void clear_brush_selection(BrushSelection *sel);

extern BrushSelection brushsel;
*/

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

typedef struct SelectionNode SelectionNode;
struct SelectionNode {
    SelectionNode *next;
    SelectionNode *prev;
    
    void *obj;
};
typedef struct Selection {
    bool (*fn_is_selected)(void *obj);
    void (*fn_select)(void *obj);
    void (*fn_unselect)(void *obj);
    size_t num_selected;
    SelectionNode *head;
    SelectionNode *tail;
    SelectionNode nodes[STATIC_STORAGE_LIMIT];
} Selection;

extern void *sel_del(Selection *sel, void *obj);
extern void sel_add(Selection *sel, void *obj);
extern void sel_toggle(Selection *sel, void *obj);
extern void sel_clear(Selection *sel);

extern void BrushSel_del(Brush *brush);
extern void BrushSel_add(Brush *brush);
extern void BrushSel_toggle(Brush *brush);
extern void BrushSel_clear(void);

extern void BrushPolySel_del(Brush *brush, uint32_t i_p);
extern void BrushPolySel_add(Brush *brush, uint32_t i_p);
extern void BrushPolySel_toggle(Brush *brush, uint32_t i_p);
extern void BrushPolySel_clear(void);

extern void BrushVertSel_del(Brush *brush, uint32_t i_v);
extern void BrushVertSel_add(Brush *brush, uint32_t i_v);
extern void BrushVertSel_toggle(Brush *brush, uint32_t i_v);
extern void BrushVertSel_clear(void);

extern Selection sel_brush;
extern Selection sel_brushpoly;
extern Selection sel_brushvert;

extern WorldVec brushsel_POR;

#endif /* SELECTION_H */
