#ifndef DRAW_TEXT_H
#define DRAW_TEXT_H

#include "zigil/zigil.h"

#include "src/misc/epm_includes.h"

#define MONOGRAM_WIDTH 5
#define MONOGRAM_HEIGHT 9
#define MONOGRAM_LINE_SPACING 1
#define MONOGRAM_LETTER_SPACING 1

#define MONOGRAM2_WIDTH 10
#define MONOGRAM2_HEIGHT 18
#define MONOGRAM2_LINE_SPACING 2
#define MONOGRAM2_LETTER_SPACING 2

typedef enum BMPFontCode {
    FC_MONOGRAM1,
    FC_MONOGRAM2,
    FC_IBMVGA,

    NUM_FC
} BMPFontCode;

typedef struct BMPFontParameters {
    int width;
    int height;
    int interchar; // this is just a suggestion
    int interline; // this is just a suggestion
} BMPFontParameters;

extern void get_font_parameters(BMPFontCode font, BMPFontParameters *params);

extern epm_Result draw_BMPFont_char
(zgl_PixelArray *pixarr,
 zgl_PixelRect *rect,
 char const ch,
 zgl_Pixit x,
 zgl_Pixit y,
 BMPFontCode font,
 zgl_Color color);

extern epm_Result draw_BMPFont_string
(zgl_PixelArray *pixarr,
 zgl_PixelRect *rect,
 char const *str,
 zgl_Pixit x,
 zgl_Pixit y,
 BMPFontCode font,
 zgl_Color color);

extern bool temp_FASTFONT;

#endif /* DRAW_TEXT_H */
