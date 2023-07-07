#include <ctype.h>

#include "src/world/mesh.h"
#include "src/world/world.h"
#include "src/draw/draw.h"
#include "src/draw/textures.h"
#include "src/system/dir.h"

//#define VERBOSITY
#include "src/locallibs/verbosity.h"

#undef LOG_LABEL
#define LOG_LABEL "MESH"

WorldVec g_vertices[MAX_MESH_VERTICES];
Edge g_edges[16384];
Face g_faces[MAX_MESH_FACES];

typedef struct DIBJFileOverview {
    size_t num_lines;
    
    size_t ofs_v;
    size_t num_v;
    
    size_t ofs_e;
    size_t num_e;
    
    size_t ofs_f;
    size_t num_f;
    
    size_t ofs_vn;
    size_t num_vn;
    
    size_t ofs_en;
    size_t num_en;
    
    size_t ofs_fn;
    size_t num_fn;
    
    size_t ofs_texname;
    size_t num_texname;
    
    size_t ofs_ft;
    size_t num_ft;

    size_t ofs_ftxl;
    size_t num_ftxl;
} DIBJFileOverview;

extern size_t read_fix_x(char const *str, Fix32 *out_num);
static void read_line(FILE *in_fp, char *str);
static void file_overview(DIBJFileOverview *data, FILE *in_fp);

static size_t num_texnames = 0;
static char texnames[64][MAX_TEXTURE_NAME_LEN] = {'\0'};

void epm_ComputeFaceNormals(WorldVec vertices[], size_t num_faces, Face faces[]) {
    for (size_t i_face = 0; i_face < num_faces; i_face++) {
        Face *face = faces+i_face;
        face->flags = 0;
        
        if (face->i_v[0] == face->i_v[1] ||
            face->i_v[1] == face->i_v[2] ||
            face->i_v[2] == face->i_v[0]) {
            epm_Log(LT_WARN,
                    "Degenerate polygon %zu : (%s, %s, %s) -> (%s, %s, %s) -> (%s, %s, %s).",
                    i_face,
                    fmt_fix_x(x_of(vertices[face->i_v[0]]), 16),
                    fmt_fix_x(y_of(vertices[face->i_v[0]]), 16),
                    fmt_fix_x(z_of(vertices[face->i_v[0]]), 16),
                
                    fmt_fix_x(x_of(vertices[face->i_v[1]]), 16),
                    fmt_fix_x(y_of(vertices[face->i_v[1]]), 16),
                    fmt_fix_x(z_of(vertices[face->i_v[1]]), 16),

                    fmt_fix_x(x_of(vertices[face->i_v[2]]), 16),
                    fmt_fix_x(y_of(vertices[face->i_v[2]]), 16),
                    fmt_fix_x(z_of(vertices[face->i_v[2]]), 16));

            face->flags |= FC_DEGEN;
            continue;
        }

        bool res = plane_normal(&face->normal,
                                &vertices[face->i_v[0]],
                                &vertices[face->i_v[1]],
                                &vertices[face->i_v[2]]);

        if (!res) {
            epm_Log(LT_WARN, "WARNING: Encountered degenerate polygon during plane normal computation: %zu : (%s, %s, %s).",
                    i_face,
                    fmt_fix_x(x_of(vertices[face->i_v[0]]), 16),
                    fmt_fix_x(y_of(vertices[face->i_v[1]]), 16),
                    fmt_fix_x(z_of(vertices[face->i_v[2]]), 16));
            face->flags |= FC_DEGEN;
            continue;
        }

        /*
        printf("HI %.10lf, %.10lf, %.10lf\n",
               FIX_dbl(x_of(face->normal)),
               FIX_dbl(y_of(face->normal)),
               FIX_dbl(z_of(face->normal)));
        */
    }
}


void epm_ComputeFaceBrightnesses(size_t num_faces, Face faces[]) {
    for (size_t i_face = 0; i_face < num_faces; i_face++) {
        Face *face = faces+i_face;
        
        if (face->flags & FC_DEGEN) {
            face->fbri = 255;
            continue;
        }
        
        Fix64 prod = dot(face->normal, directional_light_vector);   
        Fix64 tmp;
        tmp = (prod * (Fix64)directional_light_intensity)>>16; // [-1, 1] (.16)
        tmp += (1<<16); // [0, 2] (.16)
        tmp = LSHIFT64(tmp, 7); // [0, 256] (fixed .16)
        tmp >>= 16; // [0, 256] (integer .0)
        dibassert(tmp < 256);
        face->fbri = (uint8_t)tmp;
    }
}

size_t epm_CountEdgesFromFaces(size_t num_faces, Face *faces) {    
    size_t num_edges = 0;
    size_t i_v0, i_v1, i_v2, min_i_v, max_i_v;
    bool duplicate;
    
    for (size_t i_face = 0; i_face < num_faces; i_face++) {
        i_v0 = faces[i_face].i_v[0];
        i_v1 = faces[i_face].i_v[1];
        i_v2 = faces[i_face].i_v[2];
        
        min_i_v = MIN(i_v0, i_v1);
        max_i_v = MAX(i_v0, i_v1);
        duplicate = false;
        for (size_t i_edge = 0; i_edge < num_edges; i_edge++) {
            if ((g_edges[i_edge].i_v0 == min_i_v &&
                 g_edges[i_edge].i_v1 == max_i_v)) {
                duplicate = true;
                break; // from i_edge loop
            }
        }
        if ( ! duplicate) { // add to list
            g_edges[num_edges].i_v0 = min_i_v;
            g_edges[num_edges].i_v1 = max_i_v;
            num_edges++;
        }


        min_i_v = MIN(i_v1, i_v2);
        max_i_v = MAX(i_v1, i_v2);
        duplicate = false;
        for (size_t i_edge = 0; i_edge < num_edges; i_edge++) {
            if ((g_edges[i_edge].i_v0 == min_i_v &&
                 g_edges[i_edge].i_v1 == max_i_v)) {
                duplicate = true;
                break; // from i_edge loop
            }
        }
        if ( ! duplicate) { // add to list
            g_edges[num_edges].i_v0 = min_i_v;
            g_edges[num_edges].i_v1 = max_i_v;
            num_edges++;
        }        


        min_i_v = MIN(i_v2, i_v0);
        max_i_v = MAX(i_v2, i_v0);
        duplicate = false;
        for (size_t i_edge = 0; i_edge < num_edges; i_edge++) {
            if ((g_edges[i_edge].i_v0 == min_i_v &&
                 g_edges[i_edge].i_v1 == max_i_v)) {
                duplicate = true;
                break; // from i_edge loop
            }
        }
        if ( ! duplicate) { // add to list
            g_edges[num_edges].i_v0 = min_i_v;
            g_edges[num_edges].i_v1 = max_i_v;
            num_edges++;
        }
    }

    return num_edges;
}


