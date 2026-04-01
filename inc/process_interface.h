#ifndef OS_INTERFACE_H
#define OS_INTERFACE_H
#include <stddef.h>
/* ========= 内存接口 ========= */
void* os_malloc(size_t size);
void  os_free(void* ptr);
/* ========= 输出接口 ========= */
void os_print(const char* msg);
/* ========= 定时器接口 ========= */
int os_get_tick();
/* ========= 设备接口 ========= */
void os_device_request(int device_id, int pid);
void os_device_release(int device_id);
/* ========= 进程接口 ========= */
void os_create_process(void (*entry)());
void os_yield();
/* ========= 调试接口 ========= */
void os_panic(const char* msg);
#endif
