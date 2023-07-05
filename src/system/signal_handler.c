#include <signal.h>

#include "src/misc/epm_includes.h"

#undef LOG_LABEL
#define LOG_LABEL "SYS.SIG"

// TODO: Don't use printing functions in signal handlers!

volatile sig_atomic_t signal_received = 0;

static void termination_signal_handler(int sig) {
    /* Cleanup, then raise signal with default handler */

    signal_received = 1;
    
    switch (sig) {
    case SIGINT:
        epm_Log(LT_ERROR, "Signal received: SIGINT (interrupt).");
        break;
    case SIGTERM:
        epm_Log(LT_ERROR, "Signal received: SIGTERM (termination request).");
        break;
    }
    
    signal(sig, SIG_DFL);
    raise(sig);
}

static void program_error_signal_handler(int sig) {
    signal_received = 1;
        
    switch (sig) {
    case SIGFPE:
        // keep in mind SIGFPE is poorly named; not necessarily float-related
        epm_Log(LT_ERROR, "Signal received: SIGFPE (erroneous arithmetic operation).");
        break;
    case SIGILL:
        epm_Log(LT_ERROR, "Signal received: SIGILL (illegal instruction).");
        break;
    case SIGSEGV:
        epm_Log(LT_ERROR, "Signal received: SIGSEGV (segmentation violation). Invalid access to valid memory.");
        break;
#ifdef SIGBUS
    case SIGBUS:
        epm_Log(LT_ERROR, "Signal received: SIGSEGV (segmentation violation). Attempted to access invalid memory address.");
        break;
#endif
    }

    // TODO cleanly shutdown if feasible

    signal(sig, SIG_DFL);
    raise(sig);
}

static void abort_signal_handler(int sig) {
    signal_received = 1;
    
    epm_Log(LT_ERROR, "Signal received: SIGABRT (abort).");

    signal(sig, SIG_DFL);
    raise(sig);
}

epm_Result epm_InitSignals(void) {
    signal(SIGABRT, abort_signal_handler);

    signal(SIGFPE, program_error_signal_handler);
    signal(SIGILL, program_error_signal_handler);
    signal(SIGSEGV, program_error_signal_handler);
#ifdef SIGBUS
    signal(SIGBUS, program_error_signal_handler);
#endif
    
    signal(SIGINT, termination_signal_handler);
    signal(SIGTERM, termination_signal_handler);

    return EPM_SUCCESS;
}
