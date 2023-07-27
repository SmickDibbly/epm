#include "src/misc/epm_includes.h"
#include "src/system/dir.h"
#include "src/world/world.h"
#include "src/draw/textures.h"
#include "src/world/brush.h"

//#define VERBOSITY
#include "zigil/diblib_local/verbosity.h"

#undef LOG_LABEL
#define LOG_LABEL "WORLD"

#define MAX_LINE_LEN 512
#define MAX_TOKENS_PER_LINE 64
static size_t g_num_tokens;
static char *g_tokens[MAX_TOKENS_PER_LINE];
static char g_buf[MAX_LINE_LEN];

/*
static size_t g_num_used_i_tex = 0;
static size_t g_used_i_tex[256] = {0};
*/
static size_t g_num_texnames = 0;
static char g_texnames[64][MAX_TEXTURE_NAME_LEN] = {'\0'};

static size_t g_num_worldtex;
static size_t g_num_filetex;
static size_t *g_i_worldtex_to_filetex;

static Fix32 read_fix_x(char const *str, Fix32 *out_num);

// top level words:

// texture
// begin

// post-"begin" words:
// boxbrush
// end

static char *read_line(char *restrict str, int count, FILE *restrict stream) {
    char *res = fgets(g_buf, MAX_LINE_LEN, stream);
    //printf("%s", g_buf);
    return res;
}

static void tokenize_buf(char *buf, char *tokenstr, char **tokens, size_t *out_num_tokens) {
    buf[strcspn(g_buf, "\n")] = '\0';
    size_t num_tokens = 0;
    
    char *token;
    token = strtok(buf, tokenstr);
    while (token != NULL) {
        tokens[num_tokens] = token;
        (num_tokens)++;

        token = strtok(NULL, tokenstr);
    }

    *out_num_tokens = num_tokens;

    /*
    for (size_t i_tmp = 0; i_tmp < g_num_tokens; i_tmp++) {
        puts(g_tokens[i_tmp]);
    }
    */
}


#include <inttypes.h>
static void write_BrushMesh(Brush *brush, FILE *out_fp) {
    for (size_t i_v = 0; i_v < brush->num_verts; i_v++) {
        fprintf(out_fp, "    v %s %s %s\n",
                fmt_fix_x(x_of(brush->verts[i_v]), 16),
                fmt_fix_x(y_of(brush->verts[i_v]), 16),
                fmt_fix_x(z_of(brush->verts[i_v]), 16));
    }
    
    for (size_t i = 0; i < brush->num_polys; i++) {
        Poly *poly = brush->polys + i;
        BrushPolyExt *polyX = brush->poly_exts + i;
        fprintf(out_fp, "    begin poly\n");
        fprintf(out_fp, "      polyX_flags %"PRIu8"\n", polyX->brushflags);
        fprintf(out_fp, "      poly_flags %"PRIu8"\n", poly->flags);
        fprintf(out_fp, "      normal %s %s %s\n",
                fmt_fix_x(x_of(poly->normal), 16),
                fmt_fix_x(y_of(poly->normal), 16),
                fmt_fix_x(z_of(poly->normal), 16));
        fprintf(out_fp, "      i_tex %zu\n", g_i_worldtex_to_filetex[poly->i_tex] - 1);
        fprintf(out_fp, "      fbri %"PRIu8"\n", poly->fbri);
        
        fprintf(out_fp, "      num_v %zu\n", poly->num_v);
        fprintf(out_fp, "      vind");
        for (size_t j = 0; j < poly->num_v; j++) {
            fprintf(out_fp, " %zu", poly->vind[j]);
        }
        fprintf(out_fp, "\n");
        fprintf(out_fp, "      vtxl");
        for (size_t j = 0; j < poly->num_v; j++) {
            fprintf(out_fp, " %s %s",
                    fmt_fix_x(poly->vtxl[j].x, 16),
                    fmt_fix_x(poly->vtxl[j].y, 16));
        }
        fprintf(out_fp, "\n");
        fprintf(out_fp, "      vbri");
        for (size_t j = 0; j < poly->num_v; j++) {
            fprintf(out_fp, " %"PRIu8, poly->vbri[j]);
        }
        fprintf(out_fp, "\n");
        fprintf(out_fp, "    end poly\n");
    }
}

