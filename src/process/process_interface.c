#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../inc/process_interface.h"
#include "../../inc/process_process.h"
#include "../../inc/cmd.h"
/* ============================= */
/* ģ��ϵͳʱ��                  */
/* ============================= */
static int system_tick = 0;
/* ��ȡϵͳtick */
int os_get_tick()
{
    return system_tick;
}
/* ����tick */
void os_tick()
{
    system_tick++;
}
/* ============================= */
/* �ڴ�ӿ�                      */
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
/* ����ӿ�                      */
/* ============================= */
void os_print(const char* msg)
{
    self_printf("%s", msg);
}
/* ============================= */
/* �豸�ӿڣ�ģ�⣩              */
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
/* ���̽ӿ�                      */
/* ============================= */
/* �������� */
void os_create_process(void (*entry)())
{
    static int proc_id = 0;
    char name[32];
    sprintf(name, "process_%d", proc_id++);
    /* ����PCB */
    PCB* proc = process_create(name, 10);
    if (proc == NULL)
    {
        os_panic("process_create failed");
    }
    /* ������ڵ�ַ��ģ�⣩ */
    proc->pc = (int)(long)entry;
}
/* �����ó�CPU */
void os_yield()
{
    run_process();   // ���õ���������һ��ʱ��Ƭ
}
/* ============================= */
/* ϵͳ����                      */
/* ============================= */
void os_panic(const char* msg)
{
    self_printf("\n===== KERNEL PANIC =====\n");
    self_printf("%s\n", msg);
    self_printf("========================\n");
    exit(1);
}
