#include "src/misc/epm_includes.h"

#include "src/draw/textrect.h"
#include "src/draw/window/window.h"

#include "src/draw/viewport/viewport_internal.h"
#include "src/system/state.h"

#define BUFSIZE 256
#define INPUTMAX 255

extern void unmap_all_viewports(void);

static StringList output_hist;
static StringList input_hist;
static size_t i_input = 0;

typedef struct TextEntryField {
    size_t min;
    size_t beg;
    size_t point;
    size_t mark;
    size_t end;
    size_t max;

    char killbuf[BUFSIZE];
    
    char bef[BUFSIZE];
    size_t point_aft;
    char aft[BUFSIZE];
} TextEntryField;

TextEntryField tef = { 0, 0, 0, 0, 0, INPUTMAX, {0}, {0}, 0, {0} };

typedef struct Console {
    Window *last_known_win;
    //VPCode last_known_vp;

    int vert_padding;
    int hori_padding;
    zgl_PixelRect usable_rect;

    BMPFontParameters bmpf;
    TextRect tr;
        
    int blink_offset;
    size_t prompt_len;
    char *prompt;
} Console;

static Console cs = {.blink_offset = 0, .prompt = "> "};

#define MAX_LEN 256

void print_to_console(char *str) {
    if (strlen(str) > 0) {
        char *token;
        token = strtok(str, "\n");
        while (token != NULL) {
            append_to_StringList(&output_hist, token);
            token = strtok(NULL, "\n");
        }
    }
}

#include "src/system/dir.h"

static epm_Result epm_InitConsole(void) {
    initialize_StringList(&output_hist);
    initialize_StringList(&input_hist);
    initialize_TextRect(&cs.tr, FC_IBMVGA, NULL, &output_hist);
    
    get_font_parameters(FC_IBMVGA, &cs.bmpf);
    cs.prompt_len = strlen(cs.prompt);
    cs.vert_padding = 20;
    cs.hori_padding = 20;

    FILE *in_fp = fopen(DIR_LOG "console_input_history.txt", "ab+");
    if ( ! in_fp) {
        epm_Log(LT_ERROR, "Could not open file %s\n", DIR_LOG "console_input_history.txt");
        abort();
    }
    char buffer[MAX_LEN];
    while (fgets(buffer, MAX_LEN, in_fp)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        append_to_StringList(&input_hist, buffer);
        i_input++;
    }
    fclose(in_fp);
    
    return EPM_SUCCESS;
}

static epm_Result epm_TermConsole(void) {
    destroy_StringList(&output_hist);

    FILE *out_fp = fopen(DIR_LOG "console_input_history.txt", "wb");
    if ( ! out_fp) {
        epm_Log(LT_ERROR, "Could not open file %s\n", DIR_LOG "console_input_history.txt");
        abort();
    }
    for (size_t i_str = 0; i_str < input_hist.num_entries; i_str++) {
        for (size_t i_ch = 0; i_ch < strlen(input_hist.entries[i_str]); i_ch++)
            fputc(input_hist.entries[i_str][i_ch], out_fp);

        fputc('\n', out_fp);
    }
    fclose(out_fp);

    destroy_StringList(&input_hist);
    return EPM_SUCCESS;
}

static void onMap_Console(ViewportInterface *self, Viewport *p_VP) {
    Window *win = &p_VP->VPI_win;
    
    dibassert(self->i_VPI == VPI_CONSOLE);

    if (&p_VP->VPI_win != cs.last_known_win) {
        cs.last_known_win = win;
        cs.usable_rect = (zgl_PixelRect)
            {.x = win->rect.x + cs.hori_padding,
             .y = win->rect.y + cs.vert_padding,
             .w = win->rect.w - cs.hori_padding - cs.hori_padding,
             .h = win->rect.h - cs.vert_padding - cs.vert_padding};

        resize_TextRect(&cs.tr, &cs.usable_rect);
    }
}

