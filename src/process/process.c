#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../inc/process_process.h"
#include "../../inc/process_interface.h"
#include "../../inc/process_queue.h"
#include "../../inc/process_syscall.h"
#include "../../inc/cmd.h"
#include "../../inc/mem.h"

/* ========= 全局进程管理器 ========= */
ProcessManager pm;

/* ============================= */
/* 初始化进程管理器              */
/* ============================= */
void process_manager_init(void)
{
    /* 初始化所有就绪队列（MLFQ共3级） */
    for (int i = 0; i < MLFQ_LEVELS; i++) {
        queue_init(&pm.ready[i]);
    }

    /* 初始化阻塞队列 */
    queue_init(&pm.blocked);

    /* 初始状态：没有正在运行的进程 */
    pm.running = NULL;

    /* 进程ID从1开始计数（0保留给空闲进程） */
    pm.pid_counter = 1;
}

/* ============================= */
/* 创建进程                      */
/* ============================= */
PCB* process_create(const char* name, int runtime)
{
    /* 分配PCB内存 */
    PCB* proc = (PCB*)os_malloc(sizeof(PCB));
    if (proc == NULL) {
        self_printf("[Process] ERROR: Failed to allocate PCB\n");
        return NULL;
    }

    /* 清空PCB结构体 */
    memset(proc, 0, sizeof(PCB));

    /* ========= 设置基本信息 ========= */
    proc->pid = pm.pid_counter++;           /* 分配唯一PID */
    strncpy(proc->name, name, MAX_NAME_LEN - 1);
    proc->name[MAX_NAME_LEN - 1] = '\0';    /* 确保字符串结尾 */

    /* ========= 设置调度信息 ========= */
    proc->state = PROCESS_READY;             /* 初始状态为就绪 */
    proc->queue_level = 0;                  /* 新进程放入最高优先级队列 */
    proc->total_time = runtime;              /* 总运行时间 */
    proc->remaining_time = runtime;          /* 剩余时间（初始等于总时间） */
    proc->time_slice_used = 0;               /* 时间片使用量清零 */
    proc->pc = 0;                            /* 程序计数器初始为0 */

    /* ========= 设置内存信息 ========= */
    /* 调用内存模块：为进程创建独立的段页式内存空间 */
    proc->mcb = create_process_memory(proc->pid);
    if (proc->mcb == NULL) {
        self_printf("[Process] ERROR: Failed to create memory for process %d\n", proc->pid);
        os_free(proc);
        return NULL;
    }

    /* ========= 【新增】：初始化文件描述符表 ========= */
    /* 清空所有文件表项，标记为未打开 */
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        proc->file_table[i].is_open = 0;     /* 标记为关闭状态 */
        proc->file_table[i].fd = -1;         /* 文件描述符无效 */
        proc->file_table[i].device_id = 0;
        proc->file_table[i].file_cluster = 0;
        proc->file_table[i].position = 0;
    }
    /* 下一个可用的文件描述符从3开始
     * 注：0=标准输入(stdin)，1=标准输出(stdout)，2=标准错误(stderr)
     * 这些保留给控制台设备使用
     */
    proc->next_fd = 3;

    /* ========= 队列链接指针 ========= */
    proc->next = NULL;

    /* 将新进程放入最高优先级的就绪队列（队列0） */
    enqueue(&pm.ready[0], proc);

    self_printf("[Process] Created process %d (%s), runtime=%d, memory allocated\n",
        proc->pid, proc->name, runtime);

    return proc;
}

