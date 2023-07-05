#include <stdio.h>
#include "./fixpt.h"
#include <inttypes.h>

#define MAX_STRINGS 16
#define MAX_STRLEN 127
// we must have 2^SHIFT = MAX_STRINGS
static char str_ring[MAX_STRINGS][MAX_STRLEN + 1] = {0};
static size_t i_str = 0;

char *fmt_fix_x(Fix64 num, uint8_t shift) {
    i_str &= (MAX_STRINGS - 1);

    if (num >= 0)
        snprintf(str_ring[i_str], MAX_STRLEN, " %0lX.%0*lX",
                 num>>shift,
                 shift/4, // TODO: Does this work on non-multiples of 4 ?
                 (num & ((1Lu<<shift)-1)));
    else
        snprintf(str_ring[i_str], MAX_STRLEN, "-%0lX.%0*lX",
                 (-num)>>shift,
                 shift/4, // TODO: Does this work on non-multiples of 4 ?
                 ((-num) & ((1Lu<<shift)-1)));

    return str_ring[i_str++];
}

char *fmt_ufix_x(UFix64 num, uint8_t shift) {
    i_str &= (MAX_STRINGS - 1);

    snprintf(str_ring[i_str], MAX_STRLEN, " %0lX.%0*lX",
             num>>shift,
             shift/4, // TODO: Does this work on non-multiples of 4 ?
             (num & ((1Lu<<shift)-1)));
    
    return str_ring[i_str++];
}



static const uint64_t d_multiplier[] = {
    1Lu,
    10Lu,
    100Lu,
    1000Lu,
    10000Lu,
    100000Lu,
    1000000Lu,
    10000000Lu,
    100000000Lu,
    1000000000Lu,
    10000000000Lu,
    100000000000Lu,
    1000000000000Lu,
    10000000000000Lu,
    100000000000000Lu, // highest multiplier that guarantees no overflow for 16
                       // bit precision
    1000000000000000Lu,
    10000000000000000Lu,
    100000000000000000Lu,
    1000000000000000000Lu,
    10000000000000000000Lu,
};


char *fmt_fix_d(Fix64 num, uint8_t shift, uint8_t prec) {
    i_str &= (MAX_STRINGS - 1);
    
    if (num >= 0) {
        snprintf(str_ring[i_str],
                 MAX_STRLEN + 1,
                 "%0"PRIu64".%0*"PRIu64,
                 num>>shift,
                 prec,
                 ((num & ((1Lu<<shift)-1))*d_multiplier[prec])>>shift);
    }
    else {
        snprintf(str_ring[i_str],
                 MAX_STRLEN + 1,
                 "-%0"PRIu64".%0*"PRIu64,
                 (-num)>>shift,
                 prec,
                 (((-num) & ((1Lu<<shift)-1))*d_multiplier[prec])>>shift);
    }
    
    return str_ring[i_str++];
}


char *fmt_ufix_d(UFix64 num, uint8_t shift, uint8_t prec) {
    i_str &= (MAX_STRINGS - 1);

    snprintf(str_ring[i_str],
             MAX_STRLEN + 1,
             "%0"PRIu64".%0*"PRIu64,
             num>>shift,
             prec,
             ((num & ((1Lu<<shift)-1))*d_multiplier[prec])>>shift);
    
    return str_ring[i_str++];
}




