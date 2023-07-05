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
#  define SUBSPAN_SHIFT 4u
#endif

#ifndef SUBSPAN_SHIFT
#  error The preprocessing token SUBSPAN_SHIFT must be defined.
#endif
#define SUBSPAN_LEN (1<<SUBSPAN_SHIFT)
#define SUBSPAN_MOD (SUBSPAN_LEN-1)

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
    Fix64 V10_x = scr_v1.x - scr_v0.x;
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
        Z0 = tri->vpxl[0]->z,
        Z1 = tri->vpxl[1]->z,
        Z2 = tri->vpxl[2]->z;
    // .16

    Fix64
        d_W0 = FIX_DIV(V21_y, Z0),
        d_W1 = FIX_DIV(V02_y, Z1),
        d_W2 = FIX_DIV(V10_y, Z2);
    // .16
#endif

    /* Prepare increments. */
#if defined VBRI
    Fix64 const d_numer_vbri = LSHIFT64((d_W0*vbri0 + d_W1*vbri1 + d_W2*vbri2), SUBSPAN_SHIFT);
#endif
#if defined TEXEL
    Fix64 const d_numer_x = LSHIFT64((d_W0*tx0.x + d_W1*tx1.x + d_W2*tx2.x), SUBSPAN_SHIFT);
    Fix64 const d_numer_y = LSHIFT64((d_W0*tx0.y + d_W1*tx1.y + d_W2*tx2.y), SUBSPAN_SHIFT);
#endif // TEXEL
#if defined VERTEX_ATTRIBUTES
    Fix64 const d_denom   = LSHIFT64((d_W0       + d_W1       + d_W2), SUBSPAN_SHIFT);
#endif

    /* Prepare accumulators */
#if defined VERTEX_ATTRIBUTES
    Fix64 C0, C1, C2; // barycentric coordinates of current pixel
    Fix64 W0, W1, W2; // Ci / Zi
    Fix64 denom;   // W0 + W1 + W2
#endif
#if defined VBRI
    Fix64 numer_vbri;    // W0*vbri0 + W1*vbri1 + W2*vbri2
#endif
#if defined TEXEL
    Fix64 numer_x; // W0*tx0.x + W1*tx1.x + W2*tx2.x
    Fix64 numer_y; // W0*tx0.y + W1*tx1.y + W2*tx2.y
#endif

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

        uint16_t min_aligned_x = (int32_t)(min_x & ~SUBSPAN_MOD);
        
#if defined VERTEX_ATTRIBUTES
        C0 = (((fixify(min_aligned_x) - scr_v1.x)*V21_y) -
              ((fixify(    y) - scr_v1.y)*V21_x)); // 22.32
        W0 = (C0)/Z0; // .16
        
        C1 = (((fixify(min_aligned_x) - scr_v2.x)*V02_y) -
              ((fixify(    y) - scr_v2.y)*V02_x));
        W1 = (C1)/Z1; // .16

        C2 = (((fixify(min_aligned_x) - scr_v0.x)*V10_y) -
              ((fixify(    y) - scr_v0.y)*V10_x));
        W2 = (C2)/Z2; // .16
#endif

#if defined VBRI
        numer_vbri = W0*vbri0 + W1*vbri1 + W2*vbri2;
#endif
#if defined TEXEL
        numer_x    = W0*tx0.x + W1*tx1.x + W2*tx2.x;
        numer_y    = W0*tx0.y + W1*tx1.y + W2*tx2.y;
#endif
#if defined VERTEX_ATTRIBUTES
        denom      = W0       + W1       + W2;
        if (denom == 0) { // TODO: Investigate why this is necessary, *and* why
                          // completely ignoring such scanlines produces no
                          // noticeable effect. Are we getting degenerate
                          // scanlines?
            continue;
        }
#endif

        // VA's at subspan endpoints
#if defined TEXEL
        int64_t uA, uB, vA, vB;
        uB = (numer_x>>TEXEL_PREC)/denom;
        vB = (numer_y>>TEXEL_PREC)/denom;
#endif

#if defined VBRI
        int64_t vbriA, vbriB;
        vbriB = numer_vbri/denom;
