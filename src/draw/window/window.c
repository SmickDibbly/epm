#include <stdio.h>

#include "src/draw/window/window.h"

//#define VERBOSITY
#include "src/locallibs/verbosity.h"

extern WindowNode *unlink_WindowNode(WindowNode *node);
extern WindowNode *link_WindowNode(WindowNode *node, WindowNode *parent);
extern void bring_to_front(WindowNode *node);

extern void print_WindowTree(WindowNode *node);
static void print_WindowTree_recursion(WindowNode *node, uint32_t level);
extern void draw_WindowTree(WindowNode *node);
extern WindowNode *window_below(WindowNode *node, zgl_Pixel pt);
static WindowNode *window_below_recursion(WindowNode *wn, zgl_Pixel pt);
    
void print_WindowTree(WindowNode *node) {
    print_WindowTree_recursion(node, 0);
}

void print_WindowTree_recursion(WindowNode *root, uint32_t level)
{
    if (root == NULL)
        return;

    if (level != 0) {
        for (uint32_t i = 0; i < level-1; i++)
            printf("   ");
        printf("|--");
    }
    printf("%s\n", root->win->name);

    for (WindowNode *p_child = root->child_front; p_child != NULL; p_child = p_child->next) {
        print_WindowTree_recursion(p_child, level + 1);
    }
}

void draw_WindowTree(WindowNode *root) {
    if (root->win->winfncs.draw) {
        root->win->winfncs.draw(root->win);
    }

    if (root->child_back == NULL)
        return;
    
    for (WindowNode *p_child = root->child_back; p_child != NULL; p_child = p_child->prev) {
        draw_WindowTree(p_child);
    }

    return;
}


WindowNode *unlink_WindowNode(WindowNode *wn) {
    vbs_fprintf(stdout, "Unlinking: Child \"%s\" from Parent \"%s\"\n", wn->win->name, wn->parent == NULL? "Nothing" : wn->parent->win->name);

    if (wn->parent == NULL) {
        return wn;
    }
    
    /* unlink from parent */
    if (wn == wn->parent->child_front) { // I am first
        wn->parent->child_front = wn->next;
    }
    
    if (wn == wn->parent->child_back) { // I am last
        wn->parent->child_back = wn->prev;
    }

    wn->parent = NULL;

    /* unlink from siblings */
    if (wn->next != NULL) {
        wn->next->prev = wn->prev;
    }

    if (wn->prev != NULL) {
        wn->prev->next = wn->next;
    }
    
    return wn;
}

WindowNode *link_WindowNode(WindowNode *wn, WindowNode *parent) {
    if (! wn || ! parent)
        return NULL;
    
    vbs_printf("Linking: Child \"%s\" to Parent \"%s\"\n", wn->win->name, parent == NULL? "Nothing" : parent->win->name);
    
    if (wn->parent == parent) { // already linked
        return wn;
    }
    
    wn->prev = NULL;
    wn->parent = parent;
    if (wn->parent != NULL) {
        /* link to parent's child list */
        wn->next = wn->parent->child_front;
        wn->parent->child_front = wn;
        if (wn->next != NULL) { // Parent already had children before me
            wn->next->prev = wn;
        }
        else { // I am the first and only child
            wn->parent->child_back = wn;
        }
    }
    else {
        wn->next = NULL;
    }
    
    /*
      if wn->next is NOT NULL then:

      ???
      ^
      |
      V
      __PARENT__
      ^        ^
      |        |
      V        V
      NULL <-x WN <-> NEXT <-> ???
      x        ^
      |        |
      V        v
      NULL     ???


      OR if wn->next IS NULL then:

      ???
      ^
      |
      V
      PARENT
      ^
      |
      V
      NULL <-x WN x-> NULL
      x
      |
      V
      NULL
    */

    return wn;
}


void bring_to_front(WindowNode *wn) {
    if (wn->parent != NULL) {
        bring_to_front(wn->parent);
    }
    
    /* move self to front of sibling list */
    if (wn->prev == NULL) // already first sibling, no more work to do
        return;
    
    if (wn->next == NULL) {
        wn->parent->child_back = wn->prev;
    }

    /* Link my current neighbor(s) */
    wn->prev->next = wn->next;
    if (wn->next != NULL) { // if self is not last in list
        wn->next->prev = wn->prev;
    }

    /* Put myself in front */
    wn->prev = NULL;
    wn->next = wn->parent->child_front;
    wn->next->prev = wn;

    /* Tell parent of the change */
    wn->parent->child_front = wn;
}

char *Widget_path_str(WindowNode *wgt) {
    /* in the form root/widget1/widget2/etc... like a file path */
    // TODO
    (void)wgt;
    return "0";
}

static bool in_rect(zgl_Pixel pt, zgl_PixelRect r) {
    return ((pt.x >= r.x) &&
            (pt.y >= r.y) &&
            (pt.x < r.x + r.w) &&
            (pt.y < r.y + r.h));
}

static WindowNode *window_below_recursion(WindowNode *node, zgl_Pixel pt) {
    // for each child C of W, test if point P is in C
    // since children are ordered from the top down, stop at the FIRST child C containing P, since even if subsequent children also contain P, they would be behind the earlier children
    // if no child C of W contains P, then return W itself
    // if child C of W does contain P, then repeat the algorithm for the children of C, recursively

    for (WindowNode *p_child = node->child_front; p_child != NULL; p_child = p_child->next) {
        if (in_rect(pt, p_child->win->rect)) {
            return window_below_recursion(p_child, pt);
        }
    }

    return node;
}

WindowNode *window_below(WindowNode *node, zgl_Pixel pt) {
    WindowNode *res = window_below_recursion(node, pt);
    if (!res) return NULL;
    
    return res;
}