static void write_Brush(Brush *brush, FILE *out_fp) {
    fputs("begin Brush\n", out_fp);
    fprintf(out_fp, "  flags %x\n", brush->flags);
    fprintf(out_fp, "  BSP default\n");
    fprintf(out_fp, "  CSG %s\n", brush->CSG == CSG_SUBTRACTIVE ? "sub" : "add");
    fprintf(out_fp, "  POR %s %s %s\n",
            fmt_fix_x(x_of(brush->POR), 16),
            fmt_fix_x(y_of(brush->POR), 16),
            fmt_fix_x(z_of(brush->POR), 16));
    fprintf(out_fp, "  begin geo\n");
    write_BrushMesh(brush, out_fp);
    fprintf(out_fp, "  end geo\n");
    fprintf(out_fp, "end Brush\n\n");
}

static void write_FrameBrush(Brush *brush, FILE *out_fp) {
    /* Essentially same as a brush, but distinguished as being the frame. */
    /* Only one of these blocks can appear in a single world file. */
    
    fputs("begin FrameBrush\n", out_fp);
    fprintf(out_fp, "  flags %x\n", brush->flags);
    fprintf(out_fp, "  POR %s %s %s\n",
            fmt_fix_x(x_of(brush->POR), 16),
            fmt_fix_x(y_of(brush->POR), 16),
            fmt_fix_x(z_of(brush->POR), 16));
    fprintf(out_fp, "  begin geo\n");
    write_BrushMesh(brush, out_fp);
    fprintf(out_fp, "  end geo\n");
    fprintf(out_fp, "end Brush\n\n");
}


static void read_BrushPoly(Poly *poly, BrushPolyExt *polyX, FILE *in_fp) {
    size_t num_v = 0;
    while (read_line(g_buf, MAX_LINE_LEN, in_fp)) {
        tokenize_buf(g_buf, " ", g_tokens, &g_num_tokens);
        if (g_num_tokens < 1) continue;
        
        if (streq(g_tokens[0], "polyX_flags")) {
            dibassert(g_num_tokens == 2);
            polyX->brushflags = (uint8_t)atoi(g_tokens[1]);
        }
        else if (streq(g_tokens[0], "poly_flags")) {
            dibassert(g_num_tokens == 2);
            poly->flags = (uint8_t)atoi(g_tokens[1]);
        }
        else if (streq(g_tokens[0], "normal")) {
            dibassert(g_num_tokens == 4);
            read_fix_x(g_tokens[1], &x_of(poly->normal));
            read_fix_x(g_tokens[2], &y_of(poly->normal));
            read_fix_x(g_tokens[3], &z_of(poly->normal));
        }
        else if (streq(g_tokens[0], "i_tex")) {
            dibassert(g_num_tokens == 2);
            poly->i_tex = atoi(g_tokens[1]);
        }
        else if (streq(g_tokens[0], "fbri")) {
            dibassert(g_num_tokens == 2);
            
            poly->fbri = (uint8_t)atoi(g_tokens[1]);
        }
        else if (streq(g_tokens[0], "num_v")) {
            dibassert(g_num_tokens == 2);
            
            num_v = atoi(g_tokens[1]);
            dibassert(num_v != 0);
            poly->num_v = num_v;
            poly->vind = zgl_Malloc(num_v*sizeof(*poly->vind));
            poly->vtxl = zgl_Malloc(num_v*sizeof(*poly->vtxl));
            poly->vbri = zgl_Malloc(num_v*sizeof(*poly->vbri));
        }
        else if (streq(g_tokens[0], "vind")) {
            dibassert(num_v != 0);
            dibassert(g_num_tokens == num_v + 1);
            
            for (size_t i = 0; i < num_v; i++) {
                poly->vind[i] = atoi(g_tokens[i+1]);
            }
        }
        else if (streq(g_tokens[0], "vtxl")) {
            dibassert(num_v != 0);
            dibassert(g_num_tokens == 2*num_v + 1);
            
            for (size_t i = 0; i < num_v; i++) {
                read_fix_x(g_tokens[(2*i) + 1], &poly->vtxl[i].x);
                read_fix_x(g_tokens[(2*i + 1) + 1], &poly->vtxl[i].y);
            }
        }
        else if (streq(g_tokens[0], "vbri")) {
            dibassert(num_v != 0);
            dibassert(g_num_tokens == num_v + 1);
            
            for (size_t i = 0; i < num_v; i++) {
                poly->vbri[i] = (uint8_t)atoi(g_tokens[i+1]);
            }
        }
        else if (streq(g_tokens[0], "end")) {
            dibassert(streq(g_tokens[1], "poly"));
            break;
        }
    }
}

