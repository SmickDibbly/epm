#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <limits.h>

//#define VERBOSITY 1
#include "verbosity.h"
    

/* -------------------------------------------------------------------------- */

#include <malloc.h>
static int num_allocs = 0;
static long unsigned int alloc_bytes = 0;
void *_hash_Malloc(size_t size, char const *file, int line) {
    void *result = malloc(size);
    if ( ! result) {
        printf("ZIGIL.ERROR: Failed to allocate memory %s : %i.\n", file, line);
        exit(0);
    }

    num_allocs++;
    alloc_bytes += malloc_usable_size(result);
    return result;
}

void *_hash_Calloc(size_t nmemb, size_t size, char const *file, int line) {
    void *result = calloc(nmemb, size);
    if ( ! result) {
        printf("ZIGIL.ERROR: Failed to allocate memory %s : %i.\n", file, line);
        exit(0);
    }

    num_allocs++;
    alloc_bytes += malloc_usable_size(result);
    return result;
}

void *_hash_Realloc(void *ptr, size_t size, char const *file, int line) {
    alloc_bytes -= malloc_usable_size(ptr);
    void *result = realloc(ptr, size);
    if ( ! result) {
        printf("ZIGIL.ERROR: Failed to reallocate memory %s : %i.\n", file, line);
        exit(0);
    }
    alloc_bytes += malloc_usable_size(result);
    return result;
}

void _hash_Free(void *ptr, char const *file, int line) {
    (void)file, (void)line;
    alloc_bytes -= malloc_usable_size(ptr);
    free(ptr);

    num_allocs--;
}

void dibhash_print_mem_status(void) {
    printf(" Memory Status \n"
           "+-------------+\n");
    printf("Remaining unfreed: %i allocs; %lu bytes\n", num_allocs, alloc_bytes);
}

#define malloc(SIZE) _hash_Malloc((SIZE), __FILE__, __LINE__)
#define calloc(NMEMB, SIZE) _hash_Calloc((NMEMB), (SIZE), __FILE__, __LINE__)
#define realloc(PTR, SIZE) _hash_Realloc((PTR), (SIZE), __FILE__, __LINE__)
#define free(PTR) _hash_Free((PTR), __FILE__, __LINE__)


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#include "./hash_functions.h"
#include "./dibhash.h"

// +------+
// | Meta |
// +------+

typedef enum HashTableType {
    HT_IDLIST,
    HT_UINTMAP32,
    HT_UINTMAP64,
    HT_INDEX,
    HT_DICT
} HashTableType;

static void *create_HashTable(HashTableType type);
static void destroy_HashTable(HashTableType type, void *table);
static void HashTable_set_name(char **out_name, char const *in_name);

#define MAX_LOAD_FACTOR 0.7
#define REHASH_FACTOR 2u
#define MIN_LOAD_FACTOR (MAX_LOAD_FACTOR/(2.0 * (double)REHASH_FACTOR))

void print_summary_IDList(IDList *list);
void print_summary_UIntMap32(UIntMap32 *map);
void print_summary_UIntMap64(UIntMap64 *map);
void print_summary_Index(Index *idx);
void print_summary_Dict(Dict *dict);

double uniformity_IDList(IDList *list);
double uniformity_UIntMap32(UIntMap32 *map);
double uniformity_UIntMap64(UIntMap64 *map);
double uniformity_Index(Index *idx);
double uniformity_Dict(Dict *dict);

void measure_lookup_len_IDList(IDList *list, double *avg, size_t *max);
void measure_lookup_len_UIntMap32(UIntMap32 *map, double *avg, size_t *max);
void measure_lookup_len_UIntMap64(UIntMap64 *map, double *avg, size_t *max);
void measure_lookup_len_Index(Index *idx, double *avg, size_t *max);
void measure_lookup_len_Dict(Dict *dict, double *avg, size_t *max);

/* -------------------------------------------------------------------------- */

// +------------+
// |   IDList   |
// +------------+

// Key type: uint64_t
// Val type: size_t
// Open-addressing.

typedef struct IDListEntry {
    //uint8_t flags;
    //size_t next; // 0 if last of same hash; otherwise offset to next of same hash
    bool occupied;
    //size_t hash;
    IDListKey key;
} IDListEntry;

struct IDList {
    char *name;
    IDListKey seed;
    double load_factor;
    size_t num_occupied;
    size_t num_slots;
    IDListEntry *slots;
};

extern double IDList_uniformity(IDList *list);
extern void IDList_assert_accuracy(IDList *list);

static inline size_t probe_slot_triangular(size_t hash, size_t index) {
    return hash + ((index + (index*index))>>1);
}

static inline size_t probe_slot_trivial(size_t hash, size_t index) {
    return hash + index;
}

#define probe_slot(HASH, INDEX) probe_slot_trivial((HASH), (INDEX))


IDList *create_IDList(size_t num_inputs, IDListKey inputs[], char const *in_name, uint64_t seed) {
    double load_factor = 0;
    size_t num_occupied = 0;

    size_t num_slots = num_inputs;
    for (size_t i = 1; ; i *= 2) {
        if (num_slots <= i && (double)num_inputs/(double)i < MAX_LOAD_FACTOR) {
            num_slots = i;
            break;
        }
    }
    
    IDList *list = create_HashTable(HT_IDLIST);
    if ( ! list) return NULL;
    list->slots = calloc(num_slots, sizeof(*list->slots));
    if ( ! list->slots) {
        free(list);
        return NULL;
    }

    HashTable_set_name(&list->name, in_name);
    list->seed = seed;
    
    // load it up
    IDListEntry *slots = list->slots;
    for (size_t i_in = 0; i_in < num_inputs; i_in++) {
        size_t i_probe = 0;
        size_t hash = hash_u64(inputs[i_in], num_slots, seed);
        size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
        bool dup = false;
        while (slots[i_slot].occupied) {
            if (slots[i_slot].key == inputs[i_in]) {
                dup = true;
                break;
            }
            i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
        }
        if (dup) continue;

        slots[i_slot].key = inputs[i_in];
        slots[i_slot].occupied = true;
        num_occupied++;
    }
    load_factor = (double)num_occupied / (double)num_slots;
    
    list->num_slots = num_slots;
    list->num_occupied = num_occupied;
    list->load_factor = load_factor;

    assert(load_factor <= MAX_LOAD_FACTOR);
    if (num_occupied != 0) assert(MIN_LOAD_FACTOR <= load_factor);
    IDList_assert_accuracy(list);

#if VERBOSITY
    print_summary_IDList(list);
#endif
    
    return list;
}

static bool rehash_IDList(IDList *list, size_t new_num_slots, IDListKey new_seed) {
    size_t old_num_slots = list->num_slots;

    size_t old_num_occupied = list->num_occupied;
    size_t new_num_occupied = 0;

#if VERBOSITY
    double old_load_factor = list->load_factor;
#endif
    double new_load_factor = 0;

#if VERBOSITY
    double old_uniformity = IDList_uniformity(list);
    double new_uniformity = 0;
#endif
    
    IDListEntry *old_slots = list->slots;
    IDListEntry *new_slots = malloc(new_num_slots*sizeof(*new_slots));
    if ( ! new_slots) return false;

    // clear new
    memset(new_slots, 0, new_num_slots*sizeof(*new_slots));
    
    // load it up
    for (size_t i_old = 0; i_old < old_num_slots; i_old++) {
        if ( ! old_slots[i_old].occupied) // skip empties
            continue;
        size_t i_probe = 0;
        size_t hash = hash_u64(old_slots[i_old].key, new_num_slots, new_seed);
        size_t i_new = probe_slot(hash, i_probe++) & (new_num_slots - 1);

        while (new_slots[i_new].occupied) {
            // if all has worked as intended up to this point, there will be no
            // duplicates
            assert(new_slots[i_new].key != old_slots[i_old].key);
            i_new = probe_slot(hash, i_probe++) & (new_num_slots - 1);
        }
        new_slots[i_new].key = old_slots[i_old].key;
        new_slots[i_new].occupied = true;
        new_num_occupied++;
    }
    new_load_factor = (double)new_num_occupied / (double)new_num_slots;

    free(list->slots);
    list->seed = new_seed;
    list->slots = new_slots;
    assert(old_num_occupied == new_num_occupied);
    list->num_slots = new_num_slots;
    list->load_factor = new_load_factor;
#if VERBOSITY
    new_uniformity = IDList_uniformity(list);
#endif
    
    assert(new_load_factor <= MAX_LOAD_FACTOR);
    assert(MIN_LOAD_FACTOR <= new_load_factor);
    IDList_assert_accuracy(list);
    
    vbs_printf("Rehashed IDList (slots %zu -> %zu) (occupied %zu -> %zu) (load %f -> %f)\n"
               "                (unif. %f -> %f)\n",
               old_num_slots, new_num_slots,
               old_num_occupied, new_num_occupied,
               old_load_factor, new_load_factor,
               old_uniformity, new_uniformity);
    
    return true;
}

void destroy_IDList(IDList *list) {
    if ( ! list)
        return;

    free(list->name);
    free(list->slots);
    
    destroy_HashTable(HT_IDLIST, list);
}


bool IDList_lookup(IDList *list, IDListKey key, size_t *out_val) {
    /*
      If key is not present, return 0. If key is present, return the index plus
      one.
    */
    
    size_t num_slots = list->num_slots;
    IDListEntry *slots = list->slots;

    // find slot
    size_t i_probe = 0;
    size_t hash = hash_u64(key, num_slots, list->seed);
    size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    while (slots[i_slot].occupied && slots[i_slot].key != key)
        i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    
    if (slots[i_slot].occupied) {
        if (out_val) *out_val = i_slot;
        return true;
    }
    
    return false;
}

