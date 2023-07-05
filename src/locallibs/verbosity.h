#ifdef VERBOSITY

/* Include this header file into a source file to gain access to the "vbs_"
   family of macros. If VERBOSITY is defined when you include this header, these
   macros are thin wrappers around the stdio.h printing functions. If
   VERBOSITY is NOT defined when you include this header, these macros
   expand to nothing.

   The purpose is to allow you to print messages that can be easily stripped
   from compilation when desired by simply undefining a preprocessing
   token. Useful for debugging / sanity checks.
*/

#include <stdio.h>

# define vbs_fprintf(...) fprintf(__VA_ARGS__)
# define vbs_vfprintf(STREAM, FMT, ARGS) vfprintf((STREAM), (FMT), (ARGS))

# define vbs_printf(...) printf(__VA_ARGS__)
# define vbs_vprintf(FMT, ARGS) vprintf((FMT), (ARGS))

# define vbs_sprintf(...) sprintf(__VA_ARGS__)
# define vbs_vsprintf(STR, FMT, ARGS) vsprintf((STR), (FMT), (ARGS))

# define vbs_snprintf(...) snprintf(__VA_ARGS__)
# define vbs_vsnprintf(STR, LEN, FMT, ARGS) vsnprintf((STR), (LEN), (FMT), (ARGS))

# define vbs_fputs(STR, STREAM) fputs((STR), (STREAM))
# define vbs_puts(STR) puts((STR))
# define vbs_putchar(CH) putchar((CH))

#else

# define vbs_fprintf(...) (void)0
# define vbs_vfprintf(STREAM, FMT, ARGS) (void)0

# define vbs_printf(...) (void)0
# define vbs_vprintf(FMT, ARGS) (void)0

# define vbs_sprintf(...) (void)0
# define vbs_vsprintf(STR, FMT, ARGS) (void)0

# define vbs_snprintf(...) (void)0
# define vbs_vsnprintf(STR, LEN, FMT, ARGS) (void)0

# define vbs_fputs(STR, STREAM) (void)0
# define vbs_puts(STR) (void)0
# define vbs_putchar(CH) (void)0

#endif
