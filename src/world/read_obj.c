#include "zigil/diblib_local/dibhash.h"
#include "src/misc/epm_includes.h"
#include "src/world/mesh.h"
#include "src/draw/textures.h"
#include <inttypes.h>

//#define VERBOSITY
#include "zigil/diblib_local/verbosity.h"

typedef struct obj_Vertex {
    double x;
    double y;
    double z;
    double w;
} obj_Vertex;

typedef struct obj_VertexNormal {
    double i;
    double j;
    double k;
} obj_VertexNormal;

typedef struct obj_Texel {
    double u;
    double v;
} obj_Texel;

typedef struct obj_Face {
    size_t num_v;
    int32_t v[4]; // hope to god I don't encounter a 5+ sided polygon
    
    bool has_vt;
    int32_t vt[4];
    
    bool has_vn;
    int32_t vn[4];
} obj_Face;

static obj_Vertex *g_Vs;
static obj_VertexNormal *g_VNs;
static obj_Texel *g_VTs;
static obj_Face *g_Fs;

typedef enum TypeGroup {
    TG_VERTEXDATA,
    TG_FREEFORM_ATTR,
    TG_ELEMENTS,
    TG_FREEFORM_BODY,
    TG_CONNECT,
    TG_GROUPING,
    TG_RENDER_ATTR
} TypeGroup;

typedef enum Type {
    T_V,
    T_VT,
    T_VN,
    T_VP,

    T_CSTYPE,
    T_DEG,
    T_BMAT,
    T_STEP,

    T_P,
    T_L,
    T_F,
    T_CURV,
    T_CURV2,
    T_SURF,

    T_PARM,
    T_TRIM,
    T_HOLE,
    T_SCRV,
    T_SP,
    T_END,

    T_CON,

    T_G,
    T_S,
    T_MG,
    T_O,

    T_BEVEL,
    T_C_INTERP,
    T_D_INTERP,
    T_LOD,
    T_USEMTL,
    T_MTLLIB,
    T_SHADOW_OBJ,
    T_TRACE_OBJ,
    T_CTECH,
    T_STECH,

    T_COMMENT,
    
    NUM_T
} Type;

static char *keywords[NUM_T] = {
    // Vertex data
    [T_V] = "v",
    [T_VT] = "vt",
    [T_VN] = "vn",
    [T_VP] = "vp",
    
    // Free-form curve/surface attributes
    [T_CSTYPE] = "cstype",
    [T_DEG] = "deg",
    [T_BMAT] = "bmat",
    [T_STEP] = "step",
    
    // Elements
    [T_P] = "p",
    [T_L] = "l",
    [T_F] = "f",
    [T_CURV] = "curv",
    [T_CURV2] = "curv2",
    [T_SURF] = "surf",
    
    // Free-form curve/surface body statements
    [T_PARM] = "parm",
    [T_TRIM] = "trim",
    [T_HOLE] = "hole",
    [T_SCRV] = "scrv",
    [T_SP] = "sp",
    [T_END] = "end",
    
    // Connectivity between free-form surfaces
    [T_CON] = "con",
    
    // Grouping
    [T_G] = "g",
    [T_S] = "s",
    [T_MG] = "mg",
    [T_O] = "o",

    // Display/render attributes
    [T_BEVEL] = "bevel",
    [T_C_INTERP] = "c_interp",
    [T_D_INTERP] = "d_interp",
    [T_LOD] = "lod",
    [T_USEMTL] = "usemtl",
    [T_MTLLIB] = "mtllib",
    [T_SHADOW_OBJ] = "shadow_obj",
    [T_TRACE_OBJ] = "trace_obj",
    [T_CTECH] = "ctech",
    [T_STECH] = "stech",
    [T_COMMENT] = "#"
};

typedef void (*fn_read)(void);

static void read_v(void);
static void read_vt(void);
static void read_vn(void);
static void read_f(void);
static void read_g(void);
static void read_o(void);

static fn_read read_functions[NUM_T] = {
    [T_V] = read_v,
    [T_VT] = read_vt,
    [T_VN] = read_vn,
    [T_F] = read_f,
    [T_G] = read_g,
    [T_O] = read_o,   
};

static void convert_to_mesh(Mesh *mesh);
static Index *type_index;

void init_OBJ_reader(void) {
    type_index = create_Index(0, NULL, "OBJ Types", 0);

    for (int i_type = 0; i_type < NUM_T; i_type++) {
        Index_insert(type_index, (StrInt){keywords[i_type], i_type});
    }
}