bool IDList_insert(IDList *list, IDListKey key, size_t *out_val) {
    /*
      If key is not present, insert it and return the index plus one.  If key is
      present, return the index plus one.
    */
    
    size_t num_slots = list->num_slots;
    IDListEntry *slots = list->slots;

    // find slot
    size_t i_probe = 0;
    size_t hash = hash_u64(key, num_slots, list->seed);
    size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    while (slots[i_slot].occupied && slots[i_slot].key != key)
        i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    
    if (slots[i_slot].occupied) {
        if (out_val) *out_val = i_slot;
        return false; // false means "not inserted (since already present)
    }
        

    // key not in table; insert it
    slots[i_slot].key = key;
    slots[i_slot].occupied = true;

    list->num_occupied++;
    list->load_factor = (double)list->num_occupied / (double)list->num_slots;

    /*
    vbs_printf("Insertion: (key %" PRIu64 ") (hash %5zu%s) (slot %5zu) (occupied %zu) (load %.4f)\n",
               key,
               hash,
               i_slot == hash ? "*" : " ",
               i_slot,
               list->num_occupied,
               list->load_factor);
    */
    
    if (list->load_factor > MAX_LOAD_FACTOR) {
        rehash_IDList(list, 2 * list->num_slots, list->seed);
    }
    
    if (out_val) *out_val = i_slot;
    
    return true; // true means "inserted (since not already present)"
}

bool IDList_delete(IDList *list, IDListKey key, size_t *out_val) {
    size_t num_slots = list->num_slots;
    IDListEntry *slots = list->slots;

    // find slot
    size_t i_probe = 0;
    size_t hash = hash_u64(num_slots, key, list->seed);
    size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    while (slots[i_slot].occupied && slots[i_slot].key != key)
        i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);

    if ( ! slots[i_slot].occupied) {
        //vbs_printf("Key %" PRIu64 " not found; no deletion.\n", key);
        return false;
    }
    
    list->num_occupied--;
    slots[i_slot].occupied = false;
    list->load_factor = (double)list->num_occupied / (double)list->num_slots;
    
    // now i_slot is index where match was found. We now need to continue,
    // backshifting any more slots with matching keyhash until we reach an
    // empty cell.
    size_t i = i_slot;
    size_t j = i;
    size_t k;
    /* IMPORTANT: This deletion handling pattern is only verified for trivial
       linear probing, that is: probe_slot(hash, i_probe) = hash + i_probe

       TODO: How to handle other probing */

    IDListKey seed = list->seed;
    while (true) {
        j = probe_slot(hash, i_probe++) & (num_slots - 1);
        if ( ! slots[j].occupied)
            break;

        k = hash_u64(slots[j].key, num_slots, seed);
        if (i <= j) {
            if (i < k && k <= j) {
                continue;
            }
        }
        else {
            if (i < k || k <= j) {
                continue;
            }
        }
        slots[i] = slots[j];
        slots[j].occupied = false;
        i = j;
    }

    /*
    vbs_printf("Deletion: (key %" PRIu64 ") (hash %5zu%s) (slot %5zu) (occupied %zu) (load %.4f)\n",
               key,
               hash,
               i_slot == hash ? "*" : " ",
               i_slot,
               list->num_occupied,
               list->load_factor);
    */
    
    if (list->load_factor < MIN_LOAD_FACTOR) {
        if (list->num_slots / 2 > 2) {
            // can't guarantee load factor within acceptable bounds when the
            // num_slots gets low. But with such small numbers speed isn't really an
            // issue, so let's not bother rehashing down to smaller than 3
            // buckets.
            rehash_IDList(list, list->num_slots / 2, list->seed);
        }
    }

    if (out_val) *out_val = i_slot;
    return true;
}

void IDList_assert_accuracy(IDList *list) {
    size_t num_occupied = 0;

    for (size_t i_slot = 0; i_slot < list->num_slots; i_slot++) {
        num_occupied += list->slots[i_slot].occupied ? 1 : 0;
    }

    assert(num_occupied == list->num_occupied);
    assert(((double)list->load_factor - (double)list->num_occupied/(double)list->num_slots) < 0.00001);
   
}

/* -------------------------------------------------------------------------- */

// +-------------------------------------------+
// | UIntMap32 (Integer-to-Integer Dictionary) |
// +-------------------------------------------+

// Key type: uint32_t
// Val type: uint32_t
// Open-addressing.

typedef uint32_t UIntMap32_Key;
typedef uint32_t UIntMap32_Val;

typedef struct UIntMap32_Entry {
    //uint8_t flags;
    //size_t next; // 0 if last of same hash; otherwise offset to next of same hash
    bool occupied;
    //size_t hash;
    UIntMap32_Key key;
    UIntMap32_Val val;
} UIntMap32_Entry;

struct UIntMap32 {
    char *name;
    UIntMap32_Key seed;
    double load_factor;
    size_t num_occupied;
    size_t num_slots;
    UIntMap32_Entry *slots;
};

extern UIntMap32 *create_UIntMap32(size_t num_inputs, UInt32Pair inputs[],
                                   char const *opt_name, uint32_t seed);
extern void destroy_UIntMap32(UIntMap32 *map);
extern bool UIntMap32_lookup(UIntMap32 *map, uint32_t key, uint32_t *out_val);
extern bool UIntMap32_insert(UIntMap32 *map, uint32_t key, uint32_t *out_val);
extern bool UIntMap32_delete(UIntMap32 *map, uint32_t key, uint32_t *out_val);
extern double UIntMap32_uniformity(UIntMap32 *map);

UIntMap32 *create_UIntMap32(size_t num_inputs, UInt32Pair inputs[], char const *in_name, uint32_t seed) {
    double  load_factor = 0;
    size_t num_occupied = 0;
    
    size_t num_slots = num_inputs;
    for (size_t i = 1; ; i *= 2) {
        if (num_slots <= i && (double)num_inputs/(double)i < MAX_LOAD_FACTOR) {
            num_slots = i;
            break;
        }
    }

    UIntMap32 *map = create_HashTable(HT_UINTMAP32);
    if ( ! map) return NULL;
    map->slots = calloc(num_slots, sizeof(*map->slots));
    if ( ! map->slots) {
        free(map);
        return NULL;
    }

    HashTable_set_name(&map->name, in_name);
    map->seed = seed;
    
    // load it up
    UIntMap32_Entry *slots = map->slots;
    for (size_t i_in = 0; i_in < num_inputs; i_in++) {
        size_t i_probe = 0;
        size_t hash = hash_u32(inputs[i_in].key, num_slots, seed);
        size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
        bool dup = false;
        while (slots[i_slot].occupied) {
            if (slots[i_slot].key == inputs[i_in].key) {
                slots[i_slot].val = inputs[i_in].val;
                dup = true;
                break;
            }
            i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
        }
        if (dup) continue;

        slots[i_slot].key = inputs[i_in].key;
        slots[i_slot].val = inputs[i_in].val;
        slots[i_slot].occupied = true;
        num_occupied++;
    }
    load_factor = (double)num_occupied / (double)num_slots;
    
    map->num_slots = num_slots;
    map->num_occupied = num_occupied;
    map->load_factor = load_factor;

    assert(load_factor <= MAX_LOAD_FACTOR);
    if (num_occupied != 0) assert(MIN_LOAD_FACTOR <= load_factor);
    //IDMap_assert_accuracy(map);
    
#if VERBOSITY
    print_summary_UIntMap32(map);
#endif
    
    return map;
}

static bool rehash_UIntMap32(UIntMap32 *map, size_t new_num_slots, uint32_t new_seed) {
    size_t old_num_slots = map->num_slots;
    //size_t old_seed = map->seed;
    
    size_t old_num_occupied = map->num_occupied;
    size_t new_num_occupied = 0;

#if VERBOSITY
    double old_load_factor = map->load_factor;
#endif
    double new_load_factor = 0;

#if VERBOSITY
    double old_uniformity = UIntMap32_uniformity(map);
    double new_uniformity = 0;
#endif
    
    UIntMap32_Entry *old_slots = map->slots;
    UIntMap32_Entry *new_slots = malloc(new_num_slots*sizeof(*new_slots));
    if ( ! new_slots) return false;

    // clear new
    memset(new_slots, 0, new_num_slots*sizeof(*new_slots));
    
    // load it up
    for (size_t i_old = 0; i_old < old_num_slots; i_old++) {
        if ( ! old_slots[i_old].occupied) // skip empties
            continue;
        size_t i_probe = 0;
        size_t hash = hash_u32(old_slots[i_old].key, new_num_slots, new_seed);
        size_t i_new = probe_slot(hash, i_probe++) & (new_num_slots - 1);

        while (new_slots[i_new].occupied) {
            // if all has worked as intended up to this point, there will be no
            // duplicates
            assert(new_slots[i_new].key != old_slots[i_old].key);
            i_new = probe_slot(hash, i_probe++) & (new_num_slots - 1);
        }
        new_slots[i_new].key = old_slots[i_old].key;
        new_slots[i_new].val = old_slots[i_old].val;
        new_slots[i_new].occupied = true;
        new_num_occupied++;
    }
    new_load_factor = (double)new_num_occupied / (double)new_num_slots;

    free(map->slots);
    map->seed = new_seed;
    map->slots = new_slots;
    assert(old_num_occupied == new_num_occupied);
    map->num_slots = new_num_slots;
    map->load_factor = new_load_factor;
#if VERBOSITY
    new_uniformity = UIntMap32_uniformity(map);
#endif
    
    assert(new_load_factor <= MAX_LOAD_FACTOR);
    assert(MIN_LOAD_FACTOR <= new_load_factor);
    //IntMap_assert_accuracy(map);
    
    vbs_printf("Rehashed IntMap (slots %zu -> %zu) (occupied %zu -> %zu) (load %f -> %f)\n"
               "                (unif. %f -> %f)\n",
               old_num_slots, new_num_slots,
               old_num_occupied, new_num_occupied,
               old_load_factor, new_load_factor,
               old_uniformity, new_uniformity);
    
    return true;
}

void destroy_UIntMap32(UIntMap32 *map) {
    if ( ! map)
        return;

    free(map->name);
    free(map->slots);

    destroy_HashTable(HT_UINTMAP32, map);
}


