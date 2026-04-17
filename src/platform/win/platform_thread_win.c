#include "../../../inc/platform_thread.h"

#ifdef _WIN32
#include <windows.h>

os_thread_t os_thread_create(os_thread_func_t func, void* arg) {
    if (func == NULL) {
        return NULL;
    }
    return (os_thread_t)CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL);
}

int os_thread_join(os_thread_t thread) {
    if (thread == NULL) {
        return -1;
    }
    return WaitForSingleObject((HANDLE)thread, INFINITE) == WAIT_OBJECT_0 ? 0 : -1;
}

void os_thread_close(os_thread_t thread) {
    if (thread != NULL) {
        CloseHandle((HANDLE)thread);
    }
}

#else

os_thread_t os_thread_create(os_thread_func_t func, void* arg) {
    (void)func;
    (void)arg;
    return NULL;
}

int os_thread_join(os_thread_t thread) {
    (void)thread;
    return -1;
}

void os_thread_close(os_thread_t thread) {
    (void)thread;
}

#endif