epm_Result epm_ComputeEdgesFromFaces
(size_t num_faces, Face *faces,
 size_t *out_num_edges, Edge *(out_edges[])) {
    size_t num_edges = epm_CountEdgesFromFaces(num_faces, faces);
    Edge *edges = zgl_Malloc(num_edges*sizeof(*edges));

    size_t max_edges = num_edges;
    num_edges = 0;
    
    size_t i_v0, i_v1, i_v2, min_i_v, max_i_v;
    bool duplicate;
    
    for (size_t i_face = 0; i_face < num_faces; i_face++) {
        i_v0 = faces[i_face].i_v[0];
        i_v1 = faces[i_face].i_v[1];
        i_v2 = faces[i_face].i_v[2];
        
        min_i_v = MIN(i_v0, i_v1);
        max_i_v = MAX(i_v0, i_v1);
        duplicate = false;
        for (size_t i_edge = 0; i_edge < num_edges; i_edge++) {
            if ((edges[i_edge].i_v0 == min_i_v &&
                 edges[i_edge].i_v1 == max_i_v)) {
                duplicate = true;
                break;
            }
        }
        if ( ! duplicate) {
            if (num_edges == max_edges)
                return EPM_ERROR;
            edges[num_edges].i_v0 = min_i_v;
            edges[num_edges].i_v1 = max_i_v;
            num_edges++;
        }


        min_i_v = MIN(i_v1, i_v2);
        max_i_v = MAX(i_v1, i_v2);
        duplicate = false;
        for (size_t i_edge = 0; i_edge < num_edges; i_edge++) {
            if ((edges[i_edge].i_v0 == min_i_v &&
                 edges[i_edge].i_v1 == max_i_v)) {
                duplicate = true;
                break;
            }
        }
        if ( ! duplicate) {
            if (num_edges == max_edges)
                return EPM_ERROR;
            edges[num_edges].i_v0 = min_i_v;
            edges[num_edges].i_v1 = max_i_v;
            num_edges++;
        }        


        min_i_v = MIN(i_v2, i_v0);
        max_i_v = MAX(i_v2, i_v0);
        duplicate = false;
        for (size_t i_edge = 0; i_edge < num_edges; i_edge++) {
            if ((edges[i_edge].i_v0 == min_i_v &&
                 edges[i_edge].i_v1 == max_i_v)) {
                duplicate = true;
                break;
            }
        }
        if ( ! duplicate) {
            if (num_edges == max_edges)
                return EPM_ERROR;
            edges[num_edges].i_v0 = min_i_v;
            edges[num_edges].i_v1 = max_i_v;
            num_edges++;
        }
    }

    dibassert(num_edges == max_edges);
    
    *out_num_edges = num_edges;
    *out_edges = edges;
    
    return EPM_SUCCESS;
}

void write_world_geometry(char const *filename) {
    char path[256];
    strcpy(path, DIR_WORLD);
    strcat(path, filename);
    strcat(path, SUF_WORLD);
    
    FILE *out_fp = fopen(path, "w");
    if (!out_fp) {
        epm_Log(LT_ERROR, "Could not open file for writing: %s\n", path);
        return;
    }

    fprintf(out_fp, "# EWD0\n\n");

    size_t num_V = g_world.geo_bsp->num_vertices;
    Fix32Vec const *V = g_world.geo_bsp->vertices;
    for (size_t i_V = 0; i_V < num_V; i_V++) {
        fprintf(out_fp, "v  %s  %s  %s\n",
                fmt_fix_x(x_of(V[i_V]), 16),
                fmt_fix_x(y_of(V[i_V]), 16),
                fmt_fix_x(z_of(V[i_V]), 16));
    }
    fputc('\n', out_fp);

    size_t num_F = g_world.geo_bsp->num_faces;
    //    BSPFace const *bsp_Fs = g_world.geo_bsp->bsp_faces;
    Face const *Fs = g_world.geo_bsp->faces;
    for (size_t i_F = 0; i_F < num_F; i_F++) {
        fprintf(out_fp, "f  %zu  %zu  %zu\n",
                Fs[i_F].i_v[0], Fs[i_F].i_v[1], Fs[i_F].i_v[2]);
    }
    fputc('\n', out_fp);

    for (size_t i_F = 0; i_F < num_F; i_F++) {
        fprintf(out_fp, "fn  %s  %s  %s\n",
                fmt_fix_x(x_of(Fs[i_F].normal), 16),
                fmt_fix_x(y_of(Fs[i_F].normal), 16),
                fmt_fix_x(z_of(Fs[i_F].normal), 16));
    }
    fputc('\n', out_fp);

    
    /* Save texture data */    
    size_t i_i_tex = 0, num_i_texs = 0;
    size_t i_texs[64];
    for (size_t i_F = 0; i_F < num_F; i_F++) {
        size_t i_tex = Fs[i_F].i_tex;
        bool new = true;
        for (size_t j = 0; j < num_i_texs; j++) {
            if (i_texs[j] == i_tex) {
                new = false;
                break;
            }
        }

        if (new) {
            i_texs[i_i_tex] = i_tex;
            i_i_tex++;
            num_i_texs++;
        }
    }

    for (size_t j = 0; j < num_i_texs; j++) {
        fprintf(out_fp, "texname  \"%s\"\n", textures[i_texs[j]].name);
    }

    fputc('\n', out_fp);

    for (size_t i_F = 0; i_F < num_F; i_F++) {
        size_t i_i_tex = 666;
        for (size_t j = 0; j < num_i_texs; j++) {
            if (i_texs[j] == Fs[i_F].i_tex) {
                i_i_tex = j;
                break;                
            }
        }
        fprintf(out_fp, "ft  %zu\n", i_i_tex);
    }
    
    fputc('\n', out_fp);

    for (size_t i_F = 0; i_F < num_F; i_F++) {
        fprintf(out_fp, "ftxl %s %s ; %s %s ; %s %s\n",
                fmt_fix_x(Fs[i_F].vtxl[0].x, 16), fmt_fix_x(Fs[i_F].vtxl[0].y, 16),
                fmt_fix_x(Fs[i_F].vtxl[1].x, 16), fmt_fix_x(Fs[i_F].vtxl[1].y, 16),
                fmt_fix_x(Fs[i_F].vtxl[2].x, 16), fmt_fix_x(Fs[i_F].vtxl[2].y, 16));
    }
    
    fclose(out_fp);
}

