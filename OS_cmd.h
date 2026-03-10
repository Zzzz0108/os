// OS_cmd 的公共接口与常量定义

#pragma once

// 基础头文件（真实 OS 提供的库 + C 标准库）
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_CMD 1024


//========================
// self_* “系统调用”接口层
//========================

// I/O 接口：上层代码只调用 self_*，内部目前使用标准库实现
int  self_printf(const char *format, ...);
int  self_scanf(const char *format, ...);
char *self_fgets(char *buffer, int size);
int  self_system(const char *command);

// 字符串/命令解析辅助函数（如需在其他模块重用）
void   remove_newline(char *s);
void   get_first_word(const char *src, char *dst, int dstSize);
int    strings_equal(const char *a, const char *b);
char  *get_echo_message(char *cmd);

// 终端命令相关接口（供需要时其他模块调用）
void help(void);
void clear(void);
void echo(const char *msg);
void dir(void);
void sysinfo(void);
void welcome(void);

