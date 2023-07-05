#ifndef TEXTRECT_H
#define TEXTRECT_H

#include "src/misc/epm_includes.h"
#include "src/draw/text.h"
#include "src/draw/window/window.h"
#include "src/misc/stringlist.h"

typedef struct TextRect {
    /* Visual Data */
    zgl_PixelRect rect;
    BMPFontParameters bmpf;
    size_t line_width;
    size_t num_lines;

    /* Text Data */
    StringList const *strlist;
} TextRect;

void initialize_TextRect(TextRect *tr, BMPFontCode font, zgl_PixelRect const *rect, StringList const *list);
void resize_TextRect(TextRect *tr, zgl_PixelRect const *rect);
void draw_TextRect(zgl_PixelArray *pixarr, TextRect *tr);


#endif /* TEXTRECT_H */