/*
epm_Result read_world(StaticGeometry *geo, char *filename) {
    geo->num_vertices = 0;
    geo->num_edges = 0;
    geo->num_faces = 0;
    
    FILE *in_fp;

    char path[256];
    strcpy(path, DIR_WORLD);
    strcat(path, filename);
    strcat(path, SUF_WORLD);

    epm_Log(LT_INFO, "Loading world from file: %s", path);

    in_fp = fopen(path, "rb");
    if ( ! in_fp) {
        epm_Log(LT_WARN, "File not found: %s", path);
        return EPM_FAILURE;
    }
    
    DIBJFileOverview data = {0};
    file_overview(&data, in_fp);

    vbs_printf("  num_lines: %zu\n", data.num_lines);
    vbs_printf("      ofs_v: %zu\n", data.ofs_v);
    vbs_printf("      num_v: %zu\n", data.num_v);
    vbs_printf("      ofs_e: %zu\n", data.ofs_e);
    vbs_printf("      num_e: %zu\n", data.num_e);
    vbs_printf("      ofs_f: %zu\n", data.ofs_f);
    vbs_printf("      num_f: %zu\n", data.num_f);
    vbs_printf("     ofs_vn: %zu\n", data.ofs_vn);
    vbs_printf("     num_vn: %zu\n", data.num_vn);
    vbs_printf("     ofs_en: %zu\n", data.ofs_en);
    vbs_printf("     num_en: %zu\n", data.num_en);
    vbs_printf("     ofs_fn: %zu\n", data.ofs_fn);
    vbs_printf("     num_fn: %zu\n", data.num_fn);
    vbs_printf("ofs_texname: %zu\n", data.ofs_texname);
    vbs_printf("num_texname: %zu\n", data.num_texname);
    vbs_printf("     ofs_ft: %zu\n", data.ofs_ft);
    vbs_printf("     num_ft: %zu\n", data.num_ft);
    vbs_printf("   ofs_ftxl: %zu\n", data.ofs_ftxl);
    vbs_printf("   num_ftxl: %zu\n", data.num_ftxl);

    if (data.num_fn > 0 && data.num_fn != data.num_f) {
        epm_Log(LT_WARN, "Geo file %s: number of face normal entries does not match number of faces.\n", filename);
        return EPM_FAILURE;
    }
    if (data.num_ft > 0 && data.num_ft != data.num_f) {
        epm_Log(LT_WARN, "Geo file %s: number of face texture entries does not match number of face entries.\n", filename);
        return EPM_FAILURE;
    }
    if (data.num_ftxl > 0 && data.num_ftxl != data.num_f) {
        epm_Log(LT_WARN, "Mesh file %s: number of face texel entries does not match number of face entries.\n", filename);
        return EPM_FAILURE;
    }
    
    char line[256] = {'\0'};    

    if (data.num_v > 0) {
        geo->num_vertices = data.num_v;
        
        fseek(in_fp, data.ofs_v, SEEK_SET);

        size_t i_vertex = 0;
        size_t i_ch;
        while (true) {
            read_line(in_fp, line);

            if (line[0] == '#') continue;
            if ( ! prefix("v ", line)) break;

            i_ch = strlen("v ");
            WorldVec vertex;

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &x_of(vertex));

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &y_of(vertex));
            
            while (line[i_ch] == ' ')  i_ch++;
            i_ch += read_fix_x(line+i_ch, &z_of(vertex));

            geo->vertices[i_vertex] = vertex;
            i_vertex++;
            
            if (feof(in_fp)) break;
        }
        
        dibassert(geo->num_vertices == i_vertex);
    }

    
    if (data.num_f > 0) {
        geo->num_faces = data.num_f;
        
        fseek(in_fp, data.ofs_f, SEEK_SET);

        size_t i_face = 0;    
        while (true) {
            read_line(in_fp, line);

            if (line[0] == '#') continue;
            if ( ! prefix("f ", line)) break;

            size_t i_v0, i_v1, i_v2;
        
            sscanf(line+2, " %zu %zu %zu", &i_v0, &i_v1, &i_v2);
            
            geo->faces[i_face].i_v0 = i_v0;
            geo->faces[i_face].i_v1 = i_v1;
            geo->faces[i_face].i_v2 = i_v2;
            i_face++;

            if (feof(in_fp)) break;
        }

        dibassert(geo->num_faces == i_face);
    }
    
    if (data.num_fn > 0) {
        fseek(in_fp, data.ofs_fn, SEEK_SET);
 
        size_t i_face = 0;
        size_t i_ch = 0;
        while (true) {
            read_line(in_fp, line);

            if (line[0] == '#') continue;
            if ( ! prefix("fn ", line)) break;

            i_ch = strlen("fn ");
            WorldVec normal;

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &x_of(normal));

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &y_of(normal));
            
            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &z_of(normal));
            
            geo->faces[i_face].normal = normal;
            i_face++;

            if (feof(in_fp)) break;
        }
    }
    
    if (data.num_texname > 0) {
        fseek(in_fp, data.ofs_texname, SEEK_SET);

        while (true) {
            read_line(in_fp, line);

            if (line[0] == '#') continue;
            if ( ! prefix("texname ", line)) break;

            size_t i_ch = strlen("texname ");
            while (line[i_ch] == ' ') i_ch++;
            i_ch++; // skip opening "
            
            size_t i = 0;
            while ((texnames[num_texnames][i++] = line[i_ch++]) != '"')
                ;
            texnames[num_texnames][i-1] = '\0';
            num_texnames++;
            // currently file-local index; still needs to be translated to
            // global texture array index.
                                
            if (feof(in_fp)) break;   
        }
    }
    
    if (data.num_ft > 0) {
        fseek(in_fp, data.ofs_ft, SEEK_SET);
    
        size_t i_face = 0;
        while (true) {
            read_line(in_fp, line);

            if (line[0] == '#') continue;
            if ( ! prefix("ft ", line)) break;

            size_t i_tex;
            
            sscanf(line+strlen("ft "), " %zu", &i_tex);
            geo->faces[i_face].i_tex = i_tex;
            get_texture_by_name(texnames[i_tex], &geo->faces[i_face].i_tex);
            
            i_face++;

            if (feof(in_fp)) break;
        }
    }

    if (data.num_ftxl > 0) {
        fseek(in_fp, data.ofs_ftxl, SEEK_SET);
        
        size_t i_ch = 0;
        size_t i_face = 0;
        while (true) {
            read_line(in_fp, line);
            
            if (line[0] == '#') continue;
            if ( ! prefix("ftxl ", line)) break;
            
            i_ch = strlen("ftxl ");
            Fix32Vec_2D tv;

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.x);

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.y);
            
            geo->faces[i_face].vtxl[0] = tv;

            
            while (line[i_ch] == ' ') i_ch++;
            i_ch++; // skip semicolon

            
            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.x);
            
            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.y);

            geo->faces[i_face].vtxl[1] = tv;


            while (line[i_ch] == ' ') i_ch++;
            i_ch++; // skip semicolon

            
            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.x);
            
            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.y);

            geo->faces[i_face].vtxl[2] = tv;
            
            i_face++;
            
            if (feof(in_fp)) break;
        }
    }

    fclose(in_fp);
    
    num_texnames = 0;

    compute_face_brightnesses(geo->num_faces, geo->faces);
    geo->num_edges = count_edges_from_faces(geo->num_faces, geo->faces);
    compute_edges_from_faces(geo->num_edges, geo->edges,
                             geo->num_faces, geo->faces);


    for (size_t i_texnames = 0; i_texnames < 64; i_texnames++) {
        texnames[i_texnames][0] = '\0';
    }
    
    return EPM_SUCCESS;
}
*/

