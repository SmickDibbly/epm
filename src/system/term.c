#include "zigil/zigil.h"
#include "src/locallibs/dibhash.h"

#include "src/misc/epm_includes.h"
#include "src/system/term_registry.h"

#include "src/world/world.h"

#undef LOG_LABEL
#define LOG_LABEL "TERM"

static epm_Result result;

static void term_call(epm_Result (*fn_init)(void), char const *label) {
    epm_Log(LT_INFO, "Terminating %s...", label);
    if (fn_init() != EPM_SUCCESS) {
        epm_Log(LT_ERROR, "Failed to terminate %s.", label);
        result = EPM_FAILURE;
        return;
    }
    epm_Log(LT_INFO, "Terminated %s.", label);

    result = EPM_SUCCESS;
    return;
}

epm_Result epm_Term(void) {
    /* ------------------------------------------------------------------ */
    // TERM PHASE 0
    /* ------------------------------------------------------------------ */
    
    set_active_log_group(LG_TERM);

    /* ------------------------------------------------------------------ */
    // TERM PHASE 1: term_call()
    /* ------------------------------------------------------------------ */

    epm_Log(LT_INFO, "Terminating the Electric Pentacle...");

    term_call(epm_TermWorld, "World");
    if (result != EPM_SUCCESS) return EPM_FAILURE;
    
    term_call(epm_TermInterface, "Interface");
    if (result != EPM_SUCCESS) return EPM_FAILURE;
    
    term_call(epm_TermInput,   "Input");
    if (result != EPM_SUCCESS) return EPM_FAILURE;

    term_call(epm_TermDraw,    "Graphics");
    if (result != EPM_SUCCESS) return EPM_FAILURE;
    
    term_call(epm_TermIOLib,   "IO library");
    if (result != EPM_SUCCESS) return EPM_FAILURE;

    term_call(epm_TermConfig,  "Config Reader");
    if (result != EPM_SUCCESS) return EPM_FAILURE;
    
    term_call(epm_TermDynamicVariables, "Dynamic Variables");
    if (result != EPM_SUCCESS) return EPM_FAILURE;

    /* ------------------------------------------------------------------ */
    // TERM PHASE 2
    /* ------------------------------------------------------------------ */

    epm_TermLog();
    
    destroy_all_HashTable(); // in case any were missed
    
    zgl_PrintMemSummary();
    
    return EPM_SUCCESS;
}
