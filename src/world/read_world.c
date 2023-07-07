#include "src/misc/epm_includes.h"
#include "src/system/dir.h"
#include "src/world/world.h"
#include "src/draw/textures.h"
#include "src/world/brush.h"

//#define VERBOSITY
#include "src/locallibs/verbosity.h"

#undef LOG_LABEL
#define LOG_LABEL "WORLD"

#define MAX_LINE_LEN 512
#define MAX_TOKENS_PER_LINE 64
static size_t num_tokens;
static char *tokens[MAX_TOKENS_PER_LINE];
static char buf[MAX_LINE_LEN];

size_t num_texnames = 0;
char texnames[64][MAX_TEXTURE_NAME_LEN] = {'\0'};

static Fix32 read_fix_x(char const *str, Fix32 *out_num);

// top level words:

// texture
// begin

// post-"begin" words:
// boxbrush
// end

static void readline_texture(void) {
    if (prefix("name=", tokens[1])) {
        char *first_quote = strchr(tokens[1], '"');
        char *second_quote = strchr(first_quote+1, '"');
        *second_quote = '\0';
        strcpy(texnames[num_texnames], first_quote+1);
        num_texnames++;
    }
    else {
        return;
    }
}

static void readline_texture_index(size_t *out_idx) {
    if (prefix("idx=", tokens[1])) {
        *out_idx = atoi(tokens[1]+4);
    }
    else {
        return;
    }
}

static void write_CuboidBrush(CuboidBrush *cuboid, FILE *out_fp) {
    for (size_t i_v = 0; i_v < cuboid->num_vertices; i_v++) {
        fprintf(out_fp, "    v %s %s %s\n",
                fmt_fix_x(x_of(cuboid->vertices[i_v]), 16),
                fmt_fix_x(y_of(cuboid->vertices[i_v]), 16),
                fmt_fix_x(z_of(cuboid->vertices[i_v]), 16));
    }
    
    for (size_t i = 0; i < 6; i++) {
        fprintf(out_fp, "    texture idx=%zu\n", cuboid->quads[i].quad.i_tex);
    }

}

void write_Brush(Brush *brush, FILE *out_fp) {
    fputs("begin Brush\n", out_fp);
    fprintf(out_fp, "  type CuboidBrush\n");
    
    fprintf(out_fp, "  flags %x\n", brush->flags);
    fprintf(out_fp, "  BSP default\n");
    fprintf(out_fp, "  CSG %s\n", brush->CSG == CSG_SUBTRACTIVE ? "sub" : "add");
    fprintf(out_fp, "  POR %s %s %s\n",
            fmt_fix_x(x_of(brush->POR), 16),
            fmt_fix_x(y_of(brush->POR), 16),
            fmt_fix_x(z_of(brush->POR), 16));

    fprintf(out_fp, "  begin geo\n");
    switch (brush->type) {
    case BT_CUBOID:
        write_CuboidBrush(Cuboid_of(brush), out_fp);
        break;
    default:
        dibassert(false);
        break;
    }
    fprintf(out_fp, "  end geo\n");
    fprintf(out_fp, "end Brush\n");
}

static void read_CuboidBrush(CuboidBrush *cuboid, FILE *in_fp) {
    init_CuboidBrush(cuboid, 1, 1, 1);

    size_t i_curr_tex = 0;
    size_t i_curr_v = 0;
    while (fgets(buf, MAX_LINE_LEN, in_fp)) {
        buf[strcspn(buf, "\n")] = '\0';
        num_tokens = 0;
        
        char *token;
        token = strtok(buf, " ");
        while (token != NULL) {
            tokens[num_tokens] = token;
            num_tokens++;

            token = strtok(NULL, " ");
        }

        if (num_tokens < 1) continue;
        
        if (streq(tokens[0], "v")) {
            read_fix_x(tokens[1], &x_of(cuboid->vertices[i_curr_v]));
            read_fix_x(tokens[2], &y_of(cuboid->vertices[i_curr_v]));
            read_fix_x(tokens[3], &z_of(cuboid->vertices[i_curr_v]));
            i_curr_v++;
        }
        else if (streq(tokens[0], "texture")) {
            readline_texture_index(&cuboid->quads[i_curr_tex].quad.i_tex);
            i_curr_tex++;
        }
        else if (streq(tokens[0], "end")) {
            break;
        }
    }


    WorldVec *v = cuboid->vertices;
    for (size_t i_quad = 0; i_quad < cuboid->num_quads; i_quad++) {
        QuadFace *quad = &cuboid->quads[i_quad].quad;
        scale_quad_texels_to_world
            (v[quad->i_v0], v[quad->i_v1],
             v[quad->i_v2], v[quad->i_v3],
             &quad->tv0, &quad->tv1,
             &quad->tv2, &quad->tv3,
             &textures[quad->i_tex]);
    }
}

