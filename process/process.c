#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"
#include "os_interface.h"
#include "queue.h"

/* ========= 全局进程管理器 ========= */
ProcessManager pm;

/* ============================= */
/* 初始化进程管理器              */
/* ============================= */
void process_manager_init()
{
    for (int i = 0; i < MLFQ_LEVELS; i++)
        queue_init(&pm.ready[i]);

    queue_init(&pm.blocked);

    pm.running = NULL;
    pm.pid_counter = 1;
}

/* ============================= */
/* 创建进程                      */
/* ============================= */
PCB* process_create(const char* name, int runtime)
{
    PCB* proc = (PCB*)os_malloc(sizeof(PCB));

    memset(proc, 0, sizeof(PCB));

    proc->pid = pm.pid_counter++;
    strncpy(proc->name, name, MAX_NAME_LEN - 1);

    proc->state = PROCESS_READY;
    proc->queue_level = 0;
    proc->total_time = runtime;
    proc->remaining_time = runtime;
    proc->time_slice_used = 0;
    proc->pc = 0;
    proc->mem_base = NULL;
    proc->mem_size = 0;
    proc->next = NULL;

    enqueue(&pm.ready[0], proc);

    return proc;
}

/* ============================= */
/* 销毁进程                      */
/* ============================= */
void process_destroy(PCB* proc)
{
    if (proc == NULL)
        return;

    proc->state = PROCESS_TERMINATED;
    os_free(proc);
}

/* ============================= */
/* 阻塞进程                      */
/* ============================= */
void process_block(PCB* proc)
{
    if (proc == NULL)
        return;

    proc->state = PROCESS_BLOCKED;
    enqueue(&pm.blocked, proc);
}

/* ============================= */
/* 唤醒进程                      */
/* ============================= */
void process_wakeup(PCB* proc)
{
    if (proc == NULL)
        return;

    queue_remove(&pm.blocked, proc);
    proc->state = PROCESS_READY;
    proc->queue_level = 0;

    enqueue(&pm.ready[0], proc);
}

/* ============================= */
/* 打印单个进程信息              */
/* ============================= */
void print_process(PCB* proc)
{
    if (proc == NULL) return;

    printf("PID:%d  NAME:%s  STATE:%d  Q:%d  REMAIN:%d\n",
        proc->pid,
        proc->name,
        proc->state,
        proc->queue_level,
        proc->remaining_time);
}

/* ============================= */
/* 打印系统状态                  */
/* ============================= */
void print_system_state()
{
    printf("\n===== SYSTEM STATE =====\n");

    printf("Running:\n");
    if (pm.running)
        print_process(pm.running);
    else
        printf("None\n");

    for (int i = 0; i < MLFQ_LEVELS; i++)
    {
        printf("\nReady Queue Q%d:\n", i);
        PCB* p = pm.ready[i].front;
        while (p)
        {
            print_process(p);
            p = p->next;
        }
    }

    printf("\nBlocked Queue:\n");
    PCB* p = pm.blocked.front;
    while (p)
    {
        print_process(p);
        p = p->next;
    }

    printf("========================\n");
}