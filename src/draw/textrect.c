#include "src/draw/textrect.h"

#undef LOG_LABEL
#define LOG_LABEL "TEXT"

#define MAX_TEXTRECTS 8

void draw_TextRect(zgl_PixelArray *pixarr, TextRect *tr) {
    char **entries = tr->strlist->entries;
    
    size_t i = 0;
    while (entries[i] != NULL) {
        i++;
    }

    if (i == 0) return;

    size_t tmp_lines;
    size_t line = tr->num_lines;
    
    for (i = i - 1; i + 1 >= 1; i--) {
        tmp_lines = 1 + (strlen(entries[i]) / tr->line_width);
        if (line < tmp_lines) break;
        line -= tmp_lines;
        draw_BMPFont_string(pixarr, &tr->rect, entries[i],
                            tr->rect.x,
                            tr->rect.y + tr->bmpf.height*(int)line,
                            FC_IBMVGA, 0xCCCCCC);
    }
}

void initialize_TextRect(TextRect *tr, BMPFontCode font, zgl_PixelRect const *rect, StringList const *list) {
    get_font_parameters(font, &tr->bmpf);
    tr->strlist = list;
    
    if (rect) {
        tr->rect = *rect;
        tr->line_width = tr->rect.w / tr->bmpf.width;
        tr->num_lines = tr->rect.h / tr->bmpf.height;
    }
}

void resize_TextRect(TextRect *tr, zgl_PixelRect const *rect) {
    tr->rect = *rect;
    tr->line_width = tr->rect.w / tr->bmpf.width;
    tr->num_lines = tr->rect.h / tr->bmpf.height;
}
