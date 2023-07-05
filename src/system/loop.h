#ifndef LOOP_H
#define LOOP_H

#include "src/misc/epm_includes.h"

#define EPM_CONTINUE 0
#define EPM_STOP 1

extern epm_Result epm_DoInput(void);  /* input.c */
extern epm_Result epm_Tic(void);    /* world.c */
extern epm_Result epm_Render(void); /* draw.c */

#endif /* LOOP_H */
