#ifndef PROCESS_SYSCALL_H
#define PROCESS_SYSCALL_H

#include "process_process.h"
#include "process_queue.h"

/* ============================================
   进程管理模块 - 系统调用接口
   供其他模块（文件系统、设备驱动、CMD）调用
   ============================================ */

   /* ---------- 进程查询接口 ---------- */

   /* 获取当前运行的进程ID
	* 返回值: 当前进程的PID，如果没有运行进程返回-1
	*/
int sys_get_pid(void);

/* 获取当前运行的进程PCB指针
 * 返回值: 当前进程的PCB指针，如果没有运行进程返回NULL
 */
PCB* sys_get_current_process(void);

/* 根据PID获取PCB指针
 * pid: 要查找的进程ID
 * 返回值: 对应进程的PCB指针，如果不存在返回NULL
 */
PCB* sys_get_process_by_pid(int pid);

/* 获取当前进程的MCB（供内存模块使用）
 * 返回值: 当前进程的内存控制块指针
 */
struct MCB* sys_get_current_mcb(void);

/* 获取指定进程的MCB
 * pid: 要查找的进程ID
 * 返回值: 对应进程的内存控制块指针
 */
struct MCB* sys_get_process_mcb(int pid);

/* ---------- 进程状态控制接口 ---------- */

/* 阻塞当前进程（等待事件）
 * 将当前进程移入阻塞队列，并调度下一个进程运行
 */
void sys_block_current(void);

/* 唤醒指定PID的进程
 * pid: 要唤醒的进程ID
 * 返回值: 0表示成功，-1表示失败（进程不存在或未阻塞）
 */
int sys_wakeup_pid(int pid);

/* 让出CPU（用户进程主动放弃）
 * 将当前进程放回就绪队列，并调度下一个进程
 */
void sys_yield(void);

/* 终止当前进程
 * 销毁当前进程，并调度下一个进程运行
 */
void sys_exit(void);

/* ---------- 等待队列同步接口 ---------- */

/* 将当前进程加入等待队列并阻塞
 * wait_queue: 等待队列指针
 * 用于实现信号量、文件锁等同步原语
 */
void sys_sleep_on(Queue* wait_queue);

/* 唤醒等待队列上的一个进程
 * wait_queue: 等待队列指针
 * 从队列头部取出一个进程唤醒
 */
void sys_wakeup_one(Queue* wait_queue);

/* 唤醒等待队列上的所有进程
 * wait_queue: 等待队列指针
 * 将队列中所有进程唤醒并放入就绪队列
 */
void sys_wakeup_all(Queue* wait_queue);

/* ---------- 【新增】文件相关系统调用（供文件系统模块调用）---------- */

/* 为当前进程分配一个文件描述符
 * file_cluster: 文件的起始簇号（磁盘文件专用）
 * 返回值: 分配的文件描述符（fd），失败返回-1
 *
 * 使用场景：
 *   - 文件系统打开文件成功后调用此函数
 *   - 将文件信息（簇号等）与进程绑定
 *   - 返回的fd供用户程序使用
 */
int sys_alloc_fd(int file_cluster);

/* 根据文件描述符获取文件信息
 * fd: 文件描述符
 * 返回值: 指向文件表项的指针，失败返回NULL
 *
 * 使用场景：
 *   - 文件系统读写文件时，通过fd获取文件信息
 *   - 获取文件的起始簇号、当前读写位置等
 */
FileTableEntry* sys_get_file_entry(int fd);

/* 关闭文件描述符
 * fd: 要关闭的文件描述符
 * 返回值: 0表示成功，-1表示失败（fd无效或未打开）
 *
 * 使用场景：
 *   - 文件系统关闭文件时调用此函数
 *   - 释放进程占用的文件描述符
 */
int sys_close_fd(int fd);

/* 获取当前进程已打开的文件数量
 * 返回值: 当前进程打开的文件数
 *
 * 使用场景：
 *   - 调试、监控进程资源使用情况
 */
int sys_get_open_file_count(void);

/* ---------- 调试/信息接口 ---------- */

/* 打印所有进程状态（供CMD的ps命令使用） */
void sys_print_processes(void);

/* 打印单个进程信息
 * proc: 进程PCB指针
 */
void sys_print_process(PCB* proc);

/* ---------- 进程管理接口（供内核使用）---------- */

/* 创建新进程（供内核初始化时使用）
 * name: 进程名称
 * runtime: 总运行时间
 * 返回值: 新创建的PCB指针
 */
PCB* sys_create_process(const char* name, int runtime);

/* 销毁进程（供内核使用）
 * proc: 要销毁的进程PCB指针
 */
void sys_destroy_process(PCB* proc);

/* 获取进程总数（包括运行、就绪、阻塞的所有进程）
 * 返回值: 系统中进程的总数
 */
int sys_get_process_count(void);

/* 获取就绪队列进程数
 * 返回值: 所有就绪队列中的进程总数
 */
int sys_get_ready_count(void);

/* 获取阻塞队列进程数
 * 返回值: 阻塞队列中的进程总数
 */
int sys_get_blocked_count(void);

#endif /* PROCESS_SYSCALL_H */