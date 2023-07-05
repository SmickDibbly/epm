#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include "src/draw/window/window.h"
#include "src/draw/viewport/viewport_internal.h"

extern epm_Result epm_Screenshot(char const *filename);
extern epm_Result epm_ScreenshotWindow(char const *filename, Window *win);
extern epm_Result epm_ScreenshotViewport(char const *_filename, Viewport *vp);

#endif /* SCREENSHOT_H */
