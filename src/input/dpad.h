#ifndef DPAD_H
#define DPAD_H

#include "zigil/zigil_keycodes.h"

#include "src/misc/epm_includes.h"

typedef int VirtualDPad_hndl;
#define FLAG_U 1
#define FLAG_L 2
#define FLAG_D 4
#define FLAG_R 8
typedef enum {
    DPAD_NULL = 0,
    U = 1,
    UL = 2,
    L = 3,
    DL = 4,
    D = 5,
    DR = 6,
    R = 7,
    UR = 8,
} DPad_Dir;

VirtualDPad_hndl register_dpad(zgl_KeyCode up, zgl_KeyCode left, zgl_KeyCode down, zgl_KeyCode right);
epm_Result get_dpad_state(VirtualDPad_hndl hndl, DPad_Dir *dir, Ang18 *ang);


#endif /* DPAD_H */
