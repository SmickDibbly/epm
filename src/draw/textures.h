#ifndef TEXTURES_H
#define TEXTURES_H

#include "zigil/zigil.h"
#include "zigil/zigil_mip.h"
#include "src/misc/epm_includes.h"

#define MAX_TEXTURE_NAME_LEN 63

// as of 2023-04-01 the epm_Texture struct is just a wrapper around a
// zgl_PixelArray; the only extra data is a NAME field.
typedef struct epm_Texture {
    char name[MAX_TEXTURE_NAME_LEN + 1];
    uint8_t log2_wh;
    uint16_t w, h; // TODO: Check and/or assert in various critical places in
                   // the code that no texture has width larger than some chosen
                   // constant, say 256, 512, or 1024
    zgl_PixelArray *pixarr;
    zgl_MipMap *mip;
} epm_Texture;

#define MAX_TEXTURES 64
extern size_t g_num_textures;
extern epm_Texture textures[MAX_TEXTURES];

/** Load an epm_Texture into the global textures[] array. */
extern epm_Texture *epm_LoadTexture(char const *texname, char const *filename);
extern epm_Result epm_UnloadTexture(epm_Texture *tex);

/** Given the name of a texture, fill in the out_i_tex variable with the index
    to it in the textures[] array. If the texture is not in the array, attempt
    to load it and then return the index. If the texture can not be loaded,
    return EPM_FAILURE */
extern int32_t epm_TextureIndexFromName(char const *name);

// IDEA: Placeholder "missing texture" textures. If, for example, a level needs
// a texture called CrackedConcrete256 but that isn't found, there should be an
// entry added to the texture array that represents this, but is marked as
// "missing" and is rendered as some chosen default instead (null.bmp probably)
//
// Saving a level with a missing texture should be the same as saving it as if
// the texture were there. Thus the missing texture placeholder would need to
// retain the name of the missing texture.




/* Move this shish */

extern void scale_texels_to_world(Fix32Vec V0, Fix32Vec V1, Fix32Vec V2, Fix32Vec_2D *TV0, Fix32Vec_2D *TV1, Fix32Vec_2D *TV2, epm_Texture *tex);

extern void scale_quad_texels_to_world(Fix32Vec V0, Fix32Vec V1, Fix32Vec V2, Fix32Vec V3, Fix32Vec_2D *TV0, Fix32Vec_2D *TV1, Fix32Vec_2D *TV2, Fix32Vec_2D *TV3, epm_Texture *tex);




////////////////////////////////////////////////////////////////////////////////

extern zgl_MipMap *MIP_light_icon;
extern zgl_MipMap *MIP_edcam_icon;

#endif /* TEXTURES_H */
