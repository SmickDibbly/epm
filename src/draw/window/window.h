#ifndef WINDOW_H
#define WINDOW_H

#include "zigil/zigil.h"
#include "zigil/zigil_event.h"

#define MAX_WINDOW_NAME_LEN 31

typedef struct Window Window;
typedef struct WindowFunctions WindowFunctions;

typedef void (*fn_draw)(Window *self);
typedef void (*fn_onPointerPress)(Window *self, zgl_PointerPressEvent *);
typedef void (*fn_onPointerRelease)(Window *self, zgl_PointerReleaseEvent *);
typedef void (*fn_onPointerMotion)(Window *self, zgl_PointerMotionEvent *);
typedef void (*fn_onPointerEnter)(Window *self, zgl_PointerEnterEvent *);
typedef void (*fn_onPointerLeave)(Window *self, zgl_PointerLeaveEvent *);
typedef void (*fn_onKeyPress)(Window *self, zgl_KeyPressEvent *);
typedef void (*fn_onKeyRelease)(Window *self, zgl_KeyReleaseEvent *);
typedef void (*fn_onFocusIn)(Window *self);
typedef void (*fn_onFocusOut)(Window *self);

struct WindowFunctions {
    fn_draw             draw;
    fn_onPointerPress   onPointerPress;
    fn_onPointerRelease onPointerRelease;
    fn_onPointerMotion  onPointerMotion;
    fn_onPointerEnter   onPointerEnter;
    fn_onPointerLeave   onPointerLeave;

    // TODO: These are not really Window-specific, perhaps a general
    // InputFunctions would helpful.
    fn_onKeyPress       onKeyPress;
    fn_onKeyRelease     onKeyRelease;
    
    fn_onFocusIn        onFocusIn;
    fn_onFocusOut       onFocusOut;
};

struct Window {
    //zgl_PixelArray *fb; // In case I ever want windows that don't render to
    //the actual screen directly.
    
    char name[MAX_WINDOW_NAME_LEN + 1];
    zgl_PixelRect rect;
    zgl_mPixelRect mrect;

    void *data; // pointer to pass arbitrary data through a window function call
    
    bool focusable;
    // if false, window will never receive the input focus, and will never have
    // its onKeyPress, onKeyRelease, onFocusIn, or onFocusOut called.
    
    bool dragged;
    zgl_Pixit last_ptr_x;
    zgl_Pixit last_ptr_y;
    zgl_CursorCode cursor;
    
    WindowFunctions winfncs;
};

typedef struct WindowNode WindowNode;
struct WindowNode {
    Window *win;
    
    WindowNode *child_front;
    WindowNode *child_back;
    WindowNode *parent;
    WindowNode *next;
    WindowNode *prev;
};

extern WindowNode *unlink_WindowNode(WindowNode *node);
extern WindowNode *link_WindowNode(WindowNode *node, WindowNode *parent);
extern void bring_to_front(WindowNode *node);

extern void print_WindowTree(WindowNode *node);
extern void draw_WindowTree(WindowNode *node);
extern WindowNode *window_below(WindowNode *node, zgl_Pixel pt);

#endif /* WINDOW_H */