/*
char *fmtp_fix32(fix32_t x, uint32_t precision) {
    // prints an fix32_t value with 4 digits of fractional precision.
    i_str &= (MAX_STRINGS - 1);
    
    if (x >= 0) {
        snprintf(str_ring[i_str],
                 MAX_FIX32_STRLEN + 1,
                 "%0d.%04d",
                 x>>precision,
                 ((x & 0xFFFF)*10000)>>precision);
    }
    else {
        snprintf(str_ring[i_str],
                 MAX_FIX32_STRLEN + 1,
                 "-%0d.%04d",
                 (-x)>>precision,
                 (((-x) & 0xFFFF)*10000)>>precision);
    }
    
    return str_ring[i_str++];
}

char *fmt_fix32(fix32_t x) {
    // prints an fix32_t value with 4 digits of fractional precision.
    i_str &= (MAX_STRINGS - 1);
    
    if (x >= 0) {
        snprintf(str_ring[i_str],
                 MAX_FIX32_STRLEN + 1,
                 "%0d.%04d",
                 x>>FIX_BITS,
                 ((x & 0xFFFF)*10000)>>FIX_BITS);
    }
    else {
        snprintf(str_ring[i_str],
                 MAX_FIX32_STRLEN + 1,
                 "-%0d.%04d",
                 (-x)>>FIX_BITS,
                 (((-x) & 0xFFFF)*10000)>>FIX_BITS);
    }
    
    return str_ring[i_str++];
}

char *fmt_fix32_exact
(fix32_t x) {
    // prints an fix32_t value with 14 digits of fractional precision needed to display exact values.

    // The 14 is maximal in the following sense: Since fix32_t has 16 binary digitsof fractional precision, then in an unsigned 64-bit integer there are 48 more bits of free space to utilize. This a fractional part of a fix32_t can be multiplied by up to 2^48 without overflowing. But we actually want to multiply a power of 10, and 10^14 is highest power of 10 less than 2^48. Thus in converting the fractional part of fix32_t to decimal, multiplying by 10^14 is safe, but 10^15 is unsafe.
    i_str &= (MAX_STRINGS - 1);

    if (x >= 0) {
        snprintf(str_ring[i_str],
                 MAX_FIX32_EXACT_STRLEN + 1,
                 "%0"PRIu32".%014"PRIu64,
                 x>>FIX_BITS,
                 (((uint64_t)x & 0xFFFF)*100000000000000Lu)>>FIX_BITS);
    }   
    else {
        snprintf(str_ring[i_str],
                 MAX_FIX32_EXACT_STRLEN + 1,
                 "-%0"PRIu32".%014"PRIu64,
                 (-x)>>FIX_BITS,
                 (((uint64_t)(-x) & 0xFFFF)*100000000000000Lu)>>FIX_BITS);
    }
    
    return str_ring[i_str++];
}

char *fmt_fix64
(fix64_t x) {
    i_str &= (MAX_STRINGS - 1);

    if (x >= 0)
        snprintf(str_ring[i_str], 51, "%0ld.%08ld", x>>FIX_BITS,
                 ((x & 0xFFFFFFFF)*100000000)>>FIX_BITS);
    else
        snprintf(str_ring[i_str], 51, "-%0ld.%08ld", (-x)>>FIX_BITS,
                 (((-x) & 0xFFFFFFFF)*100000000)>>FIX_BITS);

    return str_ring[i_str++];
}

*/




char *fmt_Fix32Vec(Fix32Vec pt) {
    i_str &= (MAX_STRINGS - 1); //modulo
    
    snprintf(str_ring[i_str], MAX_FIX32POINT_STRLEN + 1,
             "(%s, %s, %s)",
             fmt_fix_d(x_of(pt), 16, 4),
             fmt_fix_d(y_of(pt), 16, 4),
             fmt_fix_d(z_of(pt), 16, 4));
    
    return str_ring[i_str++];
}

char *fmt_Fix32Seg(Fix32Seg seg) {
    i_str &= (MAX_STRINGS - 1); //modulo
    
    snprintf(str_ring[i_str], MAX_FIX32SEG_STRLEN + 1,
             "(%s, %s, %s) --> (%s, %s, %s)",
             fmt_fix_d(x_of(seg.pt0), 16, 4),
             fmt_fix_d(y_of(seg.pt0), 16, 4),
             fmt_fix_d(z_of(seg.pt0), 16, 4),
             fmt_fix_d(x_of(seg.pt1), 16, 4),
             fmt_fix_d(y_of(seg.pt1), 16, 4),
             fmt_fix_d(z_of(seg.pt1), 16, 4));
    
    return str_ring[i_str++];
}

