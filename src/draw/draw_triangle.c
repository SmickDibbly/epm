#include "src/draw/draw_triangle.h"
#include "src/draw/xspans.h"
#include "src/draw/draw3D.h"
#include "src/draw/drawtri_functions.h"

uint8_t g_rasterflags = RASTERFLAG_TEXEL | RASTERFLAG_VBRI;

#define SUPPRESS_DRAWTRI_ASSERT
#ifdef SUPPRESS_DRAWTRI_ASSERT
#  define drawtri_assert(ignored) ((void)0)
#else /* NOT SUPPRESS_ASSERT ie. actually do assertions */
#  include <stdlib.h> // for abort()
#  include <stdio.h>
#  define drawtri_assert(expr)                                             \
    do { if (!(expr)) { printf("ASSERTION ERROR: \"" #expr "\" is FALSE.\nFILE: %s : %d\n\n", __FILE__, __LINE__); abort(); } } while (0)
# endif

static uint32_t g_num_pixels = 0;
static uint32_t g_num_drawn = 0;
bool g_all_pixels_drawn = false;

void reset_rasterizer(Window *win) {
    g_num_pixels = win->rect.w*win->rect.h;
    g_num_drawn = 0;
    g_all_pixels_drawn = false;
}

extern uint32_t drawtri_skymask(Window *win, ScreenTri *tri);

//////////////////////////////////////////////////////////////////////////
bool g_skymask = false;
void draw_Triangle(Window *win, ScreenTri *tri) {
    uint8_t triflags = 0;

    /* NOTE: As of 2023-07-04 triflags and rasterflags are the same, but I want
       to be prepared for that not to be the case in the future. */
    triflags = g_rasterflags & (RASTERFLAG_TEXEL |
                                RASTERFLAG_VBRI  |
                                RASTERFLAG_FBRI  |
                                RASTERFLAG_HILIT |
                                RASTERFLAG_COUNT |
                                RASTERFLAG_SKY   |
                                RASTERFLAG_ORTHO);
    
    /* Draw */
    if (triflags & TRIFLAG_COUNT) {
        if (g_all_pixels_drawn) return;

        if (g_skymask) {
            g_num_drawn += drawtri_skymask(win, tri);
        }
        else {
            g_num_drawn += drawtri_table[triflags](win, tri);    
        }
        
        if (g_num_drawn >= g_num_pixels)
            g_all_pixels_drawn = true;
    }
    else {
        if (g_skymask) {
            drawtri_skymask(win, tri);
        }
        else {
            drawtri_table[triflags](win, tri);    
        }
    }
}



uint32_t drawtri_skymask(Window *win, ScreenTri *tri) {
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

    zgl_Color *scr_curr;
    zgl_Color *scr_start = g_scr_pixels;
    uint16_t scr_w = (uint16_t)g_scr_w;
    uint32_t scr_scanline_offset = (uint32_t)min_y*(uint32_t)scr_w;
  
    uint32_t num_drawn = 0;

    for (uint16_t y = (uint16_t)min_y; y <= max_y; y++) {
        uint16_t
            min_x = (uint16_t)g_xspans[y<<1],
            max_x = (uint16_t)(g_xspans[(y<<1) + 1] - 1);

        scr_curr = scr_start + scr_scanline_offset + min_x;
        scr_scanline_offset += scr_w;

        for (uint16_t x = (uint16_t)min_x; x <= max_x; x++, scr_curr++) {
            if (*scr_curr & (1LL<<31)) continue;
            *scr_curr = 0xF334C7 | 1<<30 | (1LL<<31);;
            num_drawn++;
        }
    }
    
    return num_drawn;
}




#define TEXEL_PREC 5
#define SUBSPAN_SHIFT 4u
#define SUBSPAN_LEN (1<<SUBSPAN_SHIFT)
#define SUBSPAN_MOD (SUBSPAN_LEN-1)

uint32_t drawtri_skydraw(Window *win, ScreenTri *tri) {
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

    Fix64 V21_x = scr_v2.x - scr_v1.x;
    Fix64 V21_y = scr_v2.y - scr_v1.y;
    Fix64 V02_x = scr_v0.x - scr_v2.x;
    Fix64 V02_y = scr_v0.y - scr_v2.y;
    Fix64 V10_x = scr_v1.x - scr_v0.x;
    Fix64 V10_y = scr_v1.y - scr_v0.y;

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
    
    zgl_Color *scr_curr;
    zgl_Color *scr_start = g_scr_pixels;
    uint16_t scr_w = (uint16_t)g_scr_w;


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

    /* Prepare increments. */
    Fix64 const d_numer_x = LSHIFT64((d_W0*tx0.x + d_W1*tx1.x + d_W2*tx2.x), SUBSPAN_SHIFT);
    Fix64 const d_numer_y = LSHIFT64((d_W0*tx0.y + d_W1*tx1.y + d_W2*tx2.y), SUBSPAN_SHIFT);
    Fix64 const d_denom   = LSHIFT64((d_W0       + d_W1       + d_W2), SUBSPAN_SHIFT);

    /* Prepare accumulators */
    Fix64 C0, C1, C2; // barycentric coordinates of current pixel
    Fix64 W0, W1, W2; // Ci / Zi
    Fix64 denom;   // W0 + W1 + W2
    Fix64 numer_x; // W0*tx0.x + W1*tx1.x + W2*tx2.x
    Fix64 numer_y; // W0*tx0.y + W1*tx1.y + W2*tx2.y

    uint32_t scr_scanline_offset = min_y*scr_w;
  
    for (uint16_t y = (uint16_t)min_y; y <= max_y; y++) {
        uint16_t
            min_x = g_xspans[y<<1],
            max_x = g_xspans[(y<<1) + 1] - 1;

        scr_curr = scr_start + scr_scanline_offset + min_x;
        scr_scanline_offset += scr_w; // must happen before if (min_x > max_x) continue;

        //printf("(min_x %i) (max_x %i)\n", min_x, max_x);
        if (min_x > max_x) continue; // FIXME: Why does this happen (a lot!)?

        uint16_t min_aligned_x = (min_x & ~SUBSPAN_MOD);
        
        C0 = (((fixify(min_aligned_x) - scr_v1.x)*V21_y) -
              ((fixify(    y) - scr_v1.y)*V21_x)); // 22.32
        W0 = (C0)/Z0; // .16
        
        C1 = (((fixify(min_aligned_x) - scr_v2.x)*V02_y) -
              ((fixify(    y) - scr_v2.y)*V02_x));
        W1 = (C1)/Z1; // .16

        C2 = (((fixify(min_aligned_x) - scr_v0.x)*V10_y) -
              ((fixify(    y) - scr_v0.y)*V10_x));
        W2 = (C2)/Z2; // .16

        numer_x    = W0*tx0.x + W1*tx1.x + W2*tx2.x;
        numer_y    = W0*tx0.y + W1*tx1.y + W2*tx2.y;
        denom      = W0       + W1       + W2;
        if (denom == 0) { // TODO: Investigate why this is necessary, *and* why
                          // completely ignoring such scanlines produces no
                          // noticeable effect. Are we getting degenerate
                          // scanlines?
            continue;
        }

        // VA's at subspan endpoints
        int64_t uA, uB, vA, vB;
        uB = (numer_x>>TEXEL_PREC)/denom;
        vB = (numer_y>>TEXEL_PREC)/denom;

        int32_t i_head_beg = min_x - min_aligned_x;
        int32_t i_head_end =
            max_x - min_aligned_x + 1 < SUBSPAN_LEN ?
            max_x - min_aligned_x + 1 :
            SUBSPAN_LEN;

        uint16_t x = (uint16_t)min_x;
        if (min_x != min_aligned_x) { // head
            if (denom + d_denom == 0) continue;
            uA = uB;
            vA = vB;
            uB = ((numer_x + d_numer_x)>>TEXEL_PREC)/(denom + d_denom);
            vB = ((numer_y + d_numer_y)>>TEXEL_PREC)/(denom + d_denom);
        
            for (int32_t i = i_head_beg; i < i_head_end; i++, scr_curr++, x++) {
                /////////////////////////////////////////////////////////
                // INNERMOST START
                /////////////////////////////////////////////////////////
                if ( ! (*scr_curr & (1LL<<30))) continue;

                zgl_Color color;
                int32_t u = (int32_t)((uA + ((i*(uB-uA))>>SUBSPAN_SHIFT)));
                int32_t v = (int32_t)((vA + ((i*(vB-vA))>>SUBSPAN_SHIFT)));
               
                /* Determine initial color, before processing */
                //drawtri_assert((u & u_mod) >= 0 && (v & v_mod) >= 0);
                color = 0;

                color = tx_pixels[(u & u_mod) + ((v & v_mod)<<tx_w_shift)];

                    
                // Modify pixel color.

                /* Write the pixel */
                *scr_curr = color;
                /////////////////////////////////////////////////////////
                // INNERMOST END
                /////////////////////////////////////////////////////////
            }

            /* Apply VA increments */
            numer_x    += d_numer_x;
            numer_y    += d_numer_y;
            denom      += d_denom;
            
            min_x = min_aligned_x + SUBSPAN_LEN;
        }

        if (min_x > max_x) continue;
        
        int32_t length = max_x - min_x + 1;
        uint8_t num_full_spans = (uint8_t)(length>>SUBSPAN_SHIFT);
        uint8_t tail_len = (uint8_t)(length & SUBSPAN_MOD);        

        for (uint8_t i_span = 0; i_span < num_full_spans; i_span++) {
            if (denom + d_denom == 0) continue;
            uA = uB;
            vA = vB;
            uB = ((numer_x + d_numer_x)>>TEXEL_PREC)/(denom + d_denom);
            vB = ((numer_y + d_numer_y)>>TEXEL_PREC)/(denom + d_denom);

            for (int32_t i = 0; i < (int16_t)SUBSPAN_LEN; i++, scr_curr++, x++) {
                /////////////////////////////////////////////////////////
                // INNERMOST START
                /////////////////////////////////////////////////////////
                if ( ! (*scr_curr & (1LL<<30))) continue;

                zgl_Color color;
                int32_t u = (int32_t)((uA + ((i*(uB-uA))>>SUBSPAN_SHIFT)));
                int32_t v = (int32_t)((vA + ((i*(vB-vA))>>SUBSPAN_SHIFT)));
               
                /* Determine initial color, before processing */
                //drawtri_assert((u & u_mod) >= 0 && (v & v_mod) >= 0);
                color = 0;

                color = tx_pixels[(u & u_mod) + ((v & v_mod)<<tx_w_shift)];

                    
                // Modify pixel color.

                /* Write the pixel */
                *scr_curr = color;
                /////////////////////////////////////////////////////////
                // INNERMOST END
                /////////////////////////////////////////////////////////
            }
            
            /* Apply VA increments */
            numer_x    += d_numer_x;
            numer_y    += d_numer_y;
            denom      += d_denom;

        }
        {
            if (denom + d_denom == 0) continue;
            uA = uB;
            vA = vB;
            uB = ((numer_x + d_numer_x)>>TEXEL_PREC)/(denom + d_denom);
            vB = ((numer_y + d_numer_y)>>TEXEL_PREC)/(denom + d_denom);
        
            for (int32_t i = 0; i < tail_len; i++, scr_curr++, x++) {
                /////////////////////////////////////////////////////////
                // INNERMOST START
                /////////////////////////////////////////////////////////
                if ( ! (*scr_curr & (1LL<<30))) continue;

                zgl_Color color;
                int32_t u = (int32_t)((uA + ((i*(uB-uA))>>SUBSPAN_SHIFT)));
                int32_t v = (int32_t)((vA + ((i*(vB-vA))>>SUBSPAN_SHIFT)));
               
                /* Determine initial color, before processing */
                //drawtri_assert((u & u_mod) >= 0 && (v & v_mod) >= 0);
                color = 0;

                color = tx_pixels[(u & u_mod) + ((v & v_mod)<<tx_w_shift)];

                    
                // Modify pixel color.

                /* Write the pixel */
                *scr_curr = color;
                /////////////////////////////////////////////////////////
                // INNERMOST END
                /////////////////////////////////////////////////////////
            }
        }
    } // for (uint16_t y = (uint16_t)min_y; y <= max_y; y++) {
    
    return 1;
       
}

