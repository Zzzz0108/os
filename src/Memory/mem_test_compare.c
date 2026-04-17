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
        uint32_t page = rand() % 12; // 故意随机到较大的页号，以触发缺页和置换
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

void run_single_test(int algorithm) {
    const char* algorithm_names[] = {"LRU", "FIFO", "CLOCK"};
    
    self_printf("\n");
    self_printf("╔═══════════════════════════════════════════════════════════╗\n");
    self_printf("║ 测试算法 #%d: %s\n", algorithm, algorithm_names[algorithm]);
    self_printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    // 1. 初始化内存系统
    int phys_pages = 8;
    if (init_memory_system_with_algorithm(phys_pages, algorithm) != 0) {
        self_printf("内存系统初始化失败！\n");
        return;
    }
    self_printf("系统物理内存初始化成功，共分配 %d 页。\n\n", phys_pages);
    
    // 2. 临时构造一个进程的内存控制块 (MCB) 用于测试
    current_mcb = (MemControlBlock*)malloc(sizeof(MemControlBlock));
    current_mcb->pid = 1000 + algorithm;
    current_mcb->seg_count = 2;
    current_mcb->segment_table = (STE*)malloc(sizeof(STE) * 2);
    
    // 段 0：代码段，分配 8 个逻辑页
    current_mcb->segment_table[0].length = 8;
    current_mcb->segment_table[0].page_table = (PTE*)calloc(8, sizeof(PTE));
    
    // 段 1：数据段，分配 16 个逻辑页 (总共 24 > 物理页 8，必然触发置换)
    current_mcb->segment_table[1].length = 16;
    current_mcb->segment_table[1].page_table = (PTE*)calloc(16, sizeof(PTE));
    
    // 3. 创建双线程进行并发模拟
    simulation_running = 1;
    os_thread_t hAccess = os_thread_create(MemoryAccessThread, NULL);
    os_thread_t hMonitor = os_thread_create(MonitorThread, NULL);
    
    // 4. 等待模拟测试结束
    os_thread_join(hAccess);
    os_thread_join(hMonitor);
    
    // 5. 清理资源
    os_thread_close(hAccess);
    os_thread_close(hMonitor);
    free(current_mcb->segment_table[0].page_table);
    free(current_mcb->segment_table[1].page_table);
    free(current_mcb->segment_table);
    free(current_mcb);
    
    self_printf("\n[%s] 测试完成\n", algorithm_names[algorithm]);
}

int main() {
    self_printf("╔═════════════════════════════════════════════════════════════════════╗\n");
    self_printf("║  操作系统课程设计：页面置换算法性能对比测试                          ║\n");
    self_printf("║  将依次测试 LRU / FIFO / CLOCK 三种算法                              ║\n");
    self_printf("╚═════════════════════════════════════════════════════════════════════╝\n");
    
    // 测试三种算法
    run_single_test(ALGORITHM_LRU);
    run_single_test(ALGORITHM_FIFO);
    run_single_test(ALGORITHM_CLOCK);
    
    self_printf("\n");
    self_printf("╔═════════════════════════════════════════════════════════════════════╗\n");
    self_printf("║  所有测试完成！请对比上述三种算法的缺页率结果                        ║\n");
    self_printf("║  缺页率越低，算法性能越好                                            ║\n");
    self_printf("╚═════════════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}
