# 页面置换算法使用指南

## 支持的置换算法

你的内存管理系统现已支持三种经典的页面置换算法：

### 1. **LRU (Least Recently Used)** - 最近最少使用
- **算法号**：0
- **实现原理**：
  - 维护每个页面的访问时间戳 (`PTE->access`)
  - 全局时钟递增 (`++global_time`)，每次访问时更新页面的时间戳
  - 置换时扫描所有物理页框，选择时间戳最小（最久未使用）的页面
- **时间复杂度**：O(n)，n为物理页数
- **特点**：最优性好，但扫描开销大
- **适用场景**：工作集不变、局部性良好的应用

### 2. **FIFO (First In First Out)** - 先进先出
- **算法号**：1
- **实现原理**：
  - 为每个物理页框记录装入时间戳 (`fifo_install_time[frame]`)
  - 缺页时，分配页框并记录装入时间
  - 置换时扫描所有物理页框，选择装入时间最早的页面
- **时间复杂度**：O(n)
- **特点**：实现简单，但无法考虑页面的使用频率，可能出现 Belady 异常
- **适用场景**：实时系统、简单嵌入式系统

### 3. **CLOCK (时钟)算法** - 时钟/秒次机会
- **算法号**：2
- **实现原理**：
  - 维护一个指针 (`clock_ptr`) 在物理页框间循环扫描
  - 每个页面有访问位 (`PTE->access`)，访问时设为 1
  - **第一轮扫描**：若 `access=0` 则置换；若 `access=1` 则清为 0 并继续
  - **第二轮扫描**（如果第一轮未找到）：所有页面 `access` 都被清为 0，直接置换当前指针位置
  - 指针在置换后移动到下一个位置
- **时间复杂度**：O(n)，但通常迭代次数少
- **特点**：性能介于 FIFO 和 LRU 之间，扫描效率更高
- **适用场景**：通用操作系统、大多数实时应用

---

## 使用方法

### 方法 1：修改测试程序

在 `src/Memory/mem_test.c` 的 `main` 函数中，修改 `selected_algorithm` 变量：

```c
int main() {
    // ...
    
    // 算法配置：可以修改这个值来测试不同的置换算法
    int selected_algorithm = ALGORITHM_LRU;  // <-- 改这里
    
    // ...
}
```

**可选值：**
- `ALGORITHM_LRU` 或 `0` - 使用 LRU
- `ALGORITHM_FIFO` 或 `1` - 使用 FIFO
- `ALGORITHM_CLOCK` 或 `2` - 使用 CLOCK

### 方法 2：编译并运行

```bash
# 编译
gcc -finput-charset=UTF-8 -fexec-charset=UTF-8 -Iinc \
    src/Memory/mem_test.c src/Memory/mem.c src/Memory/mem_sync_win.c \
    src/cmd/cmd.c src/file/file_myfs.c \
    src/process/process.c src/process/process_queue.c \
    src/process/process_scheduler.c src/process/process_interface.c \
    -o mem_test_algo.exe

# 运行
./mem_test_algo.exe
```

### 方法 3：在进程管理中使用

如果需要为不同任务使用不同的置换算法，可以：

```c
#include "mem.h"

// 初始化内存系统并指定算法
int init_memory_system_with_algorithm(int num_phys_pages, int algorithm);

// 运行时切换算法
int set_replacement_algorithm(int algorithm);

// 例如：
int phys_pages = 16;
int algo = ALGORITHM_CLOCK;  // 使用 CLOCK 算法
init_memory_system_with_algorithm(phys_pages, algo);

// 运行时切换
set_replacement_algorithm(ALGORITHM_LRU);  // 切换到 LRU
```

### 方法 4：通过终端命令动态切换

在运行的操作系统终端中，可以使用 `memalgo` 命令来动态切换页面置换算法：

```bash
# 查看帮助
help

# 设置为 LRU 算法
memalgo lru
# 或
memalgo 0

# 设置为 FIFO 算法
memalgo fifo
# 或
memalgo 1

# 设置为 CLOCK 算法
memalgo clock
# 或
memalgo 2
```

**支持的参数：**
- `0` 或 `lru` 或 `LRU` - 使用 LRU
- `1` 或 `fifo` 或 `FIFO` - 使用 FIFO
- `2` 或 `clock` 或 `CLOCK` - 使用 CLOCK

---

## 输出说明

运行测试程序时，会输出类似信息：

```
=== 操作系统课程设计：段页式内存管理模拟 ===
支持多种置换算法：LRU / FIFO / CLOCK

选中的置换算法: LRU

系统物理内存初始化成功，共分配 8 页。

========== [监控线程] 捕获内存快照 ==========
--- 内存系统状态 ---
置换算法: LRU (Least Recently Used)
总物理页: 8, 页面大小: 1KB
总访问次数: 42, 缺页次数: 15
缺页率: 35.71%
空闲页框位图: 0x000000FF
--------------------
```

**关键指标说明：**
- **总访问次数**：内存的读写操作总数
- **缺页次数**：需要进行页面置换的次数
- **缺页率**：缺页次数 / 总访问次数 × 100%
  - 越低越好，表示算法的命中率越高
  - 用于对比不同算法的性能

---

## 性能对比建议

可以通过以下方式对比三种算法的性能：

1. **固定工作集大小**：使用相同的逻辑地址访问模式
2. **统计缺页率**：记录三种算法的缺页率差异
3. **调整参数**：
   - 改变物理页数（4~32 页之间）
   - 改变逻辑地址的随机访问范围
   - 改变读写操作的比例

### 预期结果（一般情况）

在工作集大于物理内存的场景下：
- **LRU**: 通常缺页率最低（最优）
- **CLOCK**: 缺页率接近 LRU，但扫描效率更高
- **FIFO**: 缺页率最高，可能出现 Belady 异常

---

## 内部数据结构

### 全局变量

```c
/* 置换算法选择 */
static ReplacementAlgorithm current_algorithm;

/* CLOCK 算法指针 */
static int clock_ptr = 0;

/* FIFO 时间戳数组 */
static uint32_t fifo_install_time[MAX_PHYS_PAGES];

/* 逆向映射表 - 三个算法共用 */
static PTE* frame_reverse_map[MAX_PHYS_PAGES];
```

### 关键函数

```c
/* 选择性地调用对应的置换算法 */
static int execute_replacement()

/* 三种实现函数 */
static int execute_lru_replacement()
static int execute_fifo_replacement()
static int execute_clock_replacement()
```

---

## 注意事项

1. **线程安全**：所有置换操作应在互斥锁保护下进行
2. **Belady 异常**：FIFO 算法可能在增加物理页数时导致缺页率反而上升
3. **LRU 近似**：CLOCK 算法是对 LRU 的近似实现，性能略低但效率更高
4. **时间戳溢出**：`global_time` 是 uint32_t，在长时间运行后可能溢出（可改进）

---

祝你的课设顺利！ 🎉
