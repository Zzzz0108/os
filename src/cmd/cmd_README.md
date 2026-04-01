# Mini OS 项目说明

## 终端

demo:
cl /I include src\OS_cmd_demo.c src\OS_cmd.c
run:
./OS_cmd_demo.exe


本模块在 Windows 控制台上模拟一个简化的「操作系统终端」，支持 help、clear、echo、dir、sysinfo、exit 等基础命令，用于演示从「系统调用封装层」到「终端命令解析」再到「交互循环」的完整链路。

### 文件结构概览

- [CMD/OS_cmd.h](CMD/OS_cmd.h)
- [CMD/OS_cmd.c](CMD/OS_cmd.c)
- [CMD/OS_cmd_demo.c](CMD/OS_cmd_demo.c)

下面分别介绍三个文件的功能、设计理念以及使用方法。

---

### CMD/OS_cmd.h

#### 功能

- 声明终端所需的所有对外接口与常量：
	- self_printf、self_scanf、self_fgets、self_system 等 "self_*" 封装接口。
	- remove_newline、get_first_word、strings_equal、get_echo_message 等字符串/命令解析辅助函数。
	- help、clear、echo、dir、sysinfo、welcome 等终端命令相关的函数。
- 通过 MAX_CMD 常量统一约束命令行缓冲区大小。

#### 设计理念

- 作为「终端子系统」的公共头文件，将接口与实现解耦：
	- 上层只需要包含 OS_cmd.h，即可使用终端相关功能，而无需关心内部如何调用 C 标准库或系统命令。
	- 为 future work 预留空间：如果将来不再使用标准库函数，而是使用真正的 OS 系统调用，只需在实现层修改，接口保持不变。

#### 使用方法

- 在需要使用终端功能的 C 源文件中包含头文件：
	- #include "OS_cmd.h"
- 使用头文件中声明的接口，例如：
	- 调用 welcome() 显示欢迎界面。
	- 调用 help() 打印命令帮助。
	- 调用 echo("hello") 打印一行文本。

---

### CMD/OS_cmd.c

#### 功能

- 实现 OS_cmd.h 中声明的大部分功能，主要包括两层：
	1. self_* 封装层
		 - self_printf/self_scanf/self_fgets/self_system 等函数在当前版本直接调用 C 标准库（vprintf、vscanf、fgets、system）。
		 - 这一层可以被视为「模拟的系统调用接口」，便于以后迁移到真正的 OS 环境。
	2. 终端内核/命令实现
		 - help: 输出可用命令列表及简单说明。
		 - clear: 调用 self_system("cls") 清屏（针对 Windows 控制台）。
		 - echo: 打印一行用户输入的文本。
		 - dir: 输出一个模拟目录列表（演示命令输出结构）。
		 - sysinfo: 打印简化的系统信息（OS 名称、版本、Shell 名称等）。
		 - welcome: 清屏并打印欢迎 Banner。
- 同时包含一组仅在本文件内部使用的字符串工具函数，用于处理用户输入：
	- remove_newline: 去掉 fgets 读入行末尾的 CR/LF。
	- get_first_word: 提取命令行中的第一个单词作为操作符（如 "help"、"echo"）。
	- strings_equal: 比较两个以 '\0' 结尾的字符串是否相等。
	- get_echo_message: 从整行命令中跳过 "echo" 和空白，得到要输出的消息部分。

#### 设计理念

- 分层设计：
	- 最底层：self_* 封装统一对外 I/O、系统调用接口，方便替换实现。
	- 中间层：字符串工具函数负责命令行的最小化解析逻辑。
	- 上层：具体命令（help/dir/sysinfo 等）只关心逻辑与输出格式，不直接依赖标准库细节。
- 强调「可移植性」与「可替换性」：
	- 如果未来在自制 OS 或裸机环境下运行，只需替换 self_* 的内部实现，就可以复用整套终端命令逻辑。

#### 使用方法

- 一般不直接单独编译和运行 OS_cmd.c，而是与 OS_cmd_demo.c 一起编译：
	- 终端主循环在 OS_cmd_demo.c 中，这里只提供具体命令实现和工具函数。
	- 在其他工程中，也可以复用本文件作为「终端内核」的实现模块。

---

### CMD/OS_cmd_demo.c

#### 功能

- 提供一个完整的终端 Demo：
	- main 函数中调用 welcome() 显示欢迎界面。
	- 使用一个 while(1) 循环反复：
		- 输出提示符 [MiniOS] >。
		- 使用 self_fgets 读取一行命令到缓冲区 cmd。
		- 调用 remove_newline 去掉行末换行。
		- 使用 get_first_word 提取命令关键字 op。
		- 根据 op 分发到不同命令：help / clear / echo / dir / sysinfo / exit。
		- 当用户输入 exit 时打印提示并跳出循环，程序结束。
	- 如果输入了不为空但不支持的命令，则输出 "Unsupported command" 提示。

#### 设计理念

- 将「命令解析 + 主循环」与「命令实现」分离：
	- 命令实现集中在 OS_cmd.c 中。
	- OS_cmd_demo.c 只负责读取输入、解析第一个单词并调用相应接口。
- main 的结构清晰地展示出一个最小化 shell/终端的工作流程：
	- 初始化/欢迎界面 → 循环读取命令 → 解析 → 分派执行 → 根据 exit 决定退出。
- 通过 self_* 接口，而不是直接 printf/fgets，强化了「上层只依赖系统调用封装，不依赖具体实现」的思想。

#### 使用方法

1. 编译
	 - 在 Windows + MSVC 环境下，可在 CMD 目录中编译：
		 - cl /EHsc OS_cmd_demo.c OS_cmd.c
	 - 或在 VS Code 中将活动文件设为 OS_cmd_demo.c，使用已有的 C/C++ 构建任务进行编译。

2. 运行
	 - 运行生成的可执行文件（例如 OS_cmd_demo.exe）。
	 - 将看到欢迎界面和提示符 [MiniOS] >。

3. 示例交互
	 - 输入 help 并回车：查看支持的命令列表。
	 - 输入 clear 并回车：清屏。
	 - 输入 echo hello MiniOS 并回车：终端会回显 "hello MiniOS"。
	 - 输入 dir 并回车：显示模拟的目录内容。
	 - 输入 sysinfo 并回车：查看简化的系统信息。
	 - 输入 exit 并回车：退出该终端 Demo。

