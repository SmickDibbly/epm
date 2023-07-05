#ifndef FIXPT_H
#define FIXPT_H

#include <stdint.h>

/* As of 2023-05-10 this library makes the following assumptions:

   1) Twos-complement integer representation.
   
   2) A rightshift of a signed and negative value is arithmetic
   (sign-extension). According to the C99 standard, this is
   implementation-defined behavior. (GCC abides by sign-extension).   
*/


/* Fixed-point types. Note that the number 32 or 64 in the type names and
   related identifiers specify the storage size, *not* the fractional
   precision. So, for example, "fix32_t" is a 32-bit type intended to represent
   a fixed point number of whatever precision the programmer chooses.
*/

typedef int32_t Fix;
typedef int32_t Fix32;
//typedef int32_t fix32_t;
#define FIX32_MAX INT32_MAX
#define FIX32_MIN INT32_MIN

typedef uint32_t UFix32;
//typedef uint32_t ufix32_t;
#define UFIX32_MAX UINT32_MAX
#define UFIX32_MIN (0)

typedef int64_t Fix64;
//typedef int64_t fix64_t;
#define FIX64_MAX INT64_MAX
#define FIX64_MIN INT64_MIN

typedef uint64_t UFix64;
//typedef uint64_t ufix64_t;
#define UFIX64_MAX UINT64_MAX
#define UFIX64_MIN (0)

// To avoid the undefined behavior of signed-and-negative left shift, we cast to
// the corresponding unsigned type and then back to the signed type. Macros for
// right-shift are provided for the sake of completion, though they expand to a
// plain cast-free shift and thus can be safely ignored.
#define LSHIFT32(NUM, AMT) (int32_t)((uint32_t)(NUM) << (AMT))
#define RSHIFT32(NUM, AMT) ((NUM) >> (AMT))
#define LSHIFT64(NUM, AMT) (int64_t)((uint64_t)(NUM) << (AMT))
#define RSHIFT64(NUM, AMT) ((NUM) >> (AMT))


/* -------------------------------------------------------------------------- */

/* The following macros presume a precision of 16 */
#define FIX_P16_BITS 16
#define FIX_P16_MASK 0xFFFF
#define FIX_P16_ONE  (1 << FIX_P16_BITS)

// TODO: Upcase to INTIFY and FIXIFY for consistency.
#define intify(x) RSHIFT32((x), FIX_P16_BITS)
#define fixify(x) LSHIFT32((x), FIX_P16_BITS)

#define FIX_MUL(x, y)  ( RSHIFT64(((Fix64)(x) * (Fix64)(y)), FIX_P16_BITS) )
#define FIX_DIV(x, y)  ( (LSHIFT64((Fix64)(x), FIX_P16_BITS)) / (Fix64)(y) )
#define FIX_MULDIV(x, y, z) ( ((Fix64)(x) * (Fix64)(y)) / (Fix64)(z) )
// FIX32_MULDIV(x, y, z) is "x times y, divided by z". This macro exists because
// using FIX_MUL followed by FIX_DIV causes the FIX_MUL result to be downshifted
// by 16 and then immediately upshifted by 16. This macro avoids that.

#define FIX_MUL32(x, y)  ((Fix32)(RSHIFT64(((Fix64)(x) * (Fix64)(y)), FIX_P16_BITS)))
#define FIX_DIV32(x, y)  ((Fix32)((LSHIFT64((Fix64)(x), FIX_P16_BITS)) / (Fix64)(y)))
#define FIX_MULDIV32(x, y, z) ((Fix32)(((Fix64)(x) * (Fix64)(y)) / (Fix64)(z)))

#define FIX_MUL64(x, y)  ((Fix64)(RSHIFT64(((Fix64)(x) * (Fix64)(y)), FIX_P16_BITS)))
#define FIX_DIV64(x, y)  ((Fix64)((LSHIFT64((Fix64)(x), FIX_P16_BITS)) / (Fix64)(y)))
#define FIX_MULDIV64(x, y, z) ((Fix64)(((Fix64)(x) * (Fix64)(y)) / (Fix64)(z)))

