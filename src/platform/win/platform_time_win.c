#include "../../../inc/platform_time.h"

#ifdef _WIN32
#include <windows.h>
#endif

void os_sleep_ms(unsigned int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    (void)ms;
#endif
}
