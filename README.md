﻿# Mini OS 操作系统模拟器 (Mini OS Simulator)

## 项目简介

本项目是一个基于 C 语言编写的操作系统功能模拟器，能够在 Windows 控制台环境下模拟操作系统的四大核心子系统：**命令解释器 (Shell)**、**文件系统 (File System)**、**内存管理 (Memory Management)** 与 **进程调度 (Process Management)**。
项目展示了系统调用封装层、终端命令解析、交互循环以及底层各个管理模块的完整工作链路。

## 环境与编译说明

- **环境要求**：Windows 操作系统
- **编码要求**：为了防止中文字符乱码与报错，编译时统一需附加 \/utf-8\ 参数。

### 推荐：统一使用 build.ps1 构建

在项目根目录执行：

```powershell
# 构建全部可执行文件
.\build.ps1

# 仅构建单个目标
.\build.ps1 -Target cmd_demo
.\build.ps1 -Target mem_test
.\build.ps1 -Target mem_test_compare
.\build.ps1 -Target file_main
.\build.ps1 -Target create_disk

# 清理构建产物
.\build.ps1 -Target clean
```

### 1. 编译并运行系统终端 (Shell)

该终端整合了全部的文件系统操作、内存状态与进程状态检测接口。
\cmd：

gcc -finput-charset=UTF-8-fexec-charset=UTF-8 -Iinc src/cmd/cmd_demo.c src/cmd/cmd.c src/file/file_myfs.c src/Memory/mem.c src/Memory/mem_sync_win.c src/process/process.c src/process/process_queue.c src/process/process_scheduler.c src/process/process_interface.c -o cmd_demo.exe


cl /utf-8 src/cmd/cmd_demo.c src/cmd/cmd.c src/file/file_myfs.c src/Memory/mem.c src/Memory/mem_sync_win.c src/process/process.c src/process/process_queue.c src/process/process_scheduler.c src/process/process_interface.c /Fe:cmd_demo.exe

.\cmd_demo.exe



### 2. 编译并运行内存管理测试用例

该测试用例单独用于测试段页式内存管理的分配、缺页中断与页面置换率。
cmd：

gcc -finput-charset=UTF-8 -fexec-charset=UTF-8 -Iinc src/Memory/mem_test.c src/Memory/mem.c src/cmd/cmd.c src/file/file_myfs.c src/Memory/mem_sync_win.c src/process/process.c src/process/process_queue.c src/process/process_scheduler.c src/process/process_interface.c -o mem_test.exe


cl /utf-8 src/Memory/mem_test.c src/Memory/mem.c src/cmd/cmd.c src/file/file_myfs.c src/Memory/mem_sync_win.c src/process/process.c src/process/process_queue.c src/process/process_scheduler.c src/process/process_interface.c /Fe:mem_test.exe

.\mem_test.exe

## 核心模块说明

### 1. 终端指令解析 (Shell / CMD 模块)

- **目录**：\src/cmd/\, \inc/cmd.h- **功能描述**：
  提供命令行主交互循环，接收用户的输入、清理换行并分发到各个底层虚拟子系统。
- **支持的系统命令**：
  - **Linux 风格标准指令**：\ls\ (列出目录), \cd\ (切换目录), \mkdir\ (创建目录), mdir\ (删除空目录), \	ouch\ (创建文件), \cat\ (输出文件内容), m\ (删除文件), \pwd\ (打印工作路径), \ps\ (查看所有进程状态), \ree\ (查看内存命中状况及占用)。
  - **其余基础交互**：\help\ (帮助), \clear\ (清屏), \echo\ (回显文字), \sysinfo\ (系统信息), \exit\ (退出模拟器)。

### 2. 文件系统 (File System)

- **目录**：\src/file/\, \inc/file_myfs.h- **核心数据结构**：\MyFS\ 状态机, \DirEntry\ 目录项
- **功能描述**：
  - 基于经典的 **FAT12/FAT16** 结构架构设计的虚拟文件系统。
  - 利用本地磁盘文件 \src/file/mydisk.img\ 当作物理虚拟盘，严格划分引导保留扇区、FAT 表、根目录区和数据区。
  - 支持多级树形目录的跳转和解析（利用 \current_cluster\ 向上或向下检索），实现文件的字节级创建、写入、读取与存储簇的链式分配。

### 3. 内存管理 (Memory Management)

- **目录**：\src/Memory/\, \inc/mem.h\, \inc/mem_sync.h\, \inc/mem_config.h- **功能描述**：
  - 采用分页/段页式内存管理模型，封装了具体进程的内存控制块 \MemControlBlock\。
  - **MMU 虚拟硬件**：通过拦截性的 ead_memory()\ 和 \write_memory()\ 函数模拟逻辑虚拟地址向真实物理页面地址转换的硬件运作。
  - **缺页机制 (Page Fault)**：针对访问内存界限内的合法但未装载的虚拟页抛出软缺页中断，通过特定的页面置换算法装载新页、清除无用页，同时统计缺页失败率用于性能评估。

### 4. 进程管理 (Process Scheduler)

- **目录**：\src/process/\, \inc/process_interface.h\, \inc/process_process.h- **功能描述**：
  - 定义了系统的进程控制块 \PCB\ 以及标准的生命周期五状态模型（新建、就绪、运行、阻塞、终止）。
  - **队列与调度**：底层利用 \pm.ready\、\pm.blocked\ 队列阵列维护结构流转，包含对不同优先级多级反馈队列（MLFQ）或轮转（RR）策略的时钟驱动支持。
  - 供测试与观察使用：提供了类似于 \print_system_state\ 等调用便于外部接口（如 \ps\ 命令）直观监控所有任务插槽的执行时间片及队列降级状态。

---

## 最佳实践与注意事项

1. **磁盘文件挂载**：在启动 \cmd_demo.exe\ 后，引擎会自动利用 \os_terminal_init()\ 尝试挂载位于 \src/file/mydisk.img\ 的镜像。如果该映像损坏或不具备有效的 FAT 树结构格式，文件读写等指令将报告 Failed 并返回保护状态。
2. **C 库调用隔离**：各个核心模块内的交互和输出已尽可能采用了含有前缀 \os_XXX\ 或是 \self_XXX\ (如 \self_printf\, \self_fgets\) 的转接宏系统封装。这种系统调用的隔离允许未来的系统非常平滑地移植到纯裸机内核环境，只需要替换一层简单的驱动宏。