#define UFIX_MUL(x, y)  ( ((uFix64)(x) * (uFix64)(y)) >> FIX_P16_BITS )
#define UFIX_DIV(x, y)  ( ((uFix64)(x) << FIX_P16_BITS) / (uFix64)(y) )
#define UFIX_MULDIV(x, y, z) (UFIX_DIV(UFIX_MUL((x), (y)), (z)))


// Transform a .16 fixed-point to a double with the same numerical value, and
// vice versa. TODO: Why on earth did I capitalize FIX but not dbl ???
#define FIX_dbl(x) ((double)(x)/FIX_P16_ONE)
#define dbl_FIX(x) (Fix64)((x)*FIX_P16_ONE)
#define UFIX_dbl(x) ((double)(x)/FIX_P16_ONE)
#define dbl_UFIX(x) (uFix64)((x)*FIX_P16_ONE)



// 3D geometry, s16.u16 fixed point units.
#define MAX_POINTS_PER_POLYGON 100

#define I_X (0)
#define I_Y (1)
#define I_Z (2)
#define I_DX (0)
#define I_DY (1)
#define I_DZ (2)

#define x_of(POINT_OR_RECT) ((POINT_OR_RECT).v[0])
#define y_of(POINT_OR_RECT) ((POINT_OR_RECT).v[1])
#define z_of(POINT_OR_RECT) ((POINT_OR_RECT).v[2])

#define dx_of(RECT) ((RECT).dv[0])
#define dy_of(RECT) ((RECT).dv[1])
#define dz_of(RECT) ((RECT).dv[2])

#define xmin_of(MM) ((MM).min[0])
#define ymin_of(MM) ((MM).min[1])
#define zmin_of(MM) ((MM).min[2])

#define xmax_of(MM) ((MM).max[0])
#define ymax_of(MM) ((MM).max[1])
#define zmax_of(MM) ((MM).max[2])

typedef struct Fix32Vec {
    Fix32 v[3];
} Fix32Vec;
typedef struct Fix32Seg {
    Fix32Vec pt0;
    Fix32Vec pt1;
} Fix32Seg;
typedef struct Fix32Sphere {
    Fix32Vec center;
    Fix32 radius;
} Fix32Sphere;
typedef struct Fix32MinMax {
    Fix32 min[3];
    Fix32 max[3];
} Fix32MinMax;
typedef struct Fix32Rect {
    Fix32 v[3];
    Fix32 dv[3];
} Fix32Rect;
typedef struct Fix32Line {
    Fix32Vec origin; // just some chosen point on the line
    Fix32Vec dir; // direction vector
} Fix32Line;
typedef struct Fix32LineExact {
    //unnormalized normal vector
    //   Ax + By + Cz = D
    Fix32 A;
    Fix32 B;
    Fix32 C;
    Fix64 D;
} Fix32LineExact;
typedef struct Fix32Plane {
    Fix32Vec ref; // just some chosen point on the plane
    Fix32Vec normal; // the "A", "B", and "C" in Ax + By + Cz = D
    Fix64 constant;  //               the "D" in Ax + By + Cz = D
    // note the constant is stored with a higher precision of s32.u32. I don't
    // remember exactly why I chose to do this, but there really was a use case
    // for it at one point.
} Fix32Plane;

typedef struct Fix32PointNormal {
    Fix32Vec ref; // just some chosen point on the plane
    Fix32Vec normal; // the "A", "B", and "C" in Ax + By + Cz = D
} Fix32PointNormal;

