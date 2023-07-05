#include "src/draw/textures.h"

// weird includes
#include "src/world/geometry.h"

#undef LOG_LABEL
#define LOG_LABEL "TEXTURES"

zgl_MipMap *MIP_light_icon;
zgl_MipMap *MIP_edcam_icon;

size_t g_num_textures = 0;
epm_Texture textures[MAX_TEXTURES] = {0};
static zgl_PixelArray texture_pix[MAX_TEXTURES] = {0};

static void texture_name_from_filename(char const *filename, char *tex_name);

static char g_nametmp[MAX_TEXTURE_NAME_LEN + 1] = {'\0'};

epm_Texture *load_Texture(char const *filename) {
    epm_Texture *tex = &textures[g_num_textures];
    
    texture_name_from_filename(filename, g_nametmp);
    
    zgl_PixelArray *pix = zgl_ReadBMP(filename);
    if ( ! pix) {
        epm_Log(LT_WARN, "Texture named \"%s\" not found from file.\n", g_nametmp);
        return NULL;
    }

    if (pix->w != pix->h) {
        epm_Log(LT_WARN, "Texture named \"%s\" has unequal width and height; cannot use.\n", g_nametmp);
        return NULL;
    }
    
    if (pix->w == 0 || pix->h == 0) {
        epm_Log(LT_WARN, "Texture named \"%s\" has 0 width and/or 0 height; cannot use.\n", g_nametmp);
        return NULL;
    }

    if ((pix->w & (pix->w - 1)) != 0 ||
        (pix->h & (pix->h - 1)) != 0 ) {
        epm_Log(LT_WARN, "Texture named \"%s\" has either width or height that is not a power of 2; cannot use.\n", g_nametmp);
        return NULL;
    }
    
    dibassert(pix->w <= 1024);
    dibassert(pix->h <= 1024);
    
    uint32_t v = pix->w;  // 32-bit value to find the log2 of 
    register uint32_t r;
    r  =  (v & 0xAAAAAAAA) != 0;
    r |= ((v & 0xFFFF0000) != 0) << 4;
    r |= ((v & 0xFF00FF00) != 0) << 3;
    r |= ((v & 0xF0F0F0F0) != 0) << 2;
    r |= ((v & 0xCCCCCCCC) != 0) << 1;

    snprintf(tex->name, MAX_TEXTURE_NAME_LEN + 1, "%s", g_nametmp);
    tex->log2_wh = (uint8_t)r;
    tex->w = (uint16_t)pix->w;
    tex->h = (uint16_t)pix->h;

    tex->mip = zgl_MipifyPixelArray(pix);
    tex->pixarr = &texture_pix[g_num_textures];
    tex->pixarr->pixels = tex->mip->pixels;
    tex->pixarr->w = pix->w;
    tex->pixarr->h = pix->h;
    
    zgl_DestroyPixelArray(pix);
    memset(g_nametmp, '\0', MAX_TEXTURE_NAME_LEN+1);
    g_num_textures++;
    
    return tex;
}

epm_Result unload_Texture(epm_Texture *tex) {
    tex->name[0] = '\0';
    zgl_DestroyMipMap(tex->mip);
    tex->pixarr = NULL;
    tex->mip = NULL;
    tex->w = 0;
    tex->h = 0;

    return EPM_SUCCESS;
}

epm_Result texture_index_from_name(char const *name, size_t *out_i_tex) {
    for (size_t i_tex = 0; i_tex < g_num_textures; i_tex++) {
        if (0 == strcmp(name, textures[i_tex].name)) {
            *out_i_tex = i_tex;
            return EPM_SUCCESS;
        }
    }

    return EPM_FAILURE;
}

static char g_nametmp0[MAX_TEXTURE_NAME_LEN + 1] = {'\0'};
static void texture_name_from_filename(char const *filename, char *tex_name) {
    bool reading = false;
    size_t i = 0;

    size_t len = strlen(filename);
    dibassert(len < INT_MAX);
    
    for (int i_ch = (int)len - 1; i_ch >= 0; i_ch--) {
        if (( ! reading) && (filename[i_ch] == '.')) {
            reading = true;
            continue;
        }
        if (reading && (filename[i_ch] == '/')) {
            break;
        }
        if (reading) {
            g_nametmp0[i] = filename[i_ch];
            i++;
        }
    }

    len = strlen(g_nametmp0);
    dibassert(len < INT_MAX);
    
    for (int i_ch0 = (int)len - 1, i_ch1 = 0; i_ch0 >= 0; i_ch0--, i_ch1++) {
        tex_name[i_ch1] = g_nametmp0[i_ch0];
    }

    memset(g_nametmp0, 0, MAX_TEXTURE_NAME_LEN+1);
}