static inline void strcpy_reverse(char *dst, char *src) {
    size_t len = strlen(src);
    if (len == 0) return;
    
    for (size_t i = 0, j = len-1; i < len; i++, j--) {
        dst[i] = src[j];
    }
    dst[len] = '\0';
}

static inline void strmov_reverse(char *dst, char *src) {
    size_t len = strlen(src);
    if (len == 0) return;
    
    for (size_t i = 0, j = len-1; i < len; i++, j--) {
        dst[i] = src[j];
        src[j] = '\0';
    }
    dst[len] = '\0';
}

static void draw_Console(Window *win) {
    (void)win;

    char tmp_buf[512];
    size_t len = sprintf(tmp_buf, "(b %zu) (p %zu) (P %zu) (e %zu)", tef.beg, tef.point, tef.point_aft, tef.end);
    dibassert(len <= INT_MAX);
    draw_BMPFont_string(g_scr, &win->rect, tmp_buf, cs.usable_rect.x + cs.usable_rect.w - 8*(int)len, win->rect.y+win->rect.h - 20, FC_IBMVGA, 0xCCCCCC);
    
    char tmp_str[BUFSIZE] = {'\0'};
    strcpy(tmp_str, tef.bef);
    size_t tmp_len = strlen(tmp_str);
    for (size_t j = 0; j < tef.point_aft; j++) {
        tmp_str[tmp_len] = tef.aft[tef.point_aft-1-j];
        tmp_len++;
    }
    tmp_str[tmp_len] = '\0';
    // Now tmp_str is the current text input



    size_t line = cs.tr.num_lines, index;
    size_t tmp_lines = 1 + ((cs.prompt_len + strlen(tef.bef) + strlen(tef.aft)) / cs.tr.line_width); // >= 1
    line -= tmp_lines;
    index = cs.tr.line_width * line;
    tmp_buf[0] = '\0';
    strcat(tmp_buf, cs.prompt);
    strcat(tmp_buf, tef.bef);
    strcpy_reverse(tmp_buf + strlen(tmp_buf), tef.aft);

    draw_BMPFont_string(g_scr, &cs.usable_rect, tmp_buf,
                        cs.usable_rect.x,
                        cs.usable_rect.y + cs.bmpf.height*(int)line,
                        FC_IBMVGA, 0xCCCCCC);

    
    size_t i_point = index + cs.prompt_len + tef.point;
    size_t x_point = i_point % cs.tr.line_width;
    size_t y_point = i_point / cs.tr.line_width;
    uint64_t tic = state.loop.tic - cs.blink_offset;
    tic = (tic & 127)>>6;
    if (tic) {
        zgl_FillRect(g_scr,
                     cs.usable_rect.x + 8*(int)x_point,
                     cs.usable_rect.y + 16*(int)y_point,
                     8, 16, 0xCCCCCC);
        draw_BMPFont_char(g_scr, &cs.usable_rect, *(tmp_buf + cs.prompt_len + tef.point),
                            cs.usable_rect.x + 8*(int)x_point,
                            cs.usable_rect.y + 16*(int)y_point,
                            FC_IBMVGA, 0x000000);
    }

    //    draw_TextRect(g_scr, &cs.tr);
    size_t i = 0;
    while (output_hist.entries[i] != NULL) {
        i++;
    }

    if (i == 0) return;
    
    for (i = i - 1; i + 1 >= 1; i--) {
        tmp_lines = 1 + (strlen(output_hist.entries[i]) / cs.tr.line_width);
        if (line < tmp_lines) break;
        line -= tmp_lines;
        draw_BMPFont_string(g_scr, &cs.tr.rect, output_hist.entries[i],
                            cs.tr.rect.x,
                            cs.tr.rect.y + cs.tr.bmpf.height*(int)line,
                            FC_IBMVGA, 0xCCCCCC);
    }
}