void write_Mesh_dibj_1(Mesh const *mesh, char const *filename) {
    FILE *out_fp = fopen(filename, "w");
    if (!out_fp) {
        epm_Log(LT_ERROR, "Could not open file for writing: %s\n", filename);
        return;
    }

    fprintf(out_fp, "# DIBJ\n\n");

    size_t num_V = mesh->num_vertices;
    Fix32Vec const *V = mesh->vertices;
    for (size_t i_V = 0; i_V < num_V; i_V++) {
        fprintf(out_fp, "v  %s  %s  %s\n",
                fmt_fix_x(x_of(V[i_V]), 16),
                fmt_fix_x(y_of(V[i_V]), 16),
                fmt_fix_x(z_of(V[i_V]), 16));
    }
    fputc('\n', out_fp);

    size_t num_F = mesh->num_faces;
    Face const *F = mesh->faces;
    for (size_t i_F = 0; i_F < num_F; i_F++) {
        fprintf(out_fp, "f  %zu  %zu  %zu\n",
                F[i_F].i_v[0], F[i_F].i_v[1], F[i_F].i_v[2]);
    }
    fputc('\n', out_fp);

    for (size_t i_F = 0; i_F < num_F; i_F++) {
        fprintf(out_fp, "fn  %s  %s  %s\n",
                fmt_fix_x(x_of(F[i_F].normal), 16),
                fmt_fix_x(y_of(F[i_F].normal), 16),
                fmt_fix_x(z_of(F[i_F].normal), 16));
    }
    fputc('\n', out_fp);

    
    /* Save texture data */    
    size_t i_i_tex = 0, num_i_texs = 0;
    size_t i_texs[64];
    for (size_t i_F = 0; i_F < num_F; i_F++) {
        size_t i_tex = F[i_F].i_tex;
        bool new = true;
        for (size_t j = 0; j < num_i_texs; j++) {
            if (i_texs[j] == i_tex) {
                new = false;
                break;
            }
        }

        if (new) {
            i_texs[i_i_tex] = i_tex;
            i_i_tex++;
            num_i_texs++;
        }
    }

    for (size_t j = 0; j < num_i_texs; j++) {
        fprintf(out_fp, "texname  \"%s\"\n", textures[i_texs[j]].name);
    }

    fputc('\n', out_fp);

    for (size_t i_F = 0; i_F < num_F; i_F++) {
        size_t i_i_tex = 666;
        for (size_t j = 0; j < num_i_texs; j++) {
            if (i_texs[j] == F[i_F].i_tex) {
                i_i_tex = j;
                break;                
            }
        }
        fprintf(out_fp, "ft  %zu\n", i_i_tex);
    }
    
    fputc('\n', out_fp);

    for (size_t i_F = 0; i_F < num_F; i_F++) {
        fprintf(out_fp, "ftxl %s %s ; %s %s ; %s %s\n",
                fmt_fix_x(F[i_F].vtxl[0].x, 16), fmt_fix_x(F[i_F].vtxl[0].y, 16),
                fmt_fix_x(F[i_F].vtxl[1].x, 16), fmt_fix_x(F[i_F].vtxl[1].y, 16),
                fmt_fix_x(F[i_F].vtxl[2].x, 16), fmt_fix_x(F[i_F].vtxl[2].y, 16));
    }
    
    fclose(out_fp);
}



static void postread(Mesh *mesh) {
    epm_ComputeFaceNormals(mesh->vertices, mesh->num_faces, mesh->faces);
    epm_ComputeFaceBrightnesses(mesh->num_faces, mesh->faces);

    mesh->num_edges = epm_CountEdgesFromFaces(mesh->num_faces, mesh->faces);
    mesh->edges = zgl_Malloc(mesh->num_edges * sizeof(*(mesh->edges)));
    epm_Result res =
        epm_ComputeEdgesFromFaces(mesh->num_faces, mesh->faces,
                                  &mesh->num_edges, &mesh->edges);

    if (res == EPM_ERROR) {
        epm_Log(LT_ERROR, "Maximum number of edges smaller than actual number of edges.\n");
        exit(0); // TODO: Handle properly.
    }

    // Temporarily: all textures and texels the same. TODO: Unrestrict.
    for (size_t i_face = 0; i_face < mesh->num_faces; i_face++) {
        Face *face = mesh->faces + i_face;

        face->i_tex = 3;
        epm_Texture *tex = &textures[face->i_tex];
        face->vtxl[0] = (Fix32Vec_2D){(int)(tex->w << 16) - 1, 0};
        face->vtxl[1] = (Fix32Vec_2D){0, 0};
        face->vtxl[2] = (Fix32Vec_2D){0, (int)(tex->h << 16) - 1};   
    }

}

/*
static void validate_data(DIBJFileOverview const *_data) {
    DIBJFileOverview data = *_data;
}
*/

static void file_overview(DIBJFileOverview *data, FILE *in_fp) {
    char line[256] = {'\0'};
    size_t ofs_line;

    rewind(in_fp);
    
    size_t i_ch = 0;
    while (true) {
        ofs_line = ftell(in_fp);
        while ((line[i_ch] = (char)fgetc(in_fp)) != '\n' && line[i_ch] != EOF) {
            i_ch++;
        }

        data->num_lines++;
        
        if (line[0] == '\n') { // line was just a newline-character
            i_ch = 0;
            continue;
        }
        else if (line[i_ch] == EOF) {
            break;
        }

        i_ch++;
        line[i_ch] = '\0';

        if (line[0] == 'v' && line[1] == ' ') {
            if (data->ofs_v == 0) { // first vertex
                data->ofs_v = ofs_line;
            }
            data->num_v++;
        }
        else if (line[0] == 'e' && line[1] == ' ') {
            if (data->ofs_e == 0) { // first vertex
                data->ofs_e = ofs_line;
            }
            data->num_e++;
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            if (data->ofs_f == 0) { // first vertex
                data->ofs_f = ofs_line;
            }
            data->num_f++;
        }
        else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {
            if (data->ofs_vn == 0) { // first vertex
                data->ofs_vn = ofs_line;
            }
            data->num_vn++;
        }
        else if (line[0] == 'e' && line[1] == 'n' && line[2] == ' ') {
            if (data->ofs_en == 0) { // first vertex
                data->ofs_en = ofs_line;
            }
            data->num_en++;
        }
        else if (line[0] == 'f' && line[1] == 'n' && line[2] == ' ') {
            if (data->ofs_fn == 0) { // first vertex
                data->ofs_fn = ofs_line;
            }
            data->num_fn++;
        }
        else if (line[0] == 'f' && line[1] == 't' && line[2] == ' ') {
            if (data->ofs_ft == 0) { // first vertex
                data->ofs_ft = ofs_line;
            }
            data->num_ft++;
        }
        else if (prefix("texname ", line)) {
            if (data->ofs_texname == 0) { // first vertex
                data->ofs_texname = ofs_line;
            }
            data->num_texname++;
        }
        else if (prefix("ftxl ", line)) {
            if (data->ofs_ftxl == 0) { // first vertex
                data->ofs_ftxl = ofs_line;
            }
            data->num_ftxl++;
        }

        data->num_lines++;
        i_ch = 0;
    }

    rewind(in_fp);
}

size_t read_fix_x(char const *str, Fix32 *out_num) {
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
    
    return i;
}

/** Reads one line of file stream STARTING FROM NEXT NON-SPACE. This skips right over lines consisting only of whitespace. */
static void read_line(FILE *in_fp, char *str) {
    char ch;
    
    while (isspace(ch = (char)fgetc(in_fp)))
        ;

    if (ch == EOF)
        return;

    str[0] = ch;
    size_t i_ch = 1;
    while (true) {
        str[i_ch] = (char)fgetc(in_fp);
        if (str[i_ch] == '\n' || str[i_ch] == EOF) break;
        i_ch++;
    }

    str[i_ch + 1] = '\0';
}

