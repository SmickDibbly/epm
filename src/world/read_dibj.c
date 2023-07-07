#include "src/locallibs/dibhash.h"
#include "src/misc/epm_includes.h"
#include "src/world/mesh.h"
#include "src/draw/textures.h"

extern size_t read_fix_x(char const *str, Fix32 *out_num);
static void convert_to_mesh(Mesh *p_mesh);
    
typedef struct dibj_Vertex {
    Fix32 x;
    Fix32 y;
    Fix32 z;
} dibj_Vertex;

typedef struct dibj_VertexNormal {
    Fix32 i;
    Fix32 j;
    Fix32 k;
} dibj_VertexNormal;

typedef struct dibj_Texel {
    Fix32 u;
    Fix32 v;
} dibj_Texel;

typedef struct dibj_Face {
    uint32_t v1;
    uint32_t v2;
    uint32_t v3;

    bool has_vt;
    uint32_t vt1;
    uint32_t vt2;
    uint32_t vt3;

    bool has_vn;
    uint32_t vn1;
    uint32_t vn2;
    uint32_t vn3;
} dibj_Face;

static dibj_Vertex *g_Vs;
static dibj_VertexNormal *g_VNs;
static dibj_Texel *g_VTs;
static dibj_Face *g_Fs;

typedef enum Type {
    T_V,
    T_VT,
    T_VN,
    T_F,

    NUM_T
} Type;

static char *keywords[NUM_T] = {
    // Vertex data
    [T_V] = "v",
    [T_VT] = "vt",
    [T_VN] = "vn",
    [T_F] = "f",
};

typedef void (*fn_read)(void);

static void read_v(void);
static void read_vt(void);
static void read_vn(void);
static void read_f(void);

static fn_read read_functions[NUM_T] = {
    [T_V] = read_v,
    [T_VT] = read_vt,
    [T_VN] = read_vn,
    [T_F] = read_f,
};

static Index *type_index;

static bool g_init = false;
static void init_DIBJ_reader(void) {
    dibassert(g_init = false);
    
    type_index = create_Index(0, NULL, "OBJ Types", 0);

    for (int i_type = 0; i_type < NUM_T; i_type++) {
        Index_insert(type_index, (StrInt){keywords[i_type], i_type});
    }
}

static size_t g_num_v;
static size_t g_num_vt;
static size_t g_num_vn;
static size_t g_num_f;
static size_t g_avl_v;
static size_t g_avl_vt;
static size_t g_avl_vn;
static size_t g_avl_f;


#include "src/system/dir.h"
#define MAX_LINE_LEN 512
#define MAX_TOKENS_PER_LINE 64
static char buf[MAX_LINE_LEN];
static size_t num_tokens;
static char *tokens[MAX_TOKENS_PER_LINE];
static FILE *in_fp;

static void store_v(dibj_Vertex *v) {
    g_Vs[g_num_v] = *v;
    g_num_v++;

    if (g_num_v >= g_avl_v) {
        g_avl_v *= 2;
        g_Vs = zgl_Realloc(g_Vs, g_avl_v*sizeof(*g_Vs));
    }
}

static void store_vt(dibj_Texel *vt) {
    g_VTs[g_num_vt] = *vt;
    g_num_vt++;

    if (g_num_vt >= g_avl_vt) {
        g_avl_vt *= 2;
        g_VTs = zgl_Realloc(g_VTs, g_avl_vt*sizeof(*g_VTs));
    }
}

static void store_vn(dibj_VertexNormal *vn) {
    g_VNs[g_num_vn] = *vn;
    g_num_vn++;

    if (g_num_vn >= g_avl_vn) {
        g_avl_vn *= 2;
        g_VNs = zgl_Realloc(g_VNs, g_avl_vn*sizeof(*g_VNs));
    }
}

static void store_f(dibj_Face *f) {
    g_Fs[g_num_f] = *f;
    g_num_f++;

    if (g_num_f >= g_avl_f) {
        g_avl_f *= 2;
        g_Fs = zgl_Realloc(g_Fs, g_avl_f*sizeof(*g_Fs));
    }
}

static void read_v(void) {
    dibj_Vertex v;

    if (num_tokens != 4) {
        epm_Log(LT_WARN, "Bad vertex data.\n");
        return;
    }
    
    read_fix_x(tokens[1], &v.x);
    read_fix_x(tokens[2], &v.y);
    read_fix_x(tokens[3], &v.z);

    store_v(&v);
}

static void read_vt(void) {
    dibj_Texel vt;

    if (num_tokens != 3) {
        epm_Log(LT_WARN, "Bad texel data.\n");
        return;
    }
    
    read_fix_x(tokens[1], &vt.u);
    read_fix_x(tokens[2], &vt.v);

    store_vt(&vt);
}