static zgl_KeyCode char_keys[] = {
    ZK_0,
    ZK_1,
    ZK_2,
    ZK_3,
    ZK_4,
    ZK_5,
    ZK_6,
    ZK_7,
    ZK_8,
    ZK_9,
    ZK_a,
    ZK_b,
    ZK_c,
    ZK_d,
    ZK_e,
    ZK_f,
    ZK_g,
    ZK_h,
    ZK_i,
    ZK_j,
    ZK_k,
    ZK_l,
    ZK_m,
    ZK_n,
    ZK_o,
    ZK_p,
    ZK_q,
    ZK_r,
    ZK_s,
    ZK_t,
    ZK_u,
    ZK_v,
    ZK_w,
    ZK_x,
    ZK_y,
    ZK_z,
    ZK_GRAVE,       /* ` */
    ZK_MINUS,       /* - */
    ZK_EQUALS,      /* = */
    ZK_LEFTBRACKET, /* [ */
    ZK_RIGHTBRACKET,/* ] */
    ZK_BACKSLASH,   /* \ */ 
    ZK_SEMICOLON,   /* ; */
    ZK_APOSTROPHE,  /* ' */
    ZK_COMMA,       /* , */
    ZK_PERIOD,      /* . */
    ZK_SLASH,       /* / */
    ZK_SPC,
    ZK_NONE,
};

static inline bool is_char_keycode(zgl_KeyCode key) {
    for (size_t i = 0; char_keys[i] != ZK_NONE; i++) {
        if (key == char_keys[i]) {
            return true;
        }
    }

    return false;
}

static void delete_char(void) {
    if (tef.point < tef.end) {
        tef.point_aft--;
        tef.aft[tef.point_aft] = '\0';
        tef.end--;
    }
}

static void point_forward(void) {
    if (tef.point < tef.end) {
        tef.point_aft--;
        tef.bef[tef.point] = tef.aft[tef.point_aft];
        tef.aft[tef.point_aft] = '\0';
        tef.point++;
    }
}

static void point_backward(void) {
    if (tef.point > tef.beg) {
        tef.point--;
        tef.aft[tef.point_aft] = tef.bef[tef.point];
        tef.bef[tef.point] = '\0';
        tef.point_aft++;
    }
}

static void point_to_beg(void) {
    size_t n = tef.point;
    for (size_t i = 0; i < n; i++) {
        point_backward();
    }
    dibassert(tef.point == 0);
}

static void point_to_end(void) {
    size_t n = tef.point_aft;
    for (size_t j = 0; j < n; j++) {
        point_forward();
    }
}

static void transpose_chars(void) {
    if (tef.point < tef.end && tef.point > tef.beg) {
        char tmp = tef.bef[tef.point - 1];
        tef.bef[tef.point - 1] = tef.aft[tef.point_aft - 1];
        tef.aft[tef.point_aft - 1] = tmp;
        point_forward();
    }
}

static void kill_line(void) {
    if (tef.point < tef.end) {
        strmov_reverse(tef.killbuf, tef.aft);
    }
    tef.end = tef.point;
    tef.point_aft = 0;
}

static void copy_line(void) {
    if (tef.point < tef.end) {
        strcpy_reverse(tef.killbuf, tef.aft);
    }
}

static void yank(void) {
    strcpy_reverse(tef.aft + tef.point_aft, tef.killbuf);
    tef.end += strlen(tef.killbuf);
    tef.point_aft += strlen(tef.killbuf);
}

static void set_mark(void) {
    tef.mark = tef.point;
}

static void kill_region(void) {
    //TODO
}

static void copy_region(void) {
    //TODO
}

static void backspace(void) {
    if (tef.point > tef.beg) {
        tef.point--;
        tef.bef[tef.point] = '\0';
        tef.end--;
    }
}


static void quit(int argc, char **argv, char *output_str) {
    (void)argc;
    (void)argv;
    (void)output_str;
    
    zgl_PushCloseEvent();
}

