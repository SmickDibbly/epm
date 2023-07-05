#ifndef TERM_COLORS_H
#define TERM_COLORS_H

#define RESET  "\x1B[0m"

#define KBOLD          "\x1B[1m"
#define KFAINT         "\x1B[2m"
#define KITALIC        "\x1B[3m"
#define KUNDERLINE     "\x1B[4m"
#define KSLOW_BLINK    "\x1B[5m"
#define KNO_BOLD       "\x1B[22m"
#define KNO_FAINT      "\x1B[22m"
#define KNO_ITALIC     "\x1B[23m"
#define KNO_UNDERLINE  "\x1B[24m"
#define KNO_SLOW_BLINK "\x1B[25m"

#define FBOLD(x)          KBOLD  x KNO_BOLD
#define FFAINT(x)         KFAINT  x KNO_FAINT
#define FITALIC(x)        KITALIC  x KNO_ITALIC
#define FUNDERLINE(x)     KUNDERLINE  x KNO_UNDERLINE
#define FSLOW_BLINK(x)    KSLOW_BLINK  x KNO_SLOW_BLINK

/* FOREGROUND COLOR */
#define KBLACK   "\x1B[30m"
#define KRED     "\x1B[31m"
#define KGREEN   "\x1B[32m"
#define KYELLOW  "\x1B[33m"
#define KBLUE    "\x1B[34m"
#define KMAGENTA "\x1B[35m"
#define KCYAN    "\x1B[36m"
#define KWHITE   "\x1B[37m"

#define KGRAY           "\x1B[90m"
#define KBRIGHT_RED     "\x1B[91m"
#define KBRIGHT_GREEN   "\x1B[92m"
#define KBRIGHT_YELLOW  "\x1B[93m"
#define KBRIGHT_BLUE    "\x1B[94m"
#define KBRIGHT_MAGENTA "\x1B[95m"
#define KBRIGHT_CYAN    "\x1B[96m"
#define KBRIGHT_WHTIE   "\x1B[97m"

#define KDEFAULT_COLOR "\x1B[39m"

#define FRED(x) KRED x KDEFAULT_COLOR
#define FGREEN(x) KGREEN x KDEFAULT_COLOR
#define FYELLOW(x) KYELLOW x KDEFAULT_COLOR
#define FBLUE(x) KBLUE x KDEFAULT_COLOR
#define FMAGENTA(x) KMAGENTA x KDEFAULT_COLOR
#define FCYAN(x) KCYAN x KDEFAULT_COLOR
#define FWHITE(x) KWHITE x KDEFAULT_COLOR

// TODO: KDEFAULT_COLOR has no effect for the "bright" colors
#define FGRAY(x) KGRAY x RESET
#define FBRIGHT_RED(x) KBRIGHT_RED x RESET
#define FBRIGHT_GREEN(x) KBRIGHT_GREEN x RESET
#define FBRIGHT_YELLOW(x) KBRIGHT_YELLOW x RESET
#define FBRIGHT_BLUE(x) KBRIGHT_BLUE x RESET
#define FBRIGHT_MAGENTA(x) KBRIGHT_MAGENTA x RESET
#define FBRIGHT_CYAN(x) KBRIGHT_CYAN x RESET
#define FBRIGHT_WHITE(x) KBRIGHT_WHITE x RESET


#endif
