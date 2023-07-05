#ifndef HASH_FUNCTIONS_H
#define HASH_FUNCTIONS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* -------------------------------------------------------------------------- */

// +-----------------------+
// | String Hash Functions |
// +-----------------------+

/*
static inline uint64_t str_hash_initializer(int seed) {
    return seed;
}

static inline uint64_t str_hash_finalizer(size_t mod, uint64_t hash, int seed) {
    (void)seed;
    return hash & (mod - 1);
}

static inline uint64_t str_hash_step(size_t i, uint64_t hash, char ch) {
    (void)i;
    return hash ^ ((hash<<LSHIFT) + (hash>>RSHIFT) + ch);
}
*/

/*
static size_t str_hash_generic(size_t mod, char const *str) {
    uint64_t hash = str_hash_initializer(SEED);
    size_t len = strlen(str);
    
    for (size_t i_ch = 0; i_ch < len; i_ch++) {
        hash = str_hash_step(i_ch, hash, str[i_ch]);
    }

    return str_hash_finalizer(mod, hash, SEED);
}
*/

//    https://www.cs.wisc.edu/techreports/1970/TR88.pdf
#define LSHIFT 5
#define RSHIFT 2

static inline size_t hash_str_Ramakrishna_Zobel(char const *str, uint64_t seed) {
    uint64_t hash = seed;
    
    size_t len = strlen(str);
    for (size_t i_ch = 0; i_ch < len; i_ch++) {
        hash = hash ^ ((hash<<LSHIFT) + (hash>>RSHIFT) + *(str++));
    }

    return hash;
}

#define hash_str(KEY, MOD, SEED) (hash_str_Ramakrishna_Zobel((KEY), (SEED)) & ((MOD) - 1))



/* -------------------------------------------------------------------------- */

// +-----------------------+
// | uint64 Hash Functions |
// +-----------------------+

static inline uint64_t hash_u64_identity(uint64_t x) {
    return x;
}

// xmxmx
static inline uint64_t hash_u64_murmur3(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    
    return x;
}

// xmxmx
static inline uint64_t hash_u64_something(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    
    return x;
}

static inline uint64_t hash_u64_nomult(uint64_t x) {
    x = ~x;
    x += x << 21;
    x ^= x >> 24;
    x += x << 3;
    x += x << 8;
    x ^= x >> 14;
    x += x << 2;
    x += x << 4;
    x ^= x >> 28;
    x += x << 31;
    
    return x;
}

// xmx
static inline uint64_t hash_u64_fast(uint64_t x) {
    x ^= x >> 23;
    x *= 0x2127599bf4325c37ULL;
    x ^= x >> 47;
    
    return x;
}

// http://jonkagstrom.com/bit-mixer-construction/

// 8 instructions
// x c1 mul 56 xsr c2 mul
static inline uint64_t hash_u64_mxm(uint64_t x) {
	x *= 0xbf58476d1ce4e5b9ull;
	x ^= x >> 56;
	x *= 0x94d049bb133111ebull;
    
	return x;
}

// 9 instructions
// x 23 xsr c3 mul 23 xsr
static inline uint64_t xmx(uint64_t x) {
	x ^= x >> 23;
	x *= 0xff51afd7ed558ccdull;
	x ^= x >> 23;
    
	return x;
}

// 11 instructions
// x c3 mul 32 xsr c3 mul 32 asr
static inline uint64_t mxma(uint64_t x) {
	x *= 0xff51afd7ed558ccdull;
	x ^= x >> 32;
	x *= 0xff51afd7ed558ccdull;
	x += x >> 32;
    
	return x;
}

// 11 instructions
// x c3 mul 47 xsr c1 mul 32 xsr
static inline uint64_t mxmx(uint64_t x) {
	x *= 0xff51afd7ed558ccdull;
	x ^= x >> 47;
	x *= 0xbf58476d1ce4e5b9ull;
	x ^= x >> 32;
    
	return x;
}

static inline uint64_t ror64(uint64_t v, int r) {
	return (v >> r) | (v << (64 - r));
}

// 12 instructions
// x 32 xsr c3 mul 47 23 xrr
static inline uint64_t xmrx(uint64_t x) {
	x ^= x >> 32;
	x *= 0xff51afd7ed558ccdull;
	x ^= ror64(x, 47) ^ ror64(x, 23);
    
	return x;
}

// 13 instructions
// x c1 mul 32 xsr c2 mul 32 xsr c2 mul
static inline uint64_t mxmxm(uint64_t x) {
	x *= 0xbf58476d1ce4e5b9ull;
	x ^= x >> 32;
	x *= 0x94d049bb133111ebull;
	x ^= x >> 32;
	x *= 0x94d049bb133111ebull;
    
	return x;
}

// 14 instructions
// x c2 mul 56 32 xrr c3 mul 23 xsr
static inline uint64_t mxrmx(uint64_t x) {
	x *= 0x94d049bb133111ebull;
	x ^= ror64(x, 56) ^ ror64(x, 32);
	x *= 0xff51afd7ed558ccdull;
	x ^= x >> 23;
    
	return x;
}

// 14 instructions
// x 30 xsr c1  mul 27  xsr c2  mul 31  xsr (split mix)
// x 49 24  xrr c6  mul 28  xsr c6  mul 28  xsr (rrmxmx)
// x 33 xsr c3 mul 33 xsr c4 mul 33 xsr (murmur3)

#define hash_u64(KEY, MOD, SEED) (mxmxm((KEY) ^ (SEED)) & ((MOD) - 1))



/* -------------------------------------------------------------------------- */

// +-----------------------+
// | uint32 Hash Functions |
// +-----------------------+

static inline uint32_t hash_u32_murmur3(uint32_t x) {
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;

    return x;
}

#define hash_u32(KEY, MOD, SEED) (hash_u32_murmur3((uint32_t)(KEY) ^ (uint32_t)(SEED)) & ((uint32_t)(MOD) - 1))

#endif /* HASH_FUNCTIONS_H */
