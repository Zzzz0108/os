#include "../../../inc/platform_console.h"

#ifdef _WIN32
#include <windows.h>
#endif

void os_console_enable_utf8(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}
