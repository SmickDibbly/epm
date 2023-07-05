#include "src/misc/epm_includes.h"
#include "src/system/state.h"
#include "src/system/timing.h"
#include "src/system/loop.h"

epm_Result epm_Run(void) {
    while (true) { //frame loop
        static uint32_t tic_lag;
        epm_FrameTiming(&tic_lag);
        
        for (uint32_t tic = 0; tic < tic_lag; tic++) { // tic loop
            // the tic loop performs as many full game tics as can fit within
            // the amount of time the previous frame took to render.

            if (epm_DoInput() == EPM_STOP) goto Stop;
            if (epm_Tic() == EPM_STOP) goto Stop;
            state.loop.tic++;
        }
        
        if (epm_Render() == EPM_STOP) goto Stop;

        state.loop.frame++;
        state.timing.global_avg_tpf = (UFix32)((state.loop.tic << 16)/(state.loop.frame));
    }

 Stop:
    return EPM_SUCCESS;
}