static size_t g_num_v;
static size_t g_num_vt;
static size_t g_num_vn;
static size_t g_num_f;
static size_t g_num_g;
static size_t g_num_o;

static size_t g_avl_v;
static size_t g_avl_vt;
static size_t g_avl_vn;
static size_t g_avl_f;
//static size_t g_avl_g;
//static size_t g_avl_o;

#include "src/system/dir.h"
#define MAX_LINE_LEN 512
#define MAX_TOKENS_PER_LINE 64
static char buf[MAX_LINE_LEN];
static size_t num_tokens;
static char *tokens[MAX_TOKENS_PER_LINE];
static FILE *in_fp;

static void store_v(obj_Vertex *v) {
    g_Vs[g_num_v] = *v;
    g_num_v++;

    if (g_num_v >= g_avl_v) {
        g_avl_v *= 2;
        g_Vs = zgl_Realloc(g_Vs, g_avl_v*sizeof(*g_Vs));
    }
}

static void store_vt(obj_Texel *vt) {
    g_VTs[g_num_vt] = *vt;
    g_num_vt++;

    if (g_num_vt >= g_avl_vt) {
        g_avl_vt *= 2;
        g_VTs = zgl_Realloc(g_VTs, g_avl_vt*sizeof(*g_VTs));
    }
}

static void store_vn(obj_VertexNormal *vn) {
    g_VNs[g_num_vn] = *vn;
    g_num_vn++;

    if (g_num_vn >= g_avl_vn) {
        g_avl_vn *= 2;
        g_VNs = zgl_Realloc(g_VNs, g_avl_vn*sizeof(*g_VNs));
    }
}

static void store_f(obj_Face *f) {
    g_Fs[g_num_f] = *f;
    g_num_f++;

    if (g_num_f >= g_avl_f) {
        g_avl_f *= 2;
        g_Fs = zgl_Realloc(g_Fs, g_avl_f*sizeof(*g_Fs));
    }
}

static void read_v(void) {
    obj_Vertex v;

    if (num_tokens != 4 && num_tokens != 5) {
        epm_Log(LT_WARN, "Bad vertex data.\n");
        return;
    }
    
    v.x = strtod(tokens[1], NULL);
    v.y = strtod(tokens[2], NULL);
    v.z = strtod(tokens[3], NULL);

    if (num_tokens == 5) {
        v.w = strtod(tokens[4], NULL);
    }

    store_v(&v);
}

static void read_vt(void) {
    obj_Texel vt;

    if (num_tokens != 3 && num_tokens != 4) {
        epm_Log(LT_WARN, "Bad texel data.\n");
        return;
    }
    
    vt.u = strtod(tokens[1], NULL);
    vt.v = strtod(tokens[2], NULL);

    store_vt(&vt);
}

static void read_vn(void) {
    obj_VertexNormal vn;

    if (num_tokens != 4) {
        epm_Log(LT_WARN, "Bad vertex normal data.\n");
        return;
    }
    
    vn.i = strtod(tokens[1], NULL);
    vn.j = strtod(tokens[2], NULL);
    vn.k = strtod(tokens[3], NULL);
    
    store_vn(&vn);
}

static inline size_t num_slashes(char *str) {
    size_t count = 0;
    
    while (*str != '\0') {
        if (*str == '/') count++;
        str++;
    }

    return count;
}

int classify(char *str) {
    size_t count = num_slashes(str);
    
    switch (count) {
    case 0: // V
        return 0;
    case 1: // V/T
        return 1;
    case 2: {
        if (str[strcspn(str, "/")+1] == '/') { // V//N
            return 2;
        }
        else { // V/T/N
            return 3;
        }
    }
    default:
        dibassert(false);
    }
}

static void read_f(void) {
    obj_Face f;

    f.num_v = num_tokens-1;
    
    if (f.num_v < 3) {
        epm_Log(LT_WARN, "Bad face data.");
        return;
    }

    if (f.num_v > 4) {
        epm_Log(LT_WARN, "Polygons with more than 4 sides unsupported.");
        return;
    }

    size_t code = classify(tokens[1]);
    
    switch (code) {
    case 0:
        f.has_vt = false;
        f.has_vn = false;
        for (size_t i = 0; i < f.num_v; i++) {
            f.v[i] = atoi(tokens[i+1]);
        }
        break;
    case 1:
        f.has_vt = true;
        f.has_vn = false;
        for (size_t i = 0; i < f.num_v; i++) {
            sscanf(tokens[i+1], "%"SCNi32"/%"SCNi32, &f.v[i], &f.vt[i]);
        }
        break;
    case 2:
        f.has_vt = false;
        f.has_vn = true;
        for (size_t i = 0; i < f.num_v; i++) {
            sscanf(tokens[i+1], "%"SCNi32"//%"SCNi32, &f.v[i], &f.vn[i]);
        }
        break;
    case 3:
        f.has_vt = true;
        f.has_vn = true;
        for (size_t i = 0; i < f.num_v; i++) {
            sscanf(tokens[i+1],
                   "%"SCNi32"/%"SCNi32"/%"SCNi32,
                   &f.v[i], &f.vt[i], &f.vn[i]);
        }
        break;
    default: // ERROR
        epm_Log(LT_WARN, "Bad face data.");
        return;
    }
    
    store_f(&f);
}

