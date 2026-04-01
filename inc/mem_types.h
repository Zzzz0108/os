#ifndef MEM_TYPES_H
#define MEM_TYPES_H
#include <stdint.h>
/* * 页表项 (Page Table Entry, PTE)
 * 记录虚拟页到物理页（或交换区）的映射关系及状态
 */
typedef struct {
    uint32_t frame_num; // 如果 valid=1，表示物理内存的页框号；如果 valid=0，表示在交换区(Swap)的页号
    uint8_t valid;      // 驻留位：1表示当前页在物理内存中，0表示在磁盘(发生缺页中断)
    uint8_t dirty;      // 修改位：1表示内容被写过，页面置换时必须写回磁盘
    uint8_t access;     // 访问位：用于实现 LRU 或 Clock 页面置换算法
} PTE;
/* * 段表项 (Segment Table Entry, STE)
 * 记录进程逻辑段的属性，实现内存隔离与越界保护
 */
typedef struct {
    uint32_t length;    // 该段的有效长度（所包含的页面总数），用于地址越界检查
    PTE* page_table;    // 指向该段专属的页表数组首地址
} STE;
/* * 内存控制块 (Memory Control Block)
 * 负责进程管理的同学需要在他们的 PCB 结构体中包含这个结构，
 * 以便将进程与具体的内存资源绑定。
 */
typedef struct {
    uint32_t pid;       // 所属进程的ID
    STE* segment_table; // 该进程的段表基址
    uint32_t seg_count; // 该进程当前分配的段数量
} MemControlBlock;
#endif // MEM_TYPES_H
