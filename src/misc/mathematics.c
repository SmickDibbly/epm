#include "src/misc/mathematics.h"

UFix32 sqrt_ufix32(UFix32 z) {
    UFix32 prev_guess = 0;
    UFix32 guess = fixify(1);

    while (prev_guess != guess && guess != 0) {
        prev_guess = guess;
        guess = (guess + (Fix32)FIX_DIV(z, guess)) >> 1;
    }

    return guess;
}

UFix64 sqrt_ufix32_from_ufix64(UFix64 z) {
    // parameter is 32.32
    Fix64 prev_guess = 0;
    Fix64 guess = 1 << 16; // 1 as (47)1.16
    
    while (ABS(prev_guess - guess) > 10 && guess != 0) {
        prev_guess = guess;
        guess = ( ( guess  ) + ((  z  )/( guess  )) )>>1;
        //      ( (47)1.16 ) + ((32.32)/((47)1.16))  =  (47)1.16 + 31.16
    }

    return guess;
}



/* ANGLES AND TRIGONOMETRY */

#define ANG18_TABLE_LEN ((ANG18_2PI>>10)+2)
#define ANG18_TABLE_GEN_SHIFT (18-8)

//                         256+2
static Fix32 sin_lut18[ANG18_TABLE_LEN] = {
    0x0000, 0x0192, 0x0324, 0x04B6, 0x0648, 0x07DA, 0x096C, 0x0AFE, 0x0C8F, 0x0E21, 0x0FB2, 0x1144, 0x12D5, 0x1466, 0x15F6, 0x1787, 0x1917, 0x1AA7, 0x1C37, 0x1DC7, 0x1F56, 0x20E5, 0x2273, 0x2402, 0x2590, 0x271D, 0x28AA, 0x2A37, 0x2BC4, 0x2D50, 0x2EDB, 0x3066, 0x31F1, 0x337B, 0x3505, 0x368E, 0x3817, 0x399F, 0x3B26, 0x3CAD, 0x3E33, 0x3FB9, 0x413E, 0x42C3, 0x4447, 0x45CA, 0x474D, 0x48CE, 0x4A50, 0x4BD0, 0x4D50, 0x4ECF, 0x504D, 0x51CA, 0x5347, 0x54C3, 0x563E, 0x57B8, 0x5931, 0x5AAA, 0x5C22, 0x5D98, 0x5F0E, 0x6083, 0x61F7, 0x636A, 0x64DC, 0x664D, 0x67BD, 0x692D, 0x6A9B, 0x6C08, 0x6D74, 0x6EDF, 0x7049, 0x71B1, 0x7319, 0x7480, 0x75E5, 0x774A, 0x78AD, 0x7A0F, 0x7B70, 0x7CD0, 0x7E2E, 0x7F8B, 0x80E7, 0x8242, 0x839C, 0x84F4, 0x864B, 0x87A1, 0x88F5, 0x8A48, 0x8B9A, 0x8CEA, 0x8E39, 0x8F87, 0x90D3, 0x921E, 0x9368, 0x94B0, 0x95F6, 0x973C, 0x987F, 0x99C2, 0x9B02, 0x9C42, 0x9D7F, 0x9EBC, 0x9FF6, 0xA12F, 0xA267, 0xA39D, 0xA4D2, 0xA605, 0xA736, 0xA866, 0xA994, 0xAAC0, 0xABEB, 0xAD14, 0xAE3B, 0xAF61, 0xB085, 0xB1A8, 0xB2C8, 0xB3E7, 0xB504, 0xB620, 0xB73A, 0xB852, 0xB968, 0xBA7C, 0xBB8F, 0xBCA0, 0xBDAE, 0xBEBC, 0xBFC7, 0xC0D0, 0xC1D8, 0xC2DE, 0xC3E2, 0xC4E3, 0xC5E4, 0xC6E2, 0xC7DE, 0xC8D8, 0xC9D1, 0xCAC7, 0xCBBB, 0xCCAE, 0xCD9F, 0xCE8D, 0xCF7A, 0xD064, 0xD14D, 0xD233, 0xD318, 0xD3FA, 0xD4DB, 0xD5B9, 0xD695, 0xD770, 0xD848, 0xD91E, 0xD9F2, 0xDAC4, 0xDB94, 0xDC61, 0xDD2D, 0xDDF6, 0xDEBE, 0xDF83, 0xE046, 0xE106, 0xE1C5, 0xE282, 0xE33C, 0xE3F4, 0xE4AA, 0xE55E, 0xE60F, 0xE6BE, 0xE76B, 0xE816, 0xE8BF, 0xE965, 0xEA09, 0xEAAB, 0xEB4B, 0xEBE8, 0xEC83, 0xED1C, 0xEDB2, 0xEE46, 0xEED8, 0xEF68, 0xEFF5, 0xF080, 0xF109, 0xF18F, 0xF213, 0xF294, 0xF314, 0xF391, 0xF40B, 0xF484, 0xF4FA, 0xF56D, 0xF5DE, 0xF64D, 0xF6BA, 0xF724, 0xF78B, 0xF7F1, 0xF853, 0xF8B4, 0xF912, 0xF96E, 0xF9C7, 0xFA1E, 0xFA73, 0xFAC5, 0xFB14, 0xFB61, 0xFBAC, 0xFBF5, 0xFC3B, 0xFC7E, 0xFCBF, 0xFCFE, 0xFD3A, 0xFD74, 0xFDAB, 0xFDE0, 0xFE13, 0xFE43, 0xFE70, 0xFE9B, 0xFEC4, 0xFEEA, 0xFF0E, 0xFF2F, 0xFF4E, 0xFF6A, 0xFF84, 0xFF9C, 0xFFB1, 0xFFC3, 0xFFD3, 0xFFE1, 0xFFEC, 0xFFF4, 0xFFFB, 0xFFFE, 0x10000, 0xFFFE
}; 

