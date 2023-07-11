#include <stdlib.h>
#include "zigil/zigil.h"
#include "zigil/zigil_event.h"

#include "src/input/input.h"
#include "src/input/dpad.h"

#include "zigil/diblib_local/dibhash.h"
#include "src/misc/epm_includes.h"
#include "src/system/state.h"
#include "src/draw/window/window_registry.h"
#include "src/system/loop.h"
#include "src/input/globalmap.h"

WindowNode *input_focus = NULL;
WindowNode *node_below, *prev_node_below = NULL;
Window *win_below, *prev_win_below = NULL; 

static bool grabbed = false;
static zgl_Pixel pregrab_pos;
static Window *grabber;

epm_Result epm_InitInput(void) {    
    epm_LoadGlobalKeymap();

    epm_Result epm_InitCommand(void);
    epm_InitCommand();
    
    return EPM_SUCCESS;
}

epm_Result epm_TermInput(void) {
    epm_Result epm_TermCommand(void);
    epm_TermCommand();

    epm_UnloadGlobalKeymap();
    
    return EPM_SUCCESS;
}

void grab_input(Window *win) {
    if (!grabbed) {
        pregrab_pos = zgl_GetPointerPos();
        zgl_SetRelativePointer();
        grabbed = true;
    }
    grabber = win;
}

void ungrab_input(void) {
    if (grabbed) {
        zgl_ReleaseRelativePointer(&pregrab_pos);
        grabber = NULL;
        grabbed = false;
    }    
}

static epm_Result do_KeyPress(zgl_KeyPressEvent *evt) {
    state.input.last_press = evt->lk;

    do_KeyPress_global(evt);
    
    if (input_focus && input_focus->win->winfncs.onKeyPress) {
        input_focus->win->winfncs.onKeyPress(input_focus->win, evt);
    }
    
    return EPM_SUCCESS;
}

static epm_Result do_KeyRelease(zgl_KeyReleaseEvent *evt) {
    do_KeyRelease_global(evt);
    
    if (input_focus && input_focus->win->winfncs.onKeyRelease) {
        input_focus->win->winfncs.onKeyRelease(input_focus->win, evt);
    }
        
    return EPM_SUCCESS;
}

static epm_Result do_PointerPress(zgl_PointerPressEvent *evt) {
    state.input.last_press = evt->lk;

    if (grabbed) {
        // if grabbed, send directly to grabber
        grabber->winfncs.onPointerPress(win_below, evt);
        return EPM_SUCCESS;
    }
    
    prev_win_below = win_below;
    prev_node_below = node_below;
    
    node_below = window_below(root_node, zgl_GetPointerPos());
    if (!node_below) {
        win_below = NULL;
    }
    else {
        bring_to_front(node_below);
        win_below = node_below->win;

        if (win_below) {
            state.input.win_below_name = win_below->name;
            if (input_focus != node_below && node_below->win->focusable) {
                // new input focus
                if (input_focus && input_focus->win->winfncs.onFocusOut) {
                    input_focus->win->winfncs.onFocusOut(input_focus->win);
                }
                input_focus = node_below;
                if (input_focus && input_focus->win->winfncs.onFocusIn) {
                    input_focus->win->winfncs.onFocusIn(input_focus->win);
                }
            }
        
        
            if (win_below->winfncs.onPointerPress) {
                win_below->winfncs.onPointerPress(win_below, evt);
            }
        }
    }
    
    return EPM_SUCCESS;
}

static epm_Result do_PointerRelease(zgl_PointerReleaseEvent *evt) {
    if (grabbed) {
        // if grabbed, send directly to grabber
        grabber->winfncs.onPointerRelease(grabber, evt);
        return EPM_SUCCESS;
    }
    
    prev_win_below = win_below;
    node_below = window_below(root_node, zgl_GetPointerPos());
    win_below = node_below->win;
    
    if (win_below) {
        state.input.win_below_name = win_below->name;
        if (win_below->winfncs.onPointerRelease) {
            win_below->winfncs.onPointerRelease(win_below, evt);
        }
    }
    return EPM_SUCCESS;
}

