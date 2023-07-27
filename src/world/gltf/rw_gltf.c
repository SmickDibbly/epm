#include "src/world/gltf/cgltf.h"

#include "src/misc/epm_includes.h"
#include "src/system/dir.h"
#include "src/world/mesh.h"
#include "src/draw/textures.h"

//#define VERBOSITY
#include "zigil/diblib_local/verbosity.h"

#ifdef VERBOSITY
static void print_cgltf_data(FILE *out_fp, cgltf_data *data);
#endif

typedef struct Vec3 {
    double x;
    double y;
    double z;
} Vec3;

typedef struct Vec2 {
    double x;
    double y;
} Vec2;

typedef struct Ind3 {
    size_t i_v0;
    size_t i_v1;
    size_t i_v2;
} Ind3;

typedef struct Triangles {
    size_t num_vertices;
    Vec3 *vertices;

    size_t num_texels;
    Vec2 *texels;
    
    size_t num_tris;
    Ind3 *tris;
} Triangles;

static void convert_to_mesh(Mesh *p_mesh);

static size_t buf_len = 0;
static uint8_t buf[16384];

static size_t num_trises = 0;
static Triangles trises[128];

static float unpackFloat(const uint8_t *b, int *i) {
    //    uint32_t temp = 0;
    *i += 4;
    /*
    temp = ((b[3] << 24) |
            (b[2] << 16) |
            (b[1] <<  8) |
             b[0]);
    return *((float *) &temp); // FIXME: strict-alias violation!
    */
    float res;
    memcpy(&res, b, 4);
    return res;
}

Mesh *read_GLTF(Mesh *p_mesh, char const *filename) {
    char path[256] = {'\0'};
    strcat(path, DIR_GLTF);
    strcat(path, filename);
    strcat(path, SUF_GLTF);

    cgltf_options options = {0};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success) {
        epm_Log(LT_WARN, "cgltf failed to parse file: %s", path);
        return NULL;
    }

#ifdef VERBOSITY
    print_cgltf_data(stdout, data);
