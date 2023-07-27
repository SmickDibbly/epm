#include "src/misc/epm_includes.h"
#include "src/world/world.h"

#include "src/world/selection.h"

void *sel_del(Selection *sel, void *obj) {
    if ( ! sel->fn_is_selected(obj)) return NULL;
    void *res;
    SelectionNode *node;
    for (size_t i = 0; i < STATIC_STORAGE_LIMIT; i++) {
        if (sel->nodes[i].obj == obj) {
            node = sel->nodes + i;
            sel->fn_unselect(obj);
            res = node->obj;
            node->obj = NULL;
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

            return res;
        }
    }

    epm_Log(LT_WARN, "The object submitted for unselection was not found in the set of selected objects. Data integrity in question.");
    return NULL;
}

void sel_add(Selection *sel, void *obj) {
    if (sel->fn_is_selected(obj)) return;
    
    if (sel->num_selected == 0) {
        dibassert(sel->nodes->obj == NULL);
        
        sel->head = sel->nodes;
        sel->head->next = NULL;
        sel->head->prev = NULL;
        sel->head->obj = obj;


        sel->fn_select(obj);
        sel->tail = sel->head;
        
        sel->num_selected = 1;
        return;
    }
    
    // find free slot
    SelectionNode *node;
    for (size_t i = 0; i < STATIC_STORAGE_LIMIT; i++) {
        if (sel->nodes[i].obj == NULL) {
            node = sel->nodes + i;
            node->next = NULL;
            node->prev = sel->tail;
            node->obj = obj;
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
        sel->fn_unselect(node->obj);
    }

    sel->num_selected = 0;
    sel->head = NULL;
    sel->tail = NULL;
    memset(sel->nodes, 0, STATIC_STORAGE_LIMIT*sizeof(*sel->nodes));
}

