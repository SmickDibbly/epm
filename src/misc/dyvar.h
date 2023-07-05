#ifndef DYVAR_H
#define DYVAR_H

#include "src/misc/epm_includes.h"

typedef enum epm_DyVarType {
    TYPE_UNKNOWN,
    TYPE_STRING,
    TYPE_INT,
    TYPE_UINT,
    TYPE_FLOAT,
    TYPE_DOUBLE,

    NUM_TYPE
} epm_DyVarType;

typedef struct epm_DyVar {
    char id[256];

    epm_DyVarType type;
    
    union {
        int64_t i;
        double f;
        char *str;
    } data;
} epm_DyVar;

#endif /* DYVAR_H */
