#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../inc/process_interface.h"
#include "../../inc/process_process.h"
#include "../../inc/cmd.h"

/* ============================= */
/* 模拟系统时钟                  */
/* ============================= */

static int system_tick = 0;

/* 获取系统tick */

int os_get_tick()
{
    return system_tick;
}

/* 增加tick */

void os_tick()
{
    system_tick++;
}

/* ============================= */
/* 内存接口                      */
/* ============================= */

void* os_malloc(size_t size)
{
    void* ptr = malloc(size);

    if (ptr == NULL)
    {
        os_panic("os_malloc failed");
    }

    return ptr;
}

void os_free(void* ptr)
{
    if (ptr != NULL)
        free(ptr);
}

/* ============================= */
/* 打印接口                      */
/* ============================= */

void os_print(const char* msg)
{
    self_printf("%s", msg);
}

/* ============================= */
/* 设备接口（模拟）              */
/* ============================= */

void os_device_request(int device_id, int pid)
{
    self_printf("[DEVICE] process %d request device %d\n", pid, device_id);
}

void os_device_release(int device_id)
{
    self_printf("[DEVICE] device %d released\n", device_id);
}

/* ============================= */
/* 进程接口                      */
/* ============================= */

/* 创建进程 */

void os_create_process(void (*entry)())
{
    static int proc_id = 0;

    char name[32];

    sprintf(name, "process_%d", proc_id++);

    /* 创建PCB */

    PCB* proc = process_create(name, 10);

    if (proc == NULL)
    {
        os_panic("process_create failed");
    }

    /* 设置入口地址（模拟） */

    proc->pc = (int)(long)entry;
}

/* 主动让出CPU */

void os_yield()
{
    run_process();   // 调用调度器运行下一个时间片
}

/* ============================= */
/* 系统错误处理                  */
/* ============================= */

void os_panic(const char* msg)
{
    self_printf("\n===== KERNEL PANIC =====\n");
    self_printf("%s\n", msg);
    self_printf("========================\n");

    exit(1);
}