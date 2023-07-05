#include "zigil/zigil_time.h"
#include "src/system/state.h"
#include "src/system/timing.h"


#define NSEC_PER_SEC 1000000000LL
#define MAX_LAG 100000000LL
// one tenth of a second

//#define NSEC_PER_TIC 16666666LL

#define NSEC_PER_TIC 8333333LL
// 120 tics per second

#define NSEC_PER_FRAME 16666666LL
// 60 frames per second

#define FRAME_SAMPLE_PERIOD 60LL
// when things are going well, roughly once per second.

#undef LOG_LABEL
#define LOG_LABEL "SYS.TIME"

static bool fpscapped;
static ntime_t fps_window_start, fps_window_end;
static ntime_t fps_window_elapsed;
static ntime_t frame_start, frame_end;
static ntime_t frame_presleep;
static ntime_t frame_elapsed;
static ntime_t sleep;
static ntime_t lag;
static uint32_t frame_countdown; // every FRAME_SAMPLE_PERIOD frames, compute
static uint64_t frame;
static lldiv_t div_res;
static ntime_t global_start;
static ntime_t global_elapsed;

#define avg_fps(elapsed)                                                \
    (UFix32)((((uint64_t)FRAME_SAMPLE_PERIOD * (uint64_t)NSEC_PER_SEC) << 16)   \
     / (uint64_t)(elapsed))


#define global_avg_fps(elapsed)                         \
    (UFix32)((((uint64_t)frame * (uint64_t)NSEC_PER_SEC) << 16) \
     / (uint64_t)(elapsed))

/*
  Called once before game loop begins. Get an initial value for the start of the lag and FPS sample windows.
*/
epm_Result epm_InitTime(void) {
    _epm_Log("INIT.SYS", LT_INFO, "Initializing time.");
    
    zgl_ClockInit();
    zgl_ClockMode(ZGL_CLOCK_MODE_HIRES);

    global_start = zgl_ClockQueryNano();
    frame_start = global_start;
    fps_window_start = frame_start;
    frame_countdown = FRAME_SAMPLE_PERIOD;
    frame = 0;
    fpscapped = true;

    return EPM_SUCCESS;
}

void epm_FrameTiming(uint32_t *tic_lag_p) {
    /* BEGIN FRAME POST TIMING */
    if (fpscapped) {
        frame_presleep = zgl_ClockQueryNano();
        frame_elapsed = frame_presleep - frame_start;

        if (state.timing.fpscapped) {
            if (NSEC_PER_FRAME > frame_elapsed) {
                sleep = NSEC_PER_FRAME - frame_elapsed;
                //sleep -= 100000LL; // TODO: some attempt to account for sleeping too long
                //div_res = lldiv(sleep, NSEC_PER_SEC);
                //sleep_ts.tv_sec = div_res.quot;
                //sleep_ts.tv_nsec = div_res.rem;
                //debug_printf(("[%lli] nanosleep: %lli nsec\n", frame, sleep));
                zgl_ClockSleep((utime_t)(sleep/1000000));
                //nanosleep(&sleep_ts, NULL);
            }
        }
    }

    frame_end = zgl_ClockQueryNano();
    frame_elapsed = frame_end - frame_start;


    /*
    debug_printf(("[%lli] frame_start: %lli nsec\n"
                  "[%lli] frame_end: %lli nsec\n"
                  "[%lli] frame_elapsed: %lli nsec\n",
                  frame,
                  frame_start,
                  frame,
                  frame_end,
                  frame,
                  frame_elapsed));
    */
    
    if (frame_countdown == 0) {
        fps_window_end = frame_end;
        fps_window_elapsed = fps_window_end - fps_window_start;
        global_elapsed = fps_window_end - global_start;
        
        /*
        debug_printf(("\n[%lli] fps_window_start: %lli nsec\n"
                      "[%lli] fps_window_end: %lli nsec\n"
                      "[%lli] fps_window_elapsed: %lli nsec\n",
                      frame,
                      fps_window_start,
                      frame,
                      fps_window_end,
                      frame,
                      fps_window_elapsed));
        */

        state.timing.local_avg_fps = avg_fps(fps_window_elapsed);
        state.timing.global_avg_fps = global_avg_fps(global_elapsed);
        state.sys.mem = zgl_GetMemoryUsage();
        
        //debug_printf(("[%lli] Average FPS: %s\n", frame, str_fix32(avg_fps_val)));
        
        //get_mem_stat(); // TODO: Probably shouldn't do this here.
        
        frame_countdown = FRAME_SAMPLE_PERIOD;
        fps_window_start = fps_window_end;
    }

    /* END FRAME POST TIMING */

    /* TRANSITION TO NEXT FRAME */
    frame_countdown--;
    frame++;

    /* BEGIN FRAME PRE TIMING */

    // previous frame's end is the new frame's beginning.
    frame_start = frame_end;
    // TODO: But then wouldn't that mean the frame transition was at the moment
    // we read in frame_end ???

    // The "lag" is how "far behind" the game simulation is, in nsec, and equals
    // whatever the existing lag was before the previous frame, plus the length
    // of the previous frame.
    lag += frame_elapsed;

    // TODO: What was my rationale for capping the lag?
    if (lag > MAX_LAG) lag = MAX_LAG;

    // Given that the simulation is lag nsec "behind", this frame must
    // simulate a certain number of game tics, determined as the maximum number
    // of full game tics that fit into lag nsec
    div_res = lldiv(lag, NSEC_PER_TIC);
    *tic_lag_p = (uint32_t)div_res.quot; // lag / NSEC_PER_TIC
    
    // This is the remainder lag, necessarily less than a full tic duration,
    // which will be added onto the next time around.
    lag = div_res.rem; // lag % NSEC_PER_TIC

    /*
    debug_printf(("\n[%lli]  Tic Lag: %lli tics", frame, *tic_lag_p));
    debug_printf(("\n[%lli] Leftover: %lli\n", frame, lag));
    */
    
    /* END FRAME PRE TIMING */
}