#endif

        int32_t i_head_beg = min_x - min_aligned_x;
        int32_t i_head_end =
            max_x - min_aligned_x + 1 < SUBSPAN_LEN ?
            max_x - min_aligned_x + 1 :
            SUBSPAN_LEN;

        uint16_t x = (uint16_t)min_x;
        if (min_x != min_aligned_x) { // head
#if defined VERTEX_ATTRIBUTES
            if (denom + d_denom == 0) continue;
#endif
#if defined TEXEL
            uA = uB;
            vA = vB;
            uB = ((numer_x + d_numer_x)>>TEXEL_PREC)/(denom + d_denom);
            vB = ((numer_y + d_numer_y)>>TEXEL_PREC)/(denom + d_denom);
#endif
#if defined VBRI
            vbriA = vbriB;
            vbriB = (numer_vbri + d_numer_vbri)/(denom + d_denom);
#endif
        
            for (int32_t i = i_head_beg; i < i_head_end; i++, scr_curr++, x++) {
                /////////////////////////////////////////////////////////
                // INNERMOST START
                /////////////////////////////////////////////////////////
#if defined COUNT
                if (*scr_curr & (1LL<<31)) continue;
#endif

                zgl_Color color;

#if defined TEXEL
                int32_t u = (int32_t)((uA + ((i*(uB-uA))>>SUBSPAN_SHIFT)));
                int32_t v = (int32_t)((vA + ((i*(vB-vA))>>SUBSPAN_SHIFT)));
#endif
#if defined VBRI
                int64_t vbri = (int64_t)((vbriA + ((i*(vbriB-vbriA))>>SUBSPAN_SHIFT)));
#endif
               
                /* Determine initial color, before processing */
                //drawtri_assert((u & u_mod) >= 0 && (v & v_mod) >= 0);
                color = 0;

#if defined TEXEL
                color = tx_pixels[(u & u_mod) + ((v & v_mod)<<tx_w_shift)];
#endif

#if ! defined TEXEL
                color = fill_color;
#endif
                    
                // Modify pixel color.
#if (defined FBRI)
                color = MUL_BRI(color, fbri);
#endif
#if (defined VBRI)
                color = MUL_BRI(color, vbri);
#endif

#if defined HILIT
                // If dot overlay and on such dot, just draw dot and move on.
                if ((((x & 7) == 0) && ((y & 7) == 0)) ||
                    (((x & 7) == 4) && ((y & 7) == 4))) {
                    color = 0xDDAACC;
                }
#endif

#if defined SHOWN_SUBSPAN_ENDPOINTS
                if (i == 0) color = 0xFFFF00;
#endif
                /* Write the pixel */
                *scr_curr = color
#if defined COUNT
                    | (1LL<<31);
                num_drawn++
#endif
                    ;
                /////////////////////////////////////////////////////////
                // INNERMOST END
                /////////////////////////////////////////////////////////
            }

            /* Apply VA increments */
#if defined VBRI
            numer_vbri += d_numer_vbri;
#endif
#if defined TEXEL
            numer_x    += d_numer_x;
            numer_y    += d_numer_y;
#endif
#if defined VERTEX_ATTRIBUTES
            denom      += d_denom;
#endif
            
            min_x = min_aligned_x + SUBSPAN_LEN;
        }

        if (min_x > max_x) continue;
        
        int32_t length = max_x - min_x + 1;
        uint8_t num_full_spans = (uint8_t)(length>>SUBSPAN_SHIFT);
        uint8_t tail_len = (uint8_t)(length & SUBSPAN_MOD);        

        for (uint8_t i_span = 0; i_span < num_full_spans; i_span++) {
#if defined VERTEX_ATTRIBUTES
            if (denom + d_denom == 0) continue;
#endif
#if defined TEXEL
            uA = uB;
            vA = vB;
            uB = ((numer_x + d_numer_x)>>TEXEL_PREC)/(denom + d_denom);
            vB = ((numer_y + d_numer_y)>>TEXEL_PREC)/(denom + d_denom);
#endif
#if defined VBRI
            vbriA = vbriB;
            vbriB = (numer_vbri + d_numer_vbri)/(denom + d_denom);
#endif
            
            for (int32_t i = 0; i < (int16_t)SUBSPAN_LEN; i++, scr_curr++, x++) {
                /////////////////////////////////////////////////////////
                // INNERMOST START
                /////////////////////////////////////////////////////////
#if defined COUNT
                if (*scr_curr & (1LL<<31)) continue;
#endif

                zgl_Color color;

#if defined TEXEL
                int32_t u = (int32_t)((uA + ((i*(uB-uA))>>SUBSPAN_SHIFT)));
                int32_t v = (int32_t)((vA + ((i*(vB-vA))>>SUBSPAN_SHIFT)));
#endif
#if defined VBRI
                int64_t vbri = (int64_t)((vbriA + ((i*(vbriB-vbriA))>>SUBSPAN_SHIFT)));
#endif
               
                /* Determine initial color, before processing */
                //drawtri_assert((u & u_mod) >= 0 && (v & v_mod) >= 0);
                color = 0;

#if defined TEXEL
                color = tx_pixels[(u & u_mod) + ((v & v_mod)<<tx_w_shift)];
#endif

#if ! defined TEXEL
                color = fill_color;
#endif
                    
                // Modify pixel color.
#if (defined FBRI)
                color = MUL_BRI(color, fbri);
#endif
#if (defined VBRI)
                color = MUL_BRI(color, vbri);
#endif

#if defined HILIT
                // If dot overlay and on such dot, just draw dot and move on.
                if ((((x & 7) == 0) && ((y & 7) == 0)) ||
                    (((x & 7) == 4) && ((y & 7) == 4))) {
                    color = 0xDDAACC;
                }
#endif

#if defined SHOWN_SUBSPAN_ENDPOINTS
                if (i == 0) color = 0xFFFF00;
#endif
                /* Write the pixel */
                *scr_curr = color
#if defined COUNT
                    | (1LL<<31);
                num_drawn++
#endif
                    ;
                /////////////////////////////////////////////////////////
                // INNERMOST END
                /////////////////////////////////////////////////////////
            }
            
            /* Apply VA increments */
#if defined VBRI
            numer_vbri += d_numer_vbri;
#endif
#if defined TEXEL
            numer_x    += d_numer_x;
            numer_y    += d_numer_y;
#endif
#if defined VERTEX_ATTRIBUTES
            denom      += d_denom;
#endif

        }
        {
#if defined VERTEX_ATTRIBUTES
            if (denom + d_denom == 0) continue;
#endif
#if defined TEXEL
            uA = uB;
            vA = vB;
            uB = ((numer_x + d_numer_x)>>TEXEL_PREC)/(denom + d_denom);
            vB = ((numer_y + d_numer_y)>>TEXEL_PREC)/(denom + d_denom);
#endif
#if defined VBRI
            vbriA = vbriB;
            vbriB = (numer_vbri + d_numer_vbri)/(denom + d_denom);
#endif
        
            for (int32_t i = 0; i < tail_len; i++, scr_curr++, x++) {
                /////////////////////////////////////////////////////////
                // INNERMOST START
                /////////////////////////////////////////////////////////
#if defined COUNT
                if (*scr_curr & (1LL<<31)) continue;
#endif

                zgl_Color color;

#if defined TEXEL
                int32_t u = (int32_t)((uA + ((i*(uB-uA))>>SUBSPAN_SHIFT)));
                int32_t v = (int32_t)((vA + ((i*(vB-vA))>>SUBSPAN_SHIFT)));
#endif
#if defined VBRI
                int64_t vbri = (int64_t)((vbriA + ((i*(vbriB-vbriA))>>SUBSPAN_SHIFT)));
#endif
               
                /* Determine initial color, before processing */
                //drawtri_assert((u & u_mod) >= 0 && (v & v_mod) >= 0);
                color = 0;

#if defined TEXEL
                color = tx_pixels[(u & u_mod) + ((v & v_mod)<<tx_w_shift)];
#endif

#if ! defined TEXEL
                color = fill_color;
#endif
                    
                // Modify pixel color.
#if (defined FBRI)
                color = MUL_BRI(color, fbri);
#endif
#if (defined VBRI)
                color = MUL_BRI(color, vbri);
#endif

#if defined HILIT
                // If dot overlay and on such dot, just draw dot and move on.
                if ((((x & 7) == 0) && ((y & 7) == 0)) ||
                    (((x & 7) == 4) && ((y & 7) == 4))) {
                    color = 0xDDAACC;
                }
#endif

#if defined SHOWN_SUBSPAN_ENDPOINTS
                if (i == 0) color = 0xFFFF00;
#endif
                /* Write the pixel */
                *scr_curr = color
#if defined COUNT
                    | (1LL<<31);
                num_drawn++
#endif
                    ;
                /////////////////////////////////////////////////////////
                // INNERMOST END
                /////////////////////////////////////////////////////////
            }
        }
    } // for (uint16_t y = (uint16_t)min_y; y <= max_y; y++) {
    
    return 1
#if defined COUNT
        *num_drawn
#endif
        ;
       
}