static void read_BrushMesh(Brush *brush, FILE *in_fp) {
    size_t num_v = 0;
    size_t num_p = 0;

    WorldVec     *tmp_v  = zgl_Malloc(256*sizeof(*tmp_v ));
    Poly         *tmp_p  = zgl_Malloc(256*sizeof(*tmp_p ));
    BrushPolyExt *tmp_pX = zgl_Malloc(256*sizeof(*tmp_pX));
    
    size_t i_curr_v = 0;
    size_t i_curr_p = 0;
    while (read_line(g_buf, MAX_LINE_LEN, in_fp)) {
        tokenize_buf(g_buf, " ", g_tokens, &g_num_tokens);
        
        if (g_num_tokens < 1) continue;
        
        if (streq(g_tokens[0], "v")) {
            read_fix_x(g_tokens[1], &x_of(tmp_v[i_curr_v]));
            read_fix_x(g_tokens[2], &y_of(tmp_v[i_curr_v]));
            read_fix_x(g_tokens[3], &z_of(tmp_v[i_curr_v]));
            i_curr_v++;
            num_v++;
        }
        else if (streq(g_tokens[0], "begin")) {
            if (streq(g_tokens[1], "poly")) {
                Poly *poly = tmp_p + i_curr_p;
                BrushPolyExt *polyX = tmp_pX + i_curr_p;
                read_BrushPoly(poly, polyX, in_fp);
                i_curr_p++;
                num_p++;
            }
            else {
                dibassert(false);
            }
        }
        else if (streq(g_tokens[0], "end")) {
            dibassert(streq(g_tokens[1], "geo"));
            break;
        }
    }

    brush->num_verts = num_v;
    brush->verts = zgl_Malloc(num_v*sizeof(*brush->verts));
    brush->vert_exts = zgl_Calloc(num_v, sizeof(*brush->vert_exts));
    for (size_t i_v = 0; i_v < num_v; i_v++) {
        brush->verts[i_v] = tmp_v[i_v];
    }
    
    brush->num_polys = num_p;
    brush->polys = zgl_Malloc(num_p*sizeof(*brush->polys));
    brush->poly_exts = zgl_Calloc(num_p, sizeof(*brush->poly_exts));
    for (size_t i_p = 0; i_p < num_p; i_p++) {
        brush->polys[i_p]     = tmp_p[i_p];
        brush->poly_exts[i_p] = tmp_pX[i_p];
    }
    
    // NOTE: Do not free the tmp_p->vxxx arrays since the pointers to that data
    // is passed along to the actual Brush.
    zgl_Free(tmp_v);
    zgl_Free(tmp_p);
    zgl_Free(tmp_pX);
}

