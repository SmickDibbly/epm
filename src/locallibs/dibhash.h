#ifndef DIBHASH_H
#define DIBHASH_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

extern void print_all_HashTable(void);
extern char *print_all_HashTable_to_str(void);
extern void destroy_all_HashTable(void);

/* -------------------------------------------------------------------------- */

// +------------+
// |   IDList   |
// +------------+

// Key type: uint64_t
// Val type: size_t
// Open-addressing.

// Like an Integer Dictionary, but the values are implicit.

// PROBLEM: Rehashing necessarily changes the "values", so this is best used for
// data of a fixed size.

typedef uint64_t IDListKey;
typedef struct IDList IDList;

extern IDList *create_IDList(size_t num_inputs, IDListKey inputs[],
                             char const *opt_name, uint64_t seed);
extern void destroy_IDList(IDList *list);
extern bool IDList_lookup(IDList *list, IDListKey key, size_t *out_val);
extern bool IDList_insert(IDList *list, IDListKey key, size_t *out_val);
extern bool IDList_delete(IDList *list, IDListKey key, size_t *out_val);



/* -------------------------------------------------------------------------- */

// +------------------------------------------+
// | UIntMap32 (Integer-to-Integer Dictionary) |
// +------------------------------------------+

// Key type: uint32_t
// Val type: uint32_t
// Open-addressing.

typedef struct UInt32Pair {
    uint32_t key;
    uint32_t val;
} UInt32Pair;

typedef struct UIntMap32 UIntMap32;

extern UIntMap32 *create_UIntMap32(size_t num_inputs, UInt32Pair inputs[],
                                   char const *opt_name, uint32_t seed);
extern void destroy_UIntMap32(UIntMap32 *map);
extern bool UIntMap32_lookup(UIntMap32 *map, uint32_t key, uint32_t *out_val);
extern bool UIntMap32_insert(UIntMap32 *map, uint32_t key, uint32_t *out_val);
extern bool UIntMap32_delete(UIntMap32 *map, uint32_t key, uint32_t *out_val);
extern void constrain_max_lookup_UIntMap32(UIntMap32 *map, size_t desired_ll, uint32_t max_attempts);

extern void print_UIntMap32(UIntMap32 *map);

/* -------------------------------------------------------------------------- */

// +-------------------------------------------+
// | UIntMap64 (Integer-to-Integer Dictionary) |
// +-------------------------------------------+

// Key type: uint64_t
// Val type: uint64_t
// Open-addressing.

typedef struct UInt64Pair {
    uint64_t key;
    uint64_t val;
} UInt64Pair;

typedef struct UIntMap64 UIntMap64;

extern UIntMap64 *create_UIntMap64(size_t num_inputs, UInt64Pair inputs[],
                                   char const *opt_name, uint64_t seed);
extern void destroy_UIntMap64(UIntMap64 *map);
extern bool UIntMap64_lookup(UIntMap64 *map, uint64_t key, uint64_t *out_val);
extern bool UIntMap64_insert(UIntMap64 *map, uint64_t key, uint64_t *out_val);
extern bool UIntMap64_delete(UIntMap64 *map, uint64_t key, uint64_t *out_val);



/* -------------------------------------------------------------------------- */

// +--------------------------------------+
// | Index (String-to-Integer Dictionary) |
// +--------------------------------------+

// Key type: char * internal
// Val type: uint64_t
// Separate chaining.

typedef struct StrInt {
    char const *key;
    uint64_t val;
} StrInt;

typedef struct Index Index;

extern Index *create_Index(size_t num_inputs, StrInt inputs[],
                           char const *opt_name, uint64_t seed);
extern void destroy_Index(Index *idx);
extern bool Index_lookup(Index *idx, char const *key, uint64_t *out_val);
extern bool Index_insert(Index *idx, StrInt kvp);
extern bool Index_delete(Index *idx, char const *key, uint64_t *out_val);



/* -------------------------------------------------------------------------- */

// +------------+
// | Dictionary |
// +------------+

// Key type: char * internal
// Val type: char * internal
// Separate chaining.

typedef struct StrStr {
    char const *key;
    char const *val;
} StrStr;

typedef struct Dict Dict;
extern Dict *create_Dict(size_t num_inputs, StrStr inputs[],
                         char const *opt_name, uint64_t seed);
extern void destroy_Dict(Dict *dict);
extern char *Dict_lookup(Dict *dict, char const *key);
extern char *Dict_insert(Dict *dict, StrStr kvp);
extern char *Dict_delete(Dict *dict, char const *key);

#endif /* DIBHASH_H */