#define FIX_MUL8(x, y) (((x)*(y))>>8)

static Fix32 sin_lookup18(Ang18 ang) {
    Fix32 frac, intg, s0, s1;

    frac = ang & 0xff;
    intg = ang >> 8;

    // See the comments in the definition of cossin18() for why @ang can be
    // assumed to be in the interval [0, PI/2]. Hence: 

    // intg = ang>>8 in [0, (PI/2)>>8] = [0, (2PI)>>10] = [0, ANG18_TABLE_LEN-1]

    // That is, intg     in [0, @ANG18_TABLE_LEN-1] and 
    //          intg + 1 in [1, @ANG18_TABLE_LEN], as required.
    
    s0 = sin_lut18[intg];
    s1 = sin_lut18[intg + 1];
    return (s0 + FIX_MUL8(s1 - s0, frac));
}

void cossin18(Fix32 *cos, Fix32 *sin, Ang18 ang) {
    ang &= ANG18_2PI_MASK; // ang in [0, 2PI)
    Ang18 amp = ang & ANG18_PI_MASK; // amp in [0, PI)
    // amp = "angle modulo pi"
    
    if (amp >= ANG18_PI2) { // amp in [PI/2, PI)
        *cos = -sin_lookup18(amp - ANG18_PI2); // amp-PI/2 in [0, PI/2)
        *sin =  sin_lookup18(ANG18_PI - amp);  // PI-amp in (0, PI/2]
        
        // Since PI/2 <= amp, it follows that PI <= (PI/2 + amp). Altogether, we
        // have amp < PI <= (PI/2 + amp), which gives 0 < (PI - amp) <= PI/2
    }
    else { // amp in [0, PI/2)
        *cos = sin_lookup18(ANG18_PI2 - amp); // PI/2-amp in [0, PI/2)
        *sin = sin_lookup18(amp); // amp in [0, PI/2)

        // 1) 0 < amp, and so -amp < 0
        // 2) amp < PI/2, and so 0 <= (PI/2 - amp) < PI/2
        // Hence, PI/2-amp in [0, PI/2)
    }

    if (ang >= ANG18_PI) { // PI <= ang < 2PI
        *cos = -*cos;
        *sin = -*sin;
    }
}

Fix32 cos18(Ang18 ang) {
    // See the comments in the definition of cossin18() for interval analysis of
    // the angles passed to sin_lookup18().
    Fix32 cos;
    
    ang &= ANG18_2PI_MASK;
    Ang18 ang_mod_pi = ang & ANG18_PI_MASK;

    if (ang_mod_pi >= ANG18_PI2) {
        cos = -sin_lookup18(ang_mod_pi - ANG18_PI2);
    }
    else {
        cos = sin_lookup18(ANG18_PI2 - ang_mod_pi);
    }

    if (ang >= ANG18_PI) {
        cos = -cos;
    }

    return cos;
}

Fix32 sin18(Ang18 ang) {
    // See the comments in the definition of cossin18() for interval analysis of
    // the angles passed to sin_lookup18().
    Fix32 sin;
    
    ang &= ANG18_2PI_MASK;
    Ang18 ang_mod_pi = ang & ANG18_PI_MASK;

    if (ang_mod_pi >= ANG18_PI2) {
        sin =  sin_lookup18(ANG18_PI - ang_mod_pi);
    }
    else {
        sin = sin_lookup18(ang_mod_pi);
    }

    if (ang >= ANG18_PI) {
        sin = -sin;
    }
    
    return sin;
}

Fix32 tan18(Ang18 ang) {
    // TODO: Handle division by zero.
    
    Fix32 cos, sin;
    cossin18(&cos, &sin, ang);
    return (Fix32)FIX_DIV(sin, cos);
}