bool UIntMap32_lookup(UIntMap32 *map, uint32_t key, uint32_t *out_val) {
    /*
      If key is not present, return 0. If key is present, return the index plus
      one.
    */
    
    size_t num_slots = map->num_slots;
    UIntMap32_Entry *slots = map->slots;

    // find slot
    size_t i_probe = 0;
    size_t hash = hash_u32(key, num_slots, map->seed);
    size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    while (slots[i_slot].occupied && slots[i_slot].key != key)
        i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    
    if (slots[i_slot].occupied) {
        if (out_val) *out_val = slots[i_slot].val;
        return true;
    }
    
    return false;
}

bool UIntMap32_insert(UIntMap32 *map, uint32_t key, uint32_t *out_val) {
    /*
      If key is not present, insert it and return the index plus one.  If key is
      present, return the index plus one.
    */
    
    size_t num_slots = map->num_slots;
    UIntMap32_Entry *slots = map->slots;

    // find slot
    size_t i_probe = 0;
    size_t hash = hash_u32(key, num_slots, map->seed);
    size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    while (slots[i_slot].occupied && slots[i_slot].key != key)
        i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    
    if (slots[i_slot].occupied) {
        if (out_val) *out_val = slots[i_slot].val;
        return false; // false means "not inserted (since already present)
    }
        

    // key not in table; insert it
    slots[i_slot].key = key;
    slots[i_slot].occupied = true;

    map->num_occupied++;
    map->load_factor = (double)map->num_occupied / (double)map->num_slots;

    /*
    vbs_printf("Insertion: (key %" PRIu32 ") (hash %5zu%s) (slot %5zu) (occupied %zu) (load %.4f)\n",
               key,
               hash,
               i_slot == hash ? "*" : " ",
               i_slot,
               map->num_occupied,
               map->load_factor);
    */
    
    if (map->load_factor > MAX_LOAD_FACTOR) {
        rehash_UIntMap32(map, 2 * map->num_slots, map->seed);
    }
    
    if (out_val) *out_val = slots[i_slot].val;
    
    return true; // true means "inserted (since not already present)"
}

bool UIntMap32_delete(UIntMap32 *map, uint32_t key, uint32_t *out_val) {
    size_t num_slots = map->num_slots;
    UIntMap32_Entry *slots = map->slots;

    // find slot
    size_t i_probe = 0;
    size_t hash = hash_u32(key, num_slots, map->seed);
    size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    while (slots[i_slot].occupied && slots[i_slot].key != key)
        i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);

    if ( ! slots[i_slot].occupied) {
        //vbs_printf("Key %" PRIu32 " not found; no deletion.\n", key);
        return false;
    }
    
    map->num_occupied--;
    slots[i_slot].occupied = false;
    map->load_factor = (double)map->num_occupied / (double)map->num_slots;
    
    // now i_slot is index where match was found. We now need to continue,
    // backshifting any more slots with matching keyhash until we reach an
    // empty cell.
    size_t i = i_slot;
    size_t j = i;
    size_t k;
    UIntMap32_Key seed = map->seed;
    /* IMPORTANT: This deletion handling pattern is only verified for trivial
       linear probing, that is: probe_slot(hash, i_probe) = hash + i_probe

       TODO: How to handle other probing */
    while (true) {
        j = probe_slot(hash, i_probe++) & (num_slots - 1);
        if ( ! slots[j].occupied)
            break;

        k = hash_u32(slots[j].key, num_slots, seed);
        if (i <= j) {
            if (i < k && k <= j) {
                continue;
            }
        }
        else {
            if (i < k || k <= j) {
                continue;
            }
        }
        slots[i] = slots[j];
        slots[j].occupied = false;
        i = j;
    }

    /*
    vbs_printf("Deletion: (key %" PRIu32 ") (hash %5zu%s) (slot %5zu) (occupied %zu) (load %.4f)\n",
               key,
               hash,
               i_slot == hash ? "*" : " ",
               i_slot,
               map->num_occupied,
               map->load_factor);
    */
    
    if (map->load_factor < MIN_LOAD_FACTOR) {
        if (map->num_slots / 2 > 2) {
            // can't guarantee load factor within acceptable bounds when the
            // num_slots gets low. But with such small numbers speed isn't really an
            // issue, so let's not bother rehashing down to smaller than 3
            // buckets.
            rehash_UIntMap32(map, map->num_slots / 2, map->seed);
        }
    }

    if (out_val) *out_val = slots[i_slot].val;
    return true;
}

void print_UIntMap32(UIntMap32 *map) {
    for (size_t i_slot = 0; i_slot < map->num_slots; i_slot++) {
        if (map->slots[i_slot].occupied) {
            printf("(key %"PRIu32") (hash %"PRIu32") (slot %zu) (val %"PRIu32")\n",
                   map->slots[i_slot].key,
                   hash_u32(map->slots[i_slot].key, map->num_slots, map->seed),
                   i_slot,
                   map->slots[i_slot].val);
        }
    }
}

void constrain_max_lookup_UIntMap32(UIntMap32 *map, size_t desired_ll, uint32_t max_attempts) {
    size_t ll;
    double avg;
    
    size_t optimal_ll;
    uint32_t seed_of_optimal_ll;
    double avg_lookup_len_of_optimal_ll;
    
    rehash_UIntMap32(map, map->num_slots, map->seed);
    measure_lookup_len_UIntMap32(map, &avg_lookup_len_of_optimal_ll, &ll);
    optimal_ll = ll;
    seed_of_optimal_ll = map->seed;
    
    for (uint32_t i = 1; i < max_attempts; i++) {
        rehash_UIntMap32(map, map->num_slots, map->seed + 1);
        
        measure_lookup_len_UIntMap32(map, &avg, &ll);
        if (ll < optimal_ll) {
            optimal_ll = ll;
            seed_of_optimal_ll = map->seed;
            avg_lookup_len_of_optimal_ll = avg;
            if (optimal_ll <= desired_ll) {
                break;
            }
        }
        else if (ll == optimal_ll) {
            if (avg < avg_lookup_len_of_optimal_ll) {
                optimal_ll = ll;
                seed_of_optimal_ll = map->seed;
                avg_lookup_len_of_optimal_ll = avg;
                if (optimal_ll <= desired_ll) {
                    break;
                }
            }
        }

        if (0 == (i & ((1<<18)-1))) {
            printf("    Attempt %u    \n"
                   "+----------------+\n"
                   "          Optimal| %zu\n"
                   "             Seed| %"PRIu32"\n"
                   "   Avg Lookup Len| %lf\n\n",
                   i,
                   optimal_ll,
                   seed_of_optimal_ll,
                   avg_lookup_len_of_optimal_ll);
        }
    }

    rehash_UIntMap32(map, map->num_slots, seed_of_optimal_ll);
    print_summary_UIntMap32(map);
}

/* -------------------------------------------------------------------------- */

// +-------------------------------------------+
// | UIntMap64 (Integer-to-Integer Dictionary) |
// +-------------------------------------------+

// Key type: uint64_t
// Val type: uint64_t
// Open-addressing.

typedef struct UIntMap64_Entry {
    //uint8_t flags;
    //size_t next; // 0 if last of same hash; otherwise offset to next of same hash
    bool occupied;
    //size_t hash;
    uint64_t key;
    uint64_t val;
} UIntMap64_Entry;

struct UIntMap64 {
    char *name;
    uint64_t seed;
    double load_factor;
    size_t num_occupied;
    size_t num_slots;
    UIntMap64_Entry *slots;
};

extern UIntMap64 *create_UIntMap64(size_t num_inputs, UInt64Pair inputs[], char const *opt_name, uint64_t seed);
extern void destroy_UIntMap64(UIntMap64 *map);
extern bool UIntMap64_lookup(UIntMap64 *map, uint64_t key, uint64_t *out_val);
extern bool UIntMap64_insert(UIntMap64 *map, uint64_t key, uint64_t *out_val);
extern bool UIntMap64_delete(UIntMap64 *map, uint64_t key, uint64_t *out_val);
extern double UIntMap64_uniformity(UIntMap64 *map);

UIntMap64 *create_UIntMap64(size_t num_inputs, UInt64Pair inputs[], char const *in_name, uint64_t seed) {
    double  load_factor = 0;
    size_t num_occupied = 0;

    size_t num_slots = num_inputs;
    for (size_t i = 1; ; i *= 2) {
        if (num_slots <= i && (double)num_inputs/(double)i < MAX_LOAD_FACTOR) {
            num_slots = i;
            break;
        }
    }

    UIntMap64 *map = create_HashTable(HT_UINTMAP64);
    if ( ! map) return NULL;
    map->slots = calloc(num_slots, sizeof(*map->slots));
    if ( ! map->slots) {
        free(map);
        return NULL;
    }

    HashTable_set_name(&map->name, in_name);
    map->seed = seed;
    
    // load it up
    UIntMap64_Entry *slots = map->slots;
    for (size_t i_in = 0; i_in < num_inputs; i_in++) {
        size_t i_probe = 0;
        size_t hash = hash_u64(num_slots, inputs[i_in].key, seed);
        size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
        bool dup = false;
        while (slots[i_slot].occupied) {
            if (slots[i_slot].key == inputs[i_in].key) {
                slots[i_slot].val = inputs[i_in].val;
                dup = true;
                break;
            }
            i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
        }
        if (dup) continue;

        slots[i_slot].key = inputs[i_in].key;
        slots[i_slot].val = inputs[i_in].val;
        slots[i_slot].occupied = true;
        num_occupied++;
    }
    load_factor = (double)num_occupied / (double)num_slots;
    
    map->num_slots = num_slots;
    map->num_occupied = num_occupied;
    map->load_factor = load_factor;

    assert(load_factor <= MAX_LOAD_FACTOR);
    if (num_occupied != 0) assert(MIN_LOAD_FACTOR <= load_factor);
    //IDMap_assert_accuracy(map);
    
#if VERBOSITY
    print_summary_UIntMap64(map);
#endif
    
    return map;
}

