#include "os_interface.h"
#include "process.h"

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
    /* 创建两个测试进程 */
    os_create_process(process_A);
    os_create_process(process_B);

    /* 内核循环 */
    while (1)
    {
        os_yield();   // 主动让出CPU
    }

    return 0;
}