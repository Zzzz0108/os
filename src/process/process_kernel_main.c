#include "../../inc/process_interface.h"
#include "../../inc/process_process.h"
#include "../../inc/process_syscall.h"
#include "../../inc/mem.h"
#include "../../inc/cmd.h"

/* 内核主函数 - 操作系统入口 */
int main()
{
    /* ========= 第一步：初始化所有内核模块 ========= */

    // 1. 初始化内存系统（16页物理内存）
    if (init_memory_system(16) != 0) {
        os_panic("Memory system initialization failed!");
    }

    // 2. 初始化进程管理系统
    process_manager_init();

    /* ========= 第二步：创建系统初始进程 ========= */

    // 只创建 CMD 终端进程作为用户交互入口
    // 其他用户进程都通过 CMD 中的命令动态创建

    // 创建 CMD 终端进程（系统唯一的初始用户进程）
    int cmd_pid = sys_create_process_with_entry(cmd_main, "terminal", 9999);
    if (cmd_pid < 0) {
        os_panic("Failed to create terminal process!");
    }

    self_printf("========================================\n");
    self_printf("   MiniOS 操作系统已启动\n");
    self_printf("   Terminal PID: %d\n", cmd_pid);
    self_printf("========================================\n");
    self_printf("输入 'help' 查看可用命令\n\n");

    /* ========= 第三步：内核调度循环 ========= */

    // 调度器会轮流运行所有就绪进程
    // 初始只有 terminal 进程，后续通过 run 命令创建更多进程
    while (1) {
        run_process();   // 运行当前选中的进程
        // run_process() 内部会自动调度下一个进程
        // 不需要额外调用 scheduler()
    }

    return 0;
}