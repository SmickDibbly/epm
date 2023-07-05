#ifndef PLAYER_H
#define PLAYER_H

#include "src/world/geometry.h"

//Player "jogs" at 16 units per tic, so that's 120 * 16 = 1920 units per second. Since average human jogging speed is around 8 km/h, or 2.222 m/s, this means that 1920 units is around 2.222 meters. So 1 meter is approximately 864 units. The world square is therefore around 75 meters per side.
#define PLAYER_MAX_SPEED (1 << 16) // WorldUnits per tic
#define PLAYER_MIN_SPEED (1 << 9) // WorldUnits per tic
#define PLAYER_MAX_ACCELERATION (1 << 10)
#define PLAYER_MAX_TURNSPEED (128 << 2) // ang18's per tic
#define PLAYER_REL_VIEW_Z fixify(40) // WorldUnits
#define PLAYER_COLLISION_RADIUS fixify(16) // WorldUnits
#define PLAYER_COLLISION_BOX_REACH (PLAYER_COLLISION_RADIUS + fixify(8)) // WorldUnits
#define PLAYER_COLLISION_HEIGHT fixify(48) // WorldUnits
#define PLAYER_MAX_VIEW_ANGLE_V (ANG18_PIBY2)

typedef struct Player {
    // Pawn
    WorldVec pos; // x and y are center, z is foot level

    // Looker
    WorldUnit rel_view_z; // height of eyeline above z = foot level
    Ang18 view_angle_h; // valid in [0.0, 65536.0) ie. entire range, and
                           // determines the view vector x and y
    Ang18 view_angle_v; // valid between [0.0, 32768.0), and determines the
                           // view vector z

    bool key_angular_motion;
    int32_t key_angular_h;
    int32_t key_angular_v;

    bool mouse_angular_motion;
    int32_t mouse_angular_h;
    int32_t mouse_angular_v;
    
    WorldVec view_vec;
    Fix32Vec_2D view_vec_XY;
    
    // Mover
    bool key_motion;
    Ang18 key_motion_angle;
    bool mouse_motion;
    WorldVec mouse_motion_vel;
    
    WorldVec vel; // determined by above

    // Accelerator
    WorldVec acc;
    
    // Collider
    WorldUnit collision_radius;
    WorldUnit collision_height;
    WorldUnit collision_box_reach;

    // Has health
    uint16_t armor;
    uint16_t health;
} Player;

extern void onTic_player(void *self);

#endif /* PLAYER_H */