epm_Result load_Mesh_dibj_1(Mesh *mesh, char *filename) {
    mesh->num_vertices = 0;
    mesh->num_edges = 0;
    mesh->num_faces = 0;
    
    FILE *in_fp;

    in_fp = fopen(filename, "rb");
    if ( ! in_fp) {
        epm_Log(LT_ERROR, "File not found: %s\n", filename);
    }
    
    DIBJFileOverview data = {0};
    file_overview(&data, in_fp);

    vbs_printf("  num_lines: %zu\n", data.num_lines);
    vbs_printf("      ofs_v: %zu\n", data.ofs_v);
    vbs_printf("      num_v: %zu\n", data.num_v);
    vbs_printf("      ofs_e: %zu\n", data.ofs_e);
    vbs_printf("      num_e: %zu\n", data.num_e);
    vbs_printf("      ofs_f: %zu\n", data.ofs_f);
    vbs_printf("      num_f: %zu\n", data.num_f);
    vbs_printf("     ofs_vn: %zu\n", data.ofs_vn);
    vbs_printf("     num_vn: %zu\n", data.num_vn);
    vbs_printf("     ofs_en: %zu\n", data.ofs_en);
    vbs_printf("     num_en: %zu\n", data.num_en);
    vbs_printf("     ofs_fn: %zu\n", data.ofs_fn);
    vbs_printf("     num_fn: %zu\n", data.num_fn);
    vbs_printf("ofs_texname: %zu\n", data.ofs_texname);
    vbs_printf("num_texname: %zu\n", data.num_texname);
    vbs_printf("     ofs_ft: %zu\n", data.ofs_ft);
    vbs_printf("     num_ft: %zu\n", data.num_ft);
    vbs_printf("   ofs_ftxl: %zu\n", data.ofs_ftxl);
    vbs_printf("   num_ftxl: %zu\n", data.num_ftxl);

    if (data.num_fn > 0 && data.num_fn != data.num_f) {
        epm_Log(LT_WARN, "Mesh file %s: number of face normal entries does not match number of faces.\n", filename);
        return EPM_FAILURE;
    }
    if (data.num_ft > 0 && data.num_ft != data.num_f) {
        epm_Log(LT_WARN, "Mesh file %s: number of face texture entries does not match number of face entries.\n", filename);
        return EPM_FAILURE;
    }
    if (data.num_ftxl > 0 && data.num_ftxl != data.num_f) {
        epm_Log(LT_WARN, "Mesh file %s: number of face texel entries does not match number of face entries.\n", filename);
        return EPM_FAILURE;
    }
    
    char line[256] = {'\0'};    

    if (data.num_v > 0) {
        mesh->num_vertices = data.num_v;
        mesh->vertices = zgl_Malloc(mesh->num_vertices * sizeof(*(mesh->vertices)));
        
        fseek(in_fp, data.ofs_v, SEEK_SET);

        size_t i_vertex = 0;
        size_t i_ch;
        while (true) {
            read_line(in_fp, line);

            if (line[0] == '#') continue;
            if ( ! prefix("v ", line)) break;

            i_ch = strlen("v ");
            WorldVec vertex;

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &x_of(vertex));

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &y_of(vertex));
            
            while (line[i_ch] == ' ')  i_ch++;
            i_ch += read_fix_x(line+i_ch, &z_of(vertex));

            mesh->vertices[i_vertex] = vertex;
            i_vertex++;
            
            if (feof(in_fp)) break;
        }
        
        dibassert(mesh->num_vertices == i_vertex);
    }


    
    if (data.num_f > 0) {
        mesh->num_faces = data.num_f;
        mesh->faces = zgl_Malloc(mesh->num_faces * sizeof(*(mesh->faces)));
        
        fseek(in_fp, data.ofs_f, SEEK_SET);

        size_t i_face = 0;    
        while (true) {
            read_line(in_fp, line);

            if (line[0] == '#') continue;
            if ( ! prefix("f ", line)) break;

            size_t i_v0, i_v1, i_v2;
        
            sscanf(line+2, " %zu %zu %zu", &i_v0, &i_v1, &i_v2);
            
            mesh->faces[i_face].i_v[0] = i_v0;
            mesh->faces[i_face].i_v[1] = i_v1;
            mesh->faces[i_face].i_v[2] = i_v2;
            i_face++;

            if (feof(in_fp)) break;
        }

        dibassert(mesh->num_faces == i_face);
    }
    
    if (data.num_fn > 0) {
        fseek(in_fp, data.ofs_fn, SEEK_SET);
 
        size_t i_face = 0;
        size_t i_ch = 0;
        while (true) {
            read_line(in_fp, line);

            if (line[0] == '#') continue;
            if ( ! prefix("fn ", line)) break;

            i_ch = strlen("fn ");
            WorldVec normal;

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &x_of(normal));

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &y_of(normal));
            
            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &z_of(normal));
            
            mesh->faces[i_face].normal = normal;
            i_face++;

            if (feof(in_fp)) break;
        }
    }
    
    if (data.num_texname > 0) {
        fseek(in_fp, data.ofs_texname, SEEK_SET);

        while (true) {
            read_line(in_fp, line);

            if (line[0] == '#') continue;
            if ( ! prefix("texname ", line)) break;

            size_t i_ch = strlen("texname ");
            while (line[i_ch] == ' ') i_ch++;
            i_ch++; // skip opening "
            
            size_t i = 0;
            while ((texnames[num_texnames][i++] = line[i_ch++]) != '"')
                ;
            texnames[num_texnames][i-1] = '\0';
            num_texnames++;
            // currently file-local index; still needs to be translated to
            // global texture array index.
                                
            if (feof(in_fp)) break;   
        }
    }
    
    if (data.num_ft > 0) {
        fseek(in_fp, data.ofs_ft, SEEK_SET);
    
        size_t i_face = 0;
        while (true) {
            read_line(in_fp, line);

            if (line[0] == '#') continue;
            if ( ! prefix("ft ", line)) break;

            size_t i_tex;
            
            sscanf(line+strlen("ft "), " %zu", &i_tex);
            mesh->faces[i_face].i_tex = i_tex;
            mesh->faces[i_face].i_tex = epm_TextureIndexFromName(texnames[i_tex]);
            
            i_face++;

            if (feof(in_fp)) break;
        }
    }

    if (data.num_ftxl > 0) {
        fseek(in_fp, data.ofs_ftxl, SEEK_SET);
        
        size_t i_ch = 0;
        size_t i_face = 0;
        while (true) {
            read_line(in_fp, line);
            
            if (line[0] == '#') continue;
            if ( ! prefix("ftxl ", line)) break;
            
            i_ch = strlen("ftxl ");
            Fix32Vec_2D tv;

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.x);

            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.y);
            
            mesh->faces[i_face].vtxl[0] = tv;

            
            while (line[i_ch] == ' ') i_ch++;
            i_ch++; // skip semicolon

            
            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.x);
            
            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.y);

            mesh->faces[i_face].vtxl[1] = tv;


            while (line[i_ch] == ' ') i_ch++;
            i_ch++; // skip semicolon

            
            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.x);
            
            while (line[i_ch] == ' ') i_ch++;
            i_ch += read_fix_x(line+i_ch, &tv.y);

            mesh->faces[i_face].vtxl[2] = tv;
            
            i_face++;

            vbs_printf("ftxl  %s %s ; %s %s ; %s %s\n",
                       fmt_fix_x(mesh->faces[i_face].vtxl[0].x, 16),
                       fmt_fix_x(mesh->faces[i_face].vtxl[0].y, 16),
                       fmt_fix_x(mesh->faces[i_face].vtxl[1].x, 16),
                       fmt_fix_x(mesh->faces[i_face].vtxl[1].y, 16),
                       fmt_fix_x(mesh->faces[i_face].vtxl[2].x, 16),
                       fmt_fix_x(mesh->faces[i_face].vtxl[2].y, 16));
            
            if (feof(in_fp)) break;
        }
    }

    fclose(in_fp);
    
    num_texnames = 0;