static void read_Brush(FILE *in_fp) {
    BrushType type;

    char *ignore = fgets(buf, MAX_LINE_LEN, in_fp); // first line must be brush type
    (void)ignore;
    buf[strcspn(buf, "\n")] = '\0';
    num_tokens = 0;
        
    char *token;
    token = strtok(buf, " ");
    while (token != NULL) {
        tokens[num_tokens] = token;
        num_tokens++;

        token = strtok(NULL, " ");
    }

    dibassert(streq(tokens[0], "type"));
    
    if (streq(tokens[1], "CuboidBrush")) {
        type = BT_CUBOID;
    }
    else {
        dibassert(false);
        type = 0;
    }

    Brush *brush;

    switch (type) {
    case BT_CUBOID:
        brush = new_CuboidBrush();
        break;
    default:
        dibassert(false);
    }

    brush->type = type;

    while (fgets(buf, MAX_LINE_LEN, in_fp)) {
        buf[strcspn(buf, "\n")] = '\0';
        num_tokens = 0;
        
        char *token;
        token = strtok(buf, " ");
        while (token != NULL) {
            tokens[num_tokens] = token;
            num_tokens++;

            token = strtok(NULL, " ");
        }

        
        if (streq(tokens[0], "flags")) {
            brush->flags = atoi(tokens[1]);
        }
        else if (streq(tokens[0], "BSP")) {
            brush->BSP = BSP_DEFAULT;
        }
        else if (streq(tokens[0], "CSG")) {
            if (streq(tokens[1], "sub"))
                brush->CSG = CSG_SUBTRACTIVE;
            else if (streq(tokens[1], "add")) {
                brush->CSG = CSG_ADDITIVE;
            }
        }
        else if (streq(tokens[0], "POR")) {
            read_fix_x(tokens[1], &x_of(brush->POR));
            read_fix_x(tokens[2], &y_of(brush->POR));
            read_fix_x(tokens[3], &z_of(brush->POR));
        }
        else if (streq(tokens[0], "begin")) {
            if (streq(tokens[1], "geo")) {
                switch (type) {
                case BT_CUBOID:
                    read_CuboidBrush(Cuboid_of(brush), in_fp);
                    break;
                default:
                    dibassert(false);
                }
            }
            else {
                dibassert(false);
            }
        }
        else if (streq(tokens[0], "end")) { // end brush
            break;
        }
    }

    link_brush(brush);

    /*
    BrushNode *node;
    if (world.brushgeo->head == NULL) {
        dibassert(world.brushgeo->tail == NULL);
        node = world.brushgeo->head = zgl_Malloc(sizeof(BrushNode));
        node->next = node->prev = NULL;
        world.brushgeo->tail = node;
        
    }
    else {
        dibassert(world.brushgeo->tail != NULL);
        node = world.brushgeo->tail->next = zgl_Malloc(sizeof(BrushNode));
        node->next = NULL;
        node->prev = world.brushgeo->tail;
        world.brushgeo->tail = node;
    }

    node->brush = brush;
    
    */

#ifdef VERBOSITY
    write_Brush(brush, stdout); // TODO: VERBOSIFY
#endif
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
    fprintf(out_fp, "end EditorCamera\n");
}

static void read_EditorCamera(FILE *in_fp) {
    extern EditorCamera const default_cam;
    cam = default_cam;
    
    while (fgets(buf, MAX_LINE_LEN, in_fp)) {
        buf[strcspn(buf, "\n")] = '\0';
        num_tokens = 0;
        
        char *token;
        token = strtok(buf, " ");
        while (token != NULL) {
            tokens[num_tokens] = token;
            num_tokens++;

            token = strtok(NULL, " ");
        }

        if (num_tokens < 1) continue;
        
        if (streq(tokens[0], "pos")) {
            read_fix_x(tokens[1], &x_of(cam.pos));
            read_fix_x(tokens[2], &y_of(cam.pos));
            read_fix_x(tokens[3], &z_of(cam.pos));
        }
        else if (streq(tokens[0], "dir")) {
            read_fix_x(tokens[1], &x_of(cam.view_vec));
            read_fix_x(tokens[2], &y_of(cam.view_vec));
            read_fix_x(tokens[3], &z_of(cam.view_vec));
        }
        else if (streq(tokens[0], "dirxy")) {
            read_fix_x(tokens[1], &cam.view_vec_XY.x);
            read_fix_x(tokens[2], &cam.view_vec_XY.y);
        }
        else if (streq(tokens[0], "angh")) {
            cam.view_angle_h = (uint32_t)strtoul(tokens[1], NULL, 16);
        }
        else if (streq(tokens[0], "angv")) {
            cam.view_angle_v = (uint32_t)strtoul(tokens[1], NULL, 16);
        }
        else if (streq(tokens[0], "end")) {
            break;
        }
    }
    
    //onTic_cam(&cam);
}

epm_Result epm_ReadWorldFile(epm_World *world, char const *filename) {
    epm_UnloadWorld();
    
    num_texnames = 0;
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

    while (fgets(buf, MAX_LINE_LEN, in_fp)) {
        buf[strcspn(buf, "\n")] = '\0';
        num_tokens = 0;
        
        char *token;
        token = strtok(buf, " ");
        while (token != NULL) {
            tokens[num_tokens] = token;
            num_tokens++;

            token = strtok(NULL, " ");
        }

        if (num_tokens < 1) continue;
        
        // proceed depending on first token. TODO: Hash table if get big
        if (streq(tokens[0], "texture")) {
            readline_texture();
        }
        else if (streq(tokens[0], "begin")) {
            if (num_tokens < 2) goto Failure;
            
            if (streq(tokens[1], "Brush")) {
                read_Brush(in_fp);
            }
            else if (streq(tokens[1], "EditorCamera")) {
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
        CuboidBrush *cuboid = Cuboid_of(brush);
        
        for (size_t i = 0; i < cuboid->num_quads; i++) {
            size_t i_tex;
            i_tex = epm_TextureIndexFromName(texnames[cuboid->quads[i].quad.i_tex]);
            cuboid->quads[i].quad.i_tex = i_tex;
        }
    }
    world->loaded = true;

    /*
    for (BrushNode *node = world->geo_brush->head; node; node = node->next) {
        write_Brush(node->brush, stdout);    
    }
    */
    
    num_texnames = 0;
    return EPM_SUCCESS;

 Failure:
    epm_Log(LT_WARN, "Could not read world file %s.", path);
    fclose(in_fp);
    
    num_texnames = 0;
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

    // write brushes

    // write editor camera
    write_EditorCamera(&cam, out_fp);
    
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
