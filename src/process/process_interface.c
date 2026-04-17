#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../../inc/process_interface.h"
#include "../../inc/process_process.h"
#include "../../inc/cmd.h"
/* ============================= */
/* 模锟斤拷系统时锟斤拷                  */
/* ============================= */
static int system_tick = 0;
/* 锟斤拷取系统tick */
int os_get_tick()
{
    return system_tick;
}
/* 锟斤拷锟斤拷tick */
void os_tick()
{
    system_tick++;
}
/* ============================= */
/* 锟节达拷涌锟                      */
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
/* 锟斤拷锟斤拷涌锟                      */
/* ============================= */
void os_print(const char* msg)
{
    self_printf("%s", msg);
}
/* ============================= */
/* 锟借备锟接口ｏ拷模锟解）              */
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
/* 锟斤拷锟教接匡拷                      */
/* ============================= */
/* 锟斤拷锟斤拷锟斤拷锟斤拷 */
void os_create_process(void (*entry)())
{
    static int proc_id = 0;
    char name[32];
    sprintf(name, "process_%d", proc_id++);
    /* 锟斤拷锟斤拷PCB */
    PCB* proc = process_create(name, -1);
    if (proc == NULL)
    {
        os_panic("process_create failed");
    }
    /* 锟斤拷锟斤拷锟斤拷诘锟街凤拷锟侥ｏ拷猓 */
    proc->pc = (intptr_t)entry;
}
/* 锟斤拷锟斤拷锟矫筹拷CPU */
void os_yield()
{
    run_process();   // 锟斤拷锟矫碉拷锟斤拷锟斤拷锟斤拷锟斤拷一锟斤拷时锟斤拷片
}
/* ============================= */
/* 系统锟斤拷锟斤拷                      */
/* ============================= */
void os_panic(const char* msg)
{
    self_printf("\n===== KERNEL PANIC =====\n");
    self_printf("%s\n", msg);
    self_printf("========================\n");
    exit(1);
}