epm_Result get_texture_by_name(char const *name, size_t *out_i_tex) {
    for (size_t i_tex = 0; i_tex < g_num_textures; i_tex++) {
        if (0 == strcmp(name, textures[i_tex].name)) {
            *out_i_tex = i_tex;
            return EPM_SUCCESS;
        }
    }

    // texture not found in memory; try to load it

    char filename[MAX_TEXTURE_NAME_LEN + 1] = {'\0'};
    snprintf(filename, MAX_TEXTURE_NAME_LEN + 1, "../assets/textures/%s.bmp", name);
    
    if (NULL == load_Texture(filename)) {
        return EPM_FAILURE;
    }

    return EPM_SUCCESS;
}


void scale_texels_to_world(Fix32Vec V0, Fix32Vec V1, Fix32Vec V2, Fix32Vec_2D *TV0, Fix32Vec_2D *TV1, Fix32Vec_2D *TV2, epm_Texture *tex) {
    (void)tex;
    
    Fix32Vec dir10 = {{
            x_of(V0) - x_of(V1),
            y_of(V0) - y_of(V1),
            z_of(V0) - z_of(V1)}};
    Fix32Vec dir12 = {{
            x_of(V2) - x_of(V1),
            y_of(V2) - y_of(V1),
            z_of(V2) - z_of(V1)}};
    
    UFix32 norm10 = norm_Euclidean(dir10);
    UFix32 norm12 = norm_Euclidean(dir12);

    Fix32Vec_2D Tdir10 = {TV0->x - TV1->x,
                            TV0->y - TV1->y};
    Fix32Vec_2D Tdir12 = {TV2->x - TV1->x,
                            TV2->y - TV1->y};

    UFix32 Tnorm10 = norm2D_Euclidean(Tdir10);
    UFix32 Tnorm12 = norm2D_Euclidean(Tdir12);

    //printf("norm10: %s\n", fmt_ufix_d(norm10, 16, 4));
    //printf("norm12: %s\n", fmt_ufix_d(norm12, 16, 4));
    //printf("Tnorm10: %s\n", fmt_ufix_d(Tnorm10, 16, 4));
    //printf("Tnorm02: %s\n", fmt_ufix_d(Tnorm12, 16, 4));
    
    // map one world unit to one texture pixit
    TV0->x = (Fix32)(TV1->x + FIX_MULDIV(norm10, Tdir10.x, Tnorm10));
    TV0->y = (Fix32)(TV1->y + FIX_MULDIV(norm10, Tdir10.y, Tnorm10));

    TV2->x = (Fix32)(TV1->x + FIX_MULDIV(norm12, Tdir12.x, Tnorm12));
    TV2->y = (Fix32)(TV1->y + FIX_MULDIV(norm12, Tdir12.y, Tnorm12));

    // ensure u,v coordinates are non-negative
    while (TV0->x < 0 || TV1->x < 0 || TV2->x < 0) {
        TV0->x += (Fix32)((tex->w)<<16);
        TV1->x += (Fix32)((tex->w)<<16);
        TV2->x += (Fix32)((tex->w)<<16);
    }
    while (TV0->y < 0 || TV1->y < 0 || TV2->y < 0) {
        TV0->y += (Fix32)((tex->h)<<16);
        TV1->y += (Fix32)((tex->h)<<16);
        TV2->y += (Fix32)((tex->h)<<16);
    }
    
    //printf("TV0: (%s, %s)\n", fmt_fix_d(TV0->x, 16, 4), fmt_fix_d(TV0->y, 16, 4));
    //printf("TV1: (%s, %s)\n", fmt_fix_d(TV1->x, 16, 4), fmt_fix_d(TV1->y, 16, 4));
    //printf("TV2: (%s, %s)\n", fmt_fix_d(TV2->x, 16, 4), fmt_fix_d(TV2->y, 16, 4));
    //putchar('\n');
}


