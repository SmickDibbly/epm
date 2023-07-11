#include "src/draw/textures.h"
#include "src/system/dir.h"
#include "zigil/zigil_dir.h"

// weird includes
#include "src/world/geometry.h"

#undef LOG_LABEL
#define LOG_LABEL "TEXTURES"

zgl_MipMap *MIP_light_icon;
zgl_MipMap *MIP_edcam_icon;

size_t g_num_textures = 0;
epm_Texture textures[MAX_TEXTURES] = {0};
static zgl_PixelArray texture_pix[MAX_TEXTURES] = {0};

static void add_missing_texture(char const *texname) {
    epm_Texture *tex = &textures[g_num_textures];

    *tex = textures[0];
    snprintf(tex->name, MAX_TEXTURE_NAME_LEN + 1, "%s", texname);
    tex->flags |= TEXFL_NOTFOUND;

    g_num_textures++;
}

epm_Texture *epm_LoadTexture(char const *texname, char const *filename) {
    epm_Texture *tex = &textures[g_num_textures];
        
    zgl_PixelArray *pix = zgl_ReadBMP(filename);
    if ( ! pix) {
        return NULL;
    }

    if (pix->w != pix->h) {
        epm_Log(LT_WARN, "Texture in file \"%s\" has unequal width and height; cannot use.\n", filename);
        return NULL;
    }
    
    if (pix->w == 0 || pix->h == 0) {
        epm_Log(LT_WARN, "Texture in file \"%s\" has 0 width and/or 0 height; cannot use.\n", filename);
        return NULL;
    }

    if ((pix->w & (pix->w - 1)) != 0 ||
        (pix->h & (pix->h - 1)) != 0 ) {
        epm_Log(LT_WARN, "Texture in file \"%s\" has either width or height that is not a power of 2; cannot use.\n", filename);
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

    snprintf(tex->name, MAX_TEXTURE_NAME_LEN + 1, "%s", texname);
    tex->log2_wh = (uint8_t)r;
    tex->w = (uint16_t)pix->w;
    tex->h = (uint16_t)pix->h;

    tex->mip = zgl_MipifyPixelArray(pix);
    tex->pixarr = &texture_pix[g_num_textures];
    tex->pixarr->pixels = tex->mip->pixels;
    tex->pixarr->w = pix->w;
    tex->pixarr->h = pix->h;
    
    zgl_DestroyPixelArray(pix);
    g_num_textures++;
    
    return tex;
}

epm_Result epm_UnloadTexture(epm_Texture *tex) {
    tex->name[0] = '\0';
    zgl_DestroyMipMap(tex->mip);
    tex->pixarr = NULL;
    tex->mip = NULL;
    tex->w = 0;
    tex->h = 0;

    return EPM_SUCCESS;
}


/* As of 2023-07-06, texture names must be the file name without the .bmp */
int32_t epm_TextureIndexFromName(char const *texname) {
    int32_t i_tex;
    
    /* TODO: Hash table? Probably doesn't matter; not enough textures. */
    for (i_tex = 0; i_tex < (int32_t)g_num_textures; i_tex++) {
        if (streq(texname, textures[i_tex].name)) {
            return i_tex;
        }
    }

    // texture not found in memory; try to load it. And note that if found, it's
    // index will be i_tex's current value.

    char path[128] = {'\0'};
    strcpy(path, DIR_TEX);
    
    char filename[MAX_TEXTURE_NAME_LEN + 1] = {'\0'};
    //zgl_DirListing dl;
    //zgl_GetDirListing(&dl, path);
    
    strcpy(filename, path);
    strcat(filename, texname);
    strcat(filename, SUF_BMP);
    
    if (epm_LoadTexture(texname, filename)) {
        return i_tex;
    }

    strcpy(filename, path);
    strcat(filename, "default/");
    strcat(filename, texname);
    strcat(filename, SUF_BMP);

    if (epm_LoadTexture(texname, filename)) {
        return i_tex;
    }

    strcpy(filename, path);
    strcat(filename, "misc/");
    strcat(filename, texname);
    strcat(filename, SUF_BMP);

    if (epm_LoadTexture(texname, filename)) {
        return i_tex;
    }

    epm_Log(LT_WARN, "Texture named \"%s\" not found from file.\n", texname);

    add_missing_texture(texname);
    
    return i_tex;
}

/* This don't belong here. */
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




#include "src/input/command.h"

static void CMDH_loadtex(int argc, char **argv, char *output_str) {
    epm_TextureIndexFromName(argv[1]);
}

epm_Command const CMD_loadtex = {
    .name = "loadtex",
    .argc_min = 2,
    .argc_max = 2,
    .handler = CMDH_loadtex,
};

