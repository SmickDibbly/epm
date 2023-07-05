#ifndef VIEWPOINT_H
#define VIEWPOINT_H

#include <stdint.h>

struct ViewPoint {
    uint32_t x;
    uint32_t y;
    uint16_t horizontal_angle;
    uint16_t vertical_angle;
};

#endif /* VIEWPOINT_H */
