#include <stdarg.h>
#include <time.h>

#include "src/misc/epm_includes.h"
#include "src/system/dir.h"
#include "src/misc/stringlist.h"

char bigbuf[BIGBUF_LEN+1] = {0};

#define EPM_USE_COLORS_IN_STDOUT
#ifdef EPM_USE_COLORS_IN_STDOUT
# include "term_colors.h"
#endif

#define MAX_STREAMS_PER_LOG 4
StringList loglist = {0, 0, NULL};

static const char gen_filename[FILENAME_MAX] = DIR_LOG "log.txt";
static const char init_filename[FILENAME_MAX] = DIR_LOG "log_init.txt";
static const char term_filename[FILENAME_MAX] = DIR_LOG "log_term.txt";

static FILE *gen_fp;
static FILE *init_fp;
static FILE *term_fp;
static FILE *active_fp;

#ifdef EPM_USE_COLORS_IN_STDOUT
static char const *LT_colored_strs[] = {
    [LT_INFO]  = FGREEN("INFO"),
    [LT_WARN]  = FYELLOW("WARN"),
    [LT_ERROR] = FRED("ERROR"),
    [LT_FATAL] = FRED("FATAL"),
};
#endif

static char const *LT_strs[] = {
    [LT_INFO]  = "INFO",
    [LT_WARN]  = "WARN" ,
    [LT_ERROR] = "ERROR",
    [LT_FATAL] = "FATAL",
};


static time_t current_time;
static struct tm *utc_time;
static char utc_str[MAX_UTC_LEN + 1] = {'\0'};

epm_Result epm_InitLog(void) {
    initialize_StringList(&loglist);
    
    gen_fp = fopen(gen_filename, "w");
    if ( ! gen_fp)
        printf("WARNING: Could not open log file \"%s\".\n", gen_filename);
    
    init_fp = fopen(init_filename, "w");
    if ( ! init_fp)
        printf("WARNING: Could not open log file \"%s\".\n", init_filename);
    
    term_fp = fopen(term_filename, "w");
    if ( ! term_fp)
        printf("WARNING: Could not open log file \"%s\".\n", term_filename);
    
    set_active_log_group(LG_INIT);

    return EPM_SUCCESS;
}

epm_Result epm_TermLog(void) {    
    if (gen_fp)
        fclose(gen_fp);
    if (init_fp)
        fclose(init_fp);
    if (term_fp)
        fclose(term_fp);

    destroy_StringList(&loglist);
    
    return EPM_SUCCESS;
}

#ifdef EPM_USE_COLORS_IN_STDOUT
static char colored_logstr[MAX_LOGSTR_LEN + 1];
#endif
static char logstr[MAX_LOGSTR_LEN + 1];
static char formatted[MAX_FORMATTED_LEN + 1];

epm_Result _epm_Log(char const *label, epm_LogType lt, ...) {
    va_list ap;
    va_start(ap, lt);
    char const *fmt = va_arg(ap, char const *);
    vsnprintf(formatted, MAX_FORMATTED_LEN, fmt, ap);
    va_end(ap);
    
    current_time = time(NULL);
    utc_time = gmtime(&current_time);
    strftime(utc_str, MAX_UTC_LEN, "%Y-%m-%d %H:%M:%S", utc_time);

#ifdef EPM_USE_COLORS_IN_STDOUT
    snprintf(colored_logstr, MAX_LOGSTR_LEN, "[%s] %s.%s: %s", utc_str, label, LT_colored_strs[lt], formatted);
#endif
    snprintf(logstr, MAX_LOGSTR_LEN, "[%s] %s.%s: %s", utc_str, label, LT_strs[lt], formatted);

    /* To stdout, without timestamp. */
#ifdef EPM_USE_COLORS_IN_STDOUT
    fputs(colored_logstr+strlen(utc_str)+3, stdout);
    fputc('\n', stdout);
    fflush(stdout);
#else
    fputs(logstr+strlen(utc_str)+3, stdout);
    fputc('\n', stdout);
    fflush(stdout);
#endif

    /* To active filestream if available, with timestamp. */
    if (active_fp) {
        fputs(logstr, active_fp);
        fputc('\n', active_fp);
        fflush(active_fp);
    }

    /* To in-memory log history, without timestamp. */
    append_to_StringList(&loglist, logstr+strlen(utc_str)+3);
    
    return EPM_SUCCESS;    
}


extern epm_Result set_active_log_group(epm_LogGroup lg) {
    switch (lg) {
    case LG_INIT:
        active_fp = init_fp;
        break;
    case LG_TERM:
        active_fp = term_fp;
        break;
    case LG_GENERAL:
        active_fp = gen_fp;
        break;
    default:
        break;
    }
    
    return EPM_SUCCESS;
}
