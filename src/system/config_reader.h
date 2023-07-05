#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include "zigil/zigil_keycodes.h"
#include "src/misc/epm_includes.h"

extern char *get_config_str(char const *group, char *key);

extern epm_Result read_config(char const *configfile);

typedef struct ConfigMapEntry {
    char *query;
    void *var;
} ConfigMapEntry;

extern void read_config_data
(char const *group,
 size_t num_queries,
 ConfigMapEntry const queries[],
 void (*str_to_var)(void *, char const *));

#endif /* CONFIG_READER_H */
