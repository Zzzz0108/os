﻿#ifndef MEMORY_H
#define MEMORY_H
#include "mem_config.h"
#include "mem_types.h"
#include <stddef.h>
/*
 * 初始化内存管理系统。
 * 根据课设要求，用户内存容量需要在 4 页到 32 页之间。
 * num_phys_pages: 指定物理内存的页数。
 * 返回值: 0 表示成功，-1 表示失败（如参数不合法）。
 */
int init_memory_system(int num_phys_pages);
/*
 * 为新进程创建内存控制块（分配段表等核心结构）。
 * pid: 进程ID。
 * 返回值: 指向该进程内存控制块的指针，失败返回 NULL。
 */
MemControlBlock* create_process_memory(uint32_t pid);
/*
 * 销毁进程的内存结构，回收该进程占用的所有物理页和交换区空间。
 */
void destroy_process_memory(MemControlBlock* mcb);
/*
 * 模拟 MMU 读操作（包含逻辑地址到物理地址的转换、越界检查和缺页中断处理）。
 * mcb: 当前操作进程的内存控制块。
 * logical_addr: 需要读取的逻辑地址。
 * out_data: 读取到的数据存放地址。
 * 返回值: 0 表示成功，-1 表示发生越界等致命错误。
 */
int read_memory(MemControlBlock* mcb, uint32_t logical_addr, uint8_t* out_data);
/*
 * 模拟 MMU 写操作（包含逻辑地址转换、缺页处理以及更新 dirty 位）。
 * mcb: 当前操作进程的内存控制块。
 * logical_addr: 需要写入的逻辑地址。
 * data: 要写入的数据（单字节示例）。
 * 返回值: 0 表示成功，-1 表示发生越界等致命错误。
 */
int write_memory(MemControlBlock* mcb, uint32_t logical_addr, uint8_t data);
/*
 * 打印当前内存状态（供专门跟踪内存分配情况的线程使用）。
 * 打印内容包括物理内存占用率、缺页次数、置换次数等。
 */
void print_mem_status(void);
#endif // MEMORY_H