#endif
    
    ////////////////////////////////////////////////////////////////////////
    
    strcpy(path, DIR_GLTF);
    strcat(path, "SM_Deccer_Cubes.bin");

    FILE *in_fp = fopen(path, "rb");
    int ch;
    int i_buf = 0;
    while ((ch = fgetc(in_fp)) != EOF) {
        buf[i_buf++] = (char)ch;
    }
    buf_len = i_buf;
    fclose(in_fp);

    vbs_fprintf(stdout, "Loaded buffer of length %zu bytes.\n", buf_len);

    num_trises = 0;
    for (size_t i_mesh = 0; i_mesh < data->meshes_count; i_mesh++) {
        cgltf_mesh *mesh = data->meshes + i_mesh;

        for (size_t i_prim = 0; i_prim < mesh->primitives_count; i_prim++) {
            cgltf_primitive *prim = mesh->primitives + i_prim;

            cgltf_accessor *indices = prim->indices;
            dibassert(indices->type == cgltf_type_scalar);
            dibassert(indices->component_type == cgltf_component_type_r_16u);

            cgltf_buffer_view *view = indices->buffer_view;

            size_t i_buf = 0;
            i_buf += indices->offset;
            i_buf += view->offset;
            
            cgltf_size count = indices->count/3;
            cgltf_size stride = indices->stride*3;

            Triangles *tris = trises + num_trises;
            num_trises++;

            tris->num_vertices = 0;
            tris->vertices = zgl_Malloc(1*sizeof(tris->vertices));

            tris->num_texels = 0;
            tris->texels = zgl_Malloc(1*sizeof(tris->texels));
            
            
            tris->num_tris = count;
            tris->tris = zgl_Malloc(count*sizeof(*tris->tris));

            for (size_t i = 0; i < count; i++) {
                tris->tris[i].i_v0 = (buf[i_buf+1]<<8) + buf[i_buf+0];
                tris->tris[i].i_v1 = (buf[i_buf+3]<<8) + buf[i_buf+2];
                tris->tris[i].i_v2 = (buf[i_buf+5]<<8) + buf[i_buf+4];
                i_buf += stride;
            }
                
            
            for (size_t i_attr = 0; i_attr < prim->attributes_count; i_attr++) {
                cgltf_attribute *attr = prim->attributes + i_attr;
                cgltf_accessor *access = attr->data;
                
                switch (attr->type) {
                case cgltf_attribute_type_position: {
                    dibassert(access->type == cgltf_type_vec3);
                    dibassert(access->component_type == cgltf_component_type_r_32f);
                    dibassert(access->count == 24);
                
                    cgltf_buffer_view *view = access->buffer_view;

                    size_t i_buf = 0;
                    i_buf += access->offset;
                    i_buf += view->offset;

                    cgltf_size count = access->count;
                    cgltf_size stride = access->stride;
                    int ignore;

                    tris->num_vertices += count;
                    tris->vertices = zgl_Realloc(tris->vertices, tris->num_vertices*sizeof(*tris->vertices));
                
                    for (size_t i = 0; i < count; i++) {
                        tris->vertices[i].x = unpackFloat(buf + i_buf, &ignore);
                        tris->vertices[i].y = unpackFloat(buf + i_buf + 4, &ignore);;
                        tris->vertices[i].z = unpackFloat(buf + i_buf + 8, &ignore);
                        i_buf += stride;
                    }
                }
                    break;
                case cgltf_attribute_type_texcoord: {
                    dibassert(access->type == cgltf_type_vec2);
                    dibassert(access->component_type == cgltf_component_type_r_32f);
                    dibassert(access->count == 24);

                    cgltf_buffer_view *view = access->buffer_view;

                    size_t i_buf = 0;
                    i_buf += access->offset;
                    i_buf += view->offset;

                    cgltf_size count = access->count;
                    cgltf_size stride = access->stride;
                    int ignore;

                    tris->num_texels += count;
                    tris->texels = zgl_Realloc(tris->texels, tris->num_texels*sizeof(*tris->texels));
                
                    for (size_t i = 0; i < count; i++) {
                        tris->texels[i].x = unpackFloat(buf + i_buf, &ignore);
                        tris->texels[i].y = unpackFloat(buf + i_buf + 4, &ignore);;
                        i_buf += stride;
                    }
                }
                    break;
                default:
                    continue;
                }                
            }
        }
    }


    for (size_t i_node = 0; i_node < data->nodes_count; i_node++) {
        cgltf_node *node = data->nodes + i_node;
        cgltf_mesh *mesh = node->mesh;
        size_t i_mesh = mesh - data->meshes;
        
        Triangles *tris = trises + i_mesh;
        
        if (node->has_scale) {    
            for (size_t i_v = 0; i_v < tris->num_vertices; i_v++) {
                tris->vertices[i_v].x *= (double)node->scale[0];
                tris->vertices[i_v].y *= (double)node->scale[1];
                tris->vertices[i_v].z *= (double)node->scale[2];
            }
        }

        if (node->has_translation) {
            for (size_t i_v = 0; i_v < tris->num_vertices; i_v++) {
                tris->vertices[i_v].x += (double)node->translation[0];
                tris->vertices[i_v].y += (double)node->translation[1];
                tris->vertices[i_v].z += (double)node->translation[2];
            }
        }
        
    }


#ifdef VERBOSITY
    for (size_t i = 0; i < num_trises; i++) {
        printf("Triangles Primitive [%zu]: \n", i);
        Triangles *tris = trises + i;
        for (size_t j = 0; j < tris->num_vertices; j++) {
            printf("(%f, %f, %f)\n", tris->vertices[j].x, tris->vertices[j].y, tris->vertices[j].z);
        }

        for (size_t j = 0; j < tris->num_texels; j++) {
            printf("(%f, %f)\n", tris->texels[j].x, tris->texels[j].y);
        }
        
        for (size_t j = 0; j < tris->num_tris; j++) {
            printf("(%zu, %zu, %zu)\n", tris->tris[j].i_v0, tris->tris[j].i_v1, tris->tris[j].i_v2);
        }
    }