#ifdef VERBOSITY
    print_Mesh(mesh);
#endif

    epm_ComputeFaceBrightnesses(mesh->num_faces, mesh->faces);

    mesh->num_edges = epm_CountEdgesFromFaces(mesh->num_faces, mesh->faces);
    mesh->edges = zgl_Malloc(mesh->num_edges * sizeof(*(mesh->edges)));
    epm_ComputeEdgesFromFaces(mesh->num_faces, mesh->faces,
                              &mesh->num_edges, &mesh->edges);

    for (size_t i_texnames = 0; i_texnames < 64; i_texnames++) {
        texnames[i_texnames][0] = '\0';
    }

    return EPM_SUCCESS;
}

void load_Mesh_dibj_0(Mesh *mesh, char *filename) {
    int i_vertex = 0;
    int i_face = 0;
    mesh->num_vertices = 0;
    mesh->num_edges = 0;
    mesh->num_faces = 0;
    
    FILE *in_fp;

    in_fp = fopen(filename, "rb");
    if (!in_fp) {
        epm_Log(LT_ERROR, "File not found: %s\n", filename);
    }

    char line[256] = {'\0'};
    int i = 0;
    
    while (true) {
        while ((line[i++] = (char)fgetc(in_fp)) != '\n') {
            if (line[i-1] == EOF) goto NextRead;
        }
        line[i] = '\0';
        i = 0;
        
        double x, y, z;
        
        if (line[0] == 'v' && line[1] == ' ') {
            sscanf(line+2, " %lf %lf %lf", &x, &y, &z);
            x_of(mesh->vertices[i_vertex]) = (Fix32)(x * FIX_P16_ONE);
            y_of(mesh->vertices[i_vertex]) = (Fix32)(y * FIX_P16_ONE);
            z_of(mesh->vertices[i_vertex]) = (Fix32)(z * FIX_P16_ONE);
            mesh->num_vertices++;
            i_vertex++;
        }
    }

 NextRead:
    fseek(in_fp, 0, SEEK_SET);
    
    i = 0;
    while (true) {
        while ((line[i++] = (char)fgetc(in_fp)) != '\n') {
            if (line[i-1] == EOF) goto EndRead;
        }
        line[i] = '\0';
        i = 0;
        
        size_t i_v0, i_v1, i_v2;
        
        if (line[0] == 'f' && line[1] == ' ') {
            sscanf(line+2, " %zu %zu %zu", &i_v0, &i_v1, &i_v2);
            
            mesh->faces[i_face].i_v[0] = i_v0;
            mesh->faces[i_face].i_v[1] = i_v1;
            mesh->faces[i_face].i_v[2] = i_v2;
            mesh->num_faces++;
            i_face++;
        }
    }

 EndRead:
    fclose(in_fp);
    postread(mesh);
}

void load_Mesh_obj(Mesh *mesh, char *filename) {
    int i_vertex = 0;
    int i_face = 0;
    mesh->num_vertices = 0;
    mesh->num_edges = 0;
    mesh->num_faces = 0;
    
    FILE *in_fp;

    in_fp = fopen(filename, "rb");
    if (!in_fp) {
        epm_Log(LT_ERROR, "File not found: %s\n", filename);
        return;
    }
    
    char line[256] = {'\0'};
    int i = 0;
    
    while (true) {
        while ((line[i++] = (char)fgetc(in_fp)) != '\n') {
            if (line[i-1] == EOF) goto Next;
        }
        line[i] = '\0';
        i = 0;
        
        double x, y, z;
        
        if (line[0] == 'v' && line[1] == ' ') {
            sscanf(line+2, " %lf %lf %lf", &x, &y, &z);
            // obj files use y as up/down, but EPM uses z up/down
            x_of(g_vertices[i_vertex]) = (Fix32)(x * FIX_P16_ONE);
            y_of(g_vertices[i_vertex]) = (Fix32)(z * FIX_P16_ONE);
            z_of(g_vertices[i_vertex]) = (Fix32)(y * FIX_P16_ONE);
            mesh->num_vertices++;
            i_vertex++;
        }
    }

 Next:

    mesh->vertices = zgl_Malloc(mesh->num_vertices * sizeof(*(mesh->vertices)));
    memcpy(mesh->vertices, g_vertices, mesh->num_vertices * sizeof(*(mesh->vertices)));

    fseek(in_fp, 0, SEEK_SET);
    
    i = 0;
    while (true) {
        while ((line[i++] = (char)fgetc(in_fp)) != '\n') {
            if (line[i-1] == EOF) goto EndRead;
        }
        line[i] = '\0';
        i = 0;
        
        size_t i_v0, i_v1, i_v2;
        
        if (line[0] == 'f' && line[1] == ' ') {
            sscanf(line+2, " %zu %zu %zu", &i_v0, &i_v1, &i_v2);
            // obj files index vertices starting at 1
            i_v0--;
            i_v1--;
            i_v2--;
            
            g_faces[i_face].i_v[0] = i_v0;
            g_faces[i_face].i_v[1] = i_v1;
            g_faces[i_face].i_v[2] = i_v2;
            mesh->num_faces++;
            i_face++;
        }
    }

 EndRead:
    mesh->faces = zgl_Malloc(mesh->num_faces * sizeof(*(mesh->faces)));
    memcpy(mesh->faces, g_faces, mesh->num_faces * sizeof(*(mesh->faces)));    
    
    fclose(in_fp);
    postread(mesh);
}

void destroy_mesh(void);