static epm_Result do_PointerMotion(zgl_PointerMotionEvent *evt) {
    state.input.pointer_x = evt->x;
    state.input.pointer_y = evt->y;
    state.input.pointer_rel_x = evt->dx;
    state.input.pointer_rel_y = evt->dy;

    if (grabbed) {
        // if grabbed, send directly to grabber
        grabber->winfncs.onPointerMotion(grabber, evt);
        return EPM_SUCCESS;
    }
    
    prev_win_below = win_below;
    node_below = window_below(root_node, (zgl_Pixel){evt->x, evt->y});
    win_below = node_below->win;
    
    if (win_below != prev_win_below) {
        if (prev_win_below && prev_win_below->winfncs.onPointerLeave) {
            prev_win_below->winfncs.onPointerLeave(prev_win_below,
                                           &(zgl_PointerLeaveEvent){evt->x, evt->y});
            zgl_SetCursor(ZC_arrow);
        }
        if (win_below && win_below->winfncs.onPointerEnter) {
            //zgl_SetCursor(win_below->cursor);
            win_below->winfncs.onPointerEnter(win_below, &(zgl_PointerEnterEvent){evt->x, evt->y});
        }
    }

    if (win_below) {
        state.input.win_below_name = win_below->name;
        if (win_below->winfncs.onPointerMotion) {
            win_below->winfncs.onPointerMotion(win_below, evt);
        }
    }
    return EPM_SUCCESS;
}

static epm_Result do_PointerEnter(zgl_PointerEnterEvent *evt) {
    // Pointer has entered the entire application window
    node_below = window_below(root_node, zgl_GetPointerPos());
    win_below = node_below->win;

    if (win_below) {
        state.input.win_below_name = win_below->name;
        if (win_below->winfncs.onPointerEnter) {
            win_below->winfncs.onPointerEnter(win_below, evt);
        }
    }
    
    return EPM_SUCCESS;
}

static epm_Result do_PointerLeave(zgl_PointerLeaveEvent *evt) {
    if (prev_win_below && prev_win_below->winfncs.onPointerLeave) {
        prev_win_below->winfncs.onPointerLeave(win_below, evt);
    }

    state.input.win_below_name = NULL;
    win_below = NULL;
    prev_win_below = NULL;

    return EPM_SUCCESS;
}

epm_Result epm_DoInput(void) {
    zgl_UpdateEventQueue();
    
    zgl_Event evt;
    while (zgl_PopEvent(&evt) != ZR_NOEVENT) {
        switch (evt.type) {
        case EC_KeyPress:
            do_KeyPress(&evt.u.key_press);
            break;
        case EC_KeyRelease:
            do_KeyRelease(&evt.u.key_release);
            break;
        case EC_PointerPress:
            do_PointerPress(&evt.u.pointer_press);
            break;
        case EC_PointerRelease:
            do_PointerRelease(&evt.u.pointer_release);
            break;
        case EC_PointerMotion:
            do_PointerMotion(&evt.u.pointer_motion);
            break;
        case EC_PointerEnter:
            do_PointerEnter(&evt.u.pointer_enter);
            break;
        case EC_PointerLeave:
            do_PointerLeave(&evt.u.pointer_leave);
            break;
        case EC_CloseRequest:
            return EPM_STOP;
        default:
            break;
        }
    }

    return EPM_CONTINUE;
}

void epm_SetInputFocus(WindowNode *to) {
    if (input_focus && input_focus->win->winfncs.onFocusOut) {
        input_focus->win->winfncs.onFocusOut(input_focus->win);
    }
    input_focus = to;
    if (input_focus && input_focus->win->winfncs.onFocusIn) {
        input_focus->win->winfncs.onFocusIn(input_focus->win);
    }
}