void scale_quad_texels_to_world(Fix32Vec V0, Fix32Vec V1, Fix32Vec V2, Fix32Vec V3, Fix32Vec_2D *TV0, Fix32Vec_2D *TV1, Fix32Vec_2D *TV2, Fix32Vec_2D *TV3, epm_Texture *tex) {
    (void)V1;
    //printf("TV0: (%s, %s)\n", fmt_fix_d(TV0->x, 16, 4), fmt_fix_d(TV0->y, 16, 4));
    //printf("TV1: (%s, %s)\n", fmt_fix_d(TV1->x, 16, 4), fmt_fix_d(TV1->y, 16, 4));
    //printf("TV2: (%s, %s)\n", fmt_fix_d(TV2->x, 16, 4), fmt_fix_d(TV2->y, 16, 4));
    //printf("TV3: (%s, %s)\n", fmt_fix_d(TV3->x, 16, 4), fmt_fix_d(TV3->y, 16, 4));
    
    Fix32Vec dir30 = {{
            x_of(V0) - x_of(V3),
            y_of(V0) - y_of(V3),
            z_of(V0) - z_of(V3)}};
    Fix32Vec dir32 = {{
            x_of(V2) - x_of(V3),
            y_of(V2) - y_of(V3),
            z_of(V2) - z_of(V3)}};
        
    UFix32 norm30 = norm_Euclidean(dir30);
    UFix32 norm32 = norm_Euclidean(dir32);
    
    Fix32Vec_2D Tdir30 = {TV0->x - TV3->x,
                            TV0->y - TV3->y};
    Fix32Vec_2D Tdir32 = {TV2->x - TV3->x,
                            TV2->y - TV3->y};

    UFix32 Tnorm30 = norm2D_Euclidean(Tdir30);
    UFix32 Tnorm32 = norm2D_Euclidean(Tdir32);
    
    //printf("norm30: %s\n",  fmt_ufix_d(norm30,  16, 4));
    //printf("norm32: %s\n",  fmt_ufix_d(norm32,  16, 4));
    //printf("Tnorm30: %s\n", fmt_ufix_d(Tnorm30, 16, 4));
    //printf("Tnorm32: %s\n", fmt_ufix_d(Tnorm32, 16, 4));
    
    // map one world unit to one texture pixit
    TV0->x = (Fix32)(TV3->x + FIX_MULDIV(norm30, Tdir30.x, Tnorm30));
    TV0->y = (Fix32)(TV3->y + FIX_MULDIV(norm30, Tdir30.y, Tnorm30));

    TV2->x = (Fix32)(TV3->x + FIX_MULDIV(norm32, Tdir32.x, Tnorm32));
    TV2->y = (Fix32)(TV3->y + FIX_MULDIV(norm32, Tdir32.y, Tnorm32));

    TV1->x = (Fix32)(TV3->x + (TV0->x - TV3->x) + (TV2->x - TV3->x));
    TV1->y = (Fix32)(TV3->y + (TV0->y - TV3->y) + (TV2->y - TV3->y));
    
    // ensure u,v coordinates are non-negative
    while (TV0->x < 0 || TV1->x < 0 || TV2->x < 0 || TV3->x < 0) {
        TV0->x += (Fix32)((tex->w)<<16);
        TV1->x += (Fix32)((tex->w)<<16);
        TV2->x += (Fix32)((tex->w)<<16);
        TV3->x += (Fix32)((tex->w)<<16);
    }
    while (TV0->y < 0 || TV1->y < 0 || TV2->y < 0 || TV3->y < 0) {
        TV0->y += (Fix32)((tex->h)<<16);
        TV1->y += (Fix32)((tex->h)<<16);
        TV2->y += (Fix32)((tex->h)<<16);
        TV3->y += (Fix32)((tex->h)<<16);
    }
    
    //printf("TV0: (%s, %s)\n", fmt_fix_d(TV0->x, 16, 4), fmt_fix_d(TV0->y, 16, 4));
    //printf("TV1: (%s, %s)\n", fmt_fix_d(TV1->x, 16, 4), fmt_fix_d(TV1->y, 16, 4));
    //printf("TV2: (%s, %s)\n", fmt_fix_d(TV2->x, 16, 4), fmt_fix_d(TV2->y, 16, 4));
    //printf("TV3: (%s, %s)\n", fmt_fix_d(TV3->x, 16, 4), fmt_fix_d(TV3->y, 16, 4));
    //putchar('\n');
}







////////////////////////////////////////////////////////////////////////////////

