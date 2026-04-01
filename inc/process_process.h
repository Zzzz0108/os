#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
#include "process_config.h"
#include "process_queue.h"
#include "mem.h"

/* ========= 进程状态枚举 ========= */
typedef enum {
    PROCESS_NEW = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} ProcessState;

/* ========= 文件表项结构 ========= */
/* 用于记录每个进程打开的文件信息 */
typedef struct {
    int fd;                 /* 文件描述符（进程内唯一标识） */
    int device_id;          /* 设备ID（0=磁盘文件，1=键盘，2=显示器等） */
    int file_cluster;       /* 文件起始簇号（磁盘文件专用） */
    int position;           /* 当前读写位置（文件内偏移量） */
    int is_open;            /* 是否打开（1=打开，0=关闭） */
} FileTableEntry;

/* ========= 进程控制块 PCB ========= */
typedef struct PCB {
    /* 基本信息 */
    int pid;                        /* 进程ID */
    char name[MAX_NAME_LEN];        /* 进程名称 */
    ProcessState state;             /* 进程当前状态 */

    /* 调度信息（MLFQ多级反馈队列） */
    int queue_level;                /* 当前所在的就绪队列级别（0最高，2最低） */
    int total_time;                 /* 进程总运行时间（模拟用） */
    int remaining_time;             /* 剩余运行时间 */
    int time_slice_used;            /* 当前时间片已使用量 */

    /* CPU状态（模拟，用于保存/恢复上下文） */
    int pc;                         /* 程序计数器 */
    int registers[8];               /* 通用寄存器（模拟） */

    /* 内存信息 */
    struct MCB* mcb;                /* 内存控制块（指向内存模块的MCB结构） */

    /* 【新增】：文件信息 - 进程级文件描述符表 */
    FileTableEntry file_table[MAX_OPEN_FILES];  /* 文件描述符表，每个进程独立 */
    int next_fd;                                 /* 下一个可用的文件描述符编号 */
                                                  /* 注：0,1,2 保留给 stdin/stdout/stderr */

    /* 队列链接指针（用于就绪队列、阻塞队列） */
    struct PCB* next;

} PCB;

/* ========= 进程管理器 ========= */
typedef struct {
    Queue ready[MLFQ_LEVELS];   /* 多级就绪队列（MLFQ） */
    Queue blocked;               /* 阻塞队列 */
    PCB* running;                /* 当前正在运行的进程 */
    int pid_counter;             /* 进程ID计数器（用于分配新PID） */
} ProcessManager;

/* ========= 全局进程管理器 ========= */
extern ProcessManager pm;

/* ========= 初始化函数 ========= */
/* 初始化进程管理系统（清空队列、重置计数器） */
void process_manager_init(void);

/* ========= 进程创建与销毁 ========= */
/* 创建一个新进程
 *   name: 进程名称
 *   runtime: 总运行时间（模拟用）
 *   返回值: 指向新创建PCB的指针，失败返回NULL
 */
PCB* process_create(const char* name, int runtime);

/* 销毁进程（释放所有资源）
 *   proc: 要销毁的进程PCB指针
 */
void process_destroy(PCB* proc);

/* ========= 进程状态切换 ========= */
/* 阻塞当前进程（将其移入阻塞队列）
 *   proc: 要阻塞的进程PCB指针
 */
void process_block(PCB* proc);

/* 唤醒指定进程（从阻塞队列移入就绪队列）
 *   proc: 要唤醒的进程PCB指针
 */
void process_wakeup(PCB* proc);

/* ========= 进程调度 ========= */
/* 调度器：从就绪队列中选择下一个要运行的进程
 *   返回值: 被选中的进程PCB指针，如果没有就绪进程返回NULL
 */
PCB* scheduler(void);

/* 运行当前进程（模拟进程执行，包含时间片管理）
 *   如果当前没有运行进程，则调用调度器选择新进程
 */
void run_process(void);

/* ========= 调试/输出函数 ========= */
/* 打印单个进程信息 */
void print_process(PCB* proc);

/* 打印整个系统状态（运行进程、所有就绪队列、阻塞队列） */
void print_system_state(void);

#endif /* PROCESS_H */