#ifndef GLOBALMAP_H
#define GLOBALMAP_H

#include "zigil/zigil_event.h"
#include "src/misc/epm_includes.h"

extern epm_Result epm_LoadGlobalKeymap(void);
extern epm_Result epm_UnloadGlobalKeymap(void);
extern epm_Result do_KeyPress_global(zgl_KeyPressEvent *evt);
extern epm_Result do_KeyRelease_global(zgl_KeyReleaseEvent *evt);

#endif /* GLOBALMAP_H */
