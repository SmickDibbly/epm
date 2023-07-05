#include "src/draw/xspans.h"

uint16_t g_xspans[2*MAX_SCREEN_HEIGHT] = {0};

static void low_bresenham
(Window *win,
 zgl_Pixit x0,
 zgl_Pixit y0,
 zgl_Pixit x1,
 zgl_Pixit y1,
 uint8_t side_ofs) {
    int32_t
        min_x = win->rect.x,
        max_x = win->rect.x + win->rect.w - 1,
        min_y = win->rect.y,
        max_y = win->rect.y + win->rect.h - 1; // TODO: no magic
    
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    int32_t yi = 1;
    
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }

    int32_t D = (LSHIFT32(dy, 1)) - dx;
    
    zgl_Pixit x = x0, y = y0;
    bool y_was_incremented = true;
    
    uint16_t *p_xspans = g_xspans + (2*y0);
    p_xspans += side_ofs;

    while (y < min_y || y > max_y) {
        if (y_was_incremented) {
            p_xspans += LSHIFT32(yi, 1);
            y_was_incremented = false;
	    }
        
        if (D > 0) {
            y = y + yi;
            D = D + (LSHIFT32((dy - dx), 1));
            y_was_incremented = true;
        }
        else {
            D = D + (LSHIFT32(dy, 1));
        }
        
        x++;
    }
    
    while (x <= x1) {
        if (y_was_incremented) {
            if (y >= min_y && max_y >= y) {
                *p_xspans = (uint16_t)MIN(MAX(x, min_x), max_x);
            }
            p_xspans += LSHIFT32(yi, 1);
            y_was_incremented = false;
	    }
        
        if (D > 0) {
            y = y + yi;
            D = D + (LSHIFT32((dy - dx), 1));
            y_was_incremented = true;
        }
        else {
            D = D + (LSHIFT32(dy, 1));
        }

        x++;
    }

}



static void high_bresenham
(Window *win,
 zgl_Pixit x0,
 zgl_Pixit y0,
 zgl_Pixit x1,
 zgl_Pixit y1,
 uint8_t side_ofs) {
    int32_t
        min_x = win->rect.x,
        max_x = win->rect.x + win->rect.w - 1,
        min_y = win->rect.y,
        max_y = win->rect.y + win->rect.h - 1; // TODO: no magic

    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    int32_t xi = 1;
    
    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }

    int32_t D = (LSHIFT32(dx, 1)) - dy;
    
    uint16_t *p_xspans = g_xspans + (2*y0);
    p_xspans += side_ofs; // adds 1 if right side; (odd indices are max x's)
    
    zgl_Pixit x = x0, y = y0;
    while (y < min_y) {
        p_xspans += 2;
	
        if (D > 0) {
            x = x + xi;
            D = D + (LSHIFT32((dx - dy), 1));
        }
        else {
            D = D + (LSHIFT32(dx, 1));
        }

        y++;
    }

    y1 = MIN(y1, max_y); // we need go no further than max_y
    while (y <= y1) {
        *p_xspans = (uint16_t)MIN(MAX(x, min_x), max_x);
        
        p_xspans += 2;
	
        if (D > 0) {
            x = x + xi;
            D = D + (LSHIFT32((dx - dy), 1));
        }
        else {
            D = D + (LSHIFT32(dx, 1));
        }

        y++;
    }

}


static void compute_xspans_line(Window *win, zgl_Pixel L, zgl_Pixel R, uint8_t side_ofs) {
    if (ABS(L.y - R.y) < ABS(L.x - R.x)) {
        if (L.x > R.x) {
            low_bresenham(win, R.x, R.y, L.x, L.y, side_ofs);
        }
        else {
            low_bresenham(win, L.x, L.y, R.x, R.y, side_ofs);
        }
    }
    else {
        if (L.y > R.y) {
            high_bresenham(win, R.x, R.y, L.x, L.y, side_ofs);
        }
        else {
            high_bresenham(win, L.x, L.y, R.x, R.y, side_ofs);
        }
    }
}

