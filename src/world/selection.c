#include "src/misc/epm_includes.h"
#include "src/world/world.h"

#include "src/world/selection.h"

/* -------------------------------------------------------------------------- */

// Face Selections

// The user may want to batch-perform operations on a set of faces of their
// choosing. The user should be permitted to add and remove faces from
// consideration at will, in no particular order. Thus a linked list structure
// will be used.

// If the selection is at most 64 faces, the list nodes are stored
// statically. TODO: When statically-stored and a selection of more than 64
// faces is requested, automatically switch to dynamically-stored. When
// dynamically-stored and a selection of 32 or fewer faces is requeseted,
// automatically switch to statically-stored. (the difference in threshold is to
// reduce the likelihood of frequent storage-type switching when the user
// rapidly alternates between above-threshold and below-threshold)

/*
FaceSelection facesel = {0, NULL, NULL, {{NULL, NULL, NULL}}};

void del_selected_face(FaceSelection *sel, BrushQuadFace *face) {
    if ( ! (face->flags & FACEFLAG_SELECTED)) return;

    FaceSelectionNode *node;
    for (size_t i = 0; i < STATIC_STORAGE_LIMIT; i++) {
        if (sel->nodes[i].face == face) {
            node = sel->nodes + i;
            face->flags &= ~FACEFLAG_SELECTED;
            node->face = NULL;
            sel->num_selected--;

            //unlink
            if (node == sel->head) {
                sel->head = node->next;
            }
            
            if (node == sel->tail) {
                sel->tail = node->prev;
            }

            if (node->prev) {
                node->prev->next = node->next;
            }

            if (node->next) {
                node->next->prev = node->prev;
            }

            return;
        }
    }

    epm_Log(LT_WARN, "The face submitted for deselection was not found in the set of selected faces. Data integrity in question.");
}

void add_selected_face(FaceSelection *sel, BrushQuadFace *face) {
    if (face->flags & FACEFLAG_SELECTED) return;
    
    if (sel->num_selected == 0) {
        dibassert(sel->nodes->face == NULL);
        
        sel->head = sel->nodes;
        sel->head->next = NULL;
        sel->head->prev = NULL;
        sel->head->face = face;

        face->flags |= FACEFLAG_SELECTED;
        
        sel->tail = sel->head;
        
        sel->num_selected = 1;
        return;
    }

    // find free slot
    FaceSelectionNode *node;
    for (size_t i = 0; i < STATIC_STORAGE_LIMIT; i++) {
        if (sel->nodes[i].face == NULL) {
            node = sel->nodes + i;
            node->next = NULL;
            node->prev = sel->tail;
            node->face = face;
            face->flags |= FACEFLAG_SELECTED;

            // since num_selected >= 1, tail is non-NULL
            dibassert(sel->tail);
            sel->tail->next = node;
            sel->tail = node;
            sel->num_selected++;
            return;
        }
    }

    epm_Log(LT_WARN, "No room to store another selected face!");
}

void toggle_selected_face(FaceSelection *sel, BrushQuadFace *face) {
    if (face->flags & FACEFLAG_SELECTED)
        del_selected_face(sel, face);
    else
        add_selected_face(sel, face);
}

void clear_face_selection(FaceSelection *sel) {
    for (FaceSelectionNode *node = sel->head; node; node = node->next) {
        node->face->flags &= ~FACEFLAG_SELECTED;
    }

    memset(sel, 0, sizeof(*sel));
}
*/

/* -------------------------------------------------------------------------- */

// Brush Selections

BrushSelection brushsel = {{{0,0,0}}, 0, NULL, NULL, {{NULL, NULL, NULL}}};

