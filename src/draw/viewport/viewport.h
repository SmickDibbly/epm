#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "src/misc/epm_includes.h"
#include "src/draw/draw.h"
#include "src/draw/window/window.h"

/* A Viewport is essentially a region of the screen reserved for displaying
   Viewport Interfaces.

   Internally each Viewport is a Window whose size and position is typically
   unchanged, but whose WindowFunctions and generic Data pointer are swapped out
   as needed to change the mapped interface.

   A Viewport has a "ViewportBar" at the top, reserved for buttons and other
   small UI elements related to the current interface. TODO: Allow a Viewport
   Interface to opt out of having a viewbar.

   There are eight viewports, not all of which can be visible at the same time
   (see Viewport Layout for sets of simulataneously-viewable viewports)
*/
typedef enum VPCode {
    VP_NONE,
    
    VP_TL,
    VP_TR,
    VP_BR,
    VP_BL,
    VP_L,
    VP_R,
    VP_T,
    VP_B,
    VP_FULL,

    NUM_VP
} VPCode;

#define VP_0 VP_TL
#define VP_1 VP_TR
#define VP_2 VP_BR
#define VP_3 VP_BL

/* A Viewport Layout is merely a set of viewports to be shown simultaneously.
   
 */
typedef enum VPLayoutCode {
    VPL_QUAD,
    
    VPL_TRI_L,
    VPL_TRI_R,
    VPL_TRI_T,
    VPL_TRI_B,
    
    VPL_DUO_LR,
    VPL_DUO_TB,
    
    VPL_MONO,

    NUM_VPL
} VPLayoutCode;

/* A Viewport Interface is like a small program that displays information and
   recieves input when mapped to a Viewport. For example, the 3D view of the
   world is a Viewport Interface, as are the orthogonal top, side, and front
   world views. Another example is the console, which receives command strings
   as input which displays command output.
 */

typedef enum VPInterfaceCode {
    VPI_NONE,

    VPI_MONITOR,
    
    VPI_WORLD3D,
    VPI_WORLDTOP,
    VPI_WORLDSIDE,
    VPI_WORLDFRONT,
    
    VPI_FILESELECT,
    VPI_CONSOLE,
    VPI_LOGVIEW,
    VPI_TEXVIEW,
    VPI_BSPVIEW,
    
    NUM_VPI
} VPInterfaceCode;


extern VPLayoutCode i_curr_VPL;

extern void draw_viewbar(Window *win);
extern void do_PointerPress_viewbar(Window *win, zgl_PointerPressEvent *evt);
extern WindowFunctions winfncs_viewbar;

extern void epm_SetActiveVP(VPCode i_VP);
extern WindowNode *epm_NextActiveVP(void);
extern WindowNode *epm_PrevActiveVP(void);

extern void epm_SetVPInterface(VPCode i_VP, VPInterfaceCode i_VPI);
extern VPInterfaceCode epm_NextVPInterface(void);
extern VPInterfaceCode epm_PrevVPInterface(void);

extern void epm_InitVPLayout(VPLayoutCode i_VPL);
extern void epm_SetVPLayout(VPLayoutCode i_VPL);
extern VPLayoutCode epm_NextVPLayout(void);
extern VPLayoutCode epm_PrevVPLayout(void);

extern VPInterfaceCode next_vpi(VPCode i_VP);
extern VPInterfaceCode prev_vpi(VPCode i_VP);

#endif /* VIEWPORT_H */
