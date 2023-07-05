#include "src/locallibs/dibhash.h"

#include "src/misc/epm_includes.h"
#include "src/system/dir.h"
#include "src/system/config_reader.h"
#include "src/system/init_registry.h"

// Temporary: load a world upon init.
#include "src/world/world.h"

#undef LOG_LABEL
#define LOG_LABEL "INIT"

// TODO: More assertions!

static epm_Result result;

static void init_call(epm_Result (*fn_init)(void), char const *label) {
    epm_Log(LT_INFO, "Initializing %s...", label);
    if (fn_init() != EPM_SUCCESS) {
        epm_Log(LT_ERROR, "Failed to initialize %s.", label);
        result = EPM_FAILURE;
        return;
    }
    epm_Log(LT_INFO, "Initialized %s.", label);

    result = EPM_SUCCESS;
    return;
}

epm_Result epm_Init(int argc, char *argv[]) {
    (void)argc, (void)argv;
    // no command line arguments implemented yet

    /* ------------------------------------------------------------------ */
    // INIT PHASE 0
    /* ------------------------------------------------------------------ */

    // I consider the logging system initialization to be part of the pre-init
    // phase since the logging system is an essentially tool during the rest of
    // initialization.
    
    puts("Initializing logging system...");
    if (epm_InitLog() != EPM_SUCCESS) {
        puts("ERROR: Failed to initialize logging system.");
        return EPM_FAILURE;
    }
    set_active_log_group(LG_INIT);
    epm_Log(LT_INFO, "Initialized logging system.");


    
    /* ------------------------------------------------------------------ */
    // INIT PHASE 1: init_call()
    /* ------------------------------------------------------------------ */

    init_call(epm_InitDynamicVariables, "Dynamic Variables");
    if (result != EPM_SUCCESS) return EPM_FAILURE;
        
    init_call(epm_InitConfig,  "Config Reader");
    if (result != EPM_SUCCESS) return EPM_FAILURE;
    
    init_call(epm_InitSignals, "Signals");
    if (result != EPM_SUCCESS) return EPM_FAILURE;
    
    init_call(epm_InitTime,    "Timing");
    if (result != EPM_SUCCESS) return EPM_FAILURE;
    
    init_call(epm_InitIOLib,   "IO library");
    if (result != EPM_SUCCESS) return EPM_FAILURE;
    
    init_call(epm_InitDraw,    "Graphics");
    if (result != EPM_SUCCESS) return EPM_FAILURE;
    
    init_call(epm_InitInput,   "Input");
    if (result != EPM_SUCCESS) return EPM_FAILURE;

    init_call(epm_InitInterface, "Interface");
    if (result != EPM_SUCCESS) return EPM_FAILURE;

    init_call(epm_InitWorld, "World");
    if (result != EPM_SUCCESS) return EPM_FAILURE;
    
    epm_Log(LT_INFO, "Initialized the Electric Pentacle.");

    /* ------------------------------------------------------------------ */
    // INIT PHASE 2
    /* ------------------------------------------------------------------ */
    
    set_active_log_group(LG_GENERAL);

    print_all_HashTable(); // temporary
    
    return EPM_SUCCESS;
}