#endif
    /*
    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success) {
        epm_Log(LT_WARN, "cgltf failed to parse file: %s", path);
        return NULL;
    }
    */


    //////////////////////////////////////////////////////////////////////
    
    if (! p_mesh) {
        p_mesh = zgl_Malloc(sizeof(*p_mesh));
    }
    
    convert_to_mesh(p_mesh);

    cgltf_free(data);

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
        WorldVec v0 = p_mesh->verts[f->i_v[0]];
        WorldVec v1 = p_mesh->verts[f->i_v[1]];
        WorldVec v2 = p_mesh->verts[f->i_v[2]];
        
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
    p_mesh->num_verts = 0;
    p_mesh->verts = zgl_Malloc(1*sizeof(p_mesh->verts));
    
    p_mesh->num_faces = 0;
    p_mesh->faces = zgl_Malloc(1*sizeof(p_mesh->faces));
    
    for (size_t i_tris = 0; i_tris < num_trises; i_tris++) {
        Triangles *tris = trises + i_tris;

        size_t base_v = p_mesh->num_verts;
        size_t base_f = p_mesh->num_faces;
        
        p_mesh->num_verts += tris->num_vertices;
        p_mesh->verts = zgl_Realloc(p_mesh->verts, p_mesh->num_verts*sizeof(*p_mesh->verts));

        p_mesh->num_faces += tris->num_tris;
        p_mesh->faces = zgl_Realloc(p_mesh->faces, p_mesh->num_faces*sizeof(*p_mesh->faces));
   
        for (size_t i_v = 0; i_v < tris->num_vertices; i_v++) {
            x_of(p_mesh->verts[base_v + i_v]) =  64*(Fix32)(tris->vertices[i_v].x * FIX_P16_ONE);
            y_of(p_mesh->verts[base_v + i_v]) = -64*(Fix32)(tris->vertices[i_v].z * FIX_P16_ONE);
            z_of(p_mesh->verts[base_v + i_v]) =  64*(Fix32)(tris->vertices[i_v].y * FIX_P16_ONE);
        }

        for (size_t i_f = 0; i_f < tris->num_tris; i_f++) {
            Face f;
            size_t i_v0, i_v1, i_v2;
            i_v0 = tris->tris[i_f].i_v0;
            i_v1 = tris->tris[i_f].i_v1;
            i_v2 = tris->tris[i_f].i_v2;
        
            f.i_v[0] = base_v + i_v0;
            f.i_v[1] = base_v + i_v1;
            f.i_v[2] = base_v + i_v2;

            f.i_tex = epm_TextureIndexFromName("squarepave");
            if (true) {
                f.vtxl[0].x = (Fix32)(tris->texels[i_v0].x * 256 * FIX_P16_ONE);
                f.vtxl[0].y = (Fix32)(tris->texels[i_v0].y * 256 * FIX_P16_ONE);
                f.vtxl[1].x = (Fix32)(tris->texels[i_v1].x * 256 * FIX_P16_ONE);
                f.vtxl[1].y = (Fix32)(tris->texels[i_v1].y * 256 * FIX_P16_ONE);
                f.vtxl[2].x = (Fix32)(tris->texels[i_v2].x * 256 * FIX_P16_ONE);
                f.vtxl[2].y = (Fix32)(tris->texels[i_v2].y * 256 * FIX_P16_ONE);
            
            }
            else {
                epm_Texture *tex = &textures[f.i_tex];
                f.vtxl[0] = (Fix32Vec_2D){(int)(tex->w << 16) - 1, 0};
                f.vtxl[1] = (Fix32Vec_2D){0, 0};
                f.vtxl[2] = (Fix32Vec_2D){0, (int)(tex->h << 16) - 1};
            }
        
            f.flags = 0;
            //            f.brushpoly = NULL;
        
            p_mesh->faces[base_f + i_f] = f;
        }
    }
    
    

    epm_PruneDegenerates(p_mesh);
    epm_ComputeFaceNormals(p_mesh->verts, p_mesh->num_faces, p_mesh->faces);
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




///////////////////////////////////////////////////////////////////////////

#ifdef VERBOSITY
static char const * const file_type_strs[] = {
	[cgltf_file_type_invalid] = "invalid",
	[cgltf_file_type_gltf] = "gltf",
	[cgltf_file_type_glb] = "glb",
};

static char const * const buffer_view_type_strs[] = {
	[cgltf_buffer_view_type_invalid] = "invalid",
	[cgltf_buffer_view_type_indices] = "indices",
	[cgltf_buffer_view_type_vertices] = "vertices",
};

