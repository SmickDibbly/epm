#include "src/input/dpad.h"

#define FLAG_U 1
#define FLAG_L 2
#define FLAG_D 4
#define FLAG_R 8

#undef LOG_LABEL
#define LOG_LABEL "INPUT.DPAD"

// treats 4 buttons representing forward, backward, left and right, as a virtual 8 directional pad.
static const DPad_Dir dpad_mapping[] = {
    [DPAD_NULL] = DPAD_NULL,
    [FLAG_U] = U,
    [FLAG_L] = L,
    [FLAG_D] = D,
    [FLAG_R] = R,
    [FLAG_U|FLAG_L] = UL,
    [FLAG_L|FLAG_D] = DL,
    [FLAG_D|FLAG_R] = DR,
    [FLAG_U|FLAG_R] = UR,
    [FLAG_U|FLAG_D] = DPAD_NULL,
    [FLAG_L|FLAG_R] = DPAD_NULL,
    [FLAG_L|FLAG_D|FLAG_R] = D,
    [FLAG_U|FLAG_D|FLAG_R] = R,
    [FLAG_U|FLAG_L|FLAG_R] = U,
    [FLAG_U|FLAG_L|FLAG_D] = L,
    [FLAG_U|FLAG_L|FLAG_D|FLAG_R] = DPAD_NULL,
};
static const Ang18 dpad_angles[] = {
    [U]  = 0*ANG18_PI4,
    [UL] = 1*ANG18_PI4,
    [L]  = 2*ANG18_PI4,
    [DL] = 3*ANG18_PI4,
    [D]  = 4*ANG18_PI4,
    [DR] = 5*ANG18_PI4,
    [R]  = 6*ANG18_PI4,
    [UR] = 7*ANG18_PI4,
};
typedef struct VirtualDPad {
    zgl_KeyCode U_key;
    zgl_KeyCode L_key;
    zgl_KeyCode D_key;
    zgl_KeyCode R_key;
    bool registered;
} VirtualDPad;

#define MAX_VIRTUALDPADS 16
VirtualDPad dpads[MAX_VIRTUALDPADS] = {0};

epm_Result get_dpad_state(VirtualDPad_hndl hndl, DPad_Dir *dir, Ang18 *ang) {
    if (hndl < 0 || hndl > MAX_VIRTUALDPADS)
        return EPM_ERROR;
    
    VirtualDPad *dpad = &dpads[hndl];
    
    /*
      NOTHING -> 0

      U _ _ _ -> U
      _ L _ _ -> L
      _ _ D _ -> D
      _ _ _ R -> R
      
      U L _ _ -> UL
      _ L D _ -> DL
      _ _ D R -> DR
      U _ _ R -> UR
      U _ D _ -> 0
      _ L _ R -> 0
      
      _ L D R -> D
      U _ D R -> R
      U L _ R -> U
      U L D _ -> L
      
      ALL -> 0
    */

    bool p;
    uint8_t flag;
    uint8_t flags = 0;
    
    flag = FLAG_U;
    p = ZK_states[dpad->U_key];
    flags ^= (-p ^ flags) & flag; // branchless "if (p) flags |= flag;"

    flag = FLAG_L;
    p = ZK_states[dpad->L_key];
    flags ^= (-p ^ flags) & flag; // branchless "if (p) flags |= flag;"
    
    flag = FLAG_D;
    p = ZK_states[dpad->D_key];
    flags ^= (-p ^ flags) & flag; // branchless "if (p) flags |= flag;"

    flag = FLAG_R;
    p = ZK_states[dpad->R_key];
    flags ^= (-p ^ flags) & flag; // branchless "if (p) flags |= flag;"
    
    *dir = dpad_mapping[flags];
    *ang = dpad_angles[*dir];
    
    return EPM_SUCCESS;
}

VirtualDPad_hndl register_dpad(zgl_KeyCode up, zgl_KeyCode left, zgl_KeyCode down, zgl_KeyCode right) {
    VirtualDPad *dpad = NULL;

    VirtualDPad_hndl i_dpad;
    for (i_dpad = 0; i_dpad < MAX_VIRTUALDPADS; i_dpad++) {
        if ( ! dpads[i_dpad].registered) {
            dpad = &dpads[i_dpad];
            break;
        }
    }

    if (dpad == NULL) {
        epm_Log(LT_ERROR, "ERROR: Maximum number (%i) of virtual dpads reached.", MAX_VIRTUALDPADS);
        return -1;
    }

    dpad->registered = true;
    dpad->U_key = up;
    dpad->L_key = left;
    dpad->D_key = down;
    dpad->R_key = right;

    epm_Log(LT_INFO, "Registered dpad %i.", i_dpad);
    return i_dpad;
}
