#ifndef DRAW3D_BSP_H
#define DRAW3D_BSP_H

#include "src/misc/epm_includes.h"
#include "src/draw/draw.h"
#include "src/draw/draw3D.h"

#define MAX_SECTS 16
extern WorldVec sect_ring[MAX_SECTS];

extern BSPFace *BSPFace_below
(Window *win,
 zgl_Pixel pixel,
 Frustum *fr);

extern void draw_BSPTree
(BSPTree *tree,
 Window *win,
 WorldVec campos,
 TransformedVertex *vbuf);

extern void draw_face_BSP_visualizer
( Window *win,
 Face *face,
 TransformedFace *tf);

extern void draw_BSPNode_wireframe(Window *win, BSPTree *tree, BSPNode *node, WorldVec campos, TransformedVertex *vbuf);

extern WorldVec *vertex_below(Window *win, zgl_Pixel pixel, Frustum *fr);

#endif /* DRAW3D_BSP_H */
