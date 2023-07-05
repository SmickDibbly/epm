#include <string.h>

#include "src/misc/epm_includes.h"

#include "src/draw/draw.h"
#include "src/draw/text.h"
#include "src/draw/default_layout.h"

typedef struct BMPFont {
    int width;
    int height;
    int interchar;
    int interline;
    
    int fileinter;
    zgl_PixelArray *data;
} BMPFont;
 
static BMPFont monogram1;
static BMPFont monogram2;
static BMPFont ibmvga;

void get_font_parameters(BMPFontCode font, BMPFontParameters *params) {
    BMPFont bmpf;
    if (font == FC_MONOGRAM1) {
        bmpf = monogram1;
    }
    else if (font == FC_MONOGRAM2) {
        bmpf = monogram2;
    }
    else if (font == FC_IBMVGA) {
        bmpf = ibmvga;
    }
    else {
        return;
    }

    params->width = bmpf.width;
    params->height = bmpf.height;
    params->interchar = bmpf.interchar;
    params->interline = bmpf.interline;
}

epm_Result epm_InitText(void) {
    monogram1.data = zgl_ReadBMP("../assets/monogram.bmp");
    if (! monogram1.data) {
        return EPM_FAILURE;
    }
    monogram1.width = 5;
    monogram1.height = 9;
    monogram1.interchar = 1;
    monogram1.interline = 1;
    monogram1.fileinter = 1;
    
    monogram2.data = zgl_ReadBMP("../assets/monogram2.bmp");
    if (! monogram2.data) {
        return EPM_FAILURE;
    }
    monogram2.width = 10;
    monogram2.height = 18;
    monogram2.interchar = 2;
    monogram2.interline = 2;
    monogram2.fileinter = 2;

    ibmvga.data = zgl_ReadBMP("../assets/ibmvga_8x16.bmp");
    if (! ibmvga.data) {
        return EPM_FAILURE;
    }
    ibmvga.width = 8;
    ibmvga.height = 16;
    ibmvga.interchar = 0;
    ibmvga.interline = 0;
    ibmvga.fileinter = 0;
    
    return EPM_SUCCESS;
}

epm_Result epm_TermText(void) {
    zgl_DestroyPixelArray(monogram1.data);
    zgl_DestroyPixelArray(monogram2.data);
    zgl_DestroyPixelArray(ibmvga.data);
    return EPM_SUCCESS;
}

extern epm_Result draw_BMPFont_char
(zgl_PixelArray *pixarr,
 zgl_PixelRect *p_rect,
 char const ch,
 zgl_Pixit x,
 zgl_Pixit y,
 BMPFontCode font,
 zgl_Color color) {
    (void)p_rect;
    
    BMPFont bmpf;
    switch (font) {
    case FC_MONOGRAM1:
        bmpf = monogram1;
        break;
    case FC_MONOGRAM2:
        bmpf = monogram2;
        break;
    case FC_IBMVGA: {
        bmpf = ibmvga;
    
        int ch2 = (int)ch;
        if ( ch2 == '\0') {
            ch2 = ' ';
        }
        else if ( ! (32 <= ch2 && ch2 <= 126)) {
            ch2 = '*';
        }
            
        int src_y = (ch2 - 32)<<4;

        zgl_BlitBMPFont(pixarr, x, y, bmpf.data,
                        0, src_y, 8, 16, color);
            
        return EPM_SUCCESS;
    }
        break;
    default:
        return EPM_FAILURE;
    }
    
    int ch2 = (int)ch;
    if ( ch2 == '\0') {
        ch2 = ' ';
    }
    else if ( ! (32 <= ch2 && ch2 <= 126)) {
        ch2 = '*';
    }
    
    int src_y = (ch2 - 32)*(bmpf.height + bmpf.fileinter);

    zgl_BlitBMPFont(pixarr, x, y, bmpf.data,
                    0, src_y, bmpf.width, bmpf.height, color);


    return EPM_SUCCESS;
}

bool temp_FASTFONT = false;

epm_Result draw_BMPFont_string
(zgl_PixelArray *pixarr,
 zgl_PixelRect *p_rect,
 char const *str,
 zgl_Pixit x,
 zgl_Pixit y,
 BMPFontCode font,
 zgl_Color color) {
    zgl_PixelRect rect;
    if (p_rect == NULL) {
        rect.x = 0;
        rect.y = 0;
        rect.w = pixarr->w;
        rect.h = pixarr->h;
    }
    else {
        rect.x = p_rect->x;
        rect.y = p_rect->y;
        rect.w = p_rect->w;
        rect.h = p_rect->h;
    }
    
    BMPFont bmpf;
    switch (font) {
    case FC_MONOGRAM1:
        bmpf = monogram1;
        break;
    case FC_MONOGRAM2:
        bmpf = monogram2;
        break;
    case FC_IBMVGA: {
        bmpf = ibmvga;

        int
            dest_x = x,
            dest_y = y;

        size_t len = strlen(str);
    
        for (size_t i_ch = 0; i_ch < len; i_ch++) {
            int ch = (int)str[i_ch] - 32;
            if ( ! (0 <= ch && ch <= 126-32)) {
                ch = '*' - 32;
            }
            
            int src_y = ch<<4;

            zgl_BlitBMPFont(pixarr, dest_x, dest_y, bmpf.data,
                            0, src_y, 8, 16, color);    
            

            dest_x += 8;
            if (dest_x + 8 >= rect.x + rect.w) {
                dest_x = x;
                dest_y += 16;
            }
        }
    
        return EPM_SUCCESS;
    }
        break;
    default:
        return EPM_FAILURE;
    }

    int
        dest_x = x,
        dest_y = y,
        w = bmpf.width,
        h = bmpf.height;

    size_t len = strlen(str);
    
    for (size_t i_ch = 0; i_ch < len; i_ch++) {
        int ch = (int)str[i_ch] - 32;
        if ( ! (0 <= ch && ch <= 126-32)) {
            ch = '*' - 32;
        }
        
        int src_y = ch*(bmpf.height + bmpf.fileinter);

        zgl_BlitBMPFont(pixarr, dest_x, dest_y, bmpf.data,
                        0, src_y, w, h, color);

        dest_x += bmpf.width + bmpf.interchar;
        if (dest_x + bmpf.width >= rect.x + rect.w) {
            dest_x = x;
            dest_y += bmpf.height + bmpf.interline;
        }
    }
    
    return EPM_SUCCESS;
}
