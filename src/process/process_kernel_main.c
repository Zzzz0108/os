#include "../../inc/process_interface.h"
#include "../../inc/process_process.h"
#include "../../inc/mem.h"
/* ���Խ���1 */
/* 测试进程1 */
void process_A()
{
    while (1)
    {
        // 模拟任务
        for (int i = 0; i < 1000000; i++);
    }
}
/* 测试进程2 */
void process_B()
{
    while (1)
    {
        for (int i = 0; i < 1000000; i++);
    }
}
/* 内核入口 */
int main()
{
    // 【新增】：系统开机时最先初始化 16 页物理内存池
    if (init_memory_system(16) != 0) {
        os_panic("Memory system initialization failed!");
    }
    /* �����������Խ��� */
    os_create_process(process_A);
    os_create_process(process_B);
    /* 内核循环 */
    while (1)
    {
        os_yield();   // 主动让出CPU
    }
    return 0;
}