/* ============================= */
/* 销毁进程                      */
/* ============================= */
void process_destroy(PCB* proc)
{
    if (proc == NULL) {
        return;
    }

    self_printf("[Process] Destroying process %d (%s)\n", proc->pid, proc->name);

    /* 设置进程状态为已终止 */
    proc->state = PROCESS_TERMINATED;

    /* ========= 【新增】：进程退出时关闭所有打开的文件 ========= */
    /* 遍历文件描述符表，关闭所有仍处于打开状态的文件 */
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (proc->file_table[i].is_open) {
            self_printf("[Process] Closing file fd=%d for process %d (%s)\n",
                proc->file_table[i].fd, proc->pid, proc->name);

            /* 注意：这里不直接调用文件系统的关闭函数（避免循环依赖）
             * 而是通过系统调用层通知文件系统模块
             * 文件系统模块应该在收到通知后执行实际的关闭操作
             *
             * 实际实现中，可以在这里调用：
             *   sys_notify_file_close(proc->pid, proc->file_table[i].fd);
             * 或者由文件系统模块注册回调函数
             */

             /* 标记为关闭（释放文件描述符） */
            proc->file_table[i].is_open = 0;
            proc->file_table[i].fd = -1;
        }
    }

    /* ========= 回收内存资源 ========= */
    /* 调用内存模块：释放进程占用的所有物理页和段表/页表 */
    if (proc->mcb) {
        destroy_process_memory(proc->mcb);
        self_printf("[Process] Freed memory for process %d\n", proc->pid);
    }

    /* 释放PCB本身占用的内存 */
    os_free(proc);

    self_printf("[Process] Process %d destroyed\n", proc->pid);
}

/* ============================= */
/* 阻塞进程                      */
/* ============================= */
void process_block(PCB* proc)
{
    if (proc == NULL) {
        return;
    }

    self_printf("[Process] Blocking process %d (%s)\n", proc->pid, proc->name);

    /* 改变进程状态为阻塞 */
    proc->state = PROCESS_BLOCKED;

    /* 将进程移入阻塞队列 */
    enqueue(&pm.blocked, proc);
}

/* ============================= */
/* 唤醒进程                      */
/* ============================= */
void process_wakeup(PCB* proc)
{
    if (proc == NULL) {
        return;
    }

    self_printf("[Process] Waking up process %d (%s)\n", proc->pid, proc->name);

    /* 从阻塞队列中移除 */
    queue_remove(&pm.blocked, proc);

    /* 恢复为就绪状态 */
    proc->state = PROCESS_READY;

    /* 重置队列级别为最高优先级（被唤醒的进程获得优待） */
    proc->queue_level = 0;

    /* 重置时间片使用量（新时间片） */
    proc->time_slice_used = 0;

    /* 放入最高优先级的就绪队列 */
    enqueue(&pm.ready[0], proc);
}

/* ============================= */
/* 打印进程信息                  */
/* ============================= */
void print_process(PCB* proc)
{
    if (proc == NULL) {
        return;
    }

    /* 打印进程基本信息 */
    self_printf("  PID:%d  NAME:%-10s  STATE:%d  Q:%d  REMAIN:%d  FILES:%d\n",
        proc->pid,
        proc->name,
        proc->state,
        proc->queue_level,
        proc->remaining_time,
        proc->next_fd - 3  /* 已打开文件数 = next_fd - 3 */
    );

    /* 【可选】：打印进程打开的文件列表（调试用） */
#ifdef DEBUG_PROCESS_FILES
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (proc->file_table[i].is_open) {
            self_printf("      fd=%d, cluster=%d, pos=%d\n",
                proc->file_table[i].fd,
                proc->file_table[i].file_cluster,
                proc->file_table[i].position
            );
        }
    }
#endif
}

/* ============================= */
/* 打印系统状态                  */
/* ============================= */
void print_system_state(void)
{
    self_printf("\n===== SYSTEM STATE =====\n");
    self_printf("Total processes: %d (Running: %d, Ready: %d, Blocked: %d)\n",
        sys_get_process_count(),
        (pm.running != NULL) ? 1 : 0,
        sys_get_ready_count(),
        sys_get_blocked_count()
    );

    /* 打印当前运行的进程 */
    self_printf("\nRunning Process:\n");
    if (pm.running) {
        print_process(pm.running);
    }
    else {
        self_printf("  None\n");
    }

    /* 打印所有就绪队列 */
    for (int i = 0; i < MLFQ_LEVELS; i++) {
        self_printf("\nReady Queue Q%d (size=%d):\n", i, pm.ready[i].size);
        PCB* p = pm.ready[i].front;
        while (p) {
            print_process(p);
            p = p->next;
        }
        if (pm.ready[i].size == 0) {
            self_printf("  (empty)\n");
        }
    }

    /* 打印阻塞队列 */
    self_printf("\nBlocked Queue (size=%d):\n", pm.blocked.size);
    PCB* p = pm.blocked.front;
    while (p) {
        print_process(p);
        p = p->next;
    }
    if (pm.blocked.size == 0) {
        self_printf("  (empty)\n");
    }

    self_printf("========================\n");
}