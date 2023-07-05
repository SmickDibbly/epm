#ifndef MATHEMATICS_H
#define MATHEMATICS_H

#include <limits.h>

#include "src/locallibs/fixpt.h"

#define MASK64(X) ((X) >> (sizeof(int64_t) * CHAR_BIT - 1))
#define MASK32(X) ((X) >> (sizeof(int32_t) * CHAR_BIT - 1))

#define ABS_OBVIOUS(X) ((X) < 0 ? -(X) : (X))

#define ABS_BRANCHLESS1(X) (((X) ^ -((X)<0)) + ((X)<0)) // slower on testing

#define ABS64_BRANCHLESS2(X) (((X) + MASK64(X)) ^ MASK64(X))
// seems to be significantly faster when gcc -O3

#define ABS32_BRANCHLESS2(X) (((X) + MASK32(X)) ^ MASK32(X))
// seems to be faster when gcc -O3

#define MIN_OBVIOUS(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX_OBVIOUS(X, Y) ((X) > (Y) ? (X) : (Y))

// might be very slightly faster when optimized
#define MIN_BRANCHLESS(X, Y) ((Y) ^ (((X) ^ (Y)) & -((X) < (Y))))
#define MAX_BRANCHLESS(X, Y) ((X) ^ (((X) ^ (Y)) & -((Y) < (X))))

#undef ABS
#undef MIN
#undef MAX
#undef MIN3
#undef MAX3
#undef MIN4
#undef MAX4
#define ABS(X) ABS_OBVIOUS(X)
#define MIN(X, Y) MIN_OBVIOUS(X, Y)
#define MAX(X, Y) MAX_OBVIOUS(X, Y)
#define MIN3(X, Y, Z) (MIN(MIN((X), (Y)), (Z)))
#define MAX3(X, Y, Z) (MAX(MAX((X), (Y)), (Z)))
#define MIN4(W, X, Y, Z) MIN(MIN(MIN((W), (X)), (Y)), (Z))
#define MAX4(W, X, Y, Z) MAX(MAX(MAX((W), (X)), (Y)), (Z))

extern UFix32 sqrt_ufix32(UFix32 z);
extern UFix64 sqrt_ufix32_from_ufix64(UFix64 z);

/* ANGLES AND TRIGONOMETRY
 *
 * A variable of type ang18_t is used to represent an angle using 18 bits of
 * precision (in a 32-bit container). A value of Y corresponds to the angle that
 * is Y/(2**18) of a full circle. In terms of radians, a value of Y corresponds
 * to 2*PI*Y/(2**18) radians. Hence 2**18 represents a full circle, or 2PI.
*/

typedef uint32_t Ang18;

#define ANG18_2PI (1<<18)
#define ANG18_2PI_MASK (ANG18_2PI - 1)

#define ANG18_PI (ANG18_2PI>>1)
#define ANG18_PI_MASK (ANG18_PI - 1)

#define ANG18_PI2 (ANG18_2PI>>2)
#define ANG18_PI2_MASK (ANG18_PI2 - 1)

#define ANG18_PI4 (ANG18_2PI>>3)
#define ANG18_PI4_MASK (ANG18_PI4 - 1)

/** 
 * @ang is the angle.
 * @sin and @cos are filled in with the sin and cos value,
 * respectively, in game standard s16.u16 Fix32 format.
 */
extern void cossin18(Fix32 *cos, Fix32 *sin, Ang18 ang);
extern Fix32 cos18(Ang18 ang);
extern Fix32 sin18(Ang18 ang);
extern Fix32 tan18(Ang18 ang);

#define ang18_in_degrees(ANG) (360*(((uint64_t)(ANG))<<16)/(uint64_t)ANG18_2PI)

#endif /* MATHEMATICS_H */