static bool rehash_UIntMap64(UIntMap64 *map, size_t new_num_slots, uint64_t new_seed) {
    size_t old_num_slots = map->num_slots;

    size_t old_num_occupied = map->num_occupied;
    size_t new_num_occupied = 0;

#if VERBOSITY
    double old_load_factor = map->load_factor;
#endif
    double new_load_factor = 0;

#if VERBOSITY
    double old_uniformity = UIntMap64_uniformity(map);
    double new_uniformity = 0;
#endif
    
    UIntMap64_Entry *old_slots = map->slots;
    UIntMap64_Entry *new_slots = malloc(new_num_slots*sizeof(*new_slots));
    if ( ! new_slots) return false;

    // clear new
    memset(new_slots, 0, new_num_slots*sizeof(*new_slots));

    // load it up
    for (size_t i_old = 0; i_old < old_num_slots; i_old++) {
        if ( ! old_slots[i_old].occupied) // skip empties
            continue;
        size_t i_probe = 0;
        size_t hash = hash_u64(old_slots[i_old].key, new_num_slots, new_seed);
        size_t i_new = probe_slot(hash, i_probe++) & (new_num_slots - 1);

        while (new_slots[i_new].occupied) {
            // if all has worked as intended up to this point, there will be no
            // duplicates
            assert(new_slots[i_new].key != old_slots[i_old].key);
            i_new = probe_slot(hash, i_probe++) & (new_num_slots - 1);
        }
        new_slots[i_new].key = old_slots[i_old].key;
        new_slots[i_new].val = old_slots[i_old].val;
        new_slots[i_new].occupied = true;
        new_num_occupied++;
    }
    new_load_factor = (double)new_num_occupied / (double)new_num_slots;

    free(map->slots);
    map->seed = new_seed;
    map->slots = new_slots;
    assert(old_num_occupied == new_num_occupied);
    map->num_slots = new_num_slots;
    map->load_factor = new_load_factor;
#if VERBOSITY
    new_uniformity = UIntMap64_uniformity(map);
#endif
    
    assert(new_load_factor <= MAX_LOAD_FACTOR);
    assert(MIN_LOAD_FACTOR <= new_load_factor);
    //IntMap_assert_accuracy(map);
    
    vbs_printf("Rehashed IntMap (slots %zu -> %zu) (occupied %zu -> %zu) (load %f -> %f)\n"
               "                (unif. %f -> %f)\n",
               old_num_slots, new_num_slots,
               old_num_occupied, new_num_occupied,
               old_load_factor, new_load_factor,
               old_uniformity, new_uniformity);
    
    return true;
}

void destroy_UIntMap64(UIntMap64 *map) {
    if ( ! map)
        return;

    free(map->name);
    free(map->slots);

    destroy_HashTable(HT_UINTMAP64, map);
}


bool UIntMap64_lookup(UIntMap64 *map, uint64_t key, uint64_t *out_val) {
    /*
      If key is not present, return 0. If key is present, return the index plus
      one.
    */
    
    size_t num_slots = map->num_slots;
    UIntMap64_Entry *slots = map->slots;

    // find slot
    size_t i_probe = 0;
    size_t hash = hash_u64(key, num_slots, map->seed);
    size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    while (slots[i_slot].occupied && slots[i_slot].key != key)
        i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    
    if (slots[i_slot].occupied) {
        if (out_val) *out_val = slots[i_slot].val;
        return true;
    }
    
    return false;
}

bool UIntMap64_insert(UIntMap64 *map, uint64_t key, uint64_t *out_val) {
    /*
      If key is not present, insert it and return the index plus one.  If key is
      present, return the index plus one.
    */
    
    size_t num_slots = map->num_slots;
    UIntMap64_Entry *slots = map->slots;

    // find slot
    size_t i_probe = 0;
    size_t hash = hash_u64(key, num_slots, map->seed);
    size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    while (slots[i_slot].occupied && slots[i_slot].key != key)
        i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    
    if (slots[i_slot].occupied) {
        if (out_val) *out_val = slots[i_slot].val;
        return false; // false means "not inserted (since already present)
    }
        

    // key not in table; insert it
    slots[i_slot].key = key;
    slots[i_slot].occupied = true;

    map->num_occupied++;
    map->load_factor = (double)map->num_occupied / (double)map->num_slots;

    /*
    vbs_printf("Insertion: (key %" PRIu64 ") (hash %5zu%s) (slot %5zu) (occupied %zu) (load %.4f)\n",
               key,
               hash,
               i_slot == hash ? "*" : " ",
               i_slot,
               map->num_occupied,
               map->load_factor);
    */
    
    if (map->load_factor > MAX_LOAD_FACTOR) {
        rehash_UIntMap64(map, 2 * map->num_slots, map->seed);
    }
    
    if (out_val) *out_val = slots[i_slot].val;
    
    return true; // true means "inserted (since not already present)"
}

bool UIntMap64_delete(UIntMap64 *map, uint64_t key, uint64_t *out_val) {
    size_t num_slots = map->num_slots;
    UIntMap64_Entry *slots = map->slots;

    // find slot
    size_t i_probe = 0;
    size_t hash = hash_u64(key, num_slots, map->seed);
    size_t i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);
    while (slots[i_slot].occupied && slots[i_slot].key != key)
        i_slot = probe_slot(hash, i_probe++) & (num_slots - 1);

    if ( ! slots[i_slot].occupied) {
        //vbs_printf("Key %" PRIu64 " not found; no deletion.\n", key);
        return false;
    }
    
    map->num_occupied--;
    slots[i_slot].occupied = false;
    map->load_factor = (double)map->num_occupied / (double)map->num_slots;
    
    // now i_slot is index where match was found. We now need to continue,
    // backshifting any more slots with matching keyhash until we reach an
    // empty cell.
    size_t i = i_slot;
    size_t j = i;
    size_t k;
    /* IMPORTANT: This deletion handling pattern is only verified for trivial
       linear probing, that is: probe_slot(hash, i_probe) = hash + i_probe

       TODO: How to handle other probing */

    uint64_t seed = map->seed;
    while (true) {
        j = probe_slot(hash, i_probe++) & (num_slots - 1);
        if ( ! slots[j].occupied)
            break;

        k = hash_u64(slots[j].key, num_slots, seed);
        if (i <= j) {
            if (i < k && k <= j) {
                continue;
            }
        }
        else {
            if (i < k || k <= j) {
                continue;
            }
        }
        slots[i] = slots[j];
        slots[j].occupied = false;
        i = j;
    }

    /*
    vbs_printf("Deletion: (key %" PRIu64 ") (hash %5zu%s) (slot %5zu) (occupied %zu) (load %.4f)\n",
               key,
               hash,
               i_slot == hash ? "*" : " ",
               i_slot,
               map->num_occupied,
               map->load_factor);
    */
    
    if (map->load_factor < MIN_LOAD_FACTOR) {
        if (map->num_slots / 2 > 2) {
            // can't guarantee load factor within acceptable bounds when the
            // num_slots gets low. But with such small numbers speed isn't really an
            // issue, so let's not bother rehashing down to smaller than 3
            // buckets.
            rehash_UIntMap64(map, map->num_slots / 2, map->seed);
        }
    }

    if (out_val) *out_val = slots[i_slot].val;
    return true;
}


/* -------------------------------------------------------------------------- */

// +--------------------------------------+
// | Index (String-to-Integer Dictionary) |
// +--------------------------------------+

// Key type: char * internal
// Val type: uint64_t
// Separate chaining.

typedef struct IndexEntry IndexEntry;
struct IndexEntry {
    IndexEntry *next;
    IndexEntry *prev;

    char *key;
    uint64_t val;
};

typedef struct IndexBucket {
    size_t len;
    IndexEntry head;
    IndexEntry *tail;
} IndexBucket;

// Each bucket is a linked list of entries
struct Index {
    char *name;
    uint64_t seed;
    double load_factor; // num_entries / num_buckets
    size_t num_entries;
    size_t num_occupied;
    size_t num_buckets;
    IndexBucket *buckets;
};

Index *create_Index(size_t num_inputs, StrInt inputs[],
                    char const *in_name, uint64_t seed) {
    double load_factor = 0;
    size_t num_entries = 0;
    size_t num_occupied = 0;
    
    size_t num_buckets = num_inputs;
    // round up buckets to next highest power of 2. There are branchless
    // bithacky ways to do this but who cares
    for (size_t i = 1; ; i *= 2) {
        if (num_buckets <= i && (double)num_inputs/(double)i < MAX_LOAD_FACTOR) {
            num_buckets = i;
            break;
        }
    }

    Index *idx = create_HashTable(HT_INDEX);
    if ( ! idx) return NULL;
    idx->buckets = malloc(num_buckets*sizeof(*idx->buckets));
    if ( ! idx->buckets) {
        free(idx);
        return NULL;
    }

    HashTable_set_name(&idx->name, in_name);
    idx->seed = seed;
    
    IndexBucket *buckets = idx->buckets;

    // clear
    memset(buckets, 0, num_buckets*sizeof(*buckets));

    // load it up
    for (size_t i_in = 0; i_in < num_inputs; i_in++) {
        char const *in_key = inputs[i_in].key;
        uint64_t in_val = inputs[i_in].val;
        
        size_t i_bucket = hash_str(in_key, num_buckets, seed);
        IndexBucket *bucket = &buckets[i_bucket];
        IndexEntry *entry = &bucket->head;
        if (bucket->len == 0) {
            num_occupied++;
            entry->prev = NULL;
            
            entry->next = NULL;
            entry->key = malloc(1 + strlen(in_key));
            strcpy(entry->key, in_key);
            entry->val = in_val;

            bucket->tail = entry;
            bucket->len++;
            num_entries++;
        }
        else {
            bool dup = false;
            while (entry->next != NULL) {
                if (strcmp(entry->key, in_key) == 0) {
                    // already in idxionary, just update the value
                    entry->val = in_val;
                    dup = true;
                    break;
                }
                entry = entry->next;
            }
            if (dup) continue;
            
            entry->next = malloc(sizeof(*entry->next));
            entry->next->prev = entry;
            entry = entry->next;

            entry->next = NULL;
            entry->key = malloc(1 + strlen(in_key));
            strcpy(entry->key, in_key);
            entry->val = in_val;
            bucket->tail = entry;
            bucket->len++;
            num_entries++;
        }
        
    }
    load_factor = (double)num_entries / (double)num_buckets;

    idx->num_buckets = num_buckets;
    idx->num_occupied = num_occupied;
    idx->num_entries = num_entries;
    idx->load_factor = load_factor;

    assert(load_factor <= MAX_LOAD_FACTOR);
    if (num_entries != 0) assert(MIN_LOAD_FACTOR <= load_factor);

#if VERBOSITY
    print_summary_Index(idx);
#endif
    
    return idx;
}

