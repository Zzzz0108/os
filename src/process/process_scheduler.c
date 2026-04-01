#include <stdio.h>
#include <stdlib.h>
#include "../../inc/process_process.h"
#include "../../inc/process_queue.h"
#include "../../inc/process_config.h"
#include "../../inc/cmd.h"
#include "../../inc/mem.h"
/* ÿ������ʱ��Ƭ */
int time_slices[MLFQ_LEVELS] = { TIME_SLICE_Q0, TIME_SLICE_Q1, TIME_SLICE_Q2 };
/* ===============================
   ��������ѡ����һ�����н���
   =============================== */
PCB* scheduler()
{
    for (int i = 0; i < MLFQ_LEVELS; i++)
    {
        if (!queue_is_empty(&pm.ready[i]))
        {
            PCB* proc = dequeue(&pm.ready[i]);
            proc->state = PROCESS_RUNNING;
            pm.running = proc;
            return proc;
        }
    }
    return NULL;
}
/* ===============================
   ���е�ǰ����
   =============================== */
void run_process()
{
    PCB* proc = pm.running;
    if (proc == NULL)
    {
        proc = scheduler();
        if (proc == NULL)
        {
            //self_printf("No process to run\n");
            return;
        }
    }
    self_printf("Running process %d (%s)\n", proc->pid, proc->name);
    // 【新增】：在这里模拟当前进程正在随机读写内存，从而触发缺页中断
    if (proc->mcb) {
        uint32_t seg = rand() % proc->mcb->seg_count; 
        uint32_t page = rand() % 12; // 故意随机 0~11 页，必定超出物理页限制
        uint32_t offset = rand() % PAGE_SIZE;
        uint32_t logical_addr = (seg << 24) | (page << 10) | offset;
        uint8_t data_to_write = (uint8_t)(rand() % 256);
        uint8_t read_data = 0;
        if (rand() % 2) {
            write_memory(proc->mcb, logical_addr, data_to_write);
        } else {
            read_memory(proc->mcb, logical_addr, &read_data);
        }
    }
    /* ģ������һ��ʱ�䵥λ */
    proc->remaining_time--;
    proc->time_slice_used++;
    /* ���̽��� */
    if (proc->remaining_time <= 0)
    {
        self_printf("Process %d finished\n", proc->pid);
        proc->state = PROCESS_TERMINATED;
        process_destroy(proc);
        pm.running = NULL;
        return;
    }
    int level = proc->queue_level;
    /* ʱ��Ƭ���� */
    if (proc->time_slice_used >= time_slices[level])
    {
        proc->time_slice_used = 0;
        /* �����������ܳ�����ͼ��� */
        if (level < MLFQ_LEVELS - 1)
            proc->queue_level++;
        proc->state = PROCESS_READY;
        enqueue(&pm.ready[proc->queue_level], proc);
        pm.running = NULL;
    }
}
