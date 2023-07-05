#include <stdio.h>

#include "dibstr.h"

static char const *eighty_dashes = "--------------------------------------------------------------------------------";

void puts_box(char const *str) {
    int len = (int)strlen(str);
    printf("+%.*s+\n"
           "| %s |\n"
           "+%.*s+\n",
           len+2, eighty_dashes, str, len+2, eighty_dashes);
}

void puts_underline(char const *str) {
    printf(" %s \n"
           "+%.*s+\n",
           str, (int)strlen(str), eighty_dashes);
}


#include <errno.h>
#include <stdlib.h>

long str_to_subrange(const char *s, char **endptr, int base, long min, long max) {
  long y = strtol(s, endptr, base);
  if (y > max) {
    errno = ERANGE;
    return max;
  }
  if (y < min) {
    errno = ERANGE;
    return min;
  }
  return y;
}
