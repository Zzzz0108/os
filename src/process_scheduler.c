#include <stdio.h>
#include "process.h"
#include "queue.h"
#include "config.h"

/* 每级队列时间片 */
int time_slices[MLFQ_LEVELS] = { TIME_SLICE_Q0, TIME_SLICE_Q1, TIME_SLICE_Q2 };

/* ===============================
   调度器：选择下一个运行进程
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
   运行当前进程
   =============================== */
void run_process()
{
    PCB* proc = pm.running;

    if (proc == NULL)
    {
        proc = scheduler();
        if (proc == NULL)
        {
            //printf("No process to run\n");
            return;
        }
    }

    printf("Running process %d (%s)\n", proc->pid, proc->name);

    /* 模拟运行一个时间单位 */
    proc->remaining_time--;
    proc->time_slice_used++;

    /* 进程结束 */
    if (proc->remaining_time <= 0)
    {
        printf("Process %d finished\n", proc->pid);

        proc->state = PROCESS_TERMINATED;
        process_destroy(proc);

        pm.running = NULL;
        return;
    }

    int level = proc->queue_level;

    /* 时间片用完 */
    if (proc->time_slice_used >= time_slices[level])
    {
        proc->time_slice_used = 0;

        /* 降级（但不能超过最低级） */
        if (level < MLFQ_LEVELS - 1)
            proc->queue_level++;

        proc->state = PROCESS_READY;
        enqueue(&pm.ready[proc->queue_level], proc);

        pm.running = NULL;
    }
}