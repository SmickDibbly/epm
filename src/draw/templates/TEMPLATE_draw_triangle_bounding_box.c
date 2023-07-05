#if 0

#include "src/misc/epm_includes.h"
#include "src/draw/draw3D.h"

#define triangle_area_2D(a, b, c) ((Fix64)((c).x - (a).x) * (Fix64)((b).y - (a).y) - (Fix64)((c).y - (a).y) * (Fix64)((b).x - (a).x))

#define TEXEL_PREC 5


/* TEXEL and LIGHT_GOURAUD both imply BARYCENTRIC */
#if (defined TEXEL) || (defined LIGHT_GOURAUD)
#  define BARYCENTRIC
#endif

#if (defined ORTHO) && ( ! defined TEXEL) && ( ! defined LIGHT_GOURAUD)
#  undef BARYCENTRIC
#endif

uint32_t drawtri_bounding_box(Window *win, ScreenTriangle *tri) {
    Fix64Vec_2D
        scr_v0 = tri->pixels[0]->XY,
        scr_v1 = tri->pixels[1]->XY,
        scr_v2 = tri->pixels[2]->XY;
    
    int32_t min_x, min_y, max_x, max_y;
    compute_xspans(win,
                   pixel_from_mpixel(scr_v0),
                   pixel_from_mpixel(scr_v1),
                   pixel_from_mpixel(scr_v2),
                   &min_y, &max_y);
    min_y = MAX(min_y, win->rect.y);
    max_y = MIN(max_y, win->rect.y + win->rect.h - 1);

#if defined BARYCENTRIC
    Fix64 V21_x = scr_v2.x - scr_v1.x;
    Fix64 V21_y = scr_v2.y - scr_v1.y;
    Fix64 V02_x = scr_v0.x - scr_v2.x;
    Fix64 V02_y = scr_v0.y - scr_v2.y;
#  if ! defined ORTHO
    Fix64 V10_x = scr_v1.x - scr_v0.x;
#  endif
    Fix64 V10_y = scr_v1.y - scr_v0.y;
#endif

    
#if ! defined TEXEL
    zgl_Color fill_color = tri->fill_color;
#endif
#if defined TEXEL
    zgl_mPixel
        tx0 = *tri->texels[0],
        tx1 = *tri->texels[1],
        tx2 = *tri->texels[2];
    
    tx0.x >>= 16-TEXEL_PREC;
    tx0.y >>= 16-TEXEL_PREC;
    tx1.x >>= 16-TEXEL_PREC;
    tx1.y >>= 16-TEXEL_PREC;
    tx2.x >>= 16-TEXEL_PREC;
    tx2.y >>= 16-TEXEL_PREC;
    
    zgl_Color *tx_pixels = tri->texture->pixels;
    uint8_t tx_w_shift = 0;
    if (tri->texture->w == 512)
        tx_w_shift = 9;
    else if (tri->texture->w == 256)
        tx_w_shift = 8;
    else if (tri->texture->w == 128)
        tx_w_shift = 7;
    else
        dibassert(false);
    
    uint16_t u_mod = (uint16_t)(tri->texture->w - 1);
    uint16_t v_mod = (uint16_t)(tri->texture->h - 1);
#endif
#if defined LIGHT_FLAT
    uint64_t brightness = tri->brightness;
#endif
#if defined LIGHT_GOURAUD
    uint64_t vbri0 = tri->vbri[0];
    uint64_t vbri1 = tri->vbri[1];
    uint64_t vbri2 = tri->vbri[2];
#endif


    zgl_Color *scr_curr;
    zgl_Color *scr_start = g_scr_pixels;
    uint16_t scr_w = (uint16_t)g_scr_w;

    
#if defined BARYCENTRIC
#  if defined ORTHO
   Fix64
        d_W0 = V21_y,
        d_W1 = V02_y,
        d_W2 = V10_y;
    // 27.16 
#  endif
#  if ! defined ORTHO
    Fix64
        I0 = *tri->zinv[0],
        I1 = *tri->zinv[1],
        I2 = *tri->zinv[2];
    //1.32

    Fix64
        d_W0 = FIX_MUL(I0, V21_y),
        d_W1 = FIX_MUL(I1, V02_y),
        d_W2 = FIX_MUL(I2, V10_y);
    // 27.16
#  endif
#endif



    
    /* Prepare increments. */
#if defined LIGHT_GOURAUD
    Fix64 d_bri     = d_W0*vbri0 + d_W1*vbri1 + d_W2*vbri2;
#endif
#if defined TEXEL
    Fix64 d_numer_x = d_W0*tx0.x + d_W1*tx1.x + d_W2*tx2.x;
    Fix64 d_numer_y = d_W0*tx0.y + d_W1*tx1.y + d_W2*tx2.y;
#endif // TEXEL
#if defined BARYCENTRIC
#  if ! defined ORTHO
    Fix64 d_denom   = d_W0       + d_W1       + d_W2;
#  endif
#endif



    
    /* Prepare accumulators */
#if defined BARYCENTRIC
#  if ! defined ORTHO
    Fix64 C0, C1, C2; // barycentric coordinates of current pixel
#  endif
    Fix64 W0, W1, W2; // Ci / Zi
    UFix64 denom;   // W0 + W1 + W2
#  if defined ORTHO
    denom = triangle_area_2D(scr_v0, scr_v1, scr_v2)>>16;
#  endif
#endif
#if defined LIGHT_GOURAUD
    UFix64 vbri;    // W0*vbri0 + W1*vbri1 + W2*vbri2
#endif
#if defined TEXEL
    UFix64 numer_x; // W0*tx0.x + W1*tx1.x + W2*tx2.x
    UFix64 numer_y; // W0*tx0.y + W1*tx1.y + W2*tx2.y
#endif
    
    ptrdiff_t scr_inc = min_y*scr_w;

#if defined COUNT
    uint32_t num_drawn = 0;
#endif
    for (uint16_t y = (uint16_t)min_y; y <= max_y; y++) {
        min_x = g_xspans[y<<1], max_x = g_xspans[(y<<1) + 1] - 1;

        // TODO: The multiplication for W0, W1, W2 overflows when camera too
        // close to triangle. 2023-05-05
#if defined BARYCENTRIC
#  if defined ORTHO
        W0 = (((fixify(min_x) - scr_v1.x)*V21_y) -
              ((fixify(    y) - scr_v1.y)*V21_x))>>16; // 22.16
        
        W1 = (((fixify(min_x) - scr_v2.x)*V02_y) -
              ((fixify(    y) - scr_v2.y)*V02_x))>>16;

        W2 = denom - W0 - W1;
#  endif
#  if ! defined ORTHO
        C0 = (((fixify(min_x) - scr_v1.x)*V21_y) -
              ((fixify(    y) - scr_v1.y)*V21_x))>>16; // 22.32
        W0 = FIX_MUL(I0, C0); // 38.16
        
        C1 = (((fixify(min_x) - scr_v2.x)*V02_y) -
              ((fixify(    y) - scr_v2.y)*V02_x))>>16;
        W1 = FIX_MUL(I1, C1);

        C2 = (((fixify(min_x) - scr_v0.x)*V10_y) -
              ((fixify(    y) - scr_v0.y)*V10_x))>>16;
        W2 = FIX_MUL(I2, C2);
#  endif
#endif
#if defined LIGHT_GOURAUD
        vbri    = W0*vbri0 + W1*vbri1 + W2*vbri2;
#endif
#if defined TEXEL
        numer_x = W0*tx0.x + W1*tx1.x + W2*tx2.x;
        numer_y = W0*tx0.y + W1*tx1.y + W2*tx2.y;
#endif
#if defined BARYCENTRIC
#  if ! defined ORTHO
        denom   = W0       + W1       + W2;
#  endif
#endif
        
        scr_curr = scr_start + min_x + scr_inc;
        scr_inc += scr_w;

        uint16_t x = (uint16_t)min_x;
        while (x <= max_x) {
#if defined COUNT
            if (*scr_curr & (1LL<<31)) goto Continue;
#endif

            zgl_Color color;
            
#if defined LAYER
            // If dot overlay and on such dot, just draw dot and move on.
            if ((((x & 7) == 0) && ((y & 7) == 0)) ||
                (((x & 7) == 4) && ((y & 7) == 4))) {
                color = 0xDDAACC;
            }
            else {                
#endif           
                /* Complete vertex attribute interpolation. */
#if defined TEXEL
                int16_t u = (int16_t)((numer_x>>TEXEL_PREC)/denom);
                int16_t v = (int16_t)((numer_y>>TEXEL_PREC)/denom);
#endif
#if defined LIGHT_GOURAUD
                uint64_t brightness = vbri/denom;
#endif

                /* Determine initial color, before processing */
                color = 0;
#if defined TEXEL
                color = tx_pixels[(u & u_mod) + ((v & v_mod)<<tx_w_shift)];
#endif
#if ! defined TEXEL
                color = fill_color;
#endif

                /* Modify pixel color */
#if (defined LIGHT_FLAT) || (defined LIGHT_GOURAUD)
                color = (uint32_t)
                    ((((color & 0x00FF00FFu) * brightness) & 0xFF00FF00u) |
                     (((color & 0x0000FF00u) * brightness) & 0x00FF0000u)) >> 8;
#endif

#if defined LAYER
            } // else {
#endif
            
            /* Write the pixel */
            *scr_curr = color
#if defined COUNT
                | (1LL<<31);
            num_drawn++
#endif
                ;
            
            /* Apply increments */
#if defined COUNT
        Continue:
#endif
            
            x++;
            scr_curr++;
            
#if defined LIGHT_GOURAUD
            vbri += d_bri;
#endif
#if defined TEXEL
            numer_x += d_numer_x;
            numer_y += d_numer_y;
#endif
#if defined BARYCENTRIC
#  if ! defined ORTHO
            denom += d_denom;
#  endif
#endif
        } // while (x <= max_x) {
    } // for (uint16_t y = (uint16_t)min_y; y <= max_y; y++) {
    
    return 1
#if defined COUNT
        *num_drawn
#endif
        ;
}


#endif
