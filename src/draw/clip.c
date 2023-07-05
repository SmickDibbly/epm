#include "src/draw/clip.h"

extern bool g_farclip;

// NOTE: zgl_Pixit == int32_t

/* -------------------------------------------------------------------------- */

/** Sutherland-Hodgman polygon-clipping algorithm in 2D for a triangle against a
    rectangular screen region.
   
    x >= xmin
    y >= ymin
    x <= xmax
    y <= ymax
 */
#define MAX_LIST_LEN 32
void TriClip2D
(Fix64Vec_2D const *const v0,
 Fix64Vec_2D const *const v1,
 Fix64Vec_2D const *const v2,
 zgl_Pixit xmin, zgl_Pixit xmax, zgl_Pixit ymin, zgl_Pixit ymax,
 Fix64Vec_2D *out_poly, size_t *out_count, bool *out_new) {
    Fix64Vec_2D u[MAX_LIST_LEN], v[MAX_LIST_LEN], *curr_poly = u, *next_poly = v;
    bool u_new[MAX_LIST_LEN], v_new[MAX_LIST_LEN], *curr_new = u_new, *next_new = v_new;
    Fix64Vec_2D *tmp;
    bool *tmp_new;
    size_t curr_count;
    size_t next_count;

    // edge 0: x = xmin
    curr_poly[0] = *v0;
    curr_poly[1] = *v1;
    curr_poly[2] = *v2;
    curr_new[0] = false;
    curr_new[1] = false;
    curr_new[2] = false;
    curr_count = 3;
    memset(next_new, 0, 32*sizeof(bool));
    memset(next_poly, 0, 32*sizeof(Fix64Vec_2D));
    next_count = 0;
    
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec_2D curr = curr_poly[i];
        Fix64Vec_2D next = curr_poly[(i+1) % curr_count];
        Fix64Vec_2D sect;
        sect.x = xmin;
        Fix64 dx = next.x - curr.x;
        Fix64 dy = next.y - curr.y;
        
        if (curr.x >= xmin) { // curr "inside"
            next_poly[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (next.x < xmin) { // next "outside"
                sect.y = FIX_MULDIV(xmin-curr.x, dy, dx) + curr.y;
                next_poly[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (next.x >= xmin) { // curr "outside", next "inside"
            sect.y = FIX_MULDIV(xmin-curr.x, dy, dx) + curr.y;
            next_poly[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }

    
    // edge 1: x = xmax
    tmp = next_poly;
    next_poly = curr_poly;
    curr_poly = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_new, 0, 32*sizeof(bool));
    memset(next_poly, 0, 32*sizeof(Fix64Vec_2D));
    next_count = 0;
    
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec_2D curr = curr_poly[i];
        Fix64Vec_2D next = curr_poly[(i+1) % curr_count];
        Fix64Vec_2D sect;
        sect.x = xmax;
        Fix64 dx = next.x - curr.x;
        Fix64 dy = next.y - curr.y;

        if (curr.x <= xmax) { // curr "inside"
            next_poly[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (next.x > xmax) { // next "outside"
                sect.y = FIX_MULDIV(xmax-curr.x, dy, dx) + curr.y;
                next_new[next_count] = true;
                next_poly[next_count] = sect;
                next_count++;
            }
        }
        else if (next.x <= xmax) { // curr "outside", next "inside"
            sect.y = FIX_MULDIV(xmax-curr.x, dy, dx) + curr.y;
            next_poly[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }

    // edge 2: y = ymin
    tmp = next_poly;
    next_poly = curr_poly;
    curr_poly = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_new, 0, 32*sizeof(bool));
    memset(next_poly, 0, 32*sizeof(Fix64Vec_2D));
    next_count = 0;
    
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec_2D curr = curr_poly[i];
        Fix64Vec_2D next = curr_poly[(i+1) % curr_count];
        Fix64Vec_2D sect;
        sect.y = ymin;
        Fix64 dx = next.x - curr.x;
        Fix64 dy = next.y - curr.y;

        if (curr.y >= ymin) { // curr "inside"
            next_poly[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (next.y < ymin) { // next "outside"
                sect.x = FIX_MULDIV(ymin-curr.y, dx, dy) + curr.x;
                next_poly[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (next.y >= ymin) { // curr "outside", next "inside"
            sect.x = FIX_MULDIV(ymin-curr.y, dx, dy) + curr.x;
            next_poly[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }

    

    // edge 3: y = ymax
    tmp = next_poly;
    next_poly = curr_poly;
    curr_poly = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_new, 0, 32*sizeof(bool));
    memset(next_poly, 0, 32*sizeof(Fix64Vec_2D));
    next_count = 0;
    
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec_2D curr = curr_poly[i];
        Fix64Vec_2D next = curr_poly[(i+1) % curr_count];
        Fix64Vec_2D sect;
        sect.y = ymax;
        Fix64 dx = next.x - curr.x;
        Fix64 dy = next.y - curr.y;
        
        if (curr.y <= ymax) { // curr "inside"
            next_poly[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (next.y > ymax) { // next "outside"
                sect.x = FIX_MULDIV(ymax-curr.y, dx, dy) + curr.x;
                next_poly[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (next.y <= ymax) { // curr "outside", next "inside"
            sect.x = FIX_MULDIV(ymax-curr.y, dx, dy) + curr.x;
            next_poly[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }

    
    memcpy(out_poly, next_poly, 32*sizeof(Fix64Vec_2D));
    memcpy(out_new, next_new, 32*sizeof(bool));
    *out_count = next_count;
}


/** Sutherland-Hodgman polygon-clipping algorithm in 3D for a triangle against a
    perspective frustum

    x <= z
    y <= z
    x >= -z
    y >= -z
    z >= zmin
    z <= zmax    
 */
void TriClip3D_Persp
(Fix64Vec const *const v0,
 Fix64Vec const *const v1,
 Fix64Vec const *const v2,
 Frustum *fr,
 Fix64Vec *out_poly, size_t *out_count, bool *out_new) {
    Fix64 zmin = fr->F;
    Fix64 zmax = fr->B;
    
    Fix64Vec u[MAX_LIST_LEN], v[MAX_LIST_LEN], *curr_list = u, *next_list = v;
    bool u_new[MAX_LIST_LEN], v_new[MAX_LIST_LEN], *curr_new = u_new, *next_new = v_new;
    Fix64Vec *tmp;
    bool *tmp_new;
    size_t curr_count;
    size_t next_count;
    
    // z = zmin
    curr_list[0] = *v0;
    curr_list[1] = *v1;
    curr_list[2] = *v2;
    curr_new[0] = false;
    curr_new[1] = false;
    curr_new[2] = false;
    curr_count = 3;
    memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
    memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
    next_count = 0;
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec curr = curr_list[i];
        Fix64Vec next = curr_list[(i+1) % curr_count];
        Fix64Vec sect;
        z_of(sect) = zmin;
        
        Fix64 dx = x_of(next) - x_of(curr);
        Fix64 dy = y_of(next) - y_of(curr);
        Fix64 dz = z_of(next) - z_of(curr);

        if (z_of(curr) >= z_of(sect)) {
            // curr "inside"; keep point and preserve edge
            next_list[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (z_of(next) < z_of(sect)) {
                // next "outside"; find intersection point, new edge
                x_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dx, dz)
                              + x_of(curr));
                y_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dy, dz)
                              + y_of(curr));
                next_list[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (z_of(next) >= z_of(sect)) {
            // curr "outside", next "inside"; discard curr, find
            // intersect point, preserve edge
            x_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dx, dz)
                          + x_of(curr));
            y_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dy, dz)
                          + y_of(curr));
            next_list[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }

    if (g_farclip) {
        // z = zmax
        tmp = next_list;
        next_list = curr_list;
        curr_list = tmp;
        tmp_new = next_new;
        next_new = curr_new;
        curr_new = tmp_new;
        curr_count = next_count;
        memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
        memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
        next_count = 0;
        for (size_t i = 0; i < curr_count; i++) {
            Fix64Vec curr = curr_list[i];
            Fix64Vec next = curr_list[(i+1) % curr_count];
            Fix64Vec sect;
            z_of(sect) = zmax;
        
            Fix64 dx = x_of(next) - x_of(curr);
            Fix64 dy = y_of(next) - y_of(curr);
            Fix64 dz = z_of(next) - z_of(curr);

            if (z_of(curr) <= z_of(sect)) {
                // curr "inside"; keep point and preserve edge
                next_list[next_count] = curr;
                next_new[next_count] = curr_new[i];
                next_count++;
                if (z_of(next) > z_of(sect)) {
                    // next "outside"; find intersection point, new edge
                    x_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dx, dz)
                                  + x_of(curr));
                    y_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dy, dz)
                                  + y_of(curr));
                    next_list[next_count] = sect;
                    next_new[next_count] = true;
                    next_count++;
                }
            }
            else if (z_of(next) <= z_of(sect)) {
                // curr "outside", next "inside"; discard curr, find
                // intersect point, preserve edge
                x_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dx, dz)
                              + x_of(curr));
                y_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dy, dz)
                              + y_of(curr));
                next_list[next_count] = sect;
                next_new[next_count] = curr_new[i];
                next_count++;
            }
        }
    }
    
    // z = x
    tmp = next_list;
    next_list = curr_list;
    curr_list = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
    memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
    next_count = 0;
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec curr = curr_list[i];
        Fix64Vec next = curr_list[(i+1) % curr_count];
        Fix64Vec sect;
                
        Fix64 dx = x_of(next) - x_of(curr);
        Fix64 dy = y_of(next) - y_of(curr);
        Fix64 dz = z_of(next) - z_of(curr);

        if (x_of(curr) <= z_of(curr)) {
            // curr "inside"; keep point and preserve edge
            next_list[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (x_of(next) > z_of(next)) {
                // next "outside"; find intersection point, new edge
                dibassert(dz-dx != 0);
                x_of(sect) = (dz*x_of(curr)-dx*z_of(curr))/(dz-dx);
                z_of(sect) = x_of(sect);
                if (dz != 0)
                    y_of(sect) = ((z_of(sect)-z_of(curr))*dy)/dz + y_of(curr);
                else
                    y_of(sect) = ((x_of(sect)-x_of(curr))*dy)/dx + y_of(curr);
                next_list[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (x_of(next) <= z_of(next)) {
            // curr "outside", next "inside"; discard curr, find
            // intersect point, preserve edge
            dibassert(dz-dx != 0);
            x_of(sect) = (dz*x_of(curr)-dx*z_of(curr))/(dz-dx);
            z_of(sect) = x_of(sect);
            if (dz != 0)
                y_of(sect) = ((z_of(sect)-z_of(curr))*dy)/dz + y_of(curr);
            else
                y_of(sect) = ((x_of(sect)-x_of(curr))*dy)/dx + y_of(curr);
            next_list[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }
    
    // z = -x
    tmp = next_list;
    next_list = curr_list;
    curr_list = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
    memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
    next_count = 0;
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec curr = curr_list[i];
        Fix64Vec next = curr_list[(i+1) % curr_count];
        Fix64Vec sect;
                
        Fix64 dx = x_of(next) - x_of(curr);
        Fix64 dy = y_of(next) - y_of(curr);
        Fix64 dz = z_of(next) - z_of(curr);

        if (-x_of(curr) <= z_of(curr)) {
            // curr "inside"; keep point and preserve edge
            next_list[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (-x_of(next) > z_of(next)) {
                // next "outside"; find intersection point, new edge
                dibassert(dz+dx != 0);
                x_of(sect) = (dz*x_of(curr)-dx*z_of(curr))/(dz+dx);
                z_of(sect) = -x_of(sect);
                if (dz != 0)
                    y_of(sect) = ((z_of(sect)-z_of(curr))*dy)/dz + y_of(curr);
                else
                    y_of(sect) = ((x_of(sect)-x_of(curr))*dy)/dx + y_of(curr);
                next_list[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (-x_of(next) <= z_of(next)) {
            // curr "outside", next "inside"; discard curr, find
            // intersect point, preserve edge
            dibassert(dz+dx != 0);
            x_of(sect) = (dz*x_of(curr)-dx*z_of(curr))/(dz+dx);
            z_of(sect) = -x_of(sect);
            if (dz != 0)
                y_of(sect) = ((z_of(sect)-z_of(curr))*dy)/dz + y_of(curr);
            else
                y_of(sect) = ((x_of(sect)-x_of(curr))*dy)/dx + y_of(curr);
            next_list[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }

    // z = y
    tmp = next_list;
    next_list = curr_list;
    curr_list = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
    memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
    next_count = 0;
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec curr = curr_list[i];
        Fix64Vec next = curr_list[(i+1) % curr_count];
        Fix64Vec sect;
                
        Fix64 dx = x_of(next) - x_of(curr);
        Fix64 dy = y_of(next) - y_of(curr);
        Fix64 dz = z_of(next) - z_of(curr);

        if (y_of(curr) <= z_of(curr)) {
            // curr "inside"; keep point and preserve edge
            next_list[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (y_of(next) > z_of(next)) {
                // next "outside"; find intersection point, new edge
                dibassert(dz-dy != 0);
                y_of(sect) = (dz*y_of(curr)-dy*z_of(curr))/(dz-dy);
                z_of(sect) = y_of(sect);
                if (dz != 0)
                    x_of(sect) = ((z_of(sect)-z_of(curr))*dx)/dz + x_of(curr);
                else
                    x_of(sect) = ((y_of(sect)-y_of(curr))*dx)/dy + x_of(curr);
                next_list[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (y_of(next) <= z_of(next)) {
            // curr "outside", next "inside"; discard curr, find
            // intersect point, preserve edge
            dibassert(dz-dy != 0);
            y_of(sect) = (dz*y_of(curr)-dy*z_of(curr))/(dz-dy);
            z_of(sect) = y_of(sect);
            if (dz != 0)
                x_of(sect) = ((z_of(sect)-z_of(curr))*dx)/dz + x_of(curr);
            else
                x_of(sect) = ((y_of(sect)-y_of(curr))*dx)/dy + x_of(curr);
            next_list[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }

    // z = -y
    tmp = next_list;
    next_list = curr_list;
    curr_list = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
    memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
    next_count = 0;
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec curr = curr_list[i];
        Fix64Vec next = curr_list[(i+1) % curr_count];
        Fix64Vec sect;
                
        Fix64 dx = x_of(next) - x_of(curr);
        Fix64 dy = y_of(next) - y_of(curr);
        Fix64 dz = z_of(next) - z_of(curr);

        if (-y_of(curr) <= z_of(curr)) {
            // curr "inside"; keep point and preserve edge
            next_list[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (-y_of(next) > z_of(next)) {
                // next "outside"; find intersection point, new edge
                dibassert(dz+dy != 0);
                y_of(sect) = (dz*y_of(curr)-dy*z_of(curr))/(dz+dy);
                z_of(sect) = -y_of(sect);
                if (dz != 0)
                    x_of(sect) = ((z_of(sect)-z_of(curr))*dx)/dz + x_of(curr);
                else
                    x_of(sect) = ((y_of(sect)-y_of(curr))*dx)/dy + x_of(curr);
                next_list[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (-y_of(next) <= z_of(next)) {
            // curr "outside", next "inside"; discard curr, find
            // intersect point, preserve edge
            dibassert(dz+dy != 0);
            y_of(sect) = (dz*y_of(curr)-dy*z_of(curr))/(dz+dy);
            z_of(sect) = -y_of(sect);
            if (dz != 0)
                x_of(sect) = ((z_of(sect)-z_of(curr))*dx)/dz + x_of(curr);
            else
                x_of(sect) = ((y_of(sect)-y_of(curr))*dx)/dy + x_of(curr);
            next_list[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }

    memcpy(out_poly, next_list, MAX_LIST_LEN*sizeof(Fix64Vec));
    memcpy(out_new, next_new, MAX_LIST_LEN*sizeof(bool));
    *out_count = next_count;
}


/** Sutherland-Hodgman polygon-clipping algorithm in 3D for a triangle against
    an orthographic frustum
    
    x >= xmin
    y >= ymin
    z >= zmin
    x <= xmax
    y <= ymax
    z <= zmax
 */
void TriClip3D_Ortho
(Fix64Vec const *const v0,
 Fix64Vec const *const v1,
 Fix64Vec const *const v2,
 Frustum *fr,
 Fix64Vec *out_poly, size_t *out_count, bool *out_new) {
    zgl_Pixit xmin = fr->xmin;
    zgl_Pixit xmax = fr->xmax;
    zgl_Pixit ymin = fr->ymin;
    zgl_Pixit ymax = fr->ymax;
    zgl_Pixit zmin = fr->zmin;
    zgl_Pixit zmax = fr->zmax;
    
    Fix64Vec u[MAX_LIST_LEN], v[MAX_LIST_LEN], *curr_list = u, *next_list = v;
    bool u_new[MAX_LIST_LEN], v_new[MAX_LIST_LEN], *curr_new = u_new, *next_new = v_new;
    Fix64Vec *tmp;
    bool *tmp_new;
    size_t curr_count;
    size_t next_count;
    
    // z = zmin
    curr_list[0] = *v0;
    curr_list[1] = *v1;
    curr_list[2] = *v2;
    curr_new[0] = false;
    curr_new[1] = false;
    curr_new[2] = false;
    curr_count = 3;
    memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
    memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
    next_count = 0;
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec curr = curr_list[i];
        Fix64Vec next = curr_list[(i+1) % curr_count];
        Fix64Vec sect;
        z_of(sect) = zmin;
        
        Fix64 dx = x_of(next) - x_of(curr);
        Fix64 dy = y_of(next) - y_of(curr);
        Fix64 dz = z_of(next) - z_of(curr);

        if (z_of(curr) >= z_of(sect)) {
            // curr "inside"; keep point and preserve edge
            next_list[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (z_of(next) < z_of(sect)) {
                // next "outside"; find intersection point, new edge
                x_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dx, dz)
                              + x_of(curr));
                y_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dy, dz)
                              + y_of(curr));
                next_list[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (z_of(next) >= z_of(sect)) {
            // curr "outside", next "inside"; discard curr, find
            // intersect point, preserve edge
            x_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dx, dz)
                          + x_of(curr));
            y_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dy, dz)
                          + y_of(curr));
            next_list[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }

    // z = zmax
    tmp = next_list;
    next_list = curr_list;
    curr_list = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
    memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
    next_count = 0;
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec curr = curr_list[i];
        Fix64Vec next = curr_list[(i+1) % curr_count];
        Fix64Vec sect;
        z_of(sect) = zmax;
        
        Fix64 dx = x_of(next) - x_of(curr);
        Fix64 dy = y_of(next) - y_of(curr);
        Fix64 dz = z_of(next) - z_of(curr);

        if (z_of(curr) <= z_of(sect)) {
            // curr "inside"; keep point and preserve edge
            next_list[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (z_of(next) > z_of(sect)) {
                // next "outside"; find intersection point, new edge
                x_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dx, dz)
                              + x_of(curr));
                y_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dy, dz)
                              + y_of(curr));
                next_list[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (z_of(next) <= z_of(sect)) {
            // curr "outside", next "inside"; discard curr, find
            // intersect point, preserve edge
            x_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dx, dz)
                          + x_of(curr));
            y_of(sect) = (FIX_MULDIV(z_of(sect)-z_of(curr), dy, dz)
                          + y_of(curr));
            next_list[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }
    
    // x = xmin
    tmp = next_list;
    next_list = curr_list;
    curr_list = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
    memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
    next_count = 0;
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec curr = curr_list[i];
        Fix64Vec next = curr_list[(i+1) % curr_count];
        Fix64Vec sect;
        x_of(sect) = xmin;
        
        Fix64 dx = x_of(next) - x_of(curr);
        Fix64 dy = y_of(next) - y_of(curr);
        Fix64 dz = z_of(next) - z_of(curr);

        if (x_of(curr) >= x_of(sect)) {
            // curr "inside"; keep point and preserve edge
            next_list[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (x_of(next) < x_of(sect)) {
                // next "outside"; find intersection point, new edge
                y_of(sect) = (FIX_MULDIV(x_of(sect)-x_of(curr), dy, dx)
                              + y_of(curr));
                z_of(sect) = (FIX_MULDIV(x_of(sect)-x_of(curr), dz, dx)
                              + z_of(curr));
                next_list[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (x_of(next) >= x_of(sect)) {
            // curr "outside", next "inside"; discard curr, find
            // intersect point, preserve edge
            y_of(sect) = (FIX_MULDIV(x_of(sect)-x_of(curr), dy, dx)
                          + y_of(curr));
            z_of(sect) = (FIX_MULDIV(x_of(sect)-x_of(curr), dz, dx)
                          + z_of(curr));
            next_list[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }
    

    // x = xmax
    tmp = next_list;
    next_list = curr_list;
    curr_list = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
    memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
    next_count = 0;
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec curr = curr_list[i];
        Fix64Vec next = curr_list[(i+1) % curr_count];
        Fix64Vec sect;
        x_of(sect) = xmax;
        
        Fix64 dx = x_of(next) - x_of(curr);
        Fix64 dy = y_of(next) - y_of(curr);
        Fix64 dz = z_of(next) - z_of(curr);

        if (x_of(curr) <= x_of(sect)) {
            // curr "inside"; keep point and preserve edge
            next_list[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (x_of(next) > x_of(sect)) {
                // next "outside"; find intersection point, new edge
                y_of(sect) = (FIX_MULDIV(x_of(sect)-x_of(curr), dy, dx)
                              + y_of(curr));
                z_of(sect) = (FIX_MULDIV(x_of(sect)-x_of(curr), dz, dx)
                              + z_of(curr));
                next_list[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (x_of(next) <= x_of(sect)) {
            // curr "outside", next "inside"; discard curr, find
            // intersect point, preserve edge
            y_of(sect) = (FIX_MULDIV(x_of(sect)-x_of(curr), dy, dx)
                          + y_of(curr));
            z_of(sect) = (FIX_MULDIV(x_of(sect)-x_of(curr), dz, dx)
                          + z_of(curr));
            next_list[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }

    // y = ymin
    tmp = next_list;
    next_list = curr_list;
    curr_list = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
    memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
    next_count = 0;
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec curr = curr_list[i];
        Fix64Vec next = curr_list[(i+1) % curr_count];
        Fix64Vec sect;
        y_of(sect) = ymin;
        
        Fix64 dx = x_of(next) - x_of(curr);
        Fix64 dy = y_of(next) - y_of(curr);
        Fix64 dz = z_of(next) - z_of(curr);

        if (y_of(curr) >= y_of(sect)) {
            // curr "inside"; keep point and preserve edge
            next_list[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (y_of(next) < y_of(sect)) {
                // next "outside"; find intersection point, new edge
                x_of(sect) = (FIX_MULDIV(y_of(sect)-y_of(curr), dx, dy)
                              + x_of(curr));
                z_of(sect) = (FIX_MULDIV(y_of(sect)-y_of(curr), dz, dy)
                              + z_of(curr));
                next_list[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (y_of(next) >= y_of(sect)) {
            // curr "outside", next "inside"; discard curr, find
            // intersect point, preserve edge
            x_of(sect) = (FIX_MULDIV(y_of(sect)-y_of(curr), dx, dy)
                          + x_of(curr));
            z_of(sect) = (FIX_MULDIV(y_of(sect)-y_of(curr), dz, dy)
                          + z_of(curr));
            next_list[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }
    
    // y = ymax
    tmp = next_list;
    next_list = curr_list;
    curr_list = tmp;
    tmp_new = next_new;
    next_new = curr_new;
    curr_new = tmp_new;
    curr_count = next_count;
    memset(next_list, 0, MAX_LIST_LEN*sizeof(Fix64Vec));
    memset(next_new, 0, MAX_LIST_LEN*sizeof(bool));
    next_count = 0;
    for (size_t i = 0; i < curr_count; i++) {
        Fix64Vec curr = curr_list[i];
        Fix64Vec next = curr_list[(i+1) % curr_count];
        Fix64Vec sect;
        y_of(sect) = ymax;
        
        Fix64 dx = x_of(next) - x_of(curr);
        Fix64 dy = y_of(next) - y_of(curr);
        Fix64 dz = z_of(next) - z_of(curr);

        if (y_of(curr) <= y_of(sect)) {
            // curr "inside"; keep point and preserve edge
            next_list[next_count] = curr;
            next_new[next_count] = curr_new[i];
            next_count++;
            if (y_of(next) > y_of(sect)) {
                // next "outside"; find intersection point, new edge
                x_of(sect) = (FIX_MULDIV(y_of(sect)-y_of(curr), dx, dy)
                              + x_of(curr));
                z_of(sect) = (FIX_MULDIV(y_of(sect)-y_of(curr), dz, dy)
                              + z_of(curr));
                next_list[next_count] = sect;
                next_new[next_count] = true;
                next_count++;
            }
        }
        else if (y_of(next) <= y_of(sect)) {
            // curr "outside", next "inside"; discard curr, find
            // intersect point, preserve edge
            x_of(sect) = (FIX_MULDIV(y_of(sect)-y_of(curr), dx, dy)
                          + x_of(curr));
            z_of(sect) = (FIX_MULDIV(y_of(sect)-y_of(curr), dz, dy)
                          + z_of(curr));
            next_list[next_count] = sect;
            next_new[next_count] = curr_new[i];
            next_count++;
        }
    }

    memcpy(out_poly, next_list, MAX_LIST_LEN*sizeof(Fix64Vec));
    memcpy(out_new, next_new, MAX_LIST_LEN*sizeof(bool));
    *out_count = next_count;
}            



// IDEA: ensure this is inlined?
static bool compute_t(Fix64 D, Fix64 N, Fix64 *tE, Fix64 *tL) {
    Fix64 t;

    if (D > 0) {
        t = FIX_DIV(N, D);
        if (t > *tL) {
            return false;
        }
        else if (t > *tE) {
            *tE = t;
        }
    }
    else if (D < 0) {
        t = FIX_DIV(N, D);
        if (t < *tE) {
            return false;
        }
        else if (t < *tL) {
            *tL = t;
        }
    }
    else if (N > 0) { // D == 0
        return false;
    }
    return true;
}




bool SegClip3D_Persp
(Fix64 x0, Fix64 y0, Fix64 z0,
 Fix64 x1, Fix64 y1, Fix64 z1,
 Frustum *fr,
 Fix64Vec *out_c0, Fix64Vec *out_c1) {
    Fix64 zmin = fr->F;
    Fix64 zmax = fr->B;
    *out_c0 = (Fix64Vec){{x0, y0, z0}};
    *out_c1 = (Fix64Vec){{x1, y1, z1}};
    
    Fix64 tmin = fixify(0), tmax = fixify(1);
    Fix64 dx = x1-x0, dz = z1-z0;
    bool accept = false;

    if (compute_t(-dx+dz, x0-z0, &tmin, &tmax)) { // right side
        if (compute_t(dx+dz, -x0-z0, &tmin, &tmax)) { // left side
            Fix64 dy = y1-y0;
            if (compute_t(dy+dz, -y0-z0, &tmin, &tmax)) { // bottom
                if (compute_t(-dy+dz, y0-z0, &tmin, &tmax)) { // top
                    if (compute_t(dz, -z0+zmin, &tmin, &tmax)) { // front
                        if (g_farclip) {
                            if (compute_t(-dz, z0-zmax, &tmin, &tmax)) { // back
                                accept = true;
                            
                                if (tmax < fixify(1)) {
                                    x_of(*out_c1) = x0 + FIX_MUL(tmax, dx);
                                    y_of(*out_c1) = y0 + FIX_MUL(tmax, dy);
                                    z_of(*out_c1) = z0 + FIX_MUL(tmax, dz);
                                }

                                if (tmin > fixify(0)) {
                                    x_of(*out_c0) = x0 + FIX_MUL(tmin, dx);
                                    y_of(*out_c0) = y0 + FIX_MUL(tmin, dy);
                                    z_of(*out_c0) = z0 + FIX_MUL(tmin, dz);
                                }
                            }
                        }
                        else {
                            accept = true;
                            
                            if (tmax < fixify(1)) {
                                x_of(*out_c1) = x0 + FIX_MUL(tmax, dx);
                                y_of(*out_c1) = y0 + FIX_MUL(tmax, dy);
                                z_of(*out_c1) = z0 + FIX_MUL(tmax, dz);
                            }

                            if (tmin > fixify(0)) {
                                x_of(*out_c0) = x0 + FIX_MUL(tmin, dx);
                                y_of(*out_c0) = y0 + FIX_MUL(tmin, dy);
                                z_of(*out_c0) = z0 + FIX_MUL(tmin, dz);
                            }
                        }
                            
                    }
                }
            }
        }
    }

    return accept;
}



bool SegClip2D
(Fix64 x0, Fix64 y0,
 Fix64 x1, Fix64 y1,
 Fix64Rect_2D rect,
 Fix64Vec_2D *out_c0, Fix64Vec_2D *out_c1) {
    *out_c0 = (Fix64Vec_2D){x0, y0};
    *out_c1 = (Fix64Vec_2D){x1, y1};

    Fix64 xmin = rect.x;
    Fix64 xmax = rect.x + rect.w-1;
    Fix64 ymin = rect.y;
    Fix64 ymax = rect.y + rect.h-1;
    
    Fix64 tmin = fixify(0), tmax = fixify(1);
    Fix64 dx = x1-x0, dy = y1-y0;
    bool accept = false;

    if (compute_t(dx, -x0+xmin, &tmin, &tmax)) { // left
        if (compute_t(-dx, x0-xmax, &tmin, &tmax)) { // right
            if (compute_t(dy, -y0+ymin, &tmin, &tmax)) { // below
                if (compute_t(-dy, y0-ymax, &tmin, &tmax)) { // above
                    accept = true;
                    
                    if (tmax < fixify(1)) {
                        out_c1->x = x0 + FIX_MUL(tmax, dx);
                        out_c1->y = y0 + FIX_MUL(tmax, dy);
                    }

                    if (tmin > fixify(0)) {
                        out_c0->x = x0 + FIX_MUL(tmin, dx);
                        out_c0->y = y0 + FIX_MUL(tmin, dy);
                    }
                }
            }
        }
    }

    return accept;
}


/**
   Liang-Barsky line-clipping algorithm in 3D against an orthographic frustum
   
   x >= xmin
   y >= ymin
   z >= zmin
   x <= xmax
   y <= ymax
   z <= zmax
*/
bool SegClip3D_Ortho
(Fix64 x0, Fix64 y0, Fix64 z0,
 Fix64 x1, Fix64 y1, Fix64 z1,
 Frustum *fr,
 Fix64Vec *out_c0, Fix64Vec *out_c1) {
    *out_c0 = (Fix64Vec){{x0, y0, z0}};
    *out_c1 = (Fix64Vec){{x1, y1, z1}};

    Fix64 xmin = fr->xmin;
    Fix64 ymin = fr->ymin;
    Fix64 zmin = fr->zmin;
    Fix64 xmax = fr->xmax;
    Fix64 ymax = fr->ymax;
    Fix64 zmax = fr->zmax;
    
    // defining variables
    Fix32 neg_dx = -(Fix32)(x1 - x0);
    Fix32 pos_dx = -neg_dx;
    Fix32 neg_dy = -(Fix32)(y1 - y0);
    Fix32 pos_dy = -neg_dy;
    Fix32 neg_dz = -(Fix32)(z1 - z0);
    Fix32 pos_dz = -neg_dz;
    
    Fix32 q0 = (Fix32)(x0 - xmin);
    Fix32 q1 = (Fix32)(xmax - x0);
    Fix32 q2 = (Fix32)(y0 - ymin);
    Fix32 q3 = (Fix32)(ymax - y0);
    Fix32 q4 = (Fix32)(z0 - zmin);
    Fix32 q5 = (Fix32)(zmax - z0);
    
    Fix32 posarr[7], negarr[7];
    int posind = 1, negind = 1;
    posarr[0] = fixify(1);
    negarr[0] = fixify(0);

    if ((neg_dx == 0 && q0 < 0) || (pos_dx == 0 && q1 < 0) ||
        (neg_dy == 0 && q2 < 0) || (pos_dy == 0 && q3 < 0) ||
        (neg_dz == 0 && q4 < 0) || (pos_dz == 0 && q5 < 0)) {
        return false;
    }
    
    if (neg_dx != 0) {
        Fix32 r0 = (Fix32)FIX_DIV(q0, neg_dx);
        Fix32 r1 = (Fix32)FIX_DIV(q1, pos_dx);
        if (neg_dx < 0) {
            negarr[negind++] = r0; // for negative pos_dx, add it to negative array
            posarr[posind++] = r1; // and add neg_dy to positive array
        } else {
            negarr[negind++] = r1;
            posarr[posind++] = r0;
        }
    }
    
    if (neg_dy != 0) {
        Fix32 r2 = (Fix32)FIX_DIV(q2, neg_dy);
        Fix32 r3 = (Fix32)FIX_DIV(q3, pos_dy);
        if (neg_dy < 0) {
            negarr[negind++] = r2;
            posarr[posind++] = r3;
        } else {
            negarr[negind++] = r3;
            posarr[posind++] = r2;
        }
    }

    if (neg_dz != 0) {
        Fix32 r4 = (Fix32)FIX_DIV(q4, neg_dz);
        Fix32 r5 = (Fix32)FIX_DIV(q5, pos_dz);
        if (neg_dz < 0) {
            negarr[negind++] = r4;
            posarr[posind++] = r5;
        } else {
            negarr[negind++] = r5;
            posarr[posind++] = r4;
        }
    }
    
    Fix32 T0 = fixify(0);
    for (int i = 0; i < negind; ++i)
        if (T0 < negarr[i])
            T0 = negarr[i];

    Fix32 T1 = fixify(1);
    for (int i = 0; i < posind; ++i)
        if (T1 > posarr[i])
            T1 = posarr[i];

    if (T0 > T1)  {
        return false;
    }

    x_of(*out_c0) = x0 + FIX_MUL(T0, pos_dx); 
    y_of(*out_c0) = y0 + FIX_MUL(T0, pos_dy);
    z_of(*out_c0) = z0 + FIX_MUL(T0, pos_dz);
    
    x_of(*out_c1) = x0 + FIX_MUL(T1, pos_dx);
    y_of(*out_c1) = y0 + FIX_MUL(T1, pos_dy);
    z_of(*out_c1) = z0 + FIX_MUL(T1, pos_dz);
    
    return true;
}