void destroy_Index(Index *idx) {
    for (size_t i_bucket = 0; i_bucket < idx->num_buckets; i_bucket++) {
        if (idx->buckets[i_bucket].len == 0) {
            //puts("Encountered empty bucket.");
            continue;
        }

        IndexEntry *entry = &idx->buckets[i_bucket].head;
        IndexEntry *next_entry = entry->next;

        //puts("Freeing head key and val.");
        free(entry->key);
            
        for (entry = next_entry; entry; entry = next_entry) {
            next_entry = entry->next;
            //puts("Freeing nonhead key and val.");
            free(entry->key);
            free(entry);
        }
    }

    free(idx->name);
    free(idx->buckets);
    destroy_HashTable(HT_INDEX, idx);
}

bool rehash_Index(Index *idx, size_t new_num_buckets, uint64_t new_seed) {
    size_t old_num_buckets = idx->num_buckets;

    size_t old_num_entries = idx->num_entries;
    size_t new_num_entries = 0; // will count and then verify it equals old

#if VERBOSITY
    size_t old_num_occupied = idx->num_occupied;
#endif
    size_t new_num_occupied = 0;

#if VERBOSITY
    double old_load_factor = idx->load_factor;
#endif
    double new_load_factor = 0;

#if VERBOSITY
    double old_uniformity = Index_uniformity(idx);
    double new_uniformity = 0;
#endif
    
    IndexBucket *old_buckets = idx->buckets;
    IndexBucket *new_buckets = calloc(new_num_buckets, sizeof(*new_buckets));
    if ( ! new_buckets) return false;

    // clear new
    //memset(new_buckets, 0, new_num_buckets*sizeof(*new_buckets));
    // used calloc

    // load it up
    for (size_t i_old_bucket = 0; i_old_bucket < old_num_buckets; i_old_bucket++) {
        if (old_buckets[i_old_bucket].len == 0) // skip empty buckets
            continue;

        // for non-empty bucket, go through each entry, compute new hash of key,
        // then copy (by reference where possible!) the entry from the old
        // bucket to the new one, and link properly. No need for allocation nor
        // freeing of keys and values.

        IndexEntry *old_entry = &old_buckets[i_old_bucket].head;
        IndexEntry *next_old_entry = old_entry->next;
        size_t i_new_bucket = hash_str(old_entry->key, new_num_buckets, new_seed);
        if (new_buckets[i_new_bucket].len == 0) { // head-to-head transfer
            new_buckets[i_new_bucket].len = 1;
            new_num_occupied++;
            new_num_entries++;
            
            IndexEntry *new_head = &new_buckets[i_new_bucket].head;
            new_head->next = NULL;
            new_head->prev = NULL;
            new_head->key = old_entry->key;
            new_head->val = old_entry->val;
        }
        else { // head-to-nonhead
            new_buckets[i_new_bucket].len++;
            new_num_entries++;
            
            IndexEntry *new_entry = &new_buckets[i_new_bucket].head;
            while (new_entry->next != NULL) {
                new_entry = new_entry->next;
            }
            new_entry->next = malloc(sizeof(*new_entry->next));
            new_entry->next->prev = new_entry;
            new_entry = new_entry->next; // now new_entry is actually new
            new_entry->next = NULL;
            new_entry->key = old_entry->key;
            new_entry->val = old_entry->val;
        }
        
        for (old_entry = next_old_entry;
             old_entry; old_entry = next_old_entry) {
            next_old_entry = old_entry->next;
            
            size_t i_new_bucket = hash_str(old_entry->key, new_num_buckets, new_seed);
            if (new_buckets[i_new_bucket].len == 0) { // nonhead-to-head transfer
                new_buckets[i_new_bucket].len = 1;
                new_num_occupied++;
                new_num_entries++;
                
                IndexEntry *new_head = &new_buckets[i_new_bucket].head;
                new_head->next = NULL;
                new_head->prev = NULL;
                new_head->key = old_entry->key;
                new_head->val = old_entry->val;
                free(old_entry);
            }
            else { // nonhead-to-nonhead
                new_buckets[i_new_bucket].len++;
                new_num_entries++;
                
                IndexEntry *new_entry = &new_buckets[i_new_bucket].head;
                while (new_entry->next != NULL) {
                    new_entry = new_entry->next;
                }
                new_entry->next = old_entry;
                new_entry->next->prev = new_entry;
                new_entry = new_entry->next; // now new_entry is actually new
                new_entry->next = NULL;
            }
        }
    }
    new_load_factor = (double)new_num_entries / (double)new_num_buckets;

    free(idx->buckets);
    idx->seed = new_seed;
    idx->buckets = new_buckets;
    assert(old_num_entries == new_num_entries);
    idx->num_occupied = new_num_occupied;
    idx->num_buckets = new_num_buckets;
    idx->load_factor = new_load_factor;
#if VERBOSITY
    new_uniformity = Index_uniformity(idx);
#endif
    
    assert(new_load_factor <= MAX_LOAD_FACTOR);
    assert(MIN_LOAD_FACTOR <= new_load_factor);
    
    vbs_printf("Rehashed Index (buckets %zu -> %zu) (occupied %zu -> %zu) (load %f -> %f)\n"
               "               (unif. %f -> %f)\n",
               old_num_buckets, new_num_buckets,
               old_num_occupied, new_num_occupied,
               old_load_factor, new_load_factor,
               old_uniformity, new_uniformity);

    return true;
}

bool Index_lookup(Index *idx, char const *key, uint64_t *out_val) {
    if (idx->num_entries == 0)
        return false;
    
    size_t num_buckets = idx->num_buckets;
    IndexBucket *buckets = idx->buckets;
    size_t i_bucket = hash_str(key, num_buckets, idx->seed);
    IndexBucket *bucket = &buckets[i_bucket];

    if (bucket->len == 0) {
        return false;
    }
    
    IndexEntry *entry = &bucket->head;
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            if (out_val) *out_val = entry->val;
            return true;
        }
        entry = entry->next;
    }
    
    return false;
}

bool Index_insert(Index *idx, StrInt kvp) {
    /* If key already present, replace its value and return the old value. If
       key is not already present, insert it and return NULL. */
    
    size_t num_occupied = idx->num_occupied;
    size_t num_entries = idx->num_entries;
    size_t num_buckets = idx->num_buckets;
    IndexBucket *buckets = idx->buckets;
    size_t i_bucket = hash_str(kvp.key, num_buckets, idx->seed);
    IndexBucket *bucket = &buckets[i_bucket];

    IndexEntry *entry = &bucket->head;
    if (bucket->len == 0) {
        bucket->len = 1;
        num_occupied++;
        num_entries++;
        entry->prev = NULL;
            
        entry->next = NULL;
        entry->key = malloc(1 + strlen(kvp.key));
        strcpy(entry->key, kvp.key);
        entry->val = kvp.val;

        bucket->tail = entry;
    }
    else {
        bucket->len++;
        num_entries++;
        
        bool dup = false;
        while (entry->next != NULL) {
            if (strcmp(entry->key, kvp.key) == 0) {
                // already in idxionary, just update the value
                entry->val = kvp.val;
                dup = true;
                break;
            }
            entry = entry->next;
        }
        if (dup) return false;

        assert(entry->next == NULL);
        entry->next = malloc(sizeof(*entry->next));
        entry->next->prev = entry;
        entry = entry->next;

        entry->next = NULL;
        entry->key = malloc(1 + strlen(kvp.key));
        strcpy(entry->key, kvp.key);
        entry->val = kvp.val;
            
        bucket->tail = entry;
    }

    idx->num_occupied = num_occupied;
    idx->num_entries = num_entries;
    idx->load_factor = (double)num_entries / (double)num_buckets;
    
    //vbs_printf("Insertion: {\"%s\" : \"%s\"}; load = %f\n", kvp.key, kvp.val, idx->load_factor);

    if (idx->load_factor > MAX_LOAD_FACTOR) {
        rehash_Index(idx, 2 * idx->num_buckets, idx->seed);
    }
    
    return true;
}

bool Index_delete(Index *idx, char const *key, uint64_t *out_val) {
    size_t num_occupied = idx->num_occupied;
    size_t num_entries = idx->num_entries;
    size_t num_buckets = idx->num_buckets;
    size_t i_bucket = hash_str(key, num_buckets, idx->seed);
    IndexBucket *bucket = idx->buckets + i_bucket;

    if (bucket->len == 0) {
        puts("Empty bucket; key not indexed; nothing to delete.");
        return false;
    }
    
    bool found = false;
    IndexEntry *entry = &bucket->head;
    if (strcmp(entry->key, key) == 0) {
        found = true;
        bucket->len--;
        num_entries--;
        
        free(entry->key);
        if (out_val) *out_val = entry->val;

        if (entry->next) {
            IndexEntry *next = entry->next;
            // move the next entry to the head.
            entry->key = next->key;
            entry->val = next->val;
            entry->next = next->next;
            entry->prev = NULL;
            free(next);
            if (entry->next) {
                // if there was a third entry, its "prev" needs updating
                entry->next->prev = entry;
            }
        }
        else {
            num_occupied--;
        }
    }
    else {
        for (entry = entry->next; entry; entry = entry->next) {
            if (strcmp(entry->key, key) == 0) {
                found = true;
                bucket->len--;
                num_entries--;
                
                free(entry->key);
                
                if (out_val) *out_val = entry->val;
                entry->prev->next = entry->next;
                if (entry->next) {
                    entry->next->prev = entry->prev;
                }
                free(entry);
                
                break;
            }
        }
    }

    if ( ! found) { // wasn't found in idxionary; nothing to delete
        puts("Key not indexed; nothing to delete.");
        return false;
    }
    
    idx->num_occupied = num_occupied;
    idx->num_entries = num_entries;
    idx->load_factor = (double)num_entries / (double)num_buckets;
    
    //vbs_printf("Deletion: {\"%s\" : \"%s\"}; load = %f\n", key, val, idx->load_factor);

    if (idx->load_factor < MIN_LOAD_FACTOR) {
        if (idx->num_buckets / 2 > 2) // can't guarantee load factor within
                                       // acceptable bounds when the number of
                                       // buckets gets low. But with such small
                                       // numbers speed isn't really an issue,
                                       // so let's not bother rehashing down to
                                       // smaller than 3 buckets.
            rehash_Index(idx, idx->num_buckets / 2, idx->seed);
    }
        
        
    return true;
}



