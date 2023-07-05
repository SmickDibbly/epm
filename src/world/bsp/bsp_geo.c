#include "src/world/bsp/bsp_internal.h"

uint8_t BSP_sideof(WorldVec V, WorldVec planeV, WorldVec planeN) {
    WorldVec point_to_plane;
    x_of(point_to_plane) = x_of(planeV) - x_of(V);
    y_of(point_to_plane) = y_of(planeV) - y_of(V);
    z_of(point_to_plane) = z_of(planeV) - z_of(V);
    
    Fix64 S =
        (Fix64)x_of(point_to_plane)*(Fix64)x_of(planeN) +
        (Fix64)y_of(point_to_plane)*(Fix64)y_of(planeN) +
        (Fix64)z_of(point_to_plane)*(Fix64)z_of(planeN);
    // .32
    
    //    vbs_printf("%lX\n", S);
    
    if (ABS(S) < 1LL<<32) // might want less precision to avoid miniscule triangles???
        //    if (S == 0)
        return SIDE_MID;
    else if (S < 0)
        return SIDE_FRONT;
    else
        return SIDE_BACK;
}

#define FIX32_dbl(x) ((double)(x)/(1LL<<32))

static bool find_intersection2
(WorldVec lineV0, WorldVec lineV1,
 WorldVec planeV, WorldVec planeN,
 WorldVec *out_sect) {
    // if both endpoints are the same, we should not be here in the first place
    dibassert( ! (x_of(lineV0) == x_of(lineV1) &&
                  y_of(lineV0) == y_of(lineV1) &&
                  z_of(lineV0) == z_of(lineV1)));

    /*
    vbs_printf("planeV: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(planeV)),
               FIX_dbl(y_of(planeV)),
               FIX_dbl(z_of(planeV)));
    vbs_printf("planeN: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(planeN)),
               FIX_dbl(y_of(planeN)),
               FIX_dbl(z_of(planeN)));

    vbs_printf("lineV0: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(lineV0)),
               FIX_dbl(y_of(lineV0)),
               FIX_dbl(z_of(lineV0)));
    vbs_printf("lineV1: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(lineV1)),
               FIX_dbl(y_of(lineV1)),
               FIX_dbl(z_of(lineV1)));
    */
    
    WorldVec lineDir;
    x_of(lineDir) = x_of(lineV1) - x_of(lineV0);
    y_of(lineDir) = y_of(lineV1) - y_of(lineV0);
    z_of(lineDir) = z_of(lineV1) - z_of(lineV0);

    /*
    vbs_printf("lineDir: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(lineDir)),
               FIX_dbl(y_of(lineDir)),
               FIX_dbl(z_of(lineDir)));
    */
    
    WorldVec line_to_plane;
    x_of(line_to_plane) = x_of(planeV) - x_of(lineV0);
    y_of(line_to_plane) = y_of(planeV) - y_of(lineV0);
    z_of(line_to_plane) = z_of(planeV) - z_of(lineV0);

    /*
    vbs_printf("l_to_p: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(line_to_plane)),
               FIX_dbl(y_of(line_to_plane)),
               FIX_dbl(z_of(line_to_plane)));
    */
    
    // numer is essentially the "sideof".

    // This is a temporary violation my own self-imposed constraint of not using
    // floating point types. But I simply haven't been able to make this
    // function work in fixed point... YET. Precision is *extremely* important
    // here.
    double numer = FIX32_dbl((Fix64)x_of(line_to_plane)*(Fix64)x_of(planeN) +
                             (Fix64)y_of(line_to_plane)*(Fix64)y_of(planeN) +
                             (Fix64)z_of(line_to_plane)*(Fix64)z_of(planeN));
    double denom = FIX32_dbl((Fix64)x_of(lineDir)*(Fix64)x_of(planeN) +
                             (Fix64)y_of(lineDir)*(Fix64)y_of(planeN) +
                             (Fix64)z_of(lineDir)*(Fix64)z_of(planeN));
    
    //vbs_printf("(numer, denom): (%lf, %lf)\n", numer, denom);
    dibassert(denom != 0.0);
    
    double T = numer/denom;
    //vbs_printf("     T: %lf\n", T);

    dibassert(T > -0.0009);
    dibassert(T < 1.0009);

    if (ABS(T-1.0) < 0.001)
        T = 1.0;
    else if (ABS(T) < 0.001)
        T = 0.0;
    
    x_of(*out_sect) = x_of(lineV0) + (Fix32)FIX_MUL(dbl_FIX(T), x_of(lineDir));
    y_of(*out_sect) = y_of(lineV0) + (Fix32)FIX_MUL(dbl_FIX(T), y_of(lineDir));
    z_of(*out_sect) = z_of(lineV0) + (Fix32)FIX_MUL(dbl_FIX(T), z_of(lineDir));
    
    //vbs_putchar('\n');
        
    return true;
}

