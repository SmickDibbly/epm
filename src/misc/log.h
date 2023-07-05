#ifndef LOG_H
#define LOG_H

#include "src/misc/std_includes.h"
#include "src/misc/epm_includes.h"

/* The log is for messages that should be posted regardless of build type */

//#define EPM_USE_COLORS_IN_STDOUT

typedef enum epm_LogGroup {
    LG_GENERAL   = 0,
    LG_INIT  = 1,
    LG_TERM  = 2,
    
    NUM_LG
} epm_LogGroup;

typedef enum epm_LogType {
    LT_INFO = 0,
    LT_WARN = 1,
    LT_ERROR = 2,
    LT_FATAL = 3,

    NUM_LT
} epm_LogType;

extern epm_Result set_active_log_group(epm_LogGroup lg);

extern epm_Result _epm_Log(char const *label, epm_LogType lt, ...);

#define LOG_LABEL "MISC"

// need to hide the format string in the "..." to satify C99 requirement that at least one argument is supplied for the variadic part of a function-like macro call.
#define epm_Log(LT, ...) _epm_Log(LOG_LABEL, LT, __VA_ARGS__)

#define MAX_UTC_LEN 31
#define MAX_LOGGROUP_LEN 31
#define MAX_LOGTYPE_LEN 31
#define MAX_FORMATTED_LEN 2047
#define MAX_LOGSTR_LEN (1 + MAX_UTC_LEN + 2 + MAX_LOGGROUP_LEN + 1 + MAX_LOGTYPE_LEN + 2 + MAX_FORMATTED_LEN)
// [%s] %s.%s: %s

#endif /* LOG_H */
