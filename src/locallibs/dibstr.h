#ifndef DIBSTR_H
#define DIBSTR_H

#include <string.h>
#include <stdbool.h>

#define streq(STR0, STR1) (strcmp((STR0), (STR1)) == 0)

static inline bool prefix(const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

static inline bool suffix(const char *suf, const char *str) {
    return strncmp(suf, str+strlen(str)-strlen(suf), strlen(suf)) == 0;
}

extern void puts_box(char const *str);
extern void puts_underline(char const *str);


extern long str_to_subrange(const char *s, char **endptr, int base, long min, long max);

#include <stdint.h>
#define str_to_i8(P_STR, PP_ENDPTR, BASE) \
    (int8_t)str_to_subrange((P_STR), (PP_ENDPTR), (BASE), INT8_MIN, INT8_MAX)

#define str_to_i16(P_STR, PP_ENDPTR, BASE) \
    (int16_t)str_to_subrange((P_STR), (PP_ENDPTR), (BASE), INT16_MIN, INT16_MAX)

#define str_to_i32(P_STR, PP_ENDPTR, BASE) \
    (int32_t)str_to_subrange((P_STR), (PP_ENDPTR), (BASE), INT32_MIN, INT32_MAX)

#define str_to_i64(P_STR, PP_ENDPTR, BASE) \
    (int64_t)str_to_subrange((P_STR), (PP_ENDPTR), (BASE), INT64_MIN, INT64_MAX)


#endif /* DIBSTR_H */
