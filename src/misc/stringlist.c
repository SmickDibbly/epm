#include "src/misc/stringlist.h"

#define BLOCK_SIZE 16

void initialize_StringList(StringList *list) {
    if ( ! list) return;
    
    list->entries = zgl_Calloc(BLOCK_SIZE, sizeof(char *));
    list->capacity = BLOCK_SIZE;
    list->num_entries = 0;
}

void destroy_StringList(StringList *list) {
    for (size_t i = 0; i < list->num_entries; i++) {
        zgl_Free(list->entries[i]);
    }

    zgl_Free(list->entries);
}

void append_to_StringList(StringList *list, char const *str) {
    /* Do NOT call epm_Log from this function lest ye suffer infinite
       recursion. */
    if ( ! list || ! str) return;

    //size_t old_num_entries = list->num_entries;
    
    list->entries[list->num_entries] = zgl_Malloc(1 + strlen(str));
    strcpy(list->entries[list->num_entries], str);
    list->num_entries++;

    //printf("(num entries %zu -> %zu)\n", old_num_entries, list->num_entries);
    
    if (list->num_entries >= list->capacity) {
        //printf("(capacity %zu", list->capacity);
        list->entries = zgl_Realloc(list->entries, (list->capacity + BLOCK_SIZE)*sizeof(char *));
        memset(list->entries + list->capacity, 0, BLOCK_SIZE*sizeof(char *));
        list->capacity = list->capacity + BLOCK_SIZE;
        //printf(" -> %zu)\n", list->capacity);
    }
}

void replace_StringList_head(StringList *list, char const *str) {
    if ( ! list || ! str) return;

    size_t idx = list->num_entries-1;
    list->entries[idx] = zgl_Realloc(list->entries[idx], 1 + strlen(str));
    strcpy(list->entries[idx], str);
}