void print_Index_info(Index *idx) {
    for (size_t i_bucket = 0; i_bucket < idx->num_buckets; i_bucket++) {
        if (idx->buckets[i_bucket].len == 0) {
            continue;
        }
        for (IndexEntry *entry = &idx->buckets[i_bucket].head; entry; entry = entry->next) {
            printf("{\"%s\" : %"PRIu64"}\n", entry->key, entry->val);
        }
    }
}



/* -------------------------------------------------------------------------- */

// +------------+
// | Dictionary |
// +------------+

// Key type: char * internal
// Val type: char * internal
// Separate chaining.

// TODO: I read somewhere(?) that chaining performs well with load factors up to
// 100% (or more?). Look into this.

typedef struct DictEntry DictEntry;
struct DictEntry {
    DictEntry *next;
    DictEntry *prev;

    char *key;
    char *val;
};

typedef struct DictBucket {
    size_t len;
    DictEntry head;
    DictEntry *tail;
} DictBucket;

// Each bucket is a linked list of entries
struct Dict {
    char *name;
    uint64_t seed;
    double load_factor; // num_entries / num_buckets
    size_t num_entries;
    size_t num_occupied;
    size_t num_buckets;
    DictBucket *buckets;
};

extern double Dict_uniformity(Dict *dict);

Dict *create_Dict(size_t num_inputs, StrStr inputs[],
                  char const *in_name, uint64_t seed) {
    double load_factor = 0;
    size_t num_entries = 0;
    size_t num_occupied = 0;
    
    size_t num_buckets = num_inputs;
    // round up buckets to next highest power of 2. There are branchless
    // bithacky ways to do this but who cares
    for (size_t i = 1; ; i *= 2) {
        if (num_buckets <= i && (double)num_inputs/(double)i < MAX_LOAD_FACTOR) {
            num_buckets = i;
            break;
        }
    }

    Dict *dict = create_HashTable(HT_DICT);
    if ( ! dict) return NULL;
    dict->buckets = malloc(num_buckets*sizeof(*dict->buckets));
    if ( ! dict->buckets) {
        free(dict);
        return NULL;
    }

    HashTable_set_name(&dict->name, in_name);
    dict->seed = seed;
    
    DictBucket *buckets = dict->buckets;

    // clear
    memset(buckets, 0, num_buckets*sizeof(*buckets));

    // load it up
    for (size_t i_in = 0; i_in < num_inputs; i_in++) {
        char const *in_key = inputs[i_in].key;
        char const *in_val = inputs[i_in].val;
        
        size_t i_bucket = hash_str(in_key, num_buckets, seed);
        DictBucket *bucket = &buckets[i_bucket];
        DictEntry *entry = &bucket->head;
        if (bucket->len == 0) {
            num_occupied++;
            entry->prev = NULL;
            
            entry->next = NULL;
            entry->key = malloc(1 + strlen(in_key));
            strcpy(entry->key, in_key);
            entry->val = malloc(1 + strlen(in_val));
            strcpy(entry->val, in_val);

            bucket->tail = entry;
            bucket->len++;
            num_entries++;
        }
        else {
            bool dup = false;
            while (entry->next != NULL) {
                if (strcmp(entry->key, in_key) == 0) {
                    // already in dictionary, just update the value
                    entry->val = realloc(entry->val, 1 + strlen(in_val));
                    strcpy(entry->val, in_val);
                    dup = true;
                    break;
                }
                entry = entry->next;
            }
            if (dup) continue;
            
            entry->next = malloc(sizeof(*entry->next));
            entry->next->prev = entry;
            entry = entry->next;

            entry->next = NULL;
            entry->key = malloc(1 + strlen(in_key));
            strcpy(entry->key, in_key);
            entry->val = malloc(1 + strlen(in_val));
            strcpy(entry->val, in_val);
            
            bucket->tail = entry;
            bucket->len++;
            num_entries++;
        }
        
    }
    load_factor = (double)num_entries / (double)num_buckets;

    dict->num_buckets = num_buckets;
    dict->num_occupied = num_occupied;
    dict->num_entries = num_entries;
    dict->load_factor = load_factor;

    assert(load_factor <= MAX_LOAD_FACTOR);
    if (num_entries != 0) assert(MIN_LOAD_FACTOR <= load_factor);

#if VERBOSITY
    print_summary_Dict(dict);
#endif
    
    return dict;
}

void destroy_Dict(Dict *dict) {
    for (size_t i_bucket = 0; i_bucket < dict->num_buckets; i_bucket++) {
        if (dict->buckets[i_bucket].len == 0) {
            //puts("Encountered empty bucket.");
            continue;
        }

        DictEntry *entry = &dict->buckets[i_bucket].head;
        DictEntry *next_entry = entry->next;

        //puts("Freeing head key and val.");
        free(entry->key);
        free(entry->val);
            
        for (entry = next_entry; entry; entry = next_entry) {
            next_entry = entry->next;
            //puts("Freeing nonhead key and val.");
            free(entry->key);
            free(entry->val);
            free(entry);
        }
    }

    free(dict->name);
    free(dict->buckets);
    destroy_HashTable(HT_DICT, dict);
}

bool rehash_Dict(Dict *dict, size_t new_num_buckets, uint64_t new_seed) {
    size_t old_num_buckets = dict->num_buckets;

    size_t old_num_entries = dict->num_entries;
    size_t new_num_entries = 0; // will count and then verify it equals old

#if VERBOSITY
    size_t old_num_occupied = dict->num_occupied;
#endif
    size_t new_num_occupied = 0;

#if VERBOSITY
    double old_load_factor = dict->load_factor;
#endif
    double new_load_factor = 0;

#if VERBOSITY
    double old_uniformity = Dict_uniformity(dict);
    double new_uniformity = 0;
#endif
    
    DictBucket *old_buckets = dict->buckets;
    DictBucket *new_buckets = calloc(new_num_buckets, sizeof(*new_buckets));
    if ( ! new_buckets) return false;

    // clear new
    //memset(new_buckets, 0, new_num_buckets*sizeof(*new_buckets));
    // used calloc

    // load it up
    for (size_t i_old_bucket = 0; i_old_bucket < old_num_buckets; i_old_bucket++) {
        if (old_buckets[i_old_bucket].len == 0) // skip empty buckets
            continue;

        // for non-empty bucket, go through each entry, compute new hash of key,
        // then copy (by reference where possible!) the entry from the old
        // bucket to the new one, and link properly. No need for allocation nor
        // freeing of keys and values.

        DictEntry *old_entry = &old_buckets[i_old_bucket].head;
        DictEntry *next_old_entry = old_entry->next;
        size_t i_new_bucket = hash_str(old_entry->key, new_num_buckets, new_seed);
        if (new_buckets[i_new_bucket].len == 0) { // head-to-head transfer
            new_buckets[i_new_bucket].len = 1;
            new_num_occupied++;
            new_num_entries++;
            
            DictEntry *new_head = &new_buckets[i_new_bucket].head;
            new_head->next = NULL;
            new_head->prev = NULL;
            new_head->key = old_entry->key;
            new_head->val = old_entry->val;
        }
        else { // head-to-nonhead
            new_buckets[i_new_bucket].len++;
            new_num_entries++;
            
            DictEntry *new_entry = &new_buckets[i_new_bucket].head;
            while (new_entry->next != NULL) {
                new_entry = new_entry->next;
            }
            new_entry->next = malloc(sizeof(*new_entry->next));
            new_entry->next->prev = new_entry;
            new_entry = new_entry->next; // now new_entry is actually new
            new_entry->next = NULL;
            new_entry->key = old_entry->key;
            new_entry->val = old_entry->val;
        }
        
        for (old_entry = next_old_entry;
             old_entry; old_entry = next_old_entry) {
            next_old_entry = old_entry->next;
            
            size_t i_new_bucket = hash_str(old_entry->key, new_num_buckets, new_seed);
            if (new_buckets[i_new_bucket].len == 0) { // nonhead-to-head transfer
                new_buckets[i_new_bucket].len = 1;
                new_num_occupied++;
                new_num_entries++;
                
                DictEntry *new_head = &new_buckets[i_new_bucket].head;
                new_head->next = NULL;
                new_head->prev = NULL;
                new_head->key = old_entry->key;
                new_head->val = old_entry->val;
                free(old_entry);
            }
            else { // nonhead-to-nonhead
                new_buckets[i_new_bucket].len++;
                new_num_entries++;
                
                DictEntry *new_entry = &new_buckets[i_new_bucket].head;
                while (new_entry->next != NULL) {
                    new_entry = new_entry->next;
                }
                new_entry->next = old_entry;
                new_entry->next->prev = new_entry;
                new_entry = new_entry->next; // now new_entry is actually new
                new_entry->next = NULL;
            }
        }
    }
    new_load_factor = (double)new_num_entries / (double)new_num_buckets;

    free(dict->buckets);
    dict->seed = new_seed;
    dict->buckets = new_buckets;
    assert(old_num_entries == new_num_entries);
    dict->num_occupied = new_num_occupied;
    dict->num_buckets = new_num_buckets;
    dict->load_factor = new_load_factor;
#if VERBOSITY
    new_uniformity = Dict_uniformity(dict);
#endif
    
    assert(new_load_factor <= MAX_LOAD_FACTOR);
    assert(MIN_LOAD_FACTOR <= new_load_factor);
    
    vbs_printf("Rehashed Dict (buckets %zu -> %zu) (occupied %zu -> %zu) (load %f -> %f)\n"
               "              (unif. %f -> %f)\n",
               old_num_buckets, new_num_buckets,
               old_num_occupied, new_num_occupied,
               old_load_factor, new_load_factor,
               old_uniformity, new_uniformity);

    return true;
}