static void read_Brush(FILE *in_fp) {
    Brush *brush = zgl_Malloc(sizeof(*brush));
    
    while (read_line(g_buf, MAX_LINE_LEN, in_fp)) {
        tokenize_buf(g_buf, " ", g_tokens, &g_num_tokens);
        
        if (streq(g_tokens[0], "flags")) {
            brush->flags = (uint8_t)atoi(g_tokens[1]);
        }
        else if (streq(g_tokens[0], "BSP")) {
            brush->BSP = BSP_DEFAULT;
        }
        else if (streq(g_tokens[0], "CSG")) {
            if (streq(g_tokens[1], "sub")) {
                brush->CSG = CSG_SUBTRACTIVE;
            }
            else if (streq(g_tokens[1], "add")) {
                brush->CSG = CSG_ADDITIVE;
            }
        }
        else if (streq(g_tokens[0], "POR")) {
            read_fix_x(g_tokens[1], &x_of(brush->POR));
            read_fix_x(g_tokens[2], &y_of(brush->POR));
            read_fix_x(g_tokens[3], &z_of(brush->POR));
        }
        else if (streq(g_tokens[0], "begin")) {
            if (streq(g_tokens[1], "geo")) {
                read_BrushMesh(brush, in_fp);
            }
            else {
                dibassert(false);
            }
        }
        else if (streq(g_tokens[0], "end")) { // end brush
            break;
        }
    }

    link_brush(brush);

    epm_ComputeEdgesFromPolys(brush->num_polys, brush->polys,
                              &brush->num_edges, &brush->edges);
    //#ifdef VERBOSITY
    print_Brush(brush); // TODO: VERBOSIFY
    //#endif
    
    return;
}

static void read_FrameBrush(FILE *in_fp) {
    Brush *brush = zgl_Malloc(sizeof(*brush));
    
    while (read_line(g_buf, MAX_LINE_LEN, in_fp)) {
        tokenize_buf(g_buf, " ", g_tokens, &g_num_tokens);
        
        if (streq(g_tokens[0], "flags")) {
            brush->flags = (uint8_t)atoi(g_tokens[1]);
        }
        else if (streq(g_tokens[0], "POR")) {
            read_fix_x(g_tokens[1], &x_of(brush->POR));
            read_fix_x(g_tokens[2], &y_of(brush->POR));
            read_fix_x(g_tokens[3], &z_of(brush->POR));
        }
        else if (streq(g_tokens[0], "begin")) {
            if (streq(g_tokens[1], "geo")) {
                read_BrushMesh(brush, in_fp);
            }
            else {
                dibassert(false);
            }
        }
        else if (streq(g_tokens[0], "end")) { // end brush
            break;
        }
    }

    brush->BSP = BSP_DEFAULT;
    brush->CSG = CSG_SUBTRACTIVE;
    
    epm_ComputeEdgesFromPolys( brush->num_polys,  brush->polys,
                              &brush->num_edges, &brush->edges);
    
    //#ifdef VERBOSITY
    print_Brush(brush); // TODO: VERBOSIFY
    //#endif
    
    destroy_brush(g_frame);
    g_frame = NULL;
    g_frame = brush;
    
    return;
}


static void read_texnames(FILE *in_fp) {
    while (read_line(g_buf, MAX_LINE_LEN, in_fp)) {
        tokenize_buf(g_buf, " ", g_tokens, &g_num_tokens);
        if (streq(g_tokens[0], "texname")) {
            strcpy(g_texnames[g_num_texnames++], g_tokens[1]);            
        }
        else if (streq(g_tokens[0], "end")) {
            break;
        }
    }

    return;
}



static void write_EditorCamera(EditorCamera *cam, FILE *out_fp) {
    fprintf(out_fp, "begin EditorCamera\n");
    fprintf(out_fp, "pos %s %s %s\n",
            fmt_fix_x(x_of(cam->pos), 16),
            fmt_fix_x(y_of(cam->pos), 16),
            fmt_fix_x(z_of(cam->pos), 16));

    fprintf(out_fp, "angh %x\n", cam->view_angle_h);
    fprintf(out_fp, "angv %x\n", cam->view_angle_v);
    fprintf(out_fp, "dir %s %s %s\n",
            fmt_fix_x(x_of(cam->view_vec), 16),
            fmt_fix_x(y_of(cam->view_vec), 16),
            fmt_fix_x(z_of(cam->view_vec), 16));
    fprintf(out_fp, "dirxy %s %s\n",
            fmt_fix_x(cam->view_vec_XY.x, 16),
            fmt_fix_x(cam->view_vec_XY.y, 16));
    fprintf(out_fp, "end EditorCamera\n\n");
}

