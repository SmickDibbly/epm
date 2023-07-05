#include "src/misc/epm_includes.h"
#include "src/draw/window/window.h"
#include "src/draw/textures.h"
#include "src/draw/text.h"
#include "src/draw/draw.h"

typedef struct ThumbnailSlot {    
    zgl_Pixit slot_x;
    zgl_Pixit slot_y;
    zgl_Pixit dspl_x;
    zgl_Pixit dspl_y;
    zgl_Pixit click_x;
    zgl_Pixit click_y;
} ThumbnailSlot;

static zgl_Pixit slot_w;
static zgl_Pixit slot_h;
static zgl_Pixit dspl_w;
static zgl_Pixit dspl_h;
static zgl_Pixit click_w;
static zgl_Pixit click_h;

static int const pad = 4;

static ThumbnailSlot slots[64];
static int num_slots;
static int slots_per_row;
static int slots_per_col;

static zgl_Pixit display_size = 128;

static size_t i_active_tex = 0;

static void compute_thumbnail_slots(Window *win);

static void draw_texview(Window *win) {
    compute_thumbnail_slots(win);
    
    for (size_t i_tex = 0; i_tex < g_num_textures; i_tex++) {
        if ((int)i_tex == num_slots) break;
        
        ThumbnailSlot slot = slots[i_tex];
        zgl_MipMap *mip = textures[i_tex].mip;
        uint8_t level = 0;
        while ((int32_t)mip->w[level] > dspl_w) {
            level++;
        }
        
        zgl_PixelArray pix;        
        pix.pixels = mip->pixels + mip->level_offset[level];
        pix.w = pix.h = (uint32_t)mip->w[level];
        
        zgl_Color text_color = 0xFFFFFF;
        if (i_tex == i_active_tex) {
            zgl_GrayRect(g_scr,
                         slot.slot_x, slot.slot_y,
                         slot_w, slot_h,
                         0xFF);
            text_color = 0;
        }

        draw_BMPFont_string(g_scr, &win->rect, textures[i_tex].name,
                            slot.dspl_x+4, slot.dspl_y+dspl_h+2,
                            FC_IBMVGA, text_color);
        zgl_BlitEntire(g_scr, slot.dspl_x, slot.dspl_y, &pix);
    }
}

static void draw_TexView(Window *win) {
    draw_texview(win);
}

static void compute_thumbnail_slots(Window *win) {
    dspl_w = display_size;
    dspl_h = display_size;
    slot_w = pad + dspl_w + pad;
    slot_h = pad + dspl_h + 2 + 16 + 2 + pad;
    click_w = dspl_w;
    click_h = dspl_h + 2 + 16 + 2;

    slots_per_row = win->rect.w / (int32_t)slot_w;
    int32_t SW = slots_per_row;
    slots_per_col = win->rect.h / (int32_t)slot_h;
    int32_t SH = slots_per_col;
    num_slots = slots_per_row * slots_per_col;

    for (int32_t SY = 0; SY < SH; SY++) {
        for (int32_t SX = 0; SX < SW; SX++) {
            ThumbnailSlot slot;
        
            slot.slot_x = win->rect.x + SX*slot_w;
            slot.slot_y = win->rect.y + SY*slot_h;
            slot.dspl_x = slot.slot_x + pad;
            slot.dspl_y = slot.slot_y + pad;
            slot.click_x = slot.dspl_x;
            slot.click_y = slot.dspl_y;

            slots[SX + SY*SW] = slot;
        }
    }
}

static void set_display_size(Window *win, zgl_Pixit size) {
    // TODO: Ensure valid size.

    display_size = size;

    compute_thumbnail_slots(win);
}

static void do_PointerPress_TexView(Window *win, zgl_PointerPressEvent *evt) {
    size_t i_tex;
    bool found = false;
    for (i_tex = 0; i_tex < g_num_textures; i_tex++) {
        ThumbnailSlot slot = slots[i_tex];
        
        if (slot.click_x <= evt->x && evt->x < slot.click_x + click_w &&
            slot.click_y <= evt->y && evt->y < slot.click_y + click_h) {
            found = true;
            break;
        }
    }

    if (found) {
        i_active_tex = i_tex;

        extern void apply_texture(char const *texname);
        apply_texture(textures[i_tex].name);
    }
}

static void do_PointerRelease_TexView(Window *win, zgl_PointerReleaseEvent *evt) {
    //    zgl_LongKeyCode key = evt->lk;
}

static void do_PointerMotion_TexView(Window *win, zgl_PointerMotionEvent *evt) {
}

static void do_PointerEnter_TexView(Window *win, zgl_PointerEnterEvent *evt) {
    zgl_SetCursor(ZC_arrow);
}

static void do_PointerLeave_TexView(Window *win, zgl_PointerLeaveEvent *evt) {
}

static void do_KeyPress_TexView(Window *win, zgl_KeyPressEvent *evt) {
    zgl_KeyCode zk = evt->zk;

    if (zk == ZK_EQUALS) { // +
        set_display_size(win, MIN(display_size<<1, 256));
    }
    else if (zk == ZK_MINUS) { // -
        set_display_size(win, MAX(display_size>>1, 32));
    }
}

#include "src/draw/viewport/viewport_internal.h"
ViewportInterface interface_TexView = {
    .i_VPI = VPI_TEXVIEW,
    .mapped_i_VP = VP_NONE,
    .windata = NULL,
    .onUnmap = NULL,
    .onMap   = NULL,
    .winfncs = {
        .draw             = draw_TexView,
        .onPointerPress   = do_PointerPress_TexView,
        .onPointerRelease = do_PointerRelease_TexView,
        .onPointerMotion  = do_PointerMotion_TexView,
        .onPointerEnter   = do_PointerEnter_TexView,
        .onPointerLeave   = do_PointerLeave_TexView,
        .onKeyPress       = do_KeyPress_TexView,
        NULL,
    },
    .name = "TexView"
};
