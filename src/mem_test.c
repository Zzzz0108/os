#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include "../include/memory.h"
#include "../include/os_sync.h"

/* 全局共享资源 */
os_mutex_t mem_lock;
MemControlBlock* current_mcb;
int simulation_running = 1;

/* 线程1：模拟内存活动的线程 */
DWORD WINAPI MemoryAccessThread(LPVOID lpParam) {
    srand((unsigned)time(NULL));
    
    // 模拟 50 次内存访问操作
    for (int i = 0; i < 50; i++) { 
        // 1. 获取锁，保证与监控线程同步
        os_mutex_lock(mem_lock);

        // 2. 通过随机数产生一个访问序列
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
            printf("[访问线程] 执行写操作 -> 逻辑地址: 0x%08X, 数据: 0x%02X\n", logical_addr, data_to_write);
            write_memory(current_mcb, logical_addr, data_to_write);
        } else {
            printf("[访问线程] 执行读操作 -> 逻辑地址: 0x%08X\n", logical_addr);
            read_memory(current_mcb, logical_addr, &read_data);
        }

        // 3. 释放锁
        os_mutex_unlock(mem_lock);
        
        Sleep(50); // 稍作休眠，模拟进程执行间隔，并让出CPU给监控线程
    }
    
    simulation_running = 0;
    return 0;
}

/* 线程2：跟踪和打印内存信息的线程 */
DWORD WINAPI MonitorThread(LPVOID lpParam) {
    while (simulation_running) {
        os_mutex_lock(mem_lock);
        printf("\n========== [监控线程] 捕获内存快照 ==========\n");
        print_mem_status();
        os_mutex_unlock(mem_lock);
        
        Sleep(300); // 每 300ms 打印一次内存状态
    }
    return 0;
}

int main() {
    printf("=== 操作系统课程设计：段页式内存管理模拟 ===\n");

    // 1. 初始化同步锁
    mem_lock = os_mutex_create();

    // 2. 初始化内存系统 (课设要求容量限制为4页到32页)
    int phys_pages = 8; // 我们选用 8 页物理内存进行严格测试，更容易触发 LRU 置换
    if (init_memory_system(phys_pages) != 0) {
        printf("内存系统初始化失败！请检查物理页参数配置。\n");
        return -1;
    }
    printf("系统物理内存初始化成功，共分配 %d 页。\n\n", phys_pages);

    // 3. 临时构造一个进程的内存控制块 (MCB) 用于测试
    // 在最终系统中，这部分代码应该由负责“进程管理”的同学在创建进程时调用
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

    // 4. 创建双线程进行并发模拟
    HANDLE hAccess = CreateThread(NULL, 0, MemoryAccessThread, NULL, 0, NULL);
    HANDLE hMonitor = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);

    // 5. 等待模拟测试结束
    WaitForSingleObject(hAccess, INFINITE);
    WaitForSingleObject(hMonitor, INFINITE);

    // 6. 清理资源 (良好的C语言习惯)
    CloseHandle(hAccess);
    CloseHandle(hMonitor);
    os_mutex_destroy(mem_lock);
    free(current_mcb->segment_table[0].page_table);
    free(current_mcb->segment_table[1].page_table);
    free(current_mcb->segment_table);
    free(current_mcb);

    printf("\n模拟测试结束。请将上述输出保存，用于课设文档的【实验模拟结果分析】。\n");
    return 0;
}