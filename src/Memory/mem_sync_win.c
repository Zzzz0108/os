#include "../../inc/mem_sync.h"
#include <windows.h>
#include <stdlib.h>
/* 使用 Windows 的关键代码段 (Critical Section) 来实现轻量级互斥锁 */
os_mutex_t os_mutex_create(void) {
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    if (cs != NULL) {
        InitializeCriticalSection(cs);
    }
    return (os_mutex_t)cs;
}
void os_mutex_lock(os_mutex_t mutex) {
    if (mutex != NULL) {
        EnterCriticalSection((CRITICAL_SECTION*)mutex);
    }
}
void os_mutex_unlock(os_mutex_t mutex) {
    if (mutex != NULL) {
        LeaveCriticalSection((CRITICAL_SECTION*)mutex);
    }
}
void os_mutex_destroy(os_mutex_t mutex) {
    if (mutex != NULL) {
        DeleteCriticalSection((CRITICAL_SECTION*)mutex);
        free(mutex);
    }
}
