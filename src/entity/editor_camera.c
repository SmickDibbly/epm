#include "src/entity/editor_camera.h"
#include "src/world/world.h"
#include "src/input/input.h"

void onTic_cam(void *self) {
    EditorCamera *cam = (EditorCamera *)self;

    /* ANGULAR MOTION */
    if (cam->key_angular_motion) {
        cam->view_angle_h += cam->key_angular_h;
        cam->view_angle_h &= ANG18_2PI_MASK;
        cam->view_angle_v += cam->key_angular_v;
        cam->view_angle_v &= ANG18_PI_MASK;
        // key motion is continuous
    }
    if (cam->mouse_angular_motion) {
        cam->view_angle_h += cam->mouse_angular_h;
        cam->view_angle_h &= ANG18_2PI_MASK;
        cam->view_angle_v += cam->mouse_angular_v;
        cam->view_angle_v &= ANG18_PI_MASK;
        cam->mouse_angular_h = 0;
        cam->mouse_angular_v = 0;
        cam->mouse_angular_motion = false;
    }

    Fix32 vsin, vcos, hsin, hcos;
    cossin18(&vcos, &vsin, cam->view_angle_v);
    cossin18(&hcos, &hsin, cam->view_angle_h);
    
    cam->view_vec_XY.x = hcos;
    cam->view_vec_XY.y = hsin;
    
    x_of(cam->view_vec) = (Fix32)FIX_MUL(vsin, hcos);
    y_of(cam->view_vec) = (Fix32)FIX_MUL(vsin, hsin);
    z_of(cam->view_vec) = vcos;
    
    /* MOTION */
    cam->vel.v[I_X] = 0;
    cam->vel.v[I_Y] = 0;
    cam->vel.v[I_Z] = 0;

    bool fast = LK_states[LK_LSHIFT];
    int32_t vel_scale = fast ? 4 : 1;
    
    if (cam->key_motion) {
        Fix32 cos, sin;
        cossin18(&cos, &sin, cam->key_motion_angle + cam->view_angle_h);
        x_of(cam->vel) += (Fix32)FIX_MUL(CAMERA_MAX_SPEED*vel_scale, cos);
        y_of(cam->vel) += (Fix32)FIX_MUL(CAMERA_MAX_SPEED*vel_scale, sin);
        // key_motion is continuous
    }
    if (cam->key_vertical_motion) {
        z_of(cam->vel) += cam->key_vertical_vel;
    }
    if (cam->mouse_motion) {
        x_of(cam->vel) += x_of(cam->mouse_motion_vel);
        y_of(cam->vel) += y_of(cam->mouse_motion_vel);
        z_of(cam->vel) += z_of(cam->mouse_motion_vel);
        cam->mouse_motion = false;
    }

    cam->pos.v[I_X] += cam->vel.v[I_X];
    cam->pos.v[I_Y] += cam->vel.v[I_Y];
    cam->pos.v[I_Z] += cam->vel.v[I_Z];

    tf.x = cam->pos.v[I_X];
    tf.y = cam->pos.v[I_Y];
    tf.z = cam->pos.v[I_Z];
    tf.dir = cam->view_vec;
    tf.vcos = vcos;
    tf.vsin = vsin;
    tf.hcos = hcos;
    tf.hsin = hsin;
}
