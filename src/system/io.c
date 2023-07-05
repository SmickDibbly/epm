#include "zigil/zigil.h"

#include "src/misc/epm_includes.h"
#include "src/system/config_reader.h"

epm_Result epm_InitIOLib(void) {
    char const *str;
    
    str = get_config_str("Video", "screen_width");
    int w = atoi(str);
    
    str = get_config_str("Video", "screen_height");
    int h = atoi(str);
    
    str = get_config_str("Video", "scale");
    int scale = atoi(str);
    switch (scale) {
    case 1:
        scale = 0;
        break;
    case 2:
        scale = 1;
        break;
    case 4:
        scale = 2;
        break;
    case 8:
        scale = 3;
        break;
    default:
        dibassert(false);
    }
    
    if (ZR_ERROR == zgl_InitVideo("Electric Pentacle Editor", w, h, scale, ZGL_VFLAG_VIDFAST)) {
        return EPM_ERROR;
    }
    
    return EPM_SUCCESS;
}

epm_Result epm_TermIOLib(void) {
    zgl_TermVideo();
    
    return EPM_SUCCESS;
}