static void read_vn(void) {
    dibj_VertexNormal vn;

    if (num_tokens != 4) {
        epm_Log(LT_WARN, "Bad vertex normal data.\n");
        return;
    }
    
    read_fix_x(tokens[1], &vn.i);
    read_fix_x(tokens[2], &vn.j);
    read_fix_x(tokens[3], &vn.k);
    
    store_vn(&vn);
}

static void read_f(void) {
    dibj_Face f;

    if (num_tokens < 4) {
        epm_Log(LT_WARN, "Bad face data.\n");
        return;
    }

    
    sscanf(tokens[1], "%u/%u/%u", &f.v1, &f.vt1, &f.vn1);
    sscanf(tokens[2], "%u/%u/%u", &f.v2, &f.vt2, &f.vn2);
    sscanf(tokens[3], "%u/%u/%u", &f.v3, &f.vt3, &f.vn3);

    f.has_vt = true;
    f.has_vn = true;
    
    /*
    f.v1 = atol(tokens[1]);
    f.v2 = atol(tokens[2]);
    f.v3 = atol(tokens[3]);

    f.has_vt = false;
    f.has_vn = false;
    */

    
    store_f(&f);
    
    /*    
    for (int i = 1; i < (int)num_tokens; i++) {
        char *str = tokens[i];
    
    }
    */
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

Mesh *read_DIBJ(Mesh *p_mesh, char const *filename) {
    if (g_init == false) {
        init_DIBJ_reader();
        g_init = true;
    }
    
    char path[256] = {'\0'};
    strcat(path, DIR_OBJ);
    strcat(path, filename);
    strcat(path, SUF_OBJ);

    epm_Log(LT_INFO, "Reading DIBJ file: %s", path);
    
    in_fp = fopen(path, "rb");
    if ( ! in_fp) {
        epm_Log(LT_WARN, "File not found: %s", path);
        return NULL;
    }

    g_num_v  = 0;
    g_num_vt = 0;
    g_num_vn = 0;
    g_num_f  = 0;

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
            fprintf(stdout, "Unknown type \"%s\".", typestr);
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
    
    print_Mesh(p_mesh);

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
    p_mesh->num_vertices = g_num_v;
    p_mesh->vertices = zgl_Malloc(g_num_v*sizeof(*p_mesh->vertices));
    
    for (size_t i_v = 0; i_v < g_num_v; i_v++) {
        x_of(p_mesh->vertices[i_v]) =  (Fix32)(g_Vs[i_v].x * FIX_P16_ONE);
        y_of(p_mesh->vertices[i_v]) = -(Fix32)(g_Vs[i_v].z * FIX_P16_ONE);
        z_of(p_mesh->vertices[i_v]) =  (Fix32)(g_Vs[i_v].y * FIX_P16_ONE);
    }

    p_mesh->num_faces = g_num_f;
    p_mesh->faces = zgl_Malloc(g_num_f*sizeof(*p_mesh->faces));
    for (size_t i_f = 0; i_f < g_num_f; i_f++) {
        Face f;
        f.i_v[0] = g_Fs[i_f].v1 - 1;
        f.i_v[1] = g_Fs[i_f].v2 - 1;
        f.i_v[2] = g_Fs[i_f].v3 - 1;

        f.i_tex = epm_TextureIndexFromName("quaker");
        if (g_Fs[i_f].has_vt) {
            dibj_Face *ogl_f = g_Fs + i_f;
            f.vtxl[0].x = (Fix32)(g_VTs[ogl_f->vt1 - 1].u * 296 * FIX_P16_ONE);
            f.vtxl[0].y = (Fix32)((1.0 - g_VTs[ogl_f->vt1 - 1].v) * 194 * FIX_P16_ONE);
            f.vtxl[1].x = (Fix32)(g_VTs[ogl_f->vt2 - 1].u * 296 * FIX_P16_ONE);
            f.vtxl[1].y = (Fix32)((1.0 - g_VTs[ogl_f->vt2 - 1].v) * 194 * FIX_P16_ONE);
            f.vtxl[2].x = (Fix32)(g_VTs[ogl_f->vt3 - 1].u * 296 * FIX_P16_ONE);
            f.vtxl[2].y = (Fix32)((1.0 - g_VTs[ogl_f->vt3 - 1].v) * 194 * FIX_P16_ONE);
            
        }
        else {
            epm_Texture *tex = &textures[f.i_tex];
            f.vtxl[0] = (Fix32Vec_2D){(int)(tex->w << 16) - 1, 0};
            f.vtxl[1] = (Fix32Vec_2D){0, 0};
            f.vtxl[2] = (Fix32Vec_2D){0, (int)(tex->h << 16) - 1};    
        }
        

        f.flags = 0;
        f.brushface = NULL;
        
        p_mesh->faces[i_f] = f;
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
