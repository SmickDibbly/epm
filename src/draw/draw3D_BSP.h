#ifndef DRAW3D_BSP_H
#define DRAW3D_BSP_H

#include "src/misc/epm_includes.h"
#include "src/draw/draw.h"
#include "src/draw/draw3D.h"

#define MAX_SECTS 16
extern WorldVec sect_ring[MAX_SECTS];

extern zgl_Color set_fclr_bsp_depth(Face *face);

extern BSPFaceExt *BSPFace_below
(Window *win,
 zgl_Pixel pixel,
 Frustum *fr);

extern void draw_BSPTree
(BSPTree *tree,
 Window *win,
 WorldVec campos,
 TransformedVertex *vbuf);

extern void draw_BSPTreeWireframe(Window *win, BSPTree *tree, BSPNode *node, WorldVec campos, TransformedVertex *vbuf);

#endif /* DRAW3D_BSP_H */
