//#define EDITING

////////////////////////////////////////////////////////////////////////////////
//  COMMON PREAMBLE
////////////////////////////////////////////////////////////////////////////////

// Nuclear introduced me to idea of a function template file. TODO: Link to
// their github.

#ifdef EDITING
#  ifdef SOURCE_INCLUSION
#    error Undefine the EDITING preprocessing token to include this source file.
#  endif

#  include "src/misc/epm_includes.h"
#  include "src/draw/draw3D.h"
#  include "src/draw/xspans.h"

#  define TEXEL_PREC 5

#  define TEXEL
#  define VBRI
#  define FBRI
#  define HILIT
#  define COUNT
#endif

/* TEXEL and VBRI both imply VERTEX_ATTRIBUTES */
#if (defined TEXEL) || (defined VBRI)
#  define VERTEX_ATTRIBUTES
#endif

#if ( ! defined TEXEL) && ( ! defined VBRI)
#  undef VERTEX_ATTRIBUTES
#endif

#ifndef TEXEL_PREC
#  error The preprocessing token TEXEL_PREC must be defined.
#endif

#define MUL_BRI(COLOR, BRI) (uint32_t)                          \
    (((((COLOR) & 0x00FF00FFu) * (BRI)) & 0xFF00FF00u) |        \
     ((((COLOR) & 0x0000FF00u) * (BRI)) & 0x00FF0000u)) >> 8

////////////////////////////////////////////////////////////////////////////////
//  LOCAL PREAMBLE
////////////////////////////////////////////////////////////////////////////////

#ifdef EDITING
#  define VA_PREC 8
#endif

#ifndef VA_PREC
#  error The preprocessing token VA_PREC must be defined.
#endif

#define triangle_area_2D(a, b, c) ((Fix64)((c).x - (a).x) * (Fix64)((b).y - (a).y) - (Fix64)((c).y - (a).y) * (Fix64)((b).x - (a).x))

////////////////////////////////////////////////////////////////////////////////
//  TEMPLATE
////////////////////////////////////////////////////////////////////////////////

