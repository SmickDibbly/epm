#include "src/world/brush.h"
#include "src/world/world.h"
#include "src/draw/textures.h"

//#define VERBOSITY
#include "zigil/diblib_local/verbosity.h"

#undef LOG_LABEL
#define LOG_LABEL "BRUSH"

void unlink_brush(Brush *brush) {
    bool found = false;
    BrushNode *node;
    for (node = g_world.geo_brush->head; node; node = node->next) {
        if (node->brush == brush) {
            found = true;
            break;
        }
    }

    if (found) {
        BrushNode *prev = node->prev;
        BrushNode *next = node->next;
        
        if (prev) {
            prev->next = next;
        }
        if (next) {
            next->prev = prev;
        }

        node->prev = node->next = NULL;

        if (node == g_world.geo_brush->head) {
            g_world.geo_brush->head = next;
        }
        if (node == g_world.geo_brush->tail) {
            g_world.geo_brush->tail = prev;
        }
    }
}

void link_brush(Brush *brush) {
    BrushNode *node;
    
    if (g_world.geo_brush->head == NULL) {
        dibassert(g_world.geo_brush->tail == NULL);
        node = g_world.geo_brush->head = zgl_Malloc(sizeof(*node));
        node->next = node->prev = NULL;
        g_world.geo_brush->tail = node;
    }
    else {
        dibassert(g_world.geo_brush->tail != NULL);
        dibassert(g_world.geo_brush->tail->next == NULL);
        node = g_world.geo_brush->tail->next = zgl_Malloc(sizeof(*node));
        node->next = NULL;
        node->prev = g_world.geo_brush->tail;
        g_world.geo_brush->tail = node;
    }

    node->brush = brush;
}


void set_frame_size(BrushType type, /*parameters*/ WorldUnit dx, WorldUnit dy, WorldUnit dz) {
    dibassert(type == BT_CUBOID);

    frame->type = type;
    Cuboid_of(frame)->container = frame;
    init_CuboidBrush(Cuboid_of(frame), dx, dy, dz);
    
    void write_Brush(Brush *brush, FILE *out_fp);
    write_Brush(frame, stdout);
}

void brush_from_frame(int CSG, int BSP, char *name) {
    Brush *brush;
    
    switch (frame->type) {
    case BT_CUBOID: {
        brush = new_CuboidBrush();
        brush->type = BT_CUBOID;
        memcpy(Cuboid_of(brush), Cuboid_of(frame), sizeof(*Cuboid_of(brush)));
        Cuboid_of(brush)->container = brush;
    }
        break;
    case BT_CYLINDER: {
        dibassert(false); // not supported yet
        // brush = create_uninitialized_CylinderBrush();
    }
        break;
    default:
        dibassert(false);
        break;
    }
   
    brush->flags = 0;
    brush->name = name ? name : "Untitled Brush";
    brush->BSP = BSP,
    brush->CSG = CSG;
    
    brush->POR = frame->POR;
    brush->wirecolor = frame->wirecolor;

    CuboidBrush *cuboid = Cuboid_of(brush);
    brush->num_vertices = cuboid->num_vertices;
    brush->vertices = cuboid->vertices;
    brush->num_edges = cuboid->num_edges;
    brush->edges = cuboid->edges;
    brush->num_quads = cuboid->num_quads;
    brush->quads = cuboid->quads;
    

    BrushNode *node;
    if (g_world.geo_brush->head == NULL) {
        dibassert(g_world.geo_brush->tail == NULL);
        node = g_world.geo_brush->head = zgl_Malloc(sizeof(BrushNode));
        node->next = NULL;
        node->prev = NULL;
        g_world.geo_brush->tail = node;
    }
    else {
        dibassert(g_world.geo_brush->tail != NULL);
        node = g_world.geo_brush->tail->next = zgl_Malloc(sizeof(BrushNode));
        node->next = NULL;
        node->prev = g_world.geo_brush->tail;
        g_world.geo_brush->tail = node;
    }

    node->brush = brush;

    void write_Brush(Brush *brush, FILE *out_fp);
    write_Brush(brush, stdout);
}

#include "src/draw/colors.h"

