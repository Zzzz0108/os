#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../inc/mem.h"
#include "../../inc/cmd.h"
#include "../../inc/platform_thread.h"
#include "../../inc/platform_time.h"

/* 置换算法枚举 */
#define ALGORITHM_LRU 0
#define ALGORITHM_FIFO 1
#define ALGORITHM_CLOCK 2

/* 全局共享资源 */
MemControlBlock* current_mcb;
int simulation_running = 1;
/* 线程1：模拟内存活动的线程 */
unsigned int MemoryAccessThread(void* lpParam) {
    (void)lpParam;
    srand((unsigned)time(NULL));
    // 模拟 50 次内存访问操作
    for (int i = 0; i < 50; i++) { 
        // 通过随机数产生一个访问序列
        uint32_t seg = rand() % current_mcb->seg_count; 
        uint32_t page = rand() % 12; // 故意随机到较大的页号，以触发缺页和LRU置换
        uint32_t offset = rand() % PAGE_SIZE;
        // 拼装 32 位逻辑地址: 段号(8位) | 页号(14位) | 偏移(10位)
        uint32_t logical_addr = (seg << 24) | (page << 10) | offset;
        uint8_t data_to_write = (uint8_t)(rand() % 256);
        uint8_t read_data = 0;
        // 随机决定是读操作还是写操作
        int is_write = rand() % 2;
        if (is_write) {
            self_printf("[访问线程] 执行写操作 -> 逻辑地址: 0x%08X, 数据: 0x%02X\n", logical_addr, data_to_write);
            write_memory(current_mcb, logical_addr, data_to_write);
        } else {
            self_printf("[访问线程] 执行读操作 -> 逻辑地址: 0x%08X\n", logical_addr);
            read_memory(current_mcb, logical_addr, &read_data);
        }
        os_sleep_ms(50); // 稍作休眠，模拟进程执行间隔，并让出CPU给监控线程
    }
    simulation_running = 0;
    return 0;
}
/* 线程2：跟踪和打印内存信息的线程 */
unsigned int MonitorThread(void* lpParam) {
    (void)lpParam;
    while (simulation_running) {
        self_printf("\n========== [监控线程] 捕获内存快照 ==========\n");
        print_mem_status();
        os_sleep_ms(300); // 每 300ms 打印一次内存状态
    }
    return 0;
}
int main() {
    self_printf("=== 操作系统课程设计：段页式内存管理模拟 ===\n");
    self_printf("支持多种置换算法：LRU / FIFO / CLOCK\n\n");
    // 1. 初始化内存系统 (课设要求容量限制为4页到32页)
    int phys_pages = 8; // 8 页物理内存，更容易触发置换
    // 算法配置：可以修改这个值来测试不同的置换算法
    int selected_algorithm = ALGORITHM_LRU;
    
    const char* algorithm_names[] = {"LRU", "FIFO", "CLOCK"};
    self_printf("选中的置换算法: %s\n\n", algorithm_names[selected_algorithm]);
    
    if (init_memory_system_with_algorithm(phys_pages, selected_algorithm) != 0) {
        self_printf("内存系统初始化失败！请检查物理页参数配置。\n");
        return -1;
    }
    self_printf("系统物理内存初始化成功，共分配 %d 页。\n\n", phys_pages);
    // 2. 临时构造一个进程的内存控制块 (MCB) 用于测试

    current_mcb = (MemControlBlock*)malloc(sizeof(MemControlBlock));
    current_mcb->pid = 1001;
    current_mcb->seg_count = 2; // 给该测试进程分配 2 个段
    current_mcb->segment_table = (STE*)malloc(sizeof(STE) * 2);
    // 段 0：代码段，分配 8 个逻辑页
    current_mcb->segment_table[0].length = 8;
    current_mcb->segment_table[0].page_table = (PTE*)calloc(8, sizeof(PTE));
    // 段 1：数据段，分配 16 个逻辑页 (逻辑页总数 24 > 物理页 8，必然触发置换)
    current_mcb->segment_table[1].length = 16;
    current_mcb->segment_table[1].page_table = (PTE*)calloc(16, sizeof(PTE));
    // 3. 创建双线程进行并发模拟
    os_thread_t hAccess = os_thread_create(MemoryAccessThread, NULL);
    os_thread_t hMonitor = os_thread_create(MonitorThread, NULL);
    // 4. 等待模拟测试结束
    os_thread_join(hAccess);
    os_thread_join(hMonitor);
    // 5. 清理资源 (良好的C语言习惯)
    os_thread_close(hAccess);
    os_thread_close(hMonitor);
    free(current_mcb->segment_table[0].page_table);
    free(current_mcb->segment_table[1].page_table);
    free(current_mcb->segment_table);
    free(current_mcb);
    self_printf("\n模拟测试结束。请将上述输出保存，用于课设文档的【实验模拟结果分析】。\n");
    self_printf("\n提示：修改 main 函数中的 selected_algorithm 变量来测试不同的置换算法：\n");
    self_printf("  0 = LRU (Least Recently Used)\n");
    self_printf("  1 = FIFO (First In First Out)\n");
    self_printf("  2 = CLOCK (Clock Algorithm)\n");
    
    return 0;
}