void del_selected_brush(BrushSelection *sel, Brush *brush) {
    if ( ! (brush->flags & BRUSHFLAG_SELECTED)) return;

    BrushSelectionNode *node;
    for (size_t i = 0; i < STATIC_STORAGE_LIMIT; i++) {
        if (sel->nodes[i].brush == brush) {
            node = sel->nodes + i;
            brush->flags &= ~BRUSHFLAG_SELECTED;
            node->brush = NULL;
            sel->num_selected--;

            //unlink
            if (node == sel->head) {
                sel->head = node->next;
            }
            
            if (node == sel->tail) {
                sel->tail = node->prev;
            }

            if (node->prev) {
                node->prev->next = node->next;
            }

            if (node->next) {
                node->next->prev = node->prev;
            }

            return;
        }
    }

    epm_Log(LT_WARN, "The brush submitted for deselection was not found in the set of selected brushes. Data integrity in question.");
}


void add_selected_brush(BrushSelection *sel, Brush *brush) {
    if (sel->head && sel->head->brush == frame) {
        clear_brush_selection(&brushsel);
    }
    
    if (brush->flags & BRUSHFLAG_SELECTED) return;
    
    if (sel->num_selected == 0) {
        dibassert(sel->nodes->brush == NULL);
        
        sel->head = sel->nodes;
        sel->head->next = NULL;
        sel->head->prev = NULL;
        sel->head->brush = brush;
        
        brush->flags |= BRUSHFLAG_SELECTED;
        
        sel->tail = sel->head;
        
        sel->num_selected = 1;
        sel->POR = brush->POR;
        return;
    }
    
    // find free slot
    BrushSelectionNode *node;
    for (size_t i = 0; i < STATIC_STORAGE_LIMIT; i++) {
        if (sel->nodes[i].brush == NULL) {
            node = sel->nodes + i;
            node->next = NULL;
            node->prev = sel->tail;
            node->brush = brush;
            brush->flags |= BRUSHFLAG_SELECTED;

            // since num_selected >= 1, tail is non-NULL
            dibassert(sel->tail);
            sel->tail->next = node;
            sel->tail = node;
            sel->num_selected++;
            return;
        }
    }

    epm_Log(LT_WARN, "No room to store another selected brush!");
}
void toggle_selected_brush(BrushSelection *sel, Brush *brush) {
    if (brush->flags & BRUSHFLAG_SELECTED)
        del_selected_brush(sel, brush);
    else
        add_selected_brush(sel, brush);
}

void clear_brush_selection(BrushSelection *sel) {
    for (BrushSelectionNode *node = sel->head; node; node = node->next) {
        node->brush->flags &= ~BRUSHFLAG_SELECTED;
    }

    memset(sel, 0, sizeof(*sel));
}

/* -------------------------------------------------------------------------- */

// Vertex Selections

VertexSelection vertexsel = {{{0,0,0}}, 0, NULL, NULL, {{NULL, NULL, NULL}}};

void del_selected_vertex(VertexSelection *sel, WorldVec *vertex);
void add_selected_vertex(VertexSelection *sel, WorldVec *vertex);
void toggle_selected_vertex(VertexSelection *sel, WorldVec *vertex);

void clear_vertex_selection(VertexSelection *sel) {
    for (VertexSelectionNode *node = sel->head; node; node = node->next) {
        //node->brush->flags &= ~BRUSHFLAG_SELECTED;
    }

    memset(sel, 0, sizeof(*sel));
}


/* -------------------------------------------------------------------------- */

void sel_del(Selection *sel, void *obj) {
    if ( ! sel->fn_is_selected(obj)) return;

    SelectionNode *node;
    for (size_t i = 0; i < STATIC_STORAGE_LIMIT; i++) {
        if (sel->nodes[i].object == obj) {
            node = sel->nodes + i;
            sel->fn_unselect(obj);
            node->object = NULL;
            sel->num_selected--;

            //unlink
            if (node == sel->head) {
                sel->head = node->next;
            }
            
            if (node == sel->tail) {
                sel->tail = node->prev;
            }

            if (node->prev) {
                node->prev->next = node->next;
            }

            if (node->next) {
                node->next->prev = node->prev;
            }

            return;
        }
    }

    epm_Log(LT_WARN, "The object submitted for unselection was not found in the set of selected objects. Data integrity in question.");
}

