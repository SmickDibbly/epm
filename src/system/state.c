#include "src/misc/epm_includes.h"
#include "src/system/state.h"

#undef LOG_LABEL
#define LOG_LABEL "SYS.STATE"

#if EPM_WINDOWS
#include <windows.h>
#include <psapi.h>
#endif

State state = {.timing.fpscapped = true, 0};