char *Dict_lookup(Dict *dict, char const *key) {
    size_t num_buckets = dict->num_buckets;
    DictBucket *buckets = dict->buckets;
    size_t i_bucket = hash_str(key, num_buckets, dict->seed);
    DictBucket *bucket = &buckets[i_bucket];

    if (bucket->len == 0) {
        return NULL;
    }
    
    DictEntry *entry = &bucket->head;
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry->val;
        }
        entry = entry->next;
    }
    
    return NULL;    
}

char *Dict_insert(Dict *dict, StrStr kvp) {
    /* If key already present, replace its value and return the old value. If
       key is not already present, insert it and return NULL. */
    
    size_t num_occupied = dict->num_occupied;
    size_t num_entries = dict->num_entries;
    size_t num_buckets = dict->num_buckets;
    DictBucket *buckets = dict->buckets;
    size_t i_bucket = hash_str(kvp.key, num_buckets, dict->seed);
    DictBucket *bucket = &buckets[i_bucket];

    DictEntry *entry = &bucket->head;
    if (bucket->len == 0) {
        bucket->len = 1;
        num_occupied++;
        num_entries++;
        entry->prev = NULL;
            
        entry->next = NULL;
        entry->key = malloc(1 + strlen(kvp.key));
        strcpy(entry->key, kvp.key);
        entry->val = malloc(1 + strlen(kvp.val));
        strcpy(entry->val, kvp.val);

        bucket->tail = entry;
    }
    else {
        bucket->len++;
        num_entries++;
        
        bool dup = false;
        while (entry->next != NULL) {
            if (strcmp(entry->key, kvp.key) == 0) {
                // already in dictionary, just update the value
                entry->val = realloc(entry->val, 1 + strlen(kvp.val));
                strcpy(entry->val, kvp.val);
                dup = true;
                break;
            }
            entry = entry->next;
        }
        if (dup) return NULL;

        assert(entry->next == NULL);
        entry->next = malloc(sizeof(*entry->next));
        entry->next->prev = entry;
        entry = entry->next;

        entry->next = NULL;
        entry->key = malloc(1 + strlen(kvp.key));
        strcpy(entry->key, kvp.key);
        entry->val = malloc(1 + strlen(kvp.val));
        strcpy(entry->val, kvp.val);
            
        bucket->tail = entry;
    }

    dict->num_occupied = num_occupied;
    dict->num_entries = num_entries;
    dict->load_factor = (double)num_entries / (double)num_buckets;
    
    //vbs_printf("Insertion: {\"%s\" : \"%s\"}; load = %f\n", kvp.key, kvp.val, dict->load_factor);

    if (dict->load_factor > MAX_LOAD_FACTOR) {
        rehash_Dict(dict, 2 * dict->num_buckets, dict->seed);
    }
    
    return NULL;    
}

char *Dict_delete(Dict *dict, char const *key) {
    size_t num_occupied = dict->num_occupied;
    size_t num_entries = dict->num_entries;
    size_t num_buckets = dict->num_buckets;
    size_t i_bucket = hash_str(key, num_buckets, dict->seed);
    DictBucket *bucket = dict->buckets + i_bucket;

    if (bucket->len == 0) {
        puts("Empty bucket; nothing to delete.");
        return NULL;
    }
    
    char *val = "(null)";
    bool found = false;
    DictEntry *entry = &bucket->head;
    if (strcmp(entry->key, key) == 0) {
        found = true;
        bucket->len--;
        num_entries--;
        
        free(entry->key);
        val = malloc(1 + strlen(entry->val));
        strcpy(val, entry->val);
        free(entry->val);

        if (entry->next) {
            DictEntry *next = entry->next;
            // move the next entry to the head.
            entry->key = next->key;
            entry->val = next->val;
            entry->next = next->next;
            entry->prev = NULL;
            free(next);
            if (entry->next) {
                // if there was a third entry, its "prev" needs updating
                entry->next->prev = entry;
            }
        }
        else {
            num_occupied--;
        }
    }
    else {
        for (entry = entry->next; entry; entry = entry->next) {
            if (strcmp(entry->key, key) == 0) {
                found = true;
                bucket->len--;
                num_entries--;
                
                free(entry->key);
                val = malloc(1 + strlen(entry->val));
                strcpy(val, entry->val);
                free(entry->val);
                entry->prev->next = entry->next;
                if (entry->next) {
                    entry->next->prev = entry->prev;
                }
                free(entry);
                
                break;
            }
        }
    }

    if ( ! found) { // wasn't found in dictionary; nothing to delete
        puts("Key not found; nothing to delete.");
        return NULL;
    }
    
    dict->num_occupied = num_occupied;
    dict->num_entries = num_entries;
    dict->load_factor = (double)num_entries / (double)num_buckets;
    
    //vbs_printf("Deletion: {\"%s\" : \"%s\"}; load = %f\n", key, val, dict->load_factor);
    free(val);

    if (dict->load_factor < MIN_LOAD_FACTOR) {
        if (dict->num_buckets / 2 > 2) // can't guarantee load factor within
                                       // acceptable bounds when the number of
                                       // buckets gets low. But with such small
                                       // numbers speed isn't really an issue,
                                       // so let's not bother rehashing down to
                                       // smaller than 3 buckets.
            rehash_Dict(dict, dict->num_buckets / 2, dict->seed);
    }
        
        
    return NULL;
}


void print_Dict_info(Dict *dict) {
    for (size_t i_bucket = 0; i_bucket < dict->num_buckets; i_bucket++) {
        if (dict->buckets[i_bucket].len == 0) {
            continue;
        }
        for (DictEntry *entry = &dict->buckets[i_bucket].head; entry; entry = entry->next) {
            printf("{\"%s\" : \"%s\"}\n", entry->key, entry->val);
        }
    }
}




/* -------------------------------------------------------------------------- */


typedef struct HashTableNode HashTableNode;

struct HashTableNode {
    HashTableNode *next;
    HashTableNode *prev;
};

typedef struct HashTableRecord {
    HashTableNode node;
    HashTableType type;
} HashTableRecord;

size_t num_refs = 0;
HashTableRecord *records;

#define list_init(entry) (entry)->prev = (entry)->next = entry
#define container_of(ptr, member)                                       \
    ((HashTableRecord *)((char *)(ptr)))
    //    ((HashTableRecord *)((char *)(ptr)-(unsigned long)(&((HashTableRecord *)0)->member)))
#define list_node(ptr, member) container_of(ptr, member)
#define list_empty(list) ((list)->next == (list))
#define list_head(list) (list)->next
#define list_add_tail(list, entry)              \
    (entry)->next = (list);                     \
    (entry)->prev = (list)->prev;               \
    (list)->prev->next = (entry);               \
    (list)->prev = (entry)
#define list_remove(entry)                      \
    assert((entry)->next != NULL);              \
    (entry)->prev->next = (entry)->next;        \
    (entry)->next->prev = (entry)->prev

HashTableNode record_list;

static const size_t type_sizes[5] = {
    [HT_IDLIST] = sizeof(IDList),
    [HT_UINTMAP32] = sizeof(UIntMap32),
    [HT_UINTMAP64] = sizeof(UIntMap64),
    [HT_INDEX] = sizeof(Index),
    [HT_DICT] = sizeof(Dict),
};

static void *create_HashTable(HashTableType type) {
    static bool _init = false;
    if ( ! _init) {
        list_init(&record_list);
        _init = true;
    }
    
    HashTableRecord *record;

    record = malloc(sizeof(*record) + type_sizes[type]);
    record->type = type;
    list_add_tail(&record_list, &record->node);

    return record + 1;
}

static void destroy_HashTable(HashTableType type, void *table) {
    if ( ! table) return;

    HashTableRecord *record = (HashTableRecord *)table - 1;

    assert(record->type == type);
    
    list_remove(&record->node);
    free(record);
}

void destroy_all_HashTable(void) {
    HashTableNode *node = list_head(&record_list);

    while (node != &record_list) {
        HashTableNode *next_node = node->next;
        HashTableRecord *record = list_node(node, node);
        switch (record->type) {
        case HT_IDLIST:
            destroy_IDList((IDList *)(record + 1));
            break;
        case HT_UINTMAP32:
            destroy_UIntMap32((UIntMap32 *)(record + 1));
            break;
        case HT_UINTMAP64:
            destroy_UIntMap64((UIntMap64 *)(record + 1));
            break;
        case HT_INDEX:
            destroy_Index((Index *)(record + 1));
            break;
        case HT_DICT:
            destroy_Dict((Dict *)(record + 1));
            break;
        }
        node = next_node;
    }
}

void print_all_HashTable(void) {
    HashTableNode *node = list_head(&record_list);

    while (node != &record_list) {
        HashTableRecord *record = list_node(node, node);
        switch (record->type) {
        case HT_IDLIST:
            print_summary_IDList((IDList *)(record + 1));
            break;
        case HT_UINTMAP32:
            print_summary_UIntMap32((UIntMap32 *)(record + 1));
            break;
        case HT_UINTMAP64:
            print_summary_UIntMap64((UIntMap64 *)(record + 1));
            break;
        case HT_INDEX:
            print_summary_Index((Index *)(record + 1));
            break;
        case HT_DICT:
            print_summary_Dict((Dict *)(record + 1));
            break;
        }
        putchar('\n');
        node = node->next;
    }
}

#define STRLEN_UINT64_UPPER_BOUND (3*sizeof(uint64_t)*CHAR_BIT/8+2)

//static char uint64_str[STRLEN_UINT64_UPPER_BOUND + 1];
static void HashTable_set_name(char **out_name, char const *in_name) {
    if (in_name) {
        *out_name = malloc(1 + strlen(in_name));
        strcpy(*out_name, in_name);
    }
    else {
        static uint64_t i = 0;
        *out_name = malloc(1 + strlen("Unnamed HashTable ") + STRLEN_UINT64_UPPER_BOUND);
        
        sprintf(*out_name, "Unnamed HashTable %" PRIu64, i);
        i++;
    }
}