bool find_intersection
(WorldVec lineV0, WorldVec lineV1,
 WorldVec planeV, WorldVec planeN,
 WorldVec *out_sect) {
    // if both endpoints are the same, we should not be here in the first place
    dibassert( ! (x_of(lineV0) == x_of(lineV1) &&
                  y_of(lineV0) == y_of(lineV1) &&
                  z_of(lineV0) == z_of(lineV1)));

    /*
    vbs_printf("planeV: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(planeV)),
               FIX_dbl(y_of(planeV)),
               FIX_dbl(z_of(planeV)));
    vbs_printf("planeN: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(planeN)),
               FIX_dbl(y_of(planeN)),
               FIX_dbl(z_of(planeN)));

    vbs_printf("lineV0: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(lineV0)),
               FIX_dbl(y_of(lineV0)),
               FIX_dbl(z_of(lineV0)));
    vbs_printf("lineV1: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(lineV1)),
               FIX_dbl(y_of(lineV1)),
               FIX_dbl(z_of(lineV1)));
    */
    
    WorldVec line_to_plane;
    // This should be the same as the "point_to_plane" during BSP_sideof during
    // spatial relation testing. This is true as of 2023-05-22
    x_of(line_to_plane) = x_of(planeV) - x_of(lineV0);
    y_of(line_to_plane) = y_of(planeV) - y_of(lineV0);
    z_of(line_to_plane) = z_of(planeV) - z_of(lineV0);

    /*
    vbs_printf("l_to_p: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(line_to_plane)),
               FIX_dbl(y_of(line_to_plane)),
               FIX_dbl(z_of(line_to_plane)));
    */
    
    // S, the numerator, is essentially the "sideof" of lineV0.
    Fix64 S =
        (Fix64)x_of(line_to_plane)*(Fix64)x_of(planeN) +
        (Fix64)y_of(line_to_plane)*(Fix64)y_of(planeN) +
        (Fix64)z_of(line_to_plane)*(Fix64)z_of(planeN);


    
    // for consistency, always choose the point in front as lineV0
    if (S>0) { // sideof = BACK
        WorldVec tmp = lineV0;
        lineV0 = lineV1;
        lineV1 = tmp;

        x_of(line_to_plane) = x_of(planeV) - x_of(lineV0);
        y_of(line_to_plane) = y_of(planeV) - y_of(lineV0);
        z_of(line_to_plane) = z_of(planeV) - z_of(lineV0);
        /*
        vbs_printf("l_to_p: (%lf, %lf, %lf)\n",
                   FIX_dbl(x_of(line_to_plane)),
                   FIX_dbl(y_of(line_to_plane)),
                   FIX_dbl(z_of(line_to_plane)));
        */
        // S, the numerator, is essentially the "sideof" of lineV0.
        S =
            (Fix64)x_of(line_to_plane)*(Fix64)x_of(planeN) +
            (Fix64)y_of(line_to_plane)*(Fix64)y_of(planeN) +
            (Fix64)z_of(line_to_plane)*(Fix64)z_of(planeN);
    }

    
    WorldVec lineDir;
    x_of(lineDir) = x_of(lineV1) - x_of(lineV0);
    y_of(lineDir) = y_of(lineV1) - y_of(lineV0);
    z_of(lineDir) = z_of(lineV1) - z_of(lineV0);
    /*
    vbs_printf("lineDir: (%lf, %lf, %lf)\n",
               FIX_dbl(x_of(lineDir)),
               FIX_dbl(y_of(lineDir)),
               FIX_dbl(z_of(lineDir)));
    */
    Fix64 denom =
        (Fix64)x_of(lineDir)*(Fix64)x_of(planeN) +
        (Fix64)y_of(lineDir)*(Fix64)y_of(planeN) +
        (Fix64)z_of(lineDir)*(Fix64)z_of(planeN);
    // .32
    
    //vbs_printf("(numer, denom): (%lf, %lf)\n", FIX_dbl(S), FIX_dbl(denom));
    dibassert(denom != 0);
    
    Fix64 T = S/(denom>>16); //.16
    //vbs_printf("     T: %lf\n", FIX_dbl(T));

    /*
    dibassert(T > -0.0009);
    dibassert(T < 1.0009);

    if (ABS(T-(1<<16)) < 0.001)
        T = 1<<16;
    else if (ABS(T) < 0.001)
        T = 0;
    */
    
    x_of(*out_sect) = x_of(lineV0) + (Fix32)FIX_MUL(T, x_of(lineDir));
    y_of(*out_sect) = y_of(lineV0) + (Fix32)FIX_MUL(T, y_of(lineDir));
    z_of(*out_sect) = z_of(lineV0) + (Fix32)FIX_MUL(T, z_of(lineDir));
    
    vbs_putchar('\n');
        
    return true;
}