void sel_add(Selection *sel, void *obj) {
    if (sel->fn_is_selected(obj)) return;
    
    if (sel->num_selected == 0) {
        dibassert(sel->nodes->object == NULL);
        
        sel->head = sel->nodes;
        sel->head->next = NULL;
        sel->head->prev = NULL;
        sel->head->object = obj;

        sel->fn_select(obj);
        
        sel->tail = sel->head;
        
        sel->num_selected = 1;
        return;
    }
    
    // find free slot
    SelectionNode *node;
    for (size_t i = 0; i < STATIC_STORAGE_LIMIT; i++) {
        if (sel->nodes[i].object == NULL) {
            node = sel->nodes + i;
            node->next = NULL;
            node->prev = sel->tail;
            node->object = obj;
            sel->fn_select(obj);

            // since num_selected >= 1, tail is non-NULL
            dibassert(sel->tail);
            sel->tail->next = node;
            sel->tail = node;
            sel->num_selected++;
            return;
        }
    }

    epm_Log(LT_WARN, "No room to store another selected brush!");    
}

void sel_toggle(Selection *sel, void *obj) {
    dibassert(sel->fn_is_selected != NULL);
    if (sel->fn_is_selected(obj)) {
        sel_del(sel, obj);
    }
    else {
        sel_add(sel, obj);
    }        
}

void sel_clear(Selection *sel) {
    for (SelectionNode *node = sel->head; node; node = node->next) {
        sel->fn_unselect(node->object);
    }


    sel->num_selected = 0;
    sel->head = NULL;
    sel->tail = NULL;
    memset(sel->nodes, 0, STATIC_STORAGE_LIMIT*sizeof(*sel->nodes));
}




/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

// Operations on face selection.

#include "src/draw/textures.h"

void apply_texture(char const *texname) {
    size_t i_tex;
    
    if (EPM_FAILURE != get_texture_by_name(texname, &i_tex)) {
        for (SelectionNode *node = sel_face.head; node; node = node->next) {
            ((BrushQuadFace *)node->object)->quad.i_tex = i_tex;
            ((BrushQuadFace *)node->object)->subface0->i_tex = i_tex;
            ((BrushQuadFace *)node->object)->subface1->i_tex = i_tex;
        }
    }
}


#include "src/input/command.h"

static void CMDH_apply_texture(int argc, char **argv, char *output_str) {
    apply_texture(argv[1]);
}

epm_Command const CMD_apply_texture = {
    .name = "apply_texture",
    .argc_min = 2,
    .argc_max = 2,
    .handler = CMDH_apply_texture,
};


void select_all_brush_faces(void) {
    for (SelectionNode *node = sel_face.head; node; node = node->next) {
        Brush *brush = g_world.geo_prebsp->progenitor_brush[((BrushQuadFace *)node->object)->subface0 - g_world.geo_prebsp->faces];
        for (size_t i = 0; i < brush->num_quads; i++) {
            sel_add(&sel_face, &brush->quads[i]);
        }
    }
}

static void CMDH_select_all_brush_faces(int argc, char **argv, char *output_str) {
    for (SelectionNode *node = sel_face.head; node; node = node->next) {
        Brush *brush = g_world.geo_prebsp->progenitor_brush[((BrushQuadFace *)node->object)->subface0 - g_world.geo_prebsp->faces];
        for (size_t i = 0; i < brush->num_quads; i++) {
            sel_add(&sel_face, &brush->quads[i]);
        }
    }
}

epm_Command const CMD_select_all_brush_faces = {
    .name = "select_all_brush_faces",
    .argc_min = 1,
    .argc_max = 1,
    .handler = CMDH_select_all_brush_faces,
};