char *fmt_Fix32Sphere(Fix32Sphere sphere) {
    i_str &= (MAX_STRINGS - 1); //modulo

    snprintf(str_ring[i_str], MAX_FIX32SPHERE_STRLEN + 1,
             "(C: %s, R: %s)",
             fmt_Fix32Vec(sphere.center),
             fmt_fix_d(sphere.radius, 16, 4));

    return str_ring[i_str++];
}

char *fmt_Fix32Rect(Fix32Rect rect) {
    i_str &= (MAX_STRINGS - 1); //modulo
    
    snprintf(str_ring[i_str], MAX_FIX32RECT_STRLEN + 1,
             "(%s, %s, %s;  %s, %s, %s)",
             fmt_fix_d(x_of(rect), 16, 4),
             fmt_fix_d(y_of(rect), 16, 4),
             fmt_fix_d(z_of(rect), 16, 4),
             fmt_fix_d(dx_of(rect), 16, 4),
             fmt_fix_d(dy_of(rect), 16, 4),
             fmt_fix_d(dz_of(rect), 16, 4));
    
    return str_ring[i_str++];
}

char *fmt_Fix32Plane(Fix32Plane plane) {
    i_str &= (MAX_STRINGS - 1); //modulo

    // By default don't bother printing the reference point. TODO: What if want
    // to?
    
    snprintf(str_ring[i_str], MAX_FIX32PLANE_STRLEN + 1,
             "(%s)X + (%s)Y + (%s)Z = %s",
             fmt_fix_d(x_of(plane.normal), 16, 4),
             fmt_fix_d(y_of(plane.normal), 16, 4),
             fmt_fix_d(z_of(plane.normal), 16, 4),
             fmt_fix_d(plane.constant, 16, 4));
    
    return str_ring[i_str++];
}



extern char *fmt_fix_dPoint_2D(Fix32Vec_2D pt) {
    i_str &= (MAX_STRINGS - 1); //modulo

    snprintf(str_ring[i_str], MAX_FIX32POINT_2D_STRLEN + 1,
             "(%s, %s)",
             fmt_fix_d(pt.x, 16, 4),
             fmt_fix_d(pt.y, 16, 4));
    
    return str_ring[i_str++];
}

extern char *fmt_fix_dSeg_2D(Fix32Seg_2D seg) {
    i_str &= (MAX_STRINGS - 1); //modulo

    snprintf(str_ring[i_str], MAX_FIX32SEG_2D_STRLEN + 1,
             "(%s, %s) --> (%s, %s)",
             fmt_fix_d(seg.pt0.x, 16, 4),
             fmt_fix_d(seg.pt0.y, 16, 4),
             fmt_fix_d(seg.pt1.x, 16, 4),
             fmt_fix_d(seg.pt1.y, 16, 4));
    
    
    return str_ring[i_str++];
}

extern char *fmt_fix_dCircle_2D(Fix32Circle_2D circ) {
    i_str &= (MAX_STRINGS - 1); //modulo

    snprintf(str_ring[i_str], MAX_FIX32CIRCLE_2D_STRLEN + 1,
             "(C: %s, R: %s)",
             fmt_fix_dPoint_2D(circ.center),
             fmt_fix_d(circ.radius, 16, 4));

    return str_ring[i_str++];
}

extern char *fmt_fix_dRect_2D(Fix32Rect_2D rect) {
    i_str &= (MAX_STRINGS - 1); //modulo

    snprintf(str_ring[i_str], MAX_FIX32RECT_2D_STRLEN + 1,
             "(%s, %s;  %s, %s)",
             fmt_fix_d(rect.x, 16, 4),
             fmt_fix_d(rect.y, 16, 4),
             fmt_fix_d(rect.w, 16, 4),
             fmt_fix_d(rect.h, 16, 4));

    
    return str_ring[i_str++];
}
