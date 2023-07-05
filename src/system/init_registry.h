#ifndef INIT_REGISTRY_H
#define INIT_REGISTRY_H

#include "src/misc/epm_includes.h"

extern epm_Result epm_InitLog(void); //log.c

extern epm_Result epm_InitDynamicVariables(void); //variables.c
extern epm_Result epm_InitConfig(void); //config.c
extern epm_Result epm_InitSignals(void); //signal_handler.c
extern epm_Result epm_InitTime(void); //timing.c
extern epm_Result epm_InitIOLib(void); //io.c
extern epm_Result epm_InitDraw(void); //draw.c
extern epm_Result epm_InitInput(void); //input.c
extern epm_Result epm_InitInterface(void); // ???
extern epm_Result epm_InitWorld(void); //world.c

#endif /* INIT_REGISTRY_H */