static void read_EditorCamera(FILE *in_fp) {
    extern EditorCamera const default_cam;
    cam = default_cam;
    
    while (read_line(g_buf, MAX_LINE_LEN, in_fp)) {        
        tokenize_buf(g_buf, " ", g_tokens, &g_num_tokens);

        if (g_num_tokens < 1) continue;
        
        if (streq(g_tokens[0], "pos")) {
            read_fix_x(g_tokens[1], &x_of(cam.pos));
            read_fix_x(g_tokens[2], &y_of(cam.pos));
            read_fix_x(g_tokens[3], &z_of(cam.pos));
        }
        else if (streq(g_tokens[0], "dir")) {
            read_fix_x(g_tokens[1], &x_of(cam.view_vec));
            read_fix_x(g_tokens[2], &y_of(cam.view_vec));
            read_fix_x(g_tokens[3], &z_of(cam.view_vec));
        }
        else if (streq(g_tokens[0], "dirxy")) {
            read_fix_x(g_tokens[1], &cam.view_vec_XY.x);
            read_fix_x(g_tokens[2], &cam.view_vec_XY.y);
        }
        else if (streq(g_tokens[0], "angh")) {
            cam.view_angle_h = (uint32_t)strtoul(g_tokens[1], NULL, 16);
        }
        else if (streq(g_tokens[0], "angv")) {
            cam.view_angle_v = (uint32_t)strtoul(g_tokens[1], NULL, 16);
        }
        else if (streq(g_tokens[0], "end")) {
            break;
        }
    }
    
    //onTic_cam(&cam);
}

epm_Result epm_ReadWorldFile(epm_World *world, char const *filename) {
    epm_UnloadWorld();
    
    g_num_texnames = 0;
    if ( ! world) {
        epm_Log(LT_WARN, "NULL pointer to epm_World struct passed to epm_ReadWorldFile()");
        return EPM_FAILURE;
    }
    
    char path[256] = {'\0'};
    strcat(path, DIR_WORLD);
    strcat(path, filename);
    strcat(path, SUF_WORLD);

    epm_Log(LT_INFO, "Reading world from file: %s", path);
    
    FILE *in_fp = fopen(path, "rb");
    if ( ! in_fp) {
        epm_Log(LT_WARN, "File not found: %s", path);
        return EPM_FAILURE;
    }

    while (read_line(g_buf, MAX_LINE_LEN, in_fp)) {
        tokenize_buf(g_buf, " ", g_tokens, &g_num_tokens);
        
        if (g_num_tokens < 1) continue;
        
        // proceed depending on first token. TODO: Hash table if get big
        if (streq(g_tokens[0], "begin")) {
            if (g_num_tokens < 2) goto Failure;
            
            if (streq(g_tokens[1], "Brush")) {
                read_Brush(in_fp);
            }
            else if (streq(g_tokens[1], "FrameBrush")) {
                read_FrameBrush(in_fp);
            }
            else if (streq(g_tokens[1], "texnames")) {
                read_texnames(in_fp);
            }
            else if (streq(g_tokens[1], "EditorCamera")) {
                read_EditorCamera(in_fp);
            }
        }
        else {
            goto Failure;
        }
    }
    
    fclose(in_fp);

    /* Translate file-local texture indices to global. */
    for (BrushNode *node = world->geo_brush->head; node; node = node->next) {
        Brush *brush = node->brush; // TEMP: Only BoxBrushes
        
        for (size_t i = 0; i < brush->num_polys; i++) {
            size_t i_tex;
            i_tex = 0;
            i_tex = epm_TextureIndexFromName(g_texnames[brush->polys[i].i_tex]);
            brush->polys[i].i_tex = i_tex;
        }
    }


    for (BrushNode *node = world->geo_brush->head; node; node = node->next) {
        print_Brush(node->brush);
    }
    
    g_num_texnames = 0;
    return EPM_SUCCESS;

 Failure:
    epm_Log(LT_WARN, "Could not read world file %s.", path);
    fclose(in_fp);
    
    g_num_texnames = 0;
    return EPM_FAILURE;
}