void compute_xspans(Window *win, zgl_Pixel v0, zgl_Pixel v1, zgl_Pixel v2, uint16_t *min_y, uint16_t *max_y) {
    // first, sort the three pixels in order of y-value
    zgl_Pixel low, mid, hi;
    {
        low = v0;
        mid = v1;
        hi = v2;
    
        if (low.y > mid.y) {
            mid = v0;
            low = v1;
        }
    
        if (mid.y > hi.y) {
            hi = mid;
            mid = v2;
            if (low.y > mid.y) {
                mid = low;
                low = v2;
            }
        }
    }


    *min_y = (uint16_t)MAX(low.y, win->rect.y);
    *max_y = (uint16_t)MIN(hi.y, win->rect.y + win->rect.h - 1);

    //drawtri_assert(low.y >= win->rect.y - 4);
    //drawtri_assert(hi.y <= win->rect.y + win->rect.h - 1 + 4);
    
    /////////////////////////////////////////////////////////////////////////
    
#define SIDE_OFS_LEFT 0
#define SIDE_OFS_RIGHT 1

    uint8_t
        hi_to_mid_side,
        hi_to_low_side,
        mid_to_low_side;
    
    if (low.y == mid.y) {
        // flat bottom; just two lines to do
        //        drawtri_assert(low.x != mid.x); // degeneracy test
        
        if (low.x < mid.x) {
            hi_to_low_side = SIDE_OFS_LEFT;
            hi_to_mid_side = SIDE_OFS_RIGHT;
        }
        else {
            hi_to_low_side = SIDE_OFS_RIGHT;
            hi_to_mid_side = SIDE_OFS_LEFT;
        }
        compute_xspans_line(win, hi, mid, hi_to_mid_side);
        compute_xspans_line(win, hi, low, hi_to_low_side);
    }
    else if (hi.y == mid.y) {
        // flat top; just two lines to do
        //        drawtri_assert(hi.x != mid.x); // degeneracy test
        
        if (hi.x < mid.x) {
             hi_to_low_side = SIDE_OFS_LEFT;
            mid_to_low_side = SIDE_OFS_RIGHT;
        }
        else {
             hi_to_low_side = SIDE_OFS_RIGHT;
            mid_to_low_side = SIDE_OFS_LEFT;
        }
        compute_xspans_line(win,  hi, low, hi_to_low_side);
        compute_xspans_line(win, mid, low, mid_to_low_side);
    }
    else {
        // neither top nor bottom are flat; three lines to do.

        // the hi-to-lo line is the rightmost if and only if from the perspective of hi to looking towards lo, mid is on the right.

        if ((mid.y-hi.y)*(low.x-hi.x) > (low.y-hi.y)*(mid.x-hi.x)) {
             hi_to_mid_side = SIDE_OFS_RIGHT;
            mid_to_low_side = SIDE_OFS_RIGHT;
             hi_to_low_side = SIDE_OFS_LEFT;
        }
        else {
             hi_to_mid_side = SIDE_OFS_LEFT;
            mid_to_low_side = SIDE_OFS_LEFT;
             hi_to_low_side = SIDE_OFS_RIGHT;
        }
        compute_xspans_line(win,  hi, mid, hi_to_mid_side);
        compute_xspans_line(win, mid, low, mid_to_low_side);
        compute_xspans_line(win,  hi, low, hi_to_low_side);
    }

#undef SIDE_OFS_LEFT
#undef SIDE_OFS_RIGHT
}



void print_xspans(uint16_t min_y, uint16_t max_y) {
    puts("xspans:");
    for (uint16_t y = min_y; y <= max_y; y++) {
        uint16_t min_x = g_xspans[y<<1], max_x = g_xspans[(y<<1) + 1];
        printf("[%u] = (%u, %u)%s\n", y, min_x, max_x, min_x > max_x ? " <<UHOH>>" : "");

        if (min_x > max_x) {
            fgetc(stdout);
        }
    }

    putchar('\n');
}