static void read_g(void) {
    g_num_g++;
}

static void read_o(void) {
    g_num_o++;
}

static void tokenize_buf(void) {
    buf[strcspn(buf, "\n")] = '\0';
    num_tokens = 0;
        
    char *token;
    token = strtok(buf, " ");
    while (token != NULL) {
        tokens[num_tokens] = token;
        num_tokens++;

        token = strtok(NULL, " ");
    }
}

static char *g_default_texture_name = "grass256";
static char *skytex_name = "sky";
static bool tmp = false;

Mesh *read_OBJ(Mesh *p_mesh, char const *filename) {
    if (streq(filename, "skybox")) tmp = true;
    char path[256] = {'\0'};
    strcat(path, DIR_OBJ);
    strcat(path, filename);
    strcat(path, SUF_OBJ);

    epm_Log(LT_INFO, "Reading Wavefront OBJ file: %s", path);
    
    in_fp = fopen(path, "rb");
    if ( ! in_fp) {
        epm_Log(LT_WARN, "File not found: %s", path);
        return NULL;
    }

    g_num_v  = 0;
    g_num_vt = 0;
    g_num_vn = 0;
    g_num_f  = 0;
    g_num_g  = 0;
    g_num_o  = 0;

    g_avl_v  = 1024;
    g_avl_vt = 1024;
    g_avl_vn = 1024;
    g_avl_f  = 1024;
            
    g_Vs  = zgl_Malloc(g_avl_v*sizeof(*g_Vs));
    g_VTs = zgl_Malloc(g_avl_vt*sizeof(*g_VTs));
    g_VNs = zgl_Malloc(g_avl_vn*sizeof(*g_VNs));
    g_Fs  = zgl_Malloc(g_avl_f*sizeof(*g_Fs));

   
    char *typestr;
    size_t i_type;

    while (fgets(buf, MAX_LINE_LEN, in_fp)) {
        tokenize_buf();
        
        if (num_tokens < 1) continue;
        
        // proceed depending on first token.
        typestr = tokens[0];
        if (typestr[0] == '#') continue;
            
        if (Index_lookup(type_index, typestr, &i_type)) {
            if (read_functions[i_type]) read_functions[i_type]();
        }
        else {
            vbs_fprintf(stdout, "Unknown type \"%s\".", typestr);
            continue;
        }
    }
    
    fclose(in_fp);

    if (! p_mesh) {
        p_mesh = zgl_Malloc(sizeof(*p_mesh));
    }
        
    convert_to_mesh(p_mesh);

    zgl_Free(g_Vs);
    zgl_Free(g_VNs);
    zgl_Free(g_VTs);
    zgl_Free(g_Fs);

#ifdef VERBOSITY
    print_Mesh(p_mesh);
#endif
    
    return p_mesh;
}

static inline bool eq(WorldVec u, WorldVec v) {
    return (x_of(u) == x_of(v) &&
            y_of(u) == y_of(v) &&
            z_of(u) == z_of(v));
}

static void epm_PruneDegenerates(Mesh *p_mesh) {
    bool *degen_vec = zgl_Malloc(p_mesh->num_faces*sizeof(*degen_vec));

    for (size_t i_f = 0; i_f < p_mesh->num_faces; i_f++) {
        Face *f = p_mesh->faces + i_f;
        WorldVec v0 = p_mesh->vertices[f->i_v[0]];
        WorldVec v1 = p_mesh->vertices[f->i_v[1]];
        WorldVec v2 = p_mesh->vertices[f->i_v[2]];

        if (eq(v0, v1) || eq(v1, v2) || eq(v2, v0)) {
            degen_vec[i_f] = true;
        }
        else {
            degen_vec[i_f] = false;
        }

        //printf("PING %zu\n", i_f);
    }
    
    size_t num_f = 0;
    for (size_t i_f = 0; i_f < p_mesh->num_faces; i_f++) {
        if (degen_vec[i_f] == true) continue;

        p_mesh->faces[num_f] = p_mesh->faces[i_f];
        num_f++;
    }

    if (num_f != p_mesh->num_faces) {
        p_mesh->num_faces = num_f;
        p_mesh->faces = zgl_Realloc(p_mesh->faces, num_f*sizeof(*p_mesh->faces));
    }

    zgl_Free(degen_vec);
}