static void _log(int argc, char **argv, char *output_str) {
    (void)argc;
    (void)output_str;
    
    char const *str = argv[1];
    _epm_Log("CONSOLE", LT_INFO, str);
    strcpy(output_str, str);
}

// commands act kind of like input events, except that

// quit
// log a message
// display keybindings

static void CMDH_point_to_beg(int argc, char **argv, char *output_str) {
    (void)argc;
    (void)argv;
    (void)output_str;
    
    point_to_beg();
}


#include "src/input/command.h"
epm_Command const CMD_point_to_beg = {
    .name = "point_to_beg",
    .argc_min = 1,
    .argc_max = 1,
    .handler = CMDH_point_to_beg,
};

epm_Command const CMD_quit = {
    .name = "quit",
    .argc_min = 1,
    .argc_max = 1,
    .handler = quit,
};

epm_Command const CMD_log = {
    .name = "log",
    .argc_min = 2,
    .argc_max = 2,
    .handler = _log
};

static void reset_text_input(void) {
    memset(tef.bef, '\0', BUFSIZE);
    memset(tef.aft, '\0', BUFSIZE);
    tef.mark = 0;
    tef.point = 0;
    tef.point_aft = 0;
    tef.end = 0;
}

static char cmd_output[1024];

static void submit_command(void) {
    point_to_end();
    
    append_to_StringList(&input_hist, tef.bef);
    i_input = input_hist.num_entries;

    cmd_output[0] = '\0';
    epm_SubmitCommandString(tef.bef, cmd_output);
    char tmp_str[256] = {'\0'};
    strcat(tmp_str, cs.prompt);
    strcat(tmp_str, tef.bef);
    append_to_StringList(&output_hist, tmp_str);

    reset_text_input();
    
    if (cmd_output[0] != '\0') {
        char *token;
        token = strtok(cmd_output, "\n");
        while (token != NULL) {
            append_to_StringList(&output_hist, token);
            token = strtok(NULL, "\n");
        }
    }
}

static void insert_char(zgl_KeyCode key, bool shift) {
    // NEW
    char *ch = tef.bef + tef.point;
    tef.point++;
    tef.end++;
    
    if ( ! shift) {
        if (ZK_a <= key && key <= ZK_z) {
            *ch = (char)((key + 'a') - ZK_a);
        }
        else if (ZK_0 <= key && key <= ZK_9) {
            *ch = (char)((key + '0') - ZK_0);
        }
        switch (key) {
        case ZK_SPC:
            *ch = ' ';
            break;
        case ZK_GRAVE:
            *ch = '`';
            break;
        case ZK_MINUS:
            *ch = '-';
            break;
        case ZK_EQUALS:
            *ch = '=';
            break;
        case ZK_LEFTBRACKET:
            *ch = '[';
            break;
        case ZK_RIGHTBRACKET:
            *ch = ']';
            break;
        case ZK_BACKSLASH:
            *ch = '\\';
            break;
        case ZK_SEMICOLON:
            *ch = ';';
            break;
        case ZK_APOSTROPHE:
            *ch = '\'';
            break;
        case ZK_COMMA:
            *ch = ',';
            break;
        case ZK_PERIOD:
            *ch = '.';
            break;
        case ZK_SLASH:
            *ch = '/';
            break;
        default:
            break;
        }
    }
    else {
        if (ZK_a <= key && key <= ZK_z) { // regular insertion
            *ch = (char)((key + 'A') - ZK_a);
        }
        switch (key) {
        case ZK_SPC:
            *ch = ' ';
            break;
        case ZK_0:
            *ch = ')';
            break;
        case ZK_1:
            *ch = '!';
            break;
        case ZK_2:
            *ch = '@';
            break;
        case ZK_3:
            *ch = '#';
            break;
        case ZK_4:
            *ch = '$';
            break;
        case ZK_5:
            *ch = '%';
            break;
        case ZK_6:
            *ch = '^';
            break;
        case ZK_7:
            *ch = '&';
            break;
        case ZK_8:
            *ch = '*';
            break;
        case ZK_9:
            *ch = '(';
            break;
        case ZK_GRAVE:
            *ch = '~';
            break;
        case ZK_MINUS:
            *ch = '_';
            break;
        case ZK_EQUALS:
            *ch = '+';
            break;
        case ZK_LEFTBRACKET:
            *ch = '{';
            break;
        case ZK_RIGHTBRACKET:
            *ch = '}';
            break;
        case ZK_BACKSLASH:
            *ch = '|';
            break;
        case ZK_SEMICOLON:
            *ch = ':';
            break;
        case ZK_APOSTROPHE:
            *ch = '"';
            break;
        case ZK_COMMA:
            *ch = '<';
            break;
        case ZK_PERIOD:
            *ch = '>';
            break;
        case ZK_SLASH:
            *ch = '?';
            break;
        default:
            break;
        }
    }    
}