static char const * const attribute_type_strs[] = {
	[cgltf_attribute_type_invalid] = "invalid",
	[cgltf_attribute_type_position] = "position",
	[cgltf_attribute_type_normal] = "normal",
	[cgltf_attribute_type_tangent] = "tangent",
	[cgltf_attribute_type_texcoord] = "texcoord",
	[cgltf_attribute_type_color] = "color",
	[cgltf_attribute_type_joints] = "joints",
	[cgltf_attribute_type_weights] = "weights",
	[cgltf_attribute_type_custom] = "custom",
};

static char const * const component_type_strs[] = {
	[cgltf_component_type_invalid] = "invalid",
	[cgltf_component_type_r_8] = "byte", /* BYTE */
	[cgltf_component_type_r_8u] = "unsigned byte", /* UNSIGNED_BYTE */
	[cgltf_component_type_r_16] = "short", /* SHORT */
	[cgltf_component_type_r_16u] = "unsigned short", /* UNSIGNED_SHORT */
	[cgltf_component_type_r_32u] = "unsigned int", /* UNSIGNED_INT */
	[cgltf_component_type_r_32f] = "float", /* FLOAT */
};

static char const * const type_strs[] = {
    [cgltf_type_invalid] = "invalid",
	[cgltf_type_scalar] = "scalar",
	[cgltf_type_vec2] = "vec2",
	[cgltf_type_vec3] = "vec3",
	[cgltf_type_vec4] = "vec4",
	[cgltf_type_mat2] = "mat2",
	[cgltf_type_mat3] = "mat3",
	[cgltf_type_mat4] = "mat4",
};

static char const * const primitive_type_strs[] = {
    [cgltf_primitive_type_points] = "points",
	[cgltf_primitive_type_lines] = "lines",
	[cgltf_primitive_type_line_loop] = "line_loop",
	[cgltf_primitive_type_line_strip] = "line_strip",
	[cgltf_primitive_type_triangles] = "triangles",
	[cgltf_primitive_type_triangle_strip] = "triangle_strip",
	[cgltf_primitive_type_triangle_fan] = "triangle_fan",
};

static char const * const alpha_mode_strs[] = {
	[cgltf_alpha_mode_opaque] = "opaque",
	[cgltf_alpha_mode_mask] = "mask",
	[cgltf_alpha_mode_blend] = "blend",
};

static char const * const animation_path_type_strs[] = {
    [cgltf_animation_path_type_invalid] = "invalid",
	[cgltf_animation_path_type_translation] = "translation",
	[cgltf_animation_path_type_rotation] = "rotation",
	[cgltf_animation_path_type_scale] = "scale",
	[cgltf_animation_path_type_weights] = "weights",
};

static char const * const interpolation_type_strs[] = {
    [cgltf_interpolation_type_linear] = "linear",
	[cgltf_interpolation_type_step] = "step",
	[cgltf_interpolation_type_cubic_spline] = "cubic_spline",
};

static char const * const camera_type_strs[] = {
	[cgltf_camera_type_invalid] = "invalid",
	[cgltf_camera_type_perspective] = "perspective",
	[cgltf_camera_type_orthographic] = "orthographic",
};

static char const * const light_type_strs[] = {
	[cgltf_light_type_invalid] = "invalid",
	[cgltf_light_type_directional] = "directional",
	[cgltf_light_type_point] = "point",
	[cgltf_light_type_spot] = "spot",
};

static char const * const arr_data_free_method_strs[] = {
    [cgltf_data_free_method_none] = "none",
	[cgltf_data_free_method_file_release] = "file_release",
	[cgltf_data_free_method_memory_free] = "memory_free",
};

#define PRI_size "lu"
#define PRI_int "i"

static void print_cgltf_buffer_view(FILE *out_fp, cgltf_buffer_view *view) {
    fprintf(out_fp, "(cgltf_buffer_view)\n");
    fprintf(out_fp, "    (name %s)\n", view->name);
    fprintf(out_fp, "    (buffer %p)\n", (void *)view->buffer);
    fprintf(out_fp, "    (offset %"PRI_size")\n", view->offset);
    fprintf(out_fp, "    (size %"PRI_size")\n", view->size);
    fprintf(out_fp, "    (stride %"PRI_size")\n", view->stride);
    fprintf(out_fp, "    (buffer_view_type %s)\n", buffer_view_type_strs[view->type]);
    fprintf(out_fp, "    (data %p)\n", (void *)view->data);

    fprintf(out_fp, "    (has_meshopt_compression %s)\n", view->has_meshopt_compression ? "true" : "false");
}

