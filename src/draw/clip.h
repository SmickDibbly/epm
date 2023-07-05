#ifndef CLIP_H
#define CLIP_H

/* Clipping algorithms optimized for specific viewing frustums (or intended to
   be optimized eventually if not already). */

#include "src/misc/epm_includes.h"
#include "src/draw/draw3D.h"
#include "zigil/zigil.h"

extern void TriClip2D
(Fix64Vec_2D const *const v0,
 Fix64Vec_2D const *const v1,
 Fix64Vec_2D const *const v2,
 zgl_Pixit xmin, zgl_Pixit xmax, zgl_Pixit ymin, zgl_Pixit ymax,
 Fix64Vec_2D *out_poly, size_t *out_count, bool *out_new);

extern void TriClip3D_Persp
(Fix64Vec const *const v0,
 Fix64Vec const *const v1,
 Fix64Vec const *const v2,
 Frustum *fr,
 Fix64Vec *out_poly, size_t *out_count, bool *out_new);

extern void TriClip3D_Ortho
(Fix64Vec const *const v0,
 Fix64Vec const *const v1,
 Fix64Vec const *const v2,
 Frustum *fr,
 Fix64Vec *out_poly, size_t *out_count, bool *out_new);

extern bool SegClip2D
(Fix64 x0, Fix64 y0,
 Fix64 x1, Fix64 y1,
 Fix64Rect_2D rect,
 Fix64Vec_2D *out_c0, Fix64Vec_2D *out_c1);

extern bool SegClip3D_Persp
(Fix64 x0, Fix64 y0, Fix64 z0,
 Fix64 x1, Fix64 y1, Fix64 z1,
 Frustum *fr,
 Fix64Vec *out_c0, Fix64Vec *out_c1);

extern bool SegClip3D_Ortho
(Fix64 x0, Fix64 y0, Fix64 z0,
 Fix64 x1, Fix64 y1, Fix64 z1,
 Frustum *fr,
 Fix64Vec *out_c0, Fix64Vec *out_c1);

#endif /* CLIP_H */
