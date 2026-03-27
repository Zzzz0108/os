#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
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

    /* ���� */
    int queue_level;
    int total_time;
    int remaining_time;
    int time_slice_used;

    /* CPU״̬��ģ�⣩ */
    int pc;
    int registers[8];

    /* �ڴ���Ϣ */
    MemControlBlock* mcb;
    
    /* �ļ���Ϣ */
    int open_files[MAX_OPEN_FILES];

    /* ���� */
    struct PCB* next;
} PCB;

/* ========= ���̹����� ========= */
typedef struct {
    Queue ready[MLFQ_LEVELS];
    Queue blocked;
    PCB* running;
    int pid_counter;
} ProcessManager;

/* ========= ��ʼ�� ========= */
void process_manager_init();

/* ========= ���̲��� ========= */
PCB* process_create(const char* name, int runtime);
void process_destroy(PCB* proc);

/* ========= ״̬�仯 ========= */
void process_block(PCB* proc);
void process_wakeup(PCB* proc);

/* ========= ���� ========= */
PCB* scheduler();
void run_process();

/* ========= ���� ========= */
void print_process(PCB* proc);
void print_system_state();

/* ========= ȫ�ֽ��̹����� ========= */
extern ProcessManager pm;

#endif