/* -------------------------------------------------------------------------- */

#define generic_uniformity_OA(p_table)                              \
    if (p_table->num_occupied == 0 || p_table->num_slots == 1) {    \
        return -1.0;                                                \
    }                                                               \
                                                                    \
    double n = (double)p_table->num_occupied;                       \
    double m = (double)p_table->num_slots;                          \
                                                                    \
    double temp;                                                    \
    double res = 0;                                                 \
    for (size_t j = 0; j < m; j++) {                                \
        if (p_table->slots[j].occupied) temp = 1.0;                 \
        else temp = 0.0;                                            \
                                                                    \
        res += (temp - n/m)*(temp - n/m);                           \
    }                                                               \
                                                                    \
    return ((m/n)*res)/(m-1)                                        \

double IDList_uniformity(IDList *list) {
    generic_uniformity_OA(list);
}

double UIntMap32_uniformity(UIntMap32 *map) {
    generic_uniformity_OA(map);
}

double UIntMap64_uniformity(UIntMap64 *map) {
    generic_uniformity_OA(map);
}

#define generic_uniformity_CH(p_table)                                  \
    if (p_table->num_occupied == 0 || p_table->num_buckets == 1) {      \
        return -1.0f;                                                   \
    }                                                                   \
                                                                        \
    double n = (double)p_table->num_entries;                            \
    double m = (double)p_table->num_buckets;                            \
                                                                        \
    double res = 0;                                                     \
    for (size_t j = 0; j < m; j++) {                                    \
        res += ((double)p_table->buckets[j].len - n/m)*((double)p_table->buckets[j].len - n/m); \
    }                                                                   \
                                                                        \
    return ((m/n)*res)/(m-1)                                            \

    double Index_uniformity(Index *idx) {
    generic_uniformity_CH(idx);
}

double Dict_uniformity(Dict *dict) {
    generic_uniformity_CH(dict);
}


/* -------------------------------------------------------------------------- */

#define generic_measure_lookup_len_OA(p_table, p_avg, p_max, fn_hash, hash_t, key_t, seed_t) \
    size_t num_occupied = 0;                                            \
    uint64_t total_lookup_len = 0;                                      \
    size_t longest_lookup_len = 0;                                      \
                                                                        \
    seed_t seed = p_table->seed;                                        \
                                                                        \
    for (size_t i_slot = 0; i_slot < p_table->num_slots; i_slot++) {    \
        if (p_table->slots[i_slot].occupied) {                          \
            num_occupied++;                                             \
            key_t key = p_table->slots[i_slot].key;                     \
            hash_t hash = fn_hash(key, p_table->num_slots, seed);      \
                                                                        \
            size_t lookup_len;                                          \
            if (hash <= i_slot) {                                       \
                lookup_len = i_slot - hash + 1;                         \
            }                                                           \
            else {                                                      \
                lookup_len = (p_table->num_slots - hash) + i_slot + 1;  \
            }                                                           \
                                                                        \
            if (longest_lookup_len < lookup_len) {                      \
                longest_lookup_len = lookup_len;                        \
            }                                                           \
                                                                        \
            total_lookup_len += lookup_len;                             \
        }                                                               \
    }                                                                   \
                                                                        \
    assert(num_occupied == p_table->num_occupied);                      \
                                                                        \
    if (p_avg)                                                            \
        *p_avg = (double)total_lookup_len/(double)num_occupied;           \
    if (p_max)                                                            \
        *p_max = longest_lookup_len

void measure_lookup_len_IDList(IDList *list, double *avg, size_t *max) {
    generic_measure_lookup_len_OA(list, avg, max, hash_u64,
                                  uint64_t, uint64_t, uint64_t);
}

void measure_lookup_len_UIntMap32(UIntMap32 *map, double *avg, size_t *max) {
    generic_measure_lookup_len_OA(map, avg, max, hash_u32,
                                  uint32_t, uint32_t, uint32_t);
}

void measure_lookup_len_UIntMap64(UIntMap64 *map, double *avg, size_t *max) {
    generic_measure_lookup_len_OA(map, avg, max, hash_u64,
                                  uint64_t, uint64_t, uint64_t);
}

void measure_lookup_len_Index(Index *idx, double *avg, size_t *max) {
    size_t num_occupied = 0;
    size_t num_entries = 0;
    size_t total_lookup_len = 0;
    size_t longest_lookup_len = 0;

    for (size_t i_bucket = 0; i_bucket < idx->num_buckets; i_bucket++) {
        size_t bucket_len = idx->buckets[i_bucket].len;
        if (bucket_len > 0) {
            num_occupied++;
            num_entries += bucket_len;
            
            // A bucket with n > 1 entries counts for n lookups, with lengths:
            
            // n, n-1, n-2, ..., 3, 2, 1,

            // the largest of which is of course n
            
            size_t lookup_len_bucket_sum = (bucket_len*(bucket_len + 1))>>1;

            if (longest_lookup_len < bucket_len) {
                longest_lookup_len = bucket_len;
            }

            total_lookup_len += lookup_len_bucket_sum;
        }
    }

    assert(num_occupied == idx->num_occupied);

    if (avg)
        *avg = (double)total_lookup_len/(double)num_occupied;
    if (max)
        *max = longest_lookup_len;

}

void measure_lookup_len_Dict(Dict *dict, double *avg, size_t *max) {
    size_t num_occupied = 0;
    size_t num_entries = 0;
    size_t total_lookup_len = 0;
    size_t longest_lookup_len = 0;

    for (size_t i_bucket = 0; i_bucket < dict->num_buckets; i_bucket++) {
        size_t bucket_len = dict->buckets[i_bucket].len;
        if (bucket_len > 0) {
            num_occupied++;
            num_entries += bucket_len;
            
            // A bucket with n > 1 entries counts for n lookups, with lengths:
            
            // n, n-1, n-2, ..., 3, 2, 1,

            // the largest of which is of course n
            
            size_t lookup_len_bucket_sum = (bucket_len*(bucket_len + 1))>>1;

            if (longest_lookup_len < bucket_len) {
                longest_lookup_len = bucket_len;
            }

            total_lookup_len += lookup_len_bucket_sum;
        }
    }

    assert(num_occupied == dict->num_occupied);
    assert(num_entries == dict->num_entries);

    if (avg)
        *avg = (double)total_lookup_len/(double)num_entries;
    if (max)
        *max = longest_lookup_len;
}

/* -------------------------------------------------------------------------- */

void print_summary_IDList(IDList *list) {
    double avg;
    size_t max;    
    measure_lookup_len_IDList(list, &avg, &max);
    
    printf("   IDList   \n"
           "+----------+\n"
           "      Name | \"%s\"\n"
           "      Seed | %"PRIu64"\n"
           "     Slots | %zu\n"
           "  Occupied | %zu\n"
           "      Load | %f\n"
           "Uniformity | %f\n"
           "Avg Lookup | %lf\n"
           "Max Lookup | %zu\n",
           list->name,
           list->seed,
           list->num_slots,
           list->num_occupied,
           list->load_factor,
           IDList_uniformity(list),
           avg,
           max);
}

void print_summary_UIntMap32(UIntMap32 *map) {
    double avg;
    size_t max;    
    measure_lookup_len_UIntMap32(map, &avg, &max);
    
    printf("  UIntMap32  \n"
           "+-----------+\n"
           "       Name | \"%s\"\n"
           "       Seed | %"PRIu32"\n"
           "      Slots | %zu\n"
           "   Occupied | %zu\n"
           "       Load | %f\n"
           " Uniformity | %lf\n"
           " Avg Lookup | %lf\n"
           " Max Lookup | %zu\n",
           map->name,
           map->seed,
           map->num_slots,
           map->num_occupied,
           map->load_factor,
           UIntMap32_uniformity(map),
           avg,
           max);
}

void print_summary_UIntMap64(UIntMap64 *map) {
    double avg;
    size_t max;    
    measure_lookup_len_UIntMap64(map, &avg, &max);
    
    printf("  UIntMap64  \n"
           "+-----------+\n"
           "       Name | \"%s\"\n"
           "       Seed | %"PRIu64"\n"
           "      Slots | %zu\n"
           "   Occupied | %zu\n"
           "       Load | %f\n"
           " Uniformity | %f\n"
           " Avg Lookup | %lf\n"
           " Max Lookup | %zu\n",
           map->name,
           map->seed,
           map->num_slots,
           map->num_occupied,
           map->load_factor,
           UIntMap64_uniformity(map),
           avg,
           max);
}

void print_summary_Index(Index *idx) {
    double avg;
    size_t max;    
    measure_lookup_len_Index(idx, &avg, &max);
    
    printf("    Index    \n"
           "+-----------+\n"
           "       Name | \"%s\"\n"
           "       Seed | %"PRIu64"\n"
           "    Buckets | %zu\n"
           "   Occupied | %zu\n"
           "    Entries | %zu\n"
           "       Load | %f\n"
           " Uniformity | %f\n"
           " Avg Lookup | %lf\n"
           " Max Lookup | %zu\n",
           idx->name,
           idx->seed,
           idx->num_buckets,
           idx->num_occupied,
           idx->num_entries,
           idx->load_factor,
           Index_uniformity(idx),
           avg,
           max);
}

void print_summary_Dict(Dict *dict) {
    double avg;
    size_t max;    
    measure_lookup_len_Dict(dict, &avg, &max);
    
    printf("    Dict    \n"
           "+----------+\n"
           "      Name | \"%s\"\n"
           "      Seed | %"PRIu64"\n"
           "   Buckets | %zu\n"
           "  Occupied | %zu\n"
           "   Entries | %zu\n"
           "      Load | %f\n"
           "Uniformity | %f\n"
           "Avg Lookup | %lf\n"
           "Max Lookup | %zu\n",
           dict->name,
           dict->seed,
           dict->num_buckets,
           dict->num_occupied,
           dict->num_entries,
           dict->load_factor,
           Dict_uniformity(dict),
           avg,
           max);
}