static void convert_to_mesh(Mesh *p_mesh) {
    char *texture_name;
    
    if (tmp) texture_name = skytex_name;
    else texture_name = g_default_texture_name;
    tmp = false;

    p_mesh->num_vertices = g_num_v;
    p_mesh->vertices = zgl_Malloc(g_num_v*sizeof(*p_mesh->vertices));
    
    for (size_t i_v = 0; i_v < g_num_v; i_v++) {
        x_of(p_mesh->vertices[i_v]) =  (Fix32)(g_Vs[i_v].x * FIX_P16_ONE);
        y_of(p_mesh->vertices[i_v]) = -(Fix32)(g_Vs[i_v].z * FIX_P16_ONE);
        z_of(p_mesh->vertices[i_v]) =  (Fix32)(g_Vs[i_v].y * FIX_P16_ONE);
    }

    size_t num_tri = 0, num_quad = 0;
    for (size_t i_f = 0; i_f < g_num_f; i_f++) {
        if (g_Fs[i_f].num_v == 3) {
            num_tri++;
        }
        else if (g_Fs[i_f].num_v == 4){
            num_quad++;
        }
        else {
            dibassert(false);
        }
    }
    
    p_mesh->num_faces = num_tri + 2*num_quad;
    p_mesh->faces = zgl_Malloc(p_mesh->num_faces*sizeof(*p_mesh->faces));

    vbs_printf("(v %zu)\n"
               "(vt %zu)\n"
               "(vn %zu)\n"
               "(f %zu)\n"
               "(f2 %zu)\n",
               g_num_v,
               g_num_vt,
               g_num_vn,
               g_num_f,
               num_tri + 2*num_quad
               );

    size_t i_tri = 0;
    for (size_t i_f = 0; i_f < g_num_f; i_f++) {
        if (g_Fs[i_f].num_v == 3) {
            Face f;
            f.i_v[0] = g_Fs[i_f].v[0] - 1;
            f.i_v[1] = g_Fs[i_f].v[1] - 1;
            f.i_v[2] = g_Fs[i_f].v[2] - 1;

            f.i_tex = epm_TextureIndexFromName(texture_name);
            if (g_Fs[i_f].has_vt) {
                obj_Face *ogl_f = g_Fs + i_f;
                f.vtxl[0].x = (Fix32)(g_VTs[ogl_f->vt[0] - 1].u * 296 * FIX_P16_ONE);
                f.vtxl[0].y = (Fix32)((1.0 - g_VTs[ogl_f->vt[0] - 1].v) * 194 * FIX_P16_ONE);
                f.vtxl[1].x = (Fix32)(g_VTs[ogl_f->vt[1] - 1].u * 296 * FIX_P16_ONE);
                f.vtxl[1].y = (Fix32)((1.0 - g_VTs[ogl_f->vt[1] - 1].v) * 194 * FIX_P16_ONE);
                f.vtxl[2].x = (Fix32)(g_VTs[ogl_f->vt[2] - 1].u * 296 * FIX_P16_ONE);
                f.vtxl[2].y = (Fix32)((1.0 - g_VTs[ogl_f->vt[2] - 1].v) * 194 * FIX_P16_ONE);
            
            }
            else {
                epm_Texture *tex = &textures[f.i_tex];
                f.vtxl[0] = (Fix32Vec_2D){(int)(tex->w << 16) - 1, 0};
                f.vtxl[1] = (Fix32Vec_2D){0, 0};
                f.vtxl[2] = (Fix32Vec_2D){0, (int)(tex->h << 16) - 1};    
            }
        

            f.flags = 0;
            f.brushface = NULL;
        
            p_mesh->faces[i_tri++] = f;
        }
        else if (g_Fs[i_f].num_v == 4) {
            Face f;
            int i0, i1, i2;
            
            {
                i0 = 0, i1 = 1, i2 = 2;
                f.i_v[0] = g_Fs[i_f].v[i0] - 1;
                f.i_v[1] = g_Fs[i_f].v[i1] - 1;
                f.i_v[2] = g_Fs[i_f].v[i2] - 1;

                f.i_tex = epm_TextureIndexFromName(texture_name);
                if (g_Fs[i_f].has_vt) {
                    obj_Face *ogl_f = g_Fs + i_f;
                    f.vtxl[0].x = (Fix32)(g_VTs[ogl_f->vt[i0] - 1].u * 296 * FIX_P16_ONE);
                    f.vtxl[0].y = (Fix32)((1.0 - g_VTs[ogl_f->vt[i0] - 1].v) * 194 * FIX_P16_ONE);
                    f.vtxl[1].x = (Fix32)(g_VTs[ogl_f->vt[i1] - 1].u * 296 * FIX_P16_ONE);
                    f.vtxl[1].y = (Fix32)((1.0 - g_VTs[ogl_f->vt[i1] - 1].v) * 194 * FIX_P16_ONE);
                    f.vtxl[2].x = (Fix32)(g_VTs[ogl_f->vt[i2] - 1].u * 296 * FIX_P16_ONE);
                    f.vtxl[2].y = (Fix32)((1.0 - g_VTs[ogl_f->vt[i2] - 1].v) * 194 * FIX_P16_ONE);
                }
                else {
                    epm_Texture *tex = &textures[f.i_tex];
                    f.vtxl[0] = (Fix32Vec_2D){(int)(tex->w << 16) - 1, 0};
                    f.vtxl[1] = (Fix32Vec_2D){0, 0};
                    f.vtxl[2] = (Fix32Vec_2D){0, (int)(tex->h << 16) - 1};    
                }

                f.flags = 0;
                f.brushface = NULL;
        
                p_mesh->faces[i_tri++] = f;
            }
            /////////////////////////////////////////

            {
                i0 = 2, i1 = 3, i2 = 0;
                
                f.i_v[0] = g_Fs[i_f].v[i0] - 1;
                f.i_v[1] = g_Fs[i_f].v[i1] - 1;
                f.i_v[2] = g_Fs[i_f].v[i2] - 1;

                f.i_tex = epm_TextureIndexFromName(texture_name);
                if (g_Fs[i_f].has_vt) {
                    
                    obj_Face *ogl_f = g_Fs + i_f;                    
                    f.vtxl[0].x = (Fix32)(g_VTs[ogl_f->vt[i0] - 1].u * 296 * FIX_P16_ONE);
                    f.vtxl[0].y = (Fix32)((1.0 - g_VTs[ogl_f->vt[i0] - 1].v) * 194 * FIX_P16_ONE);
                    
                    f.vtxl[1].x = (Fix32)(g_VTs[ogl_f->vt[i1] - 1].u * 296 * FIX_P16_ONE);
                    f.vtxl[1].y = (Fix32)((1.0 - g_VTs[ogl_f->vt[i1] - 1].v) * 194 * FIX_P16_ONE);
                    f.vtxl[2].x = (Fix32)(g_VTs[ogl_f->vt[i2] - 1].u * 296 * FIX_P16_ONE);
                    f.vtxl[2].y = (Fix32)((1.0 - g_VTs[ogl_f->vt[i2] - 1].v) * 194 * FIX_P16_ONE);


                }
                else {
                    epm_Texture *tex = &textures[f.i_tex];
                    f.vtxl[0] = (Fix32Vec_2D){(int)(tex->w << 16) - 1, 0};
                    f.vtxl[1] = (Fix32Vec_2D){0, 0};
                    f.vtxl[2] = (Fix32Vec_2D){0, (int)(tex->h << 16) - 1};    
                }

                f.flags = 0;
                f.brushface = NULL;
        
                p_mesh->faces[i_tri++] = f;
            }

        }
        else {
            dibassert(false);
        }
    }

    epm_PruneDegenerates(p_mesh);    
    epm_ComputeFaceNormals(p_mesh->vertices, p_mesh->num_faces, p_mesh->faces);
    epm_ComputeFaceBrightnesses(p_mesh->num_faces, p_mesh->faces);

    epm_Result res =
        epm_ComputeEdgesFromFaces(p_mesh->num_faces, p_mesh->faces,
                                  &p_mesh->num_edges, &p_mesh->edges);

    if (res == EPM_ERROR) {
        epm_Log(LT_ERROR, "Maximum number of edges smaller than actual number of edges.\n");
        exit(0); // TODO: Handle properly.
    }

    epm_ComputeAABB(p_mesh);
}
