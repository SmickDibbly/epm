#ifndef COMMAND_H
#define COMMAND_H

#include "src/misc/epm_includes.h"

// Think of a command as a "String Input" as contrasted with "Key Input" or
// "Mouse Input"

typedef struct epm_Command {
    char const *const name;
    uint8_t argc_min;
    uint8_t argc_max;
    void (*handler)(int argc, char **argv, char *output_str);
} epm_Command;

extern void epm_SubmitCommandString(char const *in_cmd_str, char *output_str);

#endif /* COMMAND_H */
