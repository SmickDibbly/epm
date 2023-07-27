#include "src/misc/epm_includes.h"
#include "src/draw/draw.h"
#include "src/draw/viewport/viewport_internal.h"
#include "src/misc/screenshot.h"
#include "zigil/zigil.h"
#include "zigil/zigil_time.h"
#include <time.h>

// TODO: spamming the screenshot functions seems to make ASan unhappy. Figure
// out why. In the meantime, limit screenshot frequency with a cooldown timer.
#define COOLDOWN_MS 3000
static utime_t g_last_screenshot_time = 0;

epm_Result epm_Screenshot(char const *filename) {
    utime_t now = zgl_ClockQuery();
    if (now - g_last_screenshot_time < COOLDOWN_MS) {
        epm_Log(LT_INFO, "Wait %s seconds before taking another screenshot.",
                fmt_fix_d(((COOLDOWN_MS-(now-g_last_screenshot_time))<<16)/1000, 16, 4));
        return EPM_FAILURE;
    }
    
    zgl_WriteBMP(filename, g_scr);

    g_last_screenshot_time = now;

    return EPM_SUCCESS;
}

epm_Result epm_ScreenshotWindow(char const *_filename, Window *win) {
    utime_t now = zgl_ClockQuery();
    if (now - g_last_screenshot_time < COOLDOWN_MS) {
        epm_Log(LT_INFO, "Wait %s seconds before taking another screenshot.",
                fmt_fix_d(((COOLDOWN_MS-(now-g_last_screenshot_time))<<16)/1000, 16, 4));
        return EPM_FAILURE;
    }
    
    zgl_PixelArray *crop = zgl_CreatePixelArray(win->rect.w, win->rect.h);

    zgl_Blit(crop, 0, 0,
             g_scr, win->rect.x, win->rect.y,
             win->rect.w, win->rect.h);

    char filename[256];
    if ( ! _filename) {
        time_t current_time;
        struct tm *utc_time;
        char utc_str[MAX_UTC_LEN + 1 + 1] = {'\0'};
        current_time = time(NULL);
        utc_time = gmtime(&current_time);
        strftime(utc_str, MAX_UTC_LEN, "%Y%m%d%H%M%S_", utc_time);

        strcpy(filename, utc_str);        
        strcat(filename, win->name);
    }
    else {
        strcpy(filename, _filename);
    }

    strcat(filename, ".bmp");
    
    zgl_WriteBMP(filename, crop);

    zgl_DestroyPixelArray(crop);

    g_last_screenshot_time = now;

    return EPM_SUCCESS;
}

epm_Result epm_ScreenshotViewport(char const *_filename, Viewport *vp) {
    utime_t now = zgl_ClockQuery();
    if (now - g_last_screenshot_time < COOLDOWN_MS) {
        epm_Log(LT_INFO, "Wait %s seconds before taking another screenshot.",
                fmt_fix_d(((COOLDOWN_MS-(now-g_last_screenshot_time))<<16)/1000, 16, 4));
        return EPM_FAILURE;
    }
    
    if ( ! vp) {
        // TODO: Get active VP.
        vp = active_viewport;
        if ( ! vp) {
            epm_Log(LT_INFO, "There is no active viewport of which to take a screenshot.");
            return EPM_FAILURE;
        }
    }
    
    Window *win = &vp->VPI_win;
    
    zgl_PixelArray *crop = zgl_CreatePixelArray(vp->VPI_win.rect.w, vp->VPI_win.rect.h);

    zgl_Blit(crop, 0, 0,
             g_scr, win->rect.x, win->rect.y,
             win->rect.w, win->rect.h);

    char filename[256];
    if ( ! _filename) {
        time_t current_time;
        struct tm *utc_time;
        char utc_str[MAX_UTC_LEN + 1 + 1] = {'\0'};
        current_time = time(NULL);
        utc_time = gmtime(&current_time);
        strftime(utc_str, MAX_UTC_LEN, "%Y%m%d%H%M%S_", utc_time);

        strcpy(filename, utc_str);        
        strcat(filename, vp->mapped_p_VPI->name);
    }
    else {
        strcpy(filename, _filename);
    }

    strcat(filename, ".bmp");

    zgl_WriteBMP(filename, crop);

    zgl_DestroyPixelArray(crop);

    g_last_screenshot_time = now;

    return EPM_SUCCESS;
}