uint32_t DRAWTRI_FUNCTION(Window *win, ScreenTri *tri) {
    Fix64Vec_2D
        scr_v0 = tri->vpxl[0]->XY,
        scr_v1 = tri->vpxl[1]->XY,
        scr_v2 = tri->vpxl[2]->XY;
    
    uint16_t min_y, max_y;
    compute_xspans(win,
                   pixel_from_mpixel(scr_v0),
                   pixel_from_mpixel(scr_v1),
                   pixel_from_mpixel(scr_v2),
                   &min_y, &max_y);

#if defined VERTEX_ATTRIBUTES
    Fix64 V21_x = scr_v2.x - scr_v1.x;
    Fix64 V21_y = scr_v2.y - scr_v1.y;
    Fix64 V02_x = scr_v0.x - scr_v2.x;
    Fix64 V02_y = scr_v0.y - scr_v2.y;
    Fix64 V10_y = scr_v1.y - scr_v0.y;
#endif

#if ! defined TEXEL
    zgl_Color fill_color = tri->fclr;
#endif
#if defined TEXEL
    zgl_mPixel
        tx0 = *tri->vtxl[0],
        tx1 = *tri->vtxl[1],
        tx2 = *tri->vtxl[2];
    
    tx0.x >>= 16-TEXEL_PREC;
    tx0.y >>= 16-TEXEL_PREC;
    tx1.x >>= 16-TEXEL_PREC;
    tx1.y >>= 16-TEXEL_PREC;
    tx2.x >>= 16-TEXEL_PREC;
    tx2.y >>= 16-TEXEL_PREC;
    
    zgl_Color *tx_pixels = tri->ftex->pixels;
    uint8_t tx_w_shift = tri->shft;
    int16_t u_mod = (uint16_t)(tri->ftex->w - 1);
    int16_t v_mod = (uint16_t)(tri->ftex->h - 1);
#endif

#if defined VBRI
    int64_t vbri0 = tri->vbri[0];
    int64_t vbri1 = tri->vbri[1];
    int64_t vbri2 = tri->vbri[2];
#endif
    
#if (defined FBRI)
    uint64_t fbri = tri->fbri;
#endif

    zgl_Color *scr_curr;
    zgl_Color *scr_start = g_scr_pixels;
    uint16_t scr_w = (uint16_t)g_scr_w;


#if defined VERTEX_ATTRIBUTES
    Fix64
        d_W0 = V21_y,
        d_W1 = V02_y,
        d_W2 = V10_y;
    // 27.16 

    Fix64 W0, W1, W2; // Ci / Zi
    Fix64 denom = triangle_area_2D(scr_v0, scr_v1, scr_v2)>>16;
#endif
    
    /* Prepare VA increments & accumulators. */
#if defined VBRI
    Fix64 const d_numer_vbri = LSHIFT64((d_W0*vbri0 + d_W1*vbri1 + d_W2*vbri2), VA_PREC)/denom;
    Fix64 numer_vbri;    // W0*vbri0 + W1*vbri1 + W2*vbri2
#endif
#if defined TEXEL
    Fix64 const d_numer_x = LSHIFT64((d_W0*tx0.x + d_W1*tx1.x + d_W2*tx2.x), VA_PREC)/denom;
    Fix64 const d_numer_y = LSHIFT64((d_W0*tx0.y + d_W1*tx1.y + d_W2*tx2.y), VA_PREC)/denom;
    Fix64 numer_x; // W0*tx0.x + W1*tx1.x + W2*tx2.x
    Fix64 numer_y; // W0*tx0.y + W1*tx1.y + W2*tx2.y
#endif // TEXEL

    uint32_t scr_scanline_offset = min_y*scr_w;
  
#if defined COUNT
    uint32_t num_drawn = 0;
#endif

    for (uint16_t y = (uint16_t)min_y; y <= max_y; y++) {
        uint16_t
            min_x = g_xspans[y<<1],
            max_x = g_xspans[(y<<1) + 1] - 1;

        scr_curr = scr_start + scr_scanline_offset + min_x;
        scr_scanline_offset += scr_w; // must happen before if (min_x > max_x) continue;

        //printf("(min_x %i) (max_x %i)\n", min_x, max_x);
        if (min_x > max_x) continue; // FIXME: Why does this happen (a lot!)?

#if defined VERTEX_ATTRIBUTES
        W0 = (((fixify(min_x) - scr_v1.x)*V21_y) -
              ((fixify(    y) - scr_v1.y)*V21_x))>>16; // 22.16
        
        W1 = (((fixify(min_x) - scr_v2.x)*V02_y) -
              ((fixify(    y) - scr_v2.y)*V02_x))>>16;

        W2 = denom - W0 - W1;
#endif
        
#if defined VBRI
        numer_vbri = LSHIFT64((W0*vbri0 + W1*vbri1 + W2*vbri2), VA_PREC)/denom;
#endif
#if defined TEXEL
        numer_x    = LSHIFT64((W0*tx0.x + W1*tx1.x + W2*tx2.x), VA_PREC)/denom;
        numer_y    = LSHIFT64((W0*tx0.y + W1*tx1.y + W2*tx2.y), VA_PREC)/denom;
#endif

#if (defined VERTEX_ATTRIBUTES)
        if (denom == 0) { // TODO: Investigate why this is necessary, *and* why
                          // completely ignoring such scanlines produces no
                          // noticeable effect. Are we getting degenerate
                          // scanlines?
            continue;
        }
#endif

        
        for (uint16_t x = (uint16_t)min_x; x <= max_x; x++, scr_curr++) {
#if defined COUNT
#if defined VERTEX_ATTRIBUTES
            if (*scr_curr & (1LL<<31)) goto IncrementVAs;
#else
            if (*scr_curr & (1LL<<31)) continue;
#endif
#endif

            zgl_Color color;
            
#if defined HILIT
            // If dot overlay and on such dot, just draw dot and move on.
            if ((((x & 7) == 0) && ((y & 7) == 0)) ||
                (((x & 7) == 4) && ((y & 7) == 4))) {
                color = 0xDDAACC; // IDEA: Cleverly compute a highlight color
                                  // with high contrast to surrounding pixels?
            }
            else {                
#endif           
                /* Complete vertex attribute interpolation. */
#if defined TEXEL
                int16_t u = (int16_t)((numer_x>>TEXEL_PREC))>>VA_PREC;
                int16_t v = (int16_t)((numer_y>>TEXEL_PREC))>>VA_PREC;
#endif
#if defined VBRI
                uint64_t vbri = numer_vbri>>VA_PREC;
#endif

                /* Determine initial color, before processing */
                color = 0;
#if defined TEXEL
                color = tx_pixels[(u & u_mod) + ((v & v_mod)<<tx_w_shift)];
#else
                color = fill_color;
#endif

                /* Modify pixel color */
#if (defined FBRI)
                color = MUL_BRI(color, fbri);
#endif
#if (defined VBRI)
                color = MUL_BRI(color, vbri);
#endif

#if defined HILIT
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
#if defined COUNT && defined VERTEX_ATTRIBUTES
        IncrementVAs:
#endif
            
#if defined VBRI
            numer_vbri += d_numer_vbri;
#endif
#if defined TEXEL
            numer_x += d_numer_x;
            numer_y += d_numer_y;
#endif
        } // while (x <= max_x) {
    } // for (uint16_t y = (uint16_t)min_y; y <= max_y; y++) {
    
    return 1
#if defined COUNT
        *num_drawn
#endif
        ;
       
}
