#ifndef STATE_H
#define STATE_H

#include "zigil/zigil_event.h"

#include "src/misc/epm_includes.h"

typedef struct State {
    struct {
        bool fpscapped; // unused as of 2023-02-08
        UFix32 global_avg_tpf;
        UFix32 local_avg_tpf;
        UFix32 global_avg_fps;
        UFix32 local_avg_fps;
    } timing;
    struct {
        uint64_t tic;
        uint64_t frame;
    } loop;
    struct {
        int pointer_x;
        int pointer_y;
        int pointer_rel_x;
        int pointer_rel_y;
        zgl_LongKeyCode last_press;
        char *win_below_name;
    } input;
    struct {
        int64_t clock_ticks_per_sec;
        int64_t mem;
    } sys;
} State;

extern State state;

#endif /* STATE_H */