void init_Brush(Brush *brush, WorldVec POR, int CSG) {
    brush->flags = 0;
    brush->name = "Untitled Brush";
    brush->BSP = BSP_DEFAULT,
    brush->CSG = CSG;
    brush->POR = POR;
    brush->wirecolor = color_brush;
    dibassert(brush->brush);
}


#include "src/input/command.h"

static void CMDH_brush_from_frame(int argc, char **argv, char *output_str) {
    if (streq(argv[1], "sub")) {
        brush_from_frame(CSG_SUBTRACTIVE, BSP_DEFAULT, NULL);
    }
    else if (streq(argv[1], "add")) {
        brush_from_frame(CSG_ADDITIVE, BSP_DEFAULT, NULL);
    }
}

epm_Command const CMD_brush_from_frame = {
    .name = "brush_from_frame",
    .argc_min = 2,
    .argc_max = 2,
    .handler = CMDH_brush_from_frame,
};


static Fix32 read_fix_x(char const *str, Fix32 *out_num);

static void CMDH_set_frame(int argc, char **argv, char *output_str) {
    WorldUnit dx, dy, dz;
    read_fix_x(argv[1], &dx);
    read_fix_x(argv[2], &dy);
    read_fix_x(argv[3], &dz);
    
    set_frame_size(BT_CUBOID, dx, dy, dz);
}

static void CMDH_set_frame_point(int argc, char **argv, char *output_str) {
    if (argc < 3+1) return;

    WorldUnit x, y, z;
    read_fix_x(argv[1], &x);
    read_fix_x(argv[2], &y);
    read_fix_x(argv[3], &z);

    x_of(frame->POR) = x;
    y_of(frame->POR) = y;
    z_of(frame->POR) = z;
}

epm_Command const CMD_set_frame_point = {
    .name = "set_frame_point",
    .argc_min = 4,
    .argc_max = 4,
    .handler = CMDH_set_frame_point,
};

epm_Command const CMD_set_frame = {
    .name = "set_frame",
    .argc_min = 2,
    .argc_max = 16,
    .handler = CMDH_set_frame,
};














static Fix32 read_fix_x(char const *str, Fix32 *out_num) {
    Fix32 res = 0;
    bool neg = false;
    size_t i = 0;
    
    if (str[i] == '-') {
        neg = true;
        i++;
    }

    bool point = false;
    
    while (true) {
        switch (str[i]) {
        case '.': // if no point in string, is integer literal, so will shift up
                  // by 4 bits at end.
            point = true;
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
    if ( ! point)
        res = res<<4;
        
    if (neg)
        res = -res;

    *out_num = res;

    dibassert(i < INT_MAX);
    return (int)i;
}

extern void triangulate_CuboidBrush(CuboidBrush *cuboid, StaticGeometry *geo);

epm_Result epm_TriangulateAndMergeBrushGeometry(void) {
    g_world.geo_prebsp->num_vertices = 0;
    g_world.geo_prebsp->num_edges    = 0;
    g_world.geo_prebsp->num_faces    = 0;
    
    for (BrushNode *node = g_world.geo_brush->head; node; node = node->next) {
        triangulate_CuboidBrush(Cuboid_of(node->brush), g_world.geo_prebsp);
    }

    Edge *tmp_edges;
    epm_ComputeEdgesFromFaces(g_world.geo_prebsp->num_faces, g_world.geo_prebsp->faces,
                              &g_world.geo_prebsp->num_edges, &tmp_edges);
    memcpy(g_world.geo_prebsp->edges, tmp_edges, g_world.geo_prebsp->num_edges*sizeof(Edge));
    zgl_Free(tmp_edges);
    
    return EPM_SUCCESS;
}

#include "src/world/selection.h"

static bool face_is_selected(void *object) {
    return (bool)(((BrushQuadFace *)object)->flags & FACEFLAG_SELECTED);
}

static void face_select(void *object) {
    ((BrushQuadFace *)object)->flags |= FACEFLAG_SELECTED;
}

static void face_unselect(void *object) {
    ((BrushQuadFace *)object)->flags &= ~FACEFLAG_SELECTED;
}

Selection sel_face = {
    .fn_is_selected = face_is_selected,
    .fn_select = face_select,
    .fn_unselect = face_unselect,
    .num_selected = 0,
    .head = NULL,
    .tail = NULL,
    .nodes = {{0}}
};