static void print_cgltf_accessor(FILE *out_fp, cgltf_accessor *access) {
    fprintf(out_fp, "(cgltf_accessor)\n");
    fprintf(out_fp, "    (name %s)\n", access->name);
    fprintf(out_fp, "    (component_type %s)\n", component_type_strs[access->component_type]);
    fprintf(out_fp, "    (normalized %s)\n", access->normalized ? "true" : "false");
    fprintf(out_fp, "    (type %s)\n", type_strs[access->type]);
    fprintf(out_fp, "    (offset %"PRI_size")\n", access->offset);
    fprintf(out_fp, "    (count %"PRI_size")\n", access->count);
    fprintf(out_fp, "    (stride %"PRI_size")\n", access->stride);
    fprintf(out_fp, "    (buffer_view %p)\n", (void *)access->buffer_view);
    fprintf(out_fp, "    (has_min %s)\n", access->has_min ? "true" : "false");
    fprintf(out_fp, "    (has_max %s)\n", access->has_max ? "true" : "false");
    fprintf(out_fp, "    (is_sparse %s)\n", access->is_sparse ? "true" : "false");

    print_cgltf_buffer_view(out_fp, access->buffer_view);
}

static void print_cgltf_attribute(FILE *out_fp, cgltf_attribute *attr) {
    fprintf(out_fp, "(cgltf_attribute)\n");
    fprintf(out_fp, "    (name %s)\n", attr->name);
    fprintf(out_fp, "    (type %s)\n", attribute_type_strs[attr->type]);
    fprintf(out_fp, "    (index %"PRI_int")\n", attr->index);
    fprintf(out_fp, "    (data %p)\n", (void *)attr->data);
    
    print_cgltf_accessor(out_fp, attr->data);
}

static void print_cgltf_primitive(FILE *out_fp, cgltf_primitive *prim) {
    fprintf(out_fp, "(cgltf_primitive)\n");
    fprintf(out_fp, "    (type %s)\n", primitive_type_strs[prim->type]);
    fprintf(out_fp, "    (indices %p)\n", (void *)prim->indices);
    fprintf(out_fp, "    (material %p)\n", (void *)prim->material);
    fprintf(out_fp, "    (attributes %p)\n", (void *)prim->attributes);
    fprintf(out_fp, "    (attributes_count %"PRI_size")\n", prim->attributes_count);
    fprintf(out_fp, "    (targets %p)\n", (void *)prim->targets);
    fprintf(out_fp, "    (targets_count %"PRI_size")\n", prim->targets_count);

    print_cgltf_accessor(out_fp, prim->indices);
    
    for (size_t i = 0; i < prim->attributes_count; i++) {
        print_cgltf_attribute(out_fp, prim->attributes + i);
    }
}

static void print_cgltf_mesh(FILE *out_fp, cgltf_mesh *mesh) {
    fprintf(out_fp, "(cgltf_mesh)\n");
    fprintf(out_fp, "    (name %s)\n", mesh->name);
    fprintf(out_fp, "    (primitives %p)\n", (void *)mesh->primitives);
    fprintf(out_fp, "    (primitives_count %"PRI_size")\n", mesh->primitives_count);
    fprintf(out_fp, "    (weights %p)\n", (void *)mesh->weights);
    fprintf(out_fp, "    (weights_count %"PRI_size")\n", mesh->weights_count);
    fprintf(out_fp, "    (target_names %p)\n", (void *)mesh->target_names);
    fprintf(out_fp, "    (target_names_count %"PRI_size")\n", mesh->target_names_count);

    for (size_t i = 0; i < mesh->primitives_count; i++) {
        print_cgltf_primitive(out_fp, mesh->primitives + i);
    }
}

