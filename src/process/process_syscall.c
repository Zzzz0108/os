#include "../../inc/process_syscall.h"
#include "../../inc/process_process.h"
#include "../../inc/process_queue.h"
#include "../../inc/cmd.h"
#include "../../inc/mem.h"

/* ============================================
   进程查询接口实现
   ============================================ */

   /* 获取当前运行的进程ID */
int sys_get_pid(void) {
    if (pm.running) {
        return pm.running->pid;
    }
    return -1;
}

/* 获取当前运行的进程PCB指针 */
PCB* sys_get_current_process(void) {
    return pm.running;
}

/* 根据PID获取PCB指针 */
PCB* sys_get_process_by_pid(int pid) {
    // 检查当前运行进程
    if (pm.running && pm.running->pid == pid) {
        return pm.running;
    }

    // 检查就绪队列
    for (int i = 0; i < MLFQ_LEVELS; i++) {
        PCB* p = pm.ready[i].front;
        while (p) {
            if (p->pid == pid) {
                return p;
            }
            p = p->next;
        }
    }

    // 检查阻塞队列
    PCB* p = pm.blocked.front;
    while (p) {
        if (p->pid == pid) {
            return p;
        }
        p = p->next;
    }

    return NULL;
}

/* 获取当前进程的MCB */
struct MCB* sys_get_current_mcb(void) {
    if (pm.running) {
        return pm.running->mcb;
    }
    return NULL;
}

/* 获取指定进程的MCB */
struct MCB* sys_get_process_mcb(int pid) {
    PCB* proc = sys_get_process_by_pid(pid);
    if (proc) {
        return proc->mcb;
    }
    return NULL;
}

/* ============================================
   进程状态控制接口实现
   ============================================ */

   /* 阻塞当前进程 */
void sys_block_current(void) {
    PCB* proc = pm.running;
    if (proc == NULL) {
        self_printf("[Syscall] ERROR: No running process to block\n");
        return;
    }

    self_printf("[Syscall] Process %d (%s) blocked\n", proc->pid, proc->name);

    // 设置进程状态为阻塞
    proc->state = PROCESS_BLOCKED;

    // 从运行状态移除
    pm.running = NULL;

    // 加入阻塞队列
    enqueue(&pm.blocked, proc);

    // 调度下一个进程
    run_process();
}

/* 唤醒指定PID的进程 */
int sys_wakeup_pid(int pid) {
    // 在阻塞队列中查找
    PCB* proc = pm.blocked.front;
    PCB* prev = NULL;

    while (proc) {
        if (proc->pid == pid) {
            // 从阻塞队列移除
            if (prev == NULL) {
                pm.blocked.front = proc->next;
            }
            else {
                prev->next = proc->next;
            }
            if (pm.blocked.rear == proc) {
                pm.blocked.rear = prev;
            }
            pm.blocked.size--;

            // 放入就绪队列（最高优先级）
            proc->state = PROCESS_READY;
            proc->queue_level = 0;
            proc->time_slice_used = 0;
            enqueue(&pm.ready[0], proc);

            self_printf("[Syscall] Process %d (%s) woken up\n", pid, proc->name);
            return 0;
        }
        prev = proc;
        proc = proc->next;
    }

    self_printf("[Syscall] ERROR: Process %d not found in blocked queue\n", pid);
    return -1;
}

/* 让出CPU */
void sys_yield(void) {
    int pid = sys_get_pid();
    if (pid >= 0) {
        self_printf("[Syscall] Process %d yielded CPU\n", pid);
    }
    run_process();
}

/* 终止当前进程 */
void sys_exit(void) {
    PCB* proc = pm.running;
    if (proc == NULL) {
        self_printf("[Syscall] ERROR: No running process to exit\n");
        return;
    }

    self_printf("[Syscall] Process %d (%s) exited\n", proc->pid, proc->name);

    // 销毁进程（会回收内存）
    process_destroy(proc);
    pm.running = NULL;

    // 调度下一个进程
    run_process();
}

/* ============================================
   等待队列同步接口实现
   ============================================ */

   /* 将当前进程加入等待队列并阻塞 */
void sys_sleep_on(Queue* wait_queue) {
    PCB* proc = pm.running;
    if (proc == NULL) {
        self_printf("[Syscall] ERROR: No running process to sleep\n");
        return;
    }

    if (wait_queue == NULL) {
        self_printf("[Syscall] ERROR: Invalid wait queue\n");
        return;
    }

    self_printf("[Syscall] Process %d (%s) sleeping on queue\n", proc->pid, proc->name);

    // 设置状态为阻塞
    proc->state = PROCESS_BLOCKED;

    // 从运行状态移除
    pm.running = NULL;

    // 加入指定的等待队列
    enqueue(wait_queue, proc);

    // 调度下一个进程
    run_process();
}

