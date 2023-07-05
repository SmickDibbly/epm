#ifndef SELECTION_H
#define SELECTION_H

#include "src/misc/epm_includes.h"
#include "src/world/world.h"

#define STATIC_STORAGE_LIMIT 64

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/*
typedef struct FaceSelectionNode FaceSelectionNode;
struct FaceSelectionNode {
    FaceSelectionNode *next;
    FaceSelectionNode *prev;
    
    BrushQuadFace *face;
};
typedef struct FaceSelection {
    size_t num_selected;
    FaceSelectionNode *head;
    FaceSelectionNode *tail;
    FaceSelectionNode nodes[STATIC_STORAGE_LIMIT];
} FaceSelection;

extern void del_selected_face(FaceSelection *sel, BrushQuadFace *face);
extern void add_selected_face(FaceSelection *sel, BrushQuadFace *face);
extern void toggle_selected_face(FaceSelection *sel, BrushQuadFace *face);
extern void clear_face_selection(FaceSelection *sel);

extern FaceSelection facesel;
*/

void select_all_brush_faces(void);

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

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


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

typedef struct VertexSelectionNode VertexSelectionNode;
struct VertexSelectionNode {
    VertexSelectionNode *next;
    VertexSelectionNode *prev;

    WorldVec *vertex;
};
typedef struct VertexSelection {
    WorldVec POR;
    size_t num_selected;
    VertexSelectionNode *head;
    VertexSelectionNode *tail;
    VertexSelectionNode nodes[STATIC_STORAGE_LIMIT];
} VertexSelection;

extern void del_selected_vertex(VertexSelection *sel, WorldVec *vertex);
extern void add_selected_vertex(VertexSelection *sel, WorldVec *vertex);
extern void toggle_selected_vertex(VertexSelection *sel, WorldVec *vertex);
extern void clear_vertex_selection(VertexSelection *sel);

extern VertexSelection vertexsel;

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

typedef struct SelectionNode SelectionNode;
struct SelectionNode {
    SelectionNode *next;
    SelectionNode *prev;
    
    void *object;
};
typedef struct Selection {
    bool (*fn_is_selected)(void *object);
    void (*fn_select)(void *object);
    void (*fn_unselect)(void *object);
    size_t num_selected;
    SelectionNode *head;
    SelectionNode *tail;
    SelectionNode nodes[STATIC_STORAGE_LIMIT];
} Selection;

extern void sel_del(Selection *sel, void *object);
extern void sel_add(Selection *sel, void *object);
extern void sel_toggle(Selection *sel, void *object);
extern void sel_clear(Selection *sel);


extern Selection sel_face;
#define sel_toggle_face(P_FACE)                         \
    (sel_toggle(&sel_face, (BrushQuadFace *)(P_FACE)))

/*
extern Selection sel_brush;
#define toggle_selected_brush(P_BRUSH)                          \
    (toggle_selected_object(&sel_brush, (Brush *)(P_BRUSH)))

extern Selection sel_vertex;
#define toggle_selected_vertex(P_VERTEX)                            \
    (toggle_selected_object(&sel_vertex, (WorldVec *)(P_VERTEX)))
*/

#endif /* SELECTION_H */