static void print_cgltf_data(FILE *out_fp, cgltf_data *data) {
    fprintf(out_fp, "(cgltf_data)\n");
    fprintf(out_fp, "    (file_type %s)\n", file_type_strs[data->file_type]);
    fprintf(out_fp, "    (file_data %p)\n", data->file_data);
    fprintf(out_fp, "    (copyright \"%s\")\n", data->asset.copyright);
    fprintf(out_fp, "    (generator \"%s\")\n", data->asset.generator);
    fprintf(out_fp, "    (version \"%s\")\n", data->asset.version);
    fprintf(out_fp, "    (min_version \"%s\")\n", data->asset.min_version);
    fprintf(out_fp, "    (meshes %p)\n", (void *)data->meshes);
    fprintf(out_fp, "    (meshes_count %"PRI_size")\n", data->meshes_count);
    fprintf(out_fp, "    (materials %p)\n", (void *)data->materials);
    fprintf(out_fp, "    (materials_count %"PRI_size")\n", data->materials_count);
    fprintf(out_fp, "    (accessors %p)\n", (void *)data->accessors);
    fprintf(out_fp, "    (accessors_count %"PRI_size")\n", data->accessors_count);
    fprintf(out_fp, "    (buffer_views %p)\n", (void *)data->buffer_views);
    fprintf(out_fp, "    (buffer_views_count %"PRI_size")\n", data->buffer_views_count);
    fprintf(out_fp, "    (buffers %p)\n", (void *)data->buffers);
    fprintf(out_fp, "    (buffers_count %"PRI_size")\n", data->buffers_count);
    fprintf(out_fp, "    (images %p)\n", (void *)data->images);
    fprintf(out_fp, "    (images_count %"PRI_size")\n", data->images_count);
    fprintf(out_fp, "    (textures %p)\n", (void *)data->textures);
    fprintf(out_fp, "    (textures_count %"PRI_size")\n", data->textures_count);
    fprintf(out_fp, "    (samplers %p)\n", (void *)data->samplers);
    fprintf(out_fp, "    (samplers_count %"PRI_size")\n", data->samplers_count);
    fprintf(out_fp, "    (skins %p)\n", (void *)data->skins);
    fprintf(out_fp, "    (skins_count %"PRI_size")\n", data->skins_count);
    fprintf(out_fp, "    (cameras %p)\n", (void *)data->cameras);
    fprintf(out_fp, "    (cameras_count %"PRI_size")\n", data->cameras_count);
    fprintf(out_fp, "    (lights %p)\n", (void *)data->lights);
    fprintf(out_fp, "    (lights_count %"PRI_size")\n", data->lights_count);
    fprintf(out_fp, "    (nodes %p)\n", (void *)data->nodes);
    fprintf(out_fp, "    (nodes_count %"PRI_size")\n", data->nodes_count);
    fprintf(out_fp, "    (scenes %p)\n", (void *)data->scenes);
    fprintf(out_fp, "    (scenes_count %"PRI_size")\n", data->scenes_count);
    fprintf(out_fp, "    (scene %p)\n", (void *)data->scene);
    fprintf(out_fp, "    (animations %p)\n", (void *)data->animations);
    fprintf(out_fp, "    (animations_count %"PRI_size")\n", data->animations_count);
    fprintf(out_fp, "    (variants %p)\n", (void *)data->variants);
    fprintf(out_fp, "    (variants_count %"PRI_size")\n", data->variants_count);
    fprintf(out_fp, "    (data_extensions %p)\n", (void *)data->data_extensions);
    fprintf(out_fp, "    (data_extensions_count %"PRI_size")\n", data->data_extensions_count);
    fprintf(out_fp, "    (extensions_used %p)\n", (void *)data->extensions_used);
    fprintf(out_fp, "    (extensions_used_count %"PRI_size")\n", data->extensions_used_count);
    fprintf(out_fp, "    (extensions_required %p)\n", (void *)data->extensions_required);
    fprintf(out_fp, "    (extensions_required_count %"PRI_size")\n", data->extensions_required_count);
    fprintf(out_fp, "    (json %p)\n", (void *)data->json);
    fprintf(out_fp, "    (json_size %"PRI_size")\n", data->json_size);
    fprintf(out_fp, "    (bin %p)\n", (void *)data->bin);
    fprintf(out_fp, "    (bin_size %"PRI_size")\n", data->bin_size);

    for (size_t i = 0; i < data->meshes_count; i++) {
        print_cgltf_mesh(out_fp, data->meshes + i);
    }
}
#endif