epm_Result epm_WriteWorldFile(epm_World *world, char const *filename) {
    if ( ! world) {
        epm_Log(LT_WARN, "NULL pointer to epm_World struct passed to epm_WriteWorldFile()");
        return EPM_FAILURE;
    }
    
    char path[256] = {'\0'};
    strcat(path, DIR_WORLD);
    strcat(path, ".out.");
    strcat(path, filename);
    strcat(path, SUF_WORLD);

    FILE *out_fp = fopen(path, "wb");
    if (!out_fp) {
        epm_Log(LT_ERROR, "Could not open file for writing: %s\n", path);
        return EPM_FAILURE;
    }

    // write textures
    g_num_worldtex = g_num_textures;
    g_num_filetex = 0;
    g_i_worldtex_to_filetex = zgl_Calloc(g_num_worldtex,
                                         sizeof(*g_i_worldtex_to_filetex));

    for (BrushNode *node = g_world.geo_brush->head; node; node = node->next) {
        Brush *brush = node->brush;
        for (size_t i_p = 0; i_p < brush->num_polys; i_p++) {
            Poly *poly = brush->polys + i_p;
            size_t i_worldtex = poly->i_tex;

            if (g_i_worldtex_to_filetex[i_worldtex] == 0) { // new index to store
                g_i_worldtex_to_filetex[i_worldtex] = (int32_t)(g_num_filetex + 1);
                g_num_filetex++;
            }
        }
    }
    fprintf(out_fp, "begin texnames\n");
    for (size_t i_worldtex = 0; i_worldtex < g_num_worldtex; i_worldtex++) {
        if (g_i_worldtex_to_filetex[i_worldtex] != 0) {
            char *texname = textures[i_worldtex].name;
            fprintf(out_fp, "  texname %s\n", texname);
        }
    }
    fprintf(out_fp, "end texnames\n\n");

    // write brushes
    for (BrushNode *node = g_world.geo_brush->head; node; node = node->next) {
        write_Brush(node->brush, out_fp);
    }
    
    // write editor camera
    write_EditorCamera(&cam, out_fp);

    write_FrameBrush(g_frame, out_fp);
    
    fclose(out_fp);
    return EPM_SUCCESS;
}

#include "src/input/command.h"
static void CMDH_readworld(int argc, char **argv, char *output_str) {
    epm_ReadWorldFile(&g_world, argv[1]);
}

epm_Command const CMD_readworld = {
    .name = "readworld",
    .argc_min = 2,
    .argc_max = 2,
    .handler = CMDH_readworld,
};

static void CMDH_writeworld(int argc, char **argv, char *output_str) {
    epm_WriteWorldFile(&g_world, argv[1]);
}

epm_Command const CMD_writeworld = {
    .name = "writeworld",
    .argc_min = 2,
    .argc_max = 2,
    .handler = CMDH_writeworld,
};

static Fix32 read_fix_x(char const *str, Fix32 *out_num) {
    Fix32 res = 0;
    bool neg = false;
    size_t i = 0;
    
    if (str[i] == '-') {
        neg = true;
        i++;
    }
    
    while (true) {
        switch (str[i]) {
        case '.':
            break;
        case '0':
            res <<= 4;
            res += 0x0;
            break;
        case '1':
            res <<= 4;
            res += 0x1;
            break;
        case '2':
            res <<= 4;
            res += 0x2;
            break;
        case '3':
            res <<= 4;
            res += 0x3;
            break;
        case '4':
            res <<= 4;
            res += 0x4;
            break;
        case '5':
            res <<= 4;
            res += 0x5;
            break;
        case '6':
            res <<= 4;
            res += 0x6;
            break;
        case '7':
            res <<= 4;
            res += 0x7;
            break;
        case '8':
            res <<= 4;
            res += 0x8;
            break;
        case '9':
            res <<= 4;
            res += 0x9;
            break;
        case 'A':
            res <<= 4;
            res += 0xA;
            break;
        case 'B':
            res <<= 4;
            res += 0xB;
            break;
        case 'C':
            res <<= 4;
            res += 0xC;
            break;
        case 'D':
            res <<= 4;
            res += 0xD;
            break;
        case 'E':
            res <<= 4;
            res += 0xE;
            break;
        case 'F':
            res <<= 4;
            res += 0xF;
            break;
        default:
            goto Done;
        }
        i++;
    }

 Done:
    if (neg)
        res = -res;

    *out_num = res;

    dibassert(i < INT_MAX);
    return (int)i;
}