void epm_UnsetInputFocus(void) {
    epm_SetInputFocus(NULL);
}












#if 0
/* -------------------------------------------------------------------------- */

typedef enum epm_InputCode {
    IC_BUTTON,
    IC_KEY,
    IC_POINTER,
    IC_DPAD,
    IC_AXIS,
    IC_FOCUS,

    NUM_IC
} epm_InputCode;

typedef struct epm_ButtonInput {
    zgl_KeyCode zk;
    uint8_t mod_states;
} epm_ButtonInput;

typedef struct epm_KeyInput {
    zgl_KeyCode zk;
    bool state;
    uint8_t mod_states;
} epm_KeyInput;

typedef struct epm_PointerInput {
    int x, y;
    uint8_t pointer_states;
} epm_PointerInput;

typedef struct epm_DPadInput {
    DPad_Dir dir;
    ang18_t ang;
} epm_DPadInput;

typedef struct epm_AxisInput {
    fix32_t val;
} epm_AxisInput;

typedef struct epm_InputFocusChange {
    bool state; // true = focus in; false = focus out
} epm_InputFocusChange;

typedef struct epm_Input {
    epm_InputCode code;
    
    union {
        epm_ButtonInput button;
        epm_KeyInput key;
        epm_PointerInput pointer;
        epm_DPadInput dpad;
        epm_AxisInput axis;
        epm_InputFocusChange focus;
    } u;
} epm_Input;

typedef void (*fn_onButtonInput)(epm_ButtonInput *, void *arg);
typedef void (*fn_onKeyInput)(epm_KeyInput *, void *arg);
typedef void (*fn_onPointerInput)(epm_PointerInput *, void *arg);
typedef void (*fn_onDPadInput)(epm_DPadInput *, void *arg);
typedef void (*fn_onAxisInput)(epm_AxisInput *, void *arg);
typedef void (*fn_onInputFocusChange)(epm_InputFocusChange *, void *arg);




#define KEY 0
#define PRESS_AS_BUTTON 1
#define RELEASE_AS_BUTTON 2
#define DPAD 3

typedef struct InputContext {
    void *arg;

    int default_input_type;
    UIntMap32 *map; // Maps physical keys and pointer buttons to what kind of input
                 // they are in this context (Button, Key, or DPad). To keep the
                 // hash table small, one possible value can be chosen as
                 // default.
    
    fn_onButtonInput onButtonInput;
    fn_onKeyInput onKeyInput;
    fn_onDPadInput onDPadInput;
    fn_onPointerInput onPointerInput;
    fn_onAxisInput onAxisInput;
    fn_onInputFocusChange onInputFocusChange;
} InputContext;

InputContext *active_contexts[16];

UInt32Pair inputs_types[4] = {
    {ZK_a, PRESS_AS_BUTTON},
    {ZK_b, RELEASE_AS_BUTTON},
    {ZK_c, DPAD},
    {ZK_d, DPAD},
};

void epm_InitInputContext(InputContext *ic) {
    ic->map = create_UIntMap32(0, NULL, "Input Context", 0);
}

void do_inputcontext_keypress(InputContext *ic, zgl_KeyPressEvent *zevt) {
    epm_Input input;
    
    uint32_t input_type;
    bool res = UIntMap32_lookup(ic->map, zevt->zk, &input_type);

    if ( ! res) input_type = ic->default_input_type;

    switch (input_type) {
    case PRESS_AS_BUTTON:
        input.code = IC_BUTTON;
        input.u.button.zk = zevt->zk;
        input.u.button.mod_states = zevt->mod_flags;
        break;
    case RELEASE_AS_BUTTON: // do nothing, since this was a KeyPressEvent
        break;
    case KEY:
        input.code = IC_KEY;
        break;
    case DPAD:
        input.code = IC_DPAD;
        break;
    default:
        break;
    }

    (void)input;
}
#endif