// 2D geometry, 16.16 fixed point units. The type identifiers are suffixed with
// _2D, whereas the 3D geometry has no such suffix.
typedef struct Fix32Vec_2D {
    Fix32 x;
    Fix32 y;
} Fix32Vec_2D;
typedef struct Fix32Seg_2D {
    Fix32Vec_2D pt0;
    Fix32Vec_2D pt1;
} Fix32Seg_2D;
typedef struct Fix32Circle_2D {
    Fix32Vec_2D center;
    Fix32 radius;
} Fix32Circle_2D;
typedef struct Fix32Polygon_2D {
    uint32_t num_points;
    Fix32Vec_2D pt[MAX_POINTS_PER_POLYGON];
} Fix32Polygon_2D;
typedef struct Fix32Rect_2D {
    Fix32 x;
    Fix32 y;
    Fix32 w;
    Fix32 h;
} Fix32Rect_2D;
typedef struct Fix32Line_2D {
    Fix32Vec_2D origin; // just some chosen point on the line
    Fix32Vec_2D normal; // the "A" and "B" in Ax + By = C
    Fix32 constant;  // the "C" in Ax + By = C
} Fix32Line_2D;
typedef struct Fix32LineExact_2D {
    //unnormalized normal vector
    //   Ax + By = C
    Fix32 A;
    Fix32 B;
    Fix64 C; // note the 64 bit width
} Fix32LineExact_2D;





/* Formatting functions for printing fixed points and composites thereof. */

#define MAX_FIX32_STRLEN 11
#define MAX_FIX32_EXACT_STRLEN 23
//extern char *fmt_fix32(Fix32 x);
//extern char *fmt_fix32_exact(Fix32 x);
//extern char *fmt_fix64(Fix64 x);

extern char *fmt_fix_d(Fix64 num, uint8_t shift, uint8_t prec);
extern char *fmt_fix_x(Fix64 num, uint8_t shift);
extern char *fmt_ufix_d(UFix64 num, uint8_t shift, uint8_t prec);
extern char *fmt_ufix_x(UFix64 num, uint8_t shift);

// "(%s, %s, %s)"
extern char *fmt_Fix32Vec(Fix32Vec pt);
#define MAX_FIX32POINT_STRLEN (1 + MAX_FIX32_STRLEN +   \
                               2 + MAX_FIX32_STRLEN +   \
                               2 + MAX_FIX32_STRLEN +   \
                               1)

// "%s --> %s"
extern char *fmt_Fix32Seg(Fix32Seg seg);
#define MAX_FIX32SEG_STRLEN (0 + MAX_FIX32POINT_STRLEN +    \
                             5 + MAX_FIX32POINT_STRLEN)

// "(C: %s, R: %s)"
extern char *fmt_Fix32Sphere(Fix32Sphere sphere);
#define MAX_FIX32SPHERE_STRLEN (4 + MAX_FIX32POINT_STRLEN +  \
                                5 + MAX_FIX32_STRLEN)

// "(%s, %s, %s;  %s, %s, %s)"
extern char *fmt_Fix32Rect(Fix32Rect rect);
#define MAX_FIX32RECT_STRLEN (1 + MAX_FIX32_STRLEN + 2 + MAX_FIX32_STRLEN + \
                              2 + MAX_FIX32_STRLEN + 3 + MAX_FIX32_STRLEN + \
                              2 + MAX_FIX32_STRLEN + 2 + MAX_FIX32_STRLEN + \
                              1)

// TODO
extern char *fmt_Fix32Line(Fix32Rect line);
#define MAX_FIX32LINE_STRLEN TODO

// TODO
extern char *fmt_Fix32LineExact(Fix32Rect line);
#define MAX_FIX32LINEEXACT_STRLEN TODO

// "(%s)X + (%s)Y + (%s)Z = %s"
extern char *fmt_Fix32Plane(Fix32Plane plane);
#define MAX_FIX32PLANE_STRLEN (1 + MAX_FIX32_STRLEN +   \
                               6 + MAX_FIX32_STRLEN +   \
                               6 + MAX_FIX32_STRLEN +   \
                               5 + MAX_FIX32_STRLEN)


// "(%s, %s)"        
extern char *fmt_Fix32Vec_2D(Fix32Vec_2D pt);
#define MAX_FIX32POINT_2D_STRLEN (1 + MAX_FIX32_STRLEN + \
                                  2 + MAX_FIX32_STRLEN + \
                                  1)

// "%s --> %s"
extern char *fmt_Fix32Seg_2D(Fix32Seg_2D seg);
#define MAX_FIX32SEG_2D_STRLEN (0 + MAX_FIX32POINT_2D_STRLEN +  \
                                5 + MAX_FIX32POINT_2D_STRLEN)