void print_Mesh(Mesh *mesh) {
    printf("  Mesh  \n");
    printf("+------+\n");
    printf("(v %zu)\n", mesh->num_vertices);
    printf("(e %zu)\n", mesh->num_edges);
    printf("(f %zu)\n", mesh->num_faces);
    printf("(min (%s, %s, %s))\n"
           "(max (%s, %s, %s))\n",
           fmt_fix_x(xmin_of(mesh->AABB), 16),
           fmt_fix_x(ymin_of(mesh->AABB), 16),
           fmt_fix_x(zmin_of(mesh->AABB), 16),
           fmt_fix_x(xmax_of(mesh->AABB), 16),
           fmt_fix_x(ymax_of(mesh->AABB), 16),
           fmt_fix_x(zmax_of(mesh->AABB), 16));
    
    for (size_t i_v = 0; i_v < mesh->num_vertices; i_v++) {
        printf("v  %s  %s  %s\n",
               fmt_fix_x(x_of(mesh->vertices[i_v]), 16),
               fmt_fix_x(y_of(mesh->vertices[i_v]), 16),
               fmt_fix_x(z_of(mesh->vertices[i_v]), 16));
    }
    putchar('\n');

    for (size_t i_f = 0; i_f < mesh->num_faces; i_f++) {
        printf("f  %zu  %zu  %zu\n",
               mesh->faces[i_f].i_v[0],
               mesh->faces[i_f].i_v[1],
               mesh->faces[i_f].i_v[2]);
    }
    putchar('\n');

    for (size_t i_f = 0; i_f < mesh->num_faces; i_f++) {
        printf("fn  %s  %s  %s\n",
               fmt_fix_x(x_of(mesh->faces[i_f].normal), 16),
               fmt_fix_x(y_of(mesh->faces[i_f].normal), 16),
               fmt_fix_x(z_of(mesh->faces[i_f].normal), 16));
    }
    putchar('\n');

    for (size_t i_f = 0; i_f < mesh->num_faces; i_f++) {
        printf("ft  %zu : %s\n", mesh->faces[i_f].i_tex,
               textures[mesh->faces[i_f].i_tex].name);
    }
    putchar('\n');

    for (size_t i_f = 0; i_f < mesh->num_faces; i_f++) {
        printf("ftxl  %s %s ; %s %s ; %s %s\n",
               fmt_fix_x(mesh->faces[i_f].vtxl[0].x, 16),
               fmt_fix_x(mesh->faces[i_f].vtxl[0].y, 16),
               fmt_fix_x(mesh->faces[i_f].vtxl[1].x, 16),
               fmt_fix_x(mesh->faces[i_f].vtxl[1].y, 16),
               fmt_fix_x(mesh->faces[i_f].vtxl[2].x, 16),
               fmt_fix_x(mesh->faces[i_f].vtxl[2].y, 16));
    }
    putchar('\n');
}

void epm_ComputeAABB(Mesh *p_mesh) {
    Fix32 xmin, ymin, zmin, xmax, ymax, zmax;
    dibassert(p_mesh->num_vertices > 0);
    Fix32 pad = (1<<16);
    
    WorldVec v = p_mesh->vertices[0];
    xmin = xmax = x_of(v);
    ymin = ymax = y_of(v);
    zmin = zmax = z_of(v);
    
    for (size_t i_v = 1; i_v < p_mesh->num_vertices; i_v++) {
        v = p_mesh->vertices[i_v];
        if (x_of(v) < xmin) xmin = x_of(v);
        if (y_of(v) < ymin) ymin = y_of(v);
        if (z_of(v) < zmin) zmin = z_of(v);
        if (xmax < x_of(v)) xmax = x_of(v);
        if (ymax < y_of(v)) ymax = y_of(v);
        if (zmax < z_of(v)) zmax = z_of(v);
    }

    xmin -= pad;
    ymin -= pad;
    zmin -= pad;
    xmax += pad;
    ymax += pad;
    zmax += pad;

    p_mesh->AABB = (Fix32MinMax){.min = {xmin, ymin, zmin},
                                 .max = {xmax, ymax, zmax}};
}

#include "src/input/command.h"
static void CMDH_outputgeo(int argc, char **argv, char *output_str) {
    write_world_geometry(argv[1]);
}
epm_Command const CMD_outputgeo = {
    .name = "outputgeo",
    .argc_min = 2,
    .argc_max = 3,
    .handler = CMDH_outputgeo,
};


//////////////////////////////////////////////////////////////////////////////

#define MAX_T_JUNCTIONS 10000
size_t num_tjs = 0;
WorldVec tjs[MAX_T_JUNCTIONS] = {0};

static void FindTJunctions_EdgeWise(Mesh *mesh, UFix32 const EdgeThickness) {
    // test every vertex against every edge.
    uint32_t num_vertices = (uint32_t)mesh->num_vertices;
    WorldVec *Vs = mesh->vertices;
    uint32_t num_edges = (uint32_t)mesh->num_edges;
    Edge *Es = mesh->edges;
    
    for (uint32_t i_e = 0; i_e < 10; i_e++) {
        uint32_t i_v0 = (uint32_t)Es[i_e].i_v0;
        uint32_t i_v1 = (uint32_t)Es[i_e].i_v1;
        
        UFix32 edgelength = dist_Euclidean(Vs[i_v1], Vs[i_v0]);
        
        // Sanity check
        if (edgelength <= EdgeThickness) continue;

        edgelength -= EdgeThickness;
            
        for (uint32_t i_v = 0; i_v < num_vertices; i_v++) {
            if (i_v == i_v0 || i_v == i_v1) continue;

            if (dist_Euclidean(Vs[i_v], Vs[i_v0]) <= edgelength
                &&
                dist_Euclidean(Vs[i_v], Vs[i_v1]) <= edgelength) {
                printf("(i_e %u : i_v0 %u : i_v1 %u : len %s) (i_v %u) (num_tjs %zu)\n", i_e, i_v0, i_v1, fmt_fix_x(edgelength, 16), i_v, num_tjs);
                dibassert(num_tjs < MAX_T_JUNCTIONS);
                tjs[num_tjs++] = Vs[i_v];
            }
        }
    }
}

static void FindTJunctions_FaceWise(Mesh *mesh, Face *f0, Face *f1, UFix32 const EdgeThickness) {
    uint32_t i_v0s[3] = {(uint32_t)f0->i_v[0], (uint32_t)f0->i_v[1], (uint32_t)f0->i_v[2]};
    uint32_t i_v1s[3] = {(uint32_t)f1->i_v[0], (uint32_t)f1->i_v[1], (uint32_t)f1->i_v[2]};
    
    for (uint32_t i_i_v0 = 0; i_i_v0 < 3; i_i_v0++) {
        uint32_t i_v0 = i_v0s[i_i_v0];
        uint32_t i_v0_next = i_v0s[i_i_v0 < 2 ? i_i_v0 + 1 : 0];
        
        WorldVec *Vs = mesh->vertices;
        WorldPlane edgeplane;
        WorldPlane polyplane;

        edgeplane.ref = Vs[i_v0];
        plane_normal(&edgeplane.normal,
                     Vs + i_v0,
                     Vs + i_v0_next,
                     &diff(Vs[i_v0], f0->normal));
        edgeplane.constant =
            ((Fix64)x_of(edgeplane.ref)*(Fix64)x_of(edgeplane.normal) +
             (Fix64)y_of(edgeplane.ref)*(Fix64)y_of(edgeplane.normal) +
             (Fix64)z_of(edgeplane.ref)*(Fix64)z_of(edgeplane.normal))>>16;


        polyplane.ref = Vs[i_v0];
        polyplane.normal = f0->normal;
        polyplane.constant =
            ((Fix64)x_of(polyplane.ref)*(Fix64)x_of(polyplane.normal) +
             (Fix64)y_of(polyplane.ref)*(Fix64)y_of(polyplane.normal) +
             (Fix64)z_of(polyplane.ref)*(Fix64)z_of(polyplane.normal))>>16;

        for (uint32_t i_i_v1 = 0; i_i_v1 < 3; i_i_v1++) {
            uint32_t i_v1 = i_v1s[i_i_v1];
            uint32_t i_v1_next = i_v1s[i_i_v1 < 2 ? i_i_v1 + 1 : 0];

            if ((UFix32)ABS(signdist(Vs[i_v1], polyplane)) > EdgeThickness)
                continue;
            if ((UFix32)ABS(signdist(Vs[i_v1_next], polyplane)) > EdgeThickness)
                continue;
            if ((UFix32)ABS(signdist(Vs[i_v1], edgeplane)) > EdgeThickness)
                continue;
            if ((UFix32)ABS(signdist(Vs[i_v1_next], edgeplane)) > EdgeThickness)
                continue;

            // Now we know that i_v1 and i_v1_next lie (close enough) on the
            // line of i_v0 and i_v0_next
            
            UFix32 edgelength = norm_Euclidean(diff(Vs[i_v0_next], Vs[i_v0]));

            // Sanity check
            if (edgelength <= EdgeThickness) continue;
            
            edgelength -= EdgeThickness;
            
            if (norm_Euclidean(diff(Vs[i_v1], Vs[i_v0])) <= edgelength
                &&
                norm_Euclidean(diff(Vs[i_v1], Vs[i_v0_next])) <= edgelength) {
                tjs[num_tjs++] = Vs[i_v1];
                
                //i_v0_next = i_v0+1;
                //edgelength = norm_Euclidean(diff(Vs[i_v0_next], Vs[i_v0]));
            }
            
            if (norm_Euclidean(diff(Vs[i_v1_next], Vs[i_v0])) <= edgelength
                &&
                norm_Euclidean(diff(Vs[i_v1_next], Vs[i_v0_next])) <= edgelength) {
                tjs[num_tjs++] = Vs[i_v1_next];
                
                //i_v0_next = i_v0+1;
                //edgelength = norm_Euclidean(diff(Vs[i_v0_next], Vs[i_v0]));
            }
        }
    }
}

