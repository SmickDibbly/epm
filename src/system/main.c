#include "src/misc/epm_includes.h"

extern epm_Result epm_Init(int argc, char *argv[]);
extern epm_Result epm_Run(void);
extern epm_Result epm_Term(void);

int main(int argc, char *argv[]) {
    puts("You have chosen to activate the Electric Pentacle; this was a wise choice.");
    puts("Compiled on " __DATE__ " at " __TIME__ " for platform \"" EPM_PLATFORM_STR "\".");
    if (epm_Init(argc, argv) != EPM_SUCCESS) {
        puts("The Electric Pentacle could not be initialized. Too bad.");
        return -1;
    }
    
    epm_Run();

    if (epm_Term() != EPM_SUCCESS) {
        puts("The Electric Pentacle did not shut down properly. Good luck.");
        return -2;
    }
    puts("You have deactivated the Electric Pentacle. Good luck.");
    return EXIT_SUCCESS;
}
