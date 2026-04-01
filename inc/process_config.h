﻿#ifndef CONFIG_H
#define CONFIG_H
/* 系统参数 */
#define MAX_PROCESS        64
#define MAX_NAME_LEN       32
#define MAX_OPEN_FILES     8
/* MLFQ 队列层数 */
#define MLFQ_LEVELS        3
/* 时间片设置 */
#define TIME_SLICE_Q0      1
#define TIME_SLICE_Q1      2
#define TIME_SLICE_Q2      4
/* 默认进程栈大小（未来裸机可以用） */
#define DEFAULT_STACK_SIZE 4096
#endif