/* 唤醒等待队列上的一个进程 */
void sys_wakeup_one(Queue* wait_queue) {
    if (wait_queue == NULL || queue_is_empty(wait_queue)) {
        return;
    }

    PCB* proc = dequeue(wait_queue);
    if (proc) {
        self_printf("[Syscall] Waking up one process from queue: %d (%s)\n",
            proc->pid, proc->name);

        proc->state = PROCESS_READY;
        proc->queue_level = 0;
        proc->time_slice_used = 0;
        enqueue(&pm.ready[0], proc);
    }
}

/* 唤醒等待队列上的所有进程 */
void sys_wakeup_all(Queue* wait_queue) {
    if (wait_queue == NULL) {
        return;
    }

    int count = 0;
    while (!queue_is_empty(wait_queue)) {
        PCB* proc = dequeue(wait_queue);
        if (proc) {
            proc->state = PROCESS_READY;
            proc->queue_level = 0;
            proc->time_slice_used = 0;
            enqueue(&pm.ready[0], proc);
            count++;
        }
    }

    if (count > 0) {
        self_printf("[Syscall] Woken up %d processes from queue\n", count);
    }
}

/* ============================================
   调试/信息接口实现
   ============================================ */

   /* 打印所有进程状态 */
void sys_print_processes(void) {
    print_system_state();
}

/* 打印单个进程信息 */
void sys_print_process(PCB* proc) {
    print_process(proc);
}

/* ============================================
   进程管理接口实现（供内核使用）
   ============================================ */

   /* 创建新进程 */
PCB* sys_create_process(const char* name, int runtime) {
    PCB* proc = process_create(name, runtime);
    if (proc) {
        self_printf("[Syscall] Created process %d (%s), runtime=%d\n",
            proc->pid, proc->name, runtime);
    }
    else {
        self_printf("[Syscall] ERROR: Failed to create process %s\n", name);
    }
    return proc;
}

/* 销毁进程 */
void sys_destroy_process(PCB* proc) {
    if (proc == NULL) {
        return;
    }
    self_printf("[Syscall] Destroying process %d (%s)\n", proc->pid, proc->name);
    process_destroy(proc);
}

/* 获取进程总数 */
int sys_get_process_count(void) {
    int count = 0;

    // 运行进程
    if (pm.running) {
        count++;
    }

    // 就绪队列
    for (int i = 0; i < MLFQ_LEVELS; i++) {
        count += pm.ready[i].size;
    }

    // 阻塞队列
    count += pm.blocked.size;

    return count;
}

/* 获取就绪队列进程数 */
int sys_get_ready_count(void) {
    int count = 0;
    for (int i = 0; i < MLFQ_LEVELS; i++) {
        count += pm.ready[i].size;
    }
    return count;
}

/* 获取阻塞队列进程数 */
int sys_get_blocked_count(void) {
    return pm.blocked.size;
}

/* ============================================
   文件相关系统调用（供文件系统模块调用）
   ============================================ */

   /* 为当前进程分配一个文件描述符 */
int sys_alloc_fd(int file_cluster)
{
    PCB* proc = pm.running;
    if (proc == NULL) {
        self_printf("[Syscall] ERROR: No running process\n");
        return -1;
    }

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!proc->file_table[i].is_open) {
            proc->file_table[i].is_open = 1;
            proc->file_table[i].fd = proc->next_fd++;
            proc->file_table[i].file_cluster = file_cluster;
            proc->file_table[i].position = 0;
            proc->file_table[i].device_id = 0;  // 0=磁盘文件

            self_printf("[Syscall] Process %d allocated fd=%d for cluster=%d\n",
                proc->pid, proc->file_table[i].fd, file_cluster);
            return proc->file_table[i].fd;
        }
    }

    self_printf("[Syscall] ERROR: Process %d file table full\n", proc->pid);
    return -1;  // 文件表已满
}

/* 根据文件描述符获取文件信息 */
FileTableEntry* sys_get_file_entry(int fd)
{
    PCB* proc = pm.running;
    if (proc == NULL) {
        return NULL;
    }

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (proc->file_table[i].is_open && proc->file_table[i].fd == fd) {
            return &proc->file_table[i];
        }
    }
    return NULL;
}

/* 关闭文件描述符 */
int sys_close_fd(int fd)
{
    PCB* proc = pm.running;
    if (proc == NULL) {
        return -1;
    }

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (proc->file_table[i].is_open && proc->file_table[i].fd == fd) {
            proc->file_table[i].is_open = 0;
            self_printf("[Syscall] Process %d closed fd=%d\n", proc->pid, fd);
            return 0;
        }
    }

    self_printf("[Syscall] ERROR: Process %d fd=%d not found\n", proc->pid, fd);
    return -1;
}

/* 获取当前进程已打开的文件数量 */
int sys_get_open_file_count(void)
{
    PCB* proc = pm.running;
    if (proc == NULL) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (proc->file_table[i].is_open) {
            count++;
        }
    }
    return count;
}