#include "zigil/zigil_time.h"
void find_all_t_junctions(Mesh *mesh) {
    utime_t start = zgl_ClockQuery();
    
    num_tjs = 0;
    FindTJunctions_EdgeWise(mesh, (UFix32)(1<<14));

    utime_t end = zgl_ClockQuery();
    epm_Log(LT_INFO, "Found %zu t-junctions in %s seconds", num_tjs, fmt_fix_d(((end-start)<<16)/1000, 16, 4));

    //exit(0);
}

void FillTJunctions(Mesh *mesh, Face *f0, Face *f1, UFix32 const EdgeThickness) {
    uint32_t i_v0s[3] = {(uint32_t)f0->i_v[0], (uint32_t)f0->i_v[1], (uint32_t)f0->i_v[2]};
    uint32_t i_v1s[3] = {(uint32_t)f1->i_v[0], (uint32_t)f1->i_v[1], (uint32_t)f1->i_v[2]};
    
    for (uint32_t i_i_v0 = 0; i_i_v0 < 3; i_i_v0++) {
        uint32_t i_v0 = i_v0s[i_i_v0];
        uint32_t i_v0_next = i_v0s[i_i_v0 < 2 ? i_i_v0 + 1 : 0];
        
        WorldVec *Vs = mesh->vertices;
        WorldPlane edgeplane;
        WorldPlane polyplane;
        
        plane_normal(&edgeplane.normal,
                     Vs + i_v0,
                     Vs + i_v0_next,
                     &diff(Vs[i_v0], f0->normal));
        edgeplane.ref = Vs[i_v0];
        polyplane = (WorldPlane){Vs[i_v0], f0->normal, 0};

        edgeplane.constant =
            (Fix64)x_of(edgeplane.ref)*(Fix64)x_of(edgeplane.normal) +
            (Fix64)y_of(edgeplane.ref)*(Fix64)y_of(edgeplane.normal) +
            (Fix64)z_of(edgeplane.ref)*(Fix64)z_of(edgeplane.normal);

        polyplane.constant =
            (Fix64)x_of(polyplane.ref)*(Fix64)x_of(polyplane.normal) +
            (Fix64)y_of(polyplane.ref)*(Fix64)y_of(polyplane.normal) +
            (Fix64)z_of(polyplane.ref)*(Fix64)z_of(polyplane.normal);

        for (uint32_t i_i_v1 = 0; i_i_v1 < 3; i_i_v1++) {
            uint32_t i_v1 = i_v1s[i_i_v1];
            uint32_t i_v1_next = i_v1s[i_i_v1 < 2 ? i_i_v1 + 1 : 0];

            if ((UFix32)ABS(signdist(Vs[i_v1], polyplane)) > EdgeThickness)
                continue;
            if ((UFix32)ABS(signdist(Vs[i_v1_next], polyplane)) > EdgeThickness)
                continue;
            if ((UFix32)ABS(signdist(Vs[i_v1], edgeplane)) > EdgeThickness)
                continue;
            if ((UFix32)ABS(signdist(Vs[i_v1_next], edgeplane)) > EdgeThickness)
                continue;

            // Now we know that i_v1 and i_v1_next lie (close enough) on the
            // line of i_v0 and i_v0_next
            
            UFix32 LengthE0 = norm_Euclidean(diff(Vs[i_v0_next], Vs[i_v0]));

            // Sanity check
            if (LengthE0 <= EdgeThickness) continue;

            LengthE0 -= EdgeThickness;
            
            if (norm_Euclidean(diff(Vs[i_v1], Vs[i_v0])) <= LengthE0
                &&
                norm_Euclidean(diff(Vs[i_v1], Vs[i_v0_next])) <= LengthE0) {
                // TODO: Replace f0 by two new faces f0_0 and f0_1.
                // f0_0 : i_v0_prev -> i_v0 -> i_v1
                // f0_0 : i_v0_prev -> i_v1 -> i_v0_next
                
                i_v0_next = i_v0+1;
                LengthE0 = norm_Euclidean(diff(Vs[i_v0_next], Vs[i_v0]));
            }

            if (norm_Euclidean(diff(Vs[i_v1_next], Vs[i_v0])) <= LengthE0
                &&
                norm_Euclidean(diff(Vs[i_v1_next], Vs[i_v0_next])) <= LengthE0) {
                // TODO: Replace f0 by two new faces f0_0 and f0_1.
                // f0_0 : i_v0_prev -> i_v0 -> i_v1_next
                // f0_0 : i_v0_prev -> i_v1_next -> i_v0_next
                
                i_v0_next = i_v0+1;
                LengthE0 = norm_Euclidean(diff(Vs[i_v0_next], Vs[i_v0]));
            }
        }
    }
}

void fix_all_t_junctions(Mesh *mesh) {
    for (size_t i_obj_face = 0; i_obj_face < mesh->num_faces; i_obj_face++) {
        for (size_t i_sbj_face = 0; i_sbj_face < mesh->num_faces; i_sbj_face++) {
            FillTJunctions(mesh,
                           mesh->faces + i_obj_face,
                           mesh->faces + i_sbj_face,
                           (UFix32)(1<<14));
        }
    }
}


/////////////////////////////////////////////////////////////////////////////


LinkedMesh *epm_LinkifyMesh(Mesh *mesh) {
    LinkedMesh *lmesh = zgl_Malloc(sizeof(*lmesh));
    
    return NULL;    
}