/*
MipMap *mipmap;

MipMap *mipify(zgl_PixelArray *pixarr) {
    MipMap *mip = zgl_Malloc(sizeof(*mip));
    mip->pixarr = zgl_CreatePixelArray(pixarr->w, (3*pixarr->h)/2);
    zgl_BlitEntire(mip->pixarr, 0, 0, pixarr);
    mip->root_w = mip->pixarr->w;
    mip->level_ofs[0] = 0;
    mip->num_levels = 1;
    
    zgl_Color *dst = mip->pixarr->pixels;
    zgl_Color const *src = mip->pixarr->pixels;
    zgl_Color color;
    zgl_Color r, g, b;
    zgl_Pixit const W = mip->pixarr->w;

    int prev_size = mip->pixarr->w;
    int curr_size = prev_size / 2;
    int curr_level = 1;
    dst += prev_size * prev_size;
    mip->level_ofs[curr_level] = mip->level_ofs[curr_level-1] + prev_size*prev_size;
    while (curr_size >= 1) {
        mip->num_levels++;
        for (int y = 0; y < curr_size; y++) {
            for (int x = 0; x < curr_size; x++) {
                r = g = b = 0;

                int box_x0 = x<<curr_level;
                int box_y0 = y<<curr_level;
                int box_w = 1<<curr_level;
                int box_h = 1<<curr_level;
                for (int box_dy = 0; box_dy < box_h; box_dy++) {
                    for (int box_dx = 0; box_dx < box_w; box_dx++) {
                        int src_color = src[(box_x0 + box_dx) +
                                            (box_y0 + box_dy)*W];
                        r += r_of(src_color);
                        g += g_of(src_color);
                        b += b_of(src_color);
                    }
                }
                r /= box_w*box_h;
                g /= box_w*box_h;
                b /= box_w*box_h;
                                
                color = (r << 16) | (g << 8) | (b << 0);
                dst[x + y*W] = color;
            }        
        }
        prev_size = curr_size;
        curr_size = curr_size/2;
        dst += prev_size;
        curr_level++;
        mip->level_ofs[curr_level] = mip->level_ofs[curr_level-1] + prev_size;
    }

    printf("Root width: %zu\n", mip->root_w);
    printf("Num levels: %zu\n", mip->num_levels);
    for (size_t i = 0; i < mip->num_levels; i++) {
        printf("Level %zu: %zu\n", i, mip->level_ofs[i]);
    }

    return mip;
}


MipMap *mipify2(zgl_PixelArray *pixarr) {
    // TODO: forbid non-powers-of-two

    zgl_Color *src;
    zgl_Color *dst;
    zgl_Pixit src_w;
    zgl_Pixit dst_w;
    int level;
    
    MipMap *mip = zgl_Malloc(sizeof(*mip));
    mip->root_w = pixarr->w;
    size_t mipsize = (4*(mip->root_w)*(mip->root_w) - 1)/3;
    mip->pixels = zgl_Calloc(mipsize, sizeof(zgl_Color));

    src = pixarr->pixels;
    dst = mip->pixels;
    src_w = (zgl_Pixit)pixarr->w;
    dst_w = (zgl_Pixit)mip->root_w;

    mip->num_levels = 0;
    level = 0;
    mip->level_ofs[0] = 0;
    while (dst_w >= 1) {
        mip->num_levels++;
        for (int dst_y = 0; dst_y < dst_w; dst_y++) {
            for (int dst_x = 0; dst_x < dst_w; dst_x++) {
                zgl_Color r, g, b;
                r = g = b = 0;

                int box_x0 = dst_x<<level;
                int box_y0 = dst_y<<level;
                int box_w = 1<<level;
                int box_h = 1<<level;
                for (int box_dy = 0; box_dy < box_h; box_dy++) {
                    for (int box_dx = 0; box_dx < box_w; box_dx++) {
                        int src_color = src[(box_x0 + box_dx) +
                                            (box_y0 + box_dy)*src_w];
                        r += r_of(src_color);
                        g += g_of(src_color);
                        b += b_of(src_color);
                    }
                }
                r /= box_w*box_h;
                g /= box_w*box_h;
                b /= box_w*box_h;
                                
                dst[dst_x + dst_y*dst_w] = (r << 16) | (g << 8) | (b << 0);
            }        
        }

        level++;
        mip->level_ofs[level] = mip->level_ofs[level-1] + dst_w*dst_w;
        dst += dst_w*dst_w;
        dst_w /= 2;
    }

    printf("Root width: %zu\n", mip->root_w);
    printf("Num levels: %zu\n", mip->num_levels);
    for (size_t i = 0; i < mip->num_levels; i++) {
        printf("Level %zu: %zu\n", i, mip->level_ofs[i]);
    }

    return mip;
}
*/