// "(C: %s, R: %s)"
extern char *fmt_Fix32Circle_2D(Fix32Circle_2D circ);
#define MAX_FIX32CIRCLE_2D_STRLEN (4 + MAX_FIX32POINT_2D_STRLEN + \
                                   5 + MAX_FIX32_STRLEN +         \
                                   1)

// TODO
extern char *fmt_Fix32Polygon_2D(Fix32Polygon_2D poly);
#define MAX_FIX32POLYGON_2D_STRLEN TODO // no reasonable max

// "(%s, %s;  %s, %s)"
extern char *fmt_Fix32Rect_2D(Fix32Rect_2D rect);
#define MAX_FIX32RECT_2D_STRLEN (1 + MAX_FIX32_STRLEN + 2 + MAX_FIX32_STRLEN + \
                                 3 + MAX_FIX32_STRLEN + 2 + MAX_FIX32_STRLEN + \
                                 1)

// TODO
extern char *fmt_Fix32Line_2D(Fix32Line_2D line);
#define MAX_FIX32LINE_2D_STRLEN TODO

// TODO
extern char *fmt_Fix32LineExact_2D(Fix32LineExact_2D line);
#define MAX_FIX32LINEEXACT_2D_STRLEN TODO


#define put_fix32(VAR)                                  \
    fprintf(stdout, #VAR " = %s\n", str_fix32((VAR)))
#define put_fix32_exact(VAR)                                \
    fprintf(stdout, #VAR " = %s\n", str_fix32_exact((VAR)))
#define put_fix64(VAR)                                  \
    fprintf(stdout, #VAR " = %s\n", str_fix64((VAR)))



// TODO: Do I use these ever?
typedef struct Fix64Vec {
    Fix64 v[3];
} Fix64Vec;
typedef struct Fix64Seg {
    Fix64Vec pt0;
    Fix64Vec pt1;
} Fix64Seg;
typedef struct Fix64Sphere {
    Fix64Vec center;
    Fix64 radius;
} Fix64Sphere;
typedef struct Fix64Rect {
    Fix64 v[3];
    Fix64 dv[3];
} Fix64Rect;
typedef struct Fix64Line {
    Fix64Vec origin; // just some chosen point on the line
    Fix64Vec dir; // direction vector
} Fix64Line;
typedef struct Fix64LineExact {
    //unnormalized normal vector
    //   Ax + By + Cz = D
    Fix64 A;
    Fix64 B;
    Fix64 C;
    Fix64 D;
} Fix64LineExact;
typedef struct Fix64Plane {
    Fix64Vec ref; // just some chosen point on the plane
    Fix64Vec normal; // the "A", "B", and "C" in Ax + By + Cz = D
    Fix64 constant;  //               the "D" in Ax + By + Cz = D
    // note the constant is stored with higher precision
} Fix64Plane;

// 2D geometry, 16.16 fixed point units.
typedef struct Fix64Vec_2D {
    Fix64 x;
    Fix64 y;
} Fix64Vec_2D;
typedef struct Fix64Seg_2D {
    Fix64Vec_2D pt0;
    Fix64Vec_2D pt1;
} Fix64Seg_2D;
typedef struct Fix64Circle_2D {
    Fix64Vec_2D center;
    Fix64 radius;
} Fix64Circle_2D;
typedef struct Fix64Polygon_2D {
    uint64_t num_points;
    Fix64Vec_2D pt[MAX_POINTS_PER_POLYGON];
} Fix64Polygon_2D;
typedef struct Fix64Rect_2D {
    Fix64 x;
    Fix64 y;
    Fix64 w;
    Fix64 h;
} Fix64Rect_2D;
typedef struct Fix64Line_2D {
    Fix64Vec_2D origin; // just some chosen point on the line
    Fix64Vec_2D normal; // the "A" and "B" in Ax + By = C
    Fix64 constant;  // the "C" in Ax + By = C
} Fix64Line_2D;
typedef struct Fix64LineExact_2D {
    //unnormalized normal vector
    //   Ax + By = C
    Fix64 A;
    Fix64 B;
    Fix64 C; // note the 64 bit width
} Fix64LineExact_2D;


#endif /* FIXPT_H */