static void do_KeyPress_Console(Window *win, zgl_KeyPressEvent *zevt) {
    (void)win;

    cs.blink_offset = (state.loop.tic & 127) + 60;

    uint8_t mod_flags = zevt->mod_flags;
    zgl_KeyCode key = zevt->zk;
    
    if (mod_flags & ZGL_CTRL_MASK) { // control inputs
        switch (key) {
        case ZK_a:
            point_to_beg();
            break;
        case ZK_e:
            point_to_end();
            break;
        case ZK_b:
            point_backward();
            break;
        case ZK_f:
            point_forward();
            break;
        case ZK_d:
            delete_char();
            break;
        case ZK_t:
            transpose_chars();
            break;
        case ZK_k:
            kill_line();
            break;
        case ZK_y:
            yank();
            break;
        case ZK_w:
            kill_region();
            break;
        case ZK_SPC:
            set_mark();
            break;
        default:
            break;
        }
    }
    else if (mod_flags & ZGL_ALT_MASK) {
        switch (key) {
        case ZK_k:
            copy_line();
            break;
        case ZK_w:
            copy_region();
            break;
        case ZK_p:
            if (input_hist.num_entries > 0) {
                if (i_input > 0) {
                    i_input--;
                    reset_text_input();
                    strcpy(tef.bef, input_hist.entries[i_input]);
                    tef.end = strlen(tef.bef);
                    tef.point = tef.end;
                }
            }
            break;
        case ZK_n:
            if (input_hist.num_entries > 0) {
                if (i_input < input_hist.num_entries) {
                    i_input++;
                    reset_text_input();
                    if (i_input < input_hist.num_entries) {
                        strcpy(tef.bef, input_hist.entries[i_input]);
                        tef.end = strlen(tef.bef);
                        tef.point = tef.end;
                    }
                }
            }
            break;
        default:
        break;
        }
    }
    else {
        if (key == ZK_RET) {
            submit_command();
        }
        else if (key == ZK_BACK) {
            backspace();
        }
        else if (is_char_keycode(key) && tef.end < tef.max-1) {
            insert_char(key, mod_flags & ZGL_SHIFT_MASK);
        }   
    }

    dibassert(tef.min <= tef.beg &&
              tef.beg <= tef.point &&
              tef.point <= tef.end &&
              tef.end <= tef.max);
    dibassert(tef.point + tef.point_aft == tef.end);
    dibassert(tef.point == strlen(tef.bef));
    dibassert(tef.point_aft == strlen(tef.aft));
}

ViewportInterface interface_Console = {
    .i_VPI = VPI_CONSOLE,
    .mapped_i_VP = VP_NONE,
    .windata = NULL,
    .onUnmap = NULL,
    .onMap = onMap_Console,
    .init = epm_InitConsole,
    .term = epm_TermConsole,
    .winfncs = {
        .draw = draw_Console,
        .onKeyPress = do_KeyPress_Console,
        NULL
    },
    .name = "Console"
};
