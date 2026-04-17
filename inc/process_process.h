#ifndef PROCESS_H
#define PROCESS_H
#include <stdio.h>
#include <stdint.h>
#include "process_config.h"
#include "process_queue.h"
#include "mem.h"
/* ========= ����״̬ ========= */
typedef enum {
    PROCESS_NEW = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} ProcessState;
/* ========= PCB ========= */
typedef struct PCB {
    int pid;
    char name[MAX_NAME_LEN];
    ProcessState state;
    /* 调度 */
    int queue_level;
    int total_time;
    int remaining_time;
    int time_slice_used;
    /* CPU状态（模拟） */
    intptr_t pc;
    int registers[8];
    /* �ڴ���Ϣ */
    MemControlBlock* mcb;
    /* �ļ���Ϣ */
    int open_files[MAX_OPEN_FILES];
    /* 链表 */
    struct PCB* next;
} PCB;
/* ========= 进程管理器 ========= */
typedef struct {
    Queue ready[MLFQ_LEVELS];
    Queue blocked;
    PCB* running;
    int pid_counter;
} ProcessManager;
/* ========= 初始化 ========= */
void process_manager_init();
/* ========= 进程操作 ========= */
PCB* process_create(const char* name, int runtime);
void process_destroy(PCB* proc);
/* ========= 状态变化 ========= */
void process_block(PCB* proc);
void process_wakeup(PCB* proc);
/* ========= 调度 ========= */
PCB* scheduler();
void run_process();
void process_set_trace_enabled(int enabled);
int process_is_trace_enabled(void);
/* ========= 调试 ========= */
void print_process(PCB* proc);
void print_system_state();
/* ========= 全局进程管理器 ========= */
extern ProcessManager pm;
#endif
