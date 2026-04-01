#include "../../inc/mem.h"
#include "../../inc/mem_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../inc/cmd.h"
/* 全局变量定义 */
static uint8_t* physical_memory = NULL;
static uint8_t* swap_space = NULL;
static uint32_t free_frame_bitmap = 0; // 0表示空闲，1表示占用 (最多支持32页) [cite: 74]
static int total_phys_pages = 0;
/* 逆向映射表：用于页面置换时，通过物理页框号反查对应的 PTE */
static PTE* frame_reverse_map[MAX_PHYS_PAGES];
/* 全局时钟：用于 LRU 算法记录最后访问时间 */
static uint32_t global_time = 0;
/* 同步锁：满足课设多线程同步要求 [cite: 68] */
static os_mutex_t mem_lock = NULL;
/* 统计信息 */
static int page_faults = 0;
static int memory_accesses = 0;
/* 内部辅助函数：分配一个空闲物理页框 */
static int allocate_free_frame() {
    for (int i = 0; i < total_phys_pages; i++) {
        if ((free_frame_bitmap & (1 << i)) == 0) {
            free_frame_bitmap |= (1 << i); // 标记为占用
            return i;
        }
    }
    return -1; // 内存已满
}
/* 内部辅助函数：LRU 页面置换算法 */
static int execute_lru_replacement() {
    int victim_frame = -1;
    uint32_t oldest_time = 0xFFFFFFFF;
    // 扫描所有物理页框，找到 access 时间最早的页
    for (int i = 0; i < total_phys_pages; i++) {
        PTE* pte = frame_reverse_map[i];
        if (pte != NULL && pte->access < oldest_time) {
            oldest_time = pte->access;
            victim_frame = i;
        }
    }
    if (victim_frame != -1) {
        PTE* victim_pte = frame_reverse_map[victim_frame];
        // 如果被修改过 (dirty == 1)，需要写回 Swap 区 (这里简化为只修改标记)
        if (victim_pte->dirty) {
            // TODO: 实际应用中需要 memcpy 物理内存数据到 swap_space
            victim_pte->dirty = 0; 
        }
        // 更新被换出页的 PTE 状态
        victim_pte->valid = 0;
        // victim_pte->frame_num = 分配给它的 Swap 扇区号 (此处省略具体 Swap 管理)
        frame_reverse_map[victim_frame] = NULL;
    }
    return victim_frame;
}
/* 内部辅助函数：缺页中断处理 */
static int handle_page_fault(PTE* pte) {
    page_faults++;
    int frame = allocate_free_frame();
    if (frame == -1) {
        // 物理内存已满，触发页面置换
        frame = execute_lru_replacement();
    }
    // TODO: 从 Swap 区将数据读入 physical_memory + (frame * PAGE_SIZE)
    // 更新页表项
    pte->frame_num = frame;
    pte->valid = 1;
    pte->dirty = 0;
    // 建立逆向映射
    frame_reverse_map[frame] = pte;
    return frame;
}
/* 核心初始化接口 */
int init_memory_system(int num_phys_pages) {
    if (num_phys_pages < MIN_PHYS_PAGES || num_phys_pages > MAX_PHYS_PAGES) {
        return -1; // 不符合课设要求 [cite: 74]
    }
    total_phys_pages = num_phys_pages;
    // 使用原生的 malloc 申请一大块内存模拟物理内存和交换区
    // 注意：如果是裸机系统，这里不需要 malloc，而是直接映射到某个物理地址
    physical_memory = (uint8_t*)malloc(total_phys_pages * PAGE_SIZE);
    swap_space = (uint8_t*)malloc(SWAP_PAGES_MAX * PAGE_SIZE);
    if (!physical_memory || !swap_space) return -1;
    memset(physical_memory, 0, total_phys_pages * PAGE_SIZE);
    memset(frame_reverse_map, 0, sizeof(frame_reverse_map));
    free_frame_bitmap = 0;
    page_faults = 0;
    memory_accesses = 0;
    return 0; // 初始化成功
}
/* 核心：地址转换与访问模拟 (MMU 模拟) */
static int translate_and_access(MemControlBlock* mcb, uint32_t logical_addr, uint8_t* data, int is_write) {
    // 1. 地址解析 (10位偏移，14位页号，8位段号)
    uint32_t offset = logical_addr & 0x3FF;             // 最低 10 位
    uint32_t page_num = (logical_addr >> 10) & 0x3FFF;  // 中间 14 位
    uint32_t seg_num = (logical_addr >> 24) & 0xFF;     // 最高 8 位
    // 2. 越界检查
    if (seg_num >= mcb->seg_count) return -1; // 段越界
    STE* ste = &(mcb->segment_table[seg_num]);
    if (page_num >= ste->length) return -1;   // 页越界
    // 3. 获取页表项
    PTE* pte = &(ste->page_table[page_num]);
    // 4. 缺页检查
    if (pte->valid == 0) {
        handle_page_fault(pte); // 触发缺页中断
    }
    // 5. 更新状态与执行读写
    pte->access = ++global_time; // 更新 LRU 访问时间
    if (is_write) {
        pte->dirty = 1;
        physical_memory[(pte->frame_num * PAGE_SIZE) + offset] = *data;
    } else {
        *data = physical_memory[(pte->frame_num * PAGE_SIZE) + offset];
    }
    memory_accesses++;
    return 0; // 成功
}
int read_memory(MemControlBlock* mcb, uint32_t logical_addr, uint8_t* out_data) {
    // 课设要求线程间必须同步 [cite: 68]
    // os_mutex_lock(mem_lock); // 暂定，需在外部或此处加锁
    int res = translate_and_access(mcb, logical_addr, out_data, 0);
    // os_mutex_unlock(mem_lock);
    return res;
}
int write_memory(MemControlBlock* mcb, uint32_t logical_addr, uint8_t data) {
    // os_mutex_lock(mem_lock);
    int res = translate_and_access(mcb, logical_addr, &data, 1);
    // os_mutex_unlock(mem_lock);
    return res;
}
void print_mem_status(void) {
    self_printf("--- 内存系统状态 ---\n");
    self_printf("总物理页: %d, 页面大小: 1KB\n", total_phys_pages);
    self_printf("总访问次数: %d, 缺页次数: %d\n", memory_accesses, page_faults);
    if (memory_accesses > 0) {
        self_printf("缺页率: %.2f%%\n", (float)page_faults / memory_accesses * 100);
    }
    self_printf("空闲页框位图: 0x%08X\n", free_frame_bitmap);
    self_printf("--------------------\n");
}
/* 为新进程分配内存控制块及初始页表 */
MemControlBlock* create_process_memory(uint32_t pid) {
    MemControlBlock* mcb = (MemControlBlock*)malloc(sizeof(MemControlBlock));
    if (!mcb) return NULL;
    mcb->pid = pid;
    mcb->seg_count = 2; // 默认分配两个段：段0(代码段)，段1(数据段)
    mcb->segment_table = (STE*)malloc(sizeof(STE) * mcb->seg_count);
    // 段0：模拟代码段，分配 4 个逻辑页
    mcb->segment_table[0].length = 4;
    mcb->segment_table[0].page_table = (PTE*)calloc(4, sizeof(PTE));
    // 段1：模拟数据段，分配 8 个逻辑页
    mcb->segment_table[1].length = 8;
    mcb->segment_table[1].page_table = (PTE*)calloc(8, sizeof(PTE));
    return mcb;
}
/* 进程销毁时，回收物理内存和核心数据结构 */
void destroy_process_memory(MemControlBlock* mcb) {
    if (!mcb) return;
    // 遍历所有段和页，释放占用的物理页框
    for (uint32_t i = 0; i < mcb->seg_count; i++) {
        for (uint32_t j = 0; j < mcb->segment_table[i].length; j++) {
            PTE* pte = &mcb->segment_table[i].page_table[j];
            if (pte->valid == 1) {
                // 清除物理页框的占用位图
                free_frame_bitmap &= ~(1 << pte->frame_num);
                frame_reverse_map[pte->frame_num] = NULL;
            }
        }
        free(mcb->segment_table[i].page_table);
    }
    free(mcb->segment_table);
    free(mcb);
}
