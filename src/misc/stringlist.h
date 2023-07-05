#ifndef STRINGLIST_H
#define STRINGLIST_H

#include "src/misc/epm_includes.h"

typedef struct StringList {
    size_t num_entries;
    size_t capacity;
    char **entries;
} StringList;

void initialize_StringList(StringList *list);
void destroy_StringList(StringList *list);
void append_to_StringList(StringList *list, char const *str);
void replace_StringList_head(StringList *list, char const *str);

#endif /* STRINGLIST_H */
