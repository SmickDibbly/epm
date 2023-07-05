#ifndef INPUT_H
#define INPUT_H

#include "zigil/zigil_keycodes.h"

#include "src/misc/epm_includes.h"

#include "src/draw/window/window.h"

extern void epm_SetInputFocus(WindowNode *to);
extern void epm_UnsetInputFocus(void);
extern void grab_input(Window *win);
extern void ungrab_input(void);

#endif /* INPUT_H */
