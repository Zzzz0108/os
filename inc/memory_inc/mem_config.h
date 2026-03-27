#ifndef MEM_CONFIG_H
#define MEM_CONFIG_H

/* 系统级内存参数配置 */

/* 课设要求：页面大小为1K [cite: 73] */
#define PAGE_SIZE 1024

/* 课设要求：用户内存容量为4页到32页 [cite: 74] */
#define MIN_PHYS_PAGES 4
#define MAX_PHYS_PAGES 32

/* 模拟系统设置的默认物理内存页数（可以在初始化时由使用者设置） */
#define DEFAULT_PHYS_PAGES 16

/* 模拟的磁盘交换区大小（页数），用于支持虚拟内存和页面置换 */
#define SWAP_PAGES_MAX 1024

/* 虚拟地址空间限制 */
#define MAX_SEGMENTS_PER_PROCESS 16  // 单个进程支持的最大段数
#define MAX_PAGES_PER_SEGMENT 256    // 每个段支持的最大页面数

#endif // MEM_CONFIG_H