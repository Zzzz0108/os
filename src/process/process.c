﻿#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../inc/process_process.h"
#include "../../inc/process_interface.h"
#include "../../inc/process_queue.h"
#include "../../inc/cmd.h"
#include "../../inc/mem.h"
/* ========= ȫ�ֽ��̹����� ========= */
ProcessManager pm;
/* ============================= */
/* ��ʼ�����̹�����              */
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
/* ��������                      */
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
    proc->mcb = create_process_memory(proc->pid);
    proc->next = NULL;
    enqueue(&pm.ready[0], proc);
    return proc;
}
/* ============================= */
/* ���ٽ���                      */
/* ============================= */
void process_destroy(PCB* proc)
{
    if (proc == NULL)
        return;
    proc->state = PROCESS_TERMINATED;
    // 【修改】：在此处回收进程占用的物理内存
    if (proc->mcb) {
        destroy_process_memory(proc->mcb);
    }
    os_free(proc);
}
/* ============================= */
/* ��������                      */
/* ============================= */
void process_block(PCB* proc)
{
    if (proc == NULL)
        return;
    proc->state = PROCESS_BLOCKED;
    enqueue(&pm.blocked, proc);
}
/* ============================= */
/* ���ѽ���                      */
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
/* ��ӡ����������Ϣ              */
/* ============================= */
void print_process(PCB* proc)
{
    if (proc == NULL) return;
    self_printf("PID:%d  NAME:%s  STATE:%d  Q:%d  REMAIN:%d\n",
        proc->pid,
        proc->name,
        proc->state,
        proc->queue_level,
        proc->remaining_time);
}
/* ============================= */
/* ��ӡϵͳ״̬                  */
/* ============================= */
void print_system_state()
{
    self_printf("\n===== SYSTEM STATE =====\n");
    self_printf("Running:\n");
    if (pm.running)
        print_process(pm.running);
    else
        self_printf("None\n");
    for (int i = 0; i < MLFQ_LEVELS; i++)
    {
        self_printf("\nReady Queue Q%d:\n", i);
        PCB* p = pm.ready[i].front;
        while (p)
        {
            print_process(p);
            p = p->next;
        }
    }
    self_printf("\nBlocked Queue:\n");
    PCB* p = pm.blocked.front;
    while (p)
    {
        print_process(p);
        p = p->next;
    }
    self_printf("========================\n");
}
