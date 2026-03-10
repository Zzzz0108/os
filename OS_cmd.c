#include"OS_cmd.h"
#include <stdarg.h>

//========================
// self_* 系统调用封装层
//========================

int self_printf(const char *format, ...) {
    int ret;
    va_list args;

    va_start(args, format);
    ret = vprintf(format, args);   // 目前直接使用标准库 vprintf
    va_end(args);

    return ret;
}

int self_scanf(const char *format, ...) {
    int ret;
    va_list args;

    va_start(args, format);
    ret = vscanf(format, args);    // 目前直接使用标准库 vscanf
    va_end(args);

    return ret;
}

char *self_fgets(char *buffer, int size) {
    return fgets(buffer, size, stdin); // 目前直接使用标准库 fgets
}

int self_system(const char *command) {
    return system(command);            // 目前直接使用标准库 system
}


//========================
// 本文件内部的简单字符串工具
// 只做命令解析，不调用标准库函数
//========================

// 去掉字符串里的回车/换行（就地修改）
void remove_newline(char *s) {
    int i = 0;
    if (!s) return;
    while (s[i] != '\0') {
        if (s[i] == '\r' || s[i] == '\n') {
            s[i] = '\0';
            break;
        }
        ++i;
    }
}

// 从 src 中取出第一个单词，写入 dst
void get_first_word(const char *src, char *dst, int dstSize) {
    int i = 0;
    int j = 0;

    if (!src || !dst || dstSize <= 0) return;

    // 跳过前导空白
    while (src[i] == ' ' || src[i] == '\t') {
        ++i;
    }

    // 复制第一个单词
    while (src[i] != '\0' && src[i] != ' ' && src[i] != '\t') {
        if (j < dstSize - 1) {
            dst[j++] = src[i];
        }
        ++i;
    }
    dst[j] = '\0';
}

// 判断两个以 '\0' 结尾的字符串是否完全相等
int strings_equal(const char *a, const char *b) {
    int i = 0;
    if (!a || !b) return 0;

    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        ++i;
    }

    return (a[i] == '\0' && b[i] == '\0');
}

// 从整行命令中跳过 "echo" 以及之后的空白，返回要输出的消息部分
char *get_echo_message(char *cmd) {
    char *p = cmd;
    if (!p) return NULL;

    // 跳过前导空白
    while (*p == ' ' || *p == '\t') {
        ++p;
    }

    // 跳过第一个单词（例如 "echo"）
    while (*p != '\0' && *p != ' ' && *p != '\t') {
        ++p;
    }

    // 再跳过空白，剩下的就是参数部分
    while (*p == ' ' || *p == '\t') {
        ++p;
    }

    return p;
}


//========================
// “内核”/终端业务逻辑
//========================

void help() {
    self_printf("可用命令:\n");
    self_printf("  help       查看帮助\n");
    self_printf("  clear      清屏\n");
    self_printf("  echo ...   输出文字\n");
    self_printf("  dir        显示目录\n");
    self_printf("  sysinfo    系统信息\n");
    self_printf("  exit       退出终端\n");
}

void clear() {
    self_system("cls");
}

void echo(const char *msg) {
    self_printf("%s\n", msg);
}

void dir() {
    self_printf("  目录: C:\\MiniOS\\\n");
    self_printf("  kernel.sys\n");
    self_printf("  shell.exe\n");
    self_printf("  user.cfg\n");
    self_printf("  boot.ini\n");
}

void sysinfo() {
    self_printf("=== 系统信息 ===\n");
    self_printf("OS    : MiniOS 1.0\n");
    self_printf("Build : 2026\n");
    self_printf("Shell : Terminal\n");
    self_printf("Compiler: MSVC (C)\n");
}

void welcome() {
    clear();
    self_printf("========================================\n");
    self_printf("       Mini OS Terminal (纯 C/VS)       \n");
    self_printf("          输入 help 查看命令            \n");
    self_printf("========================================\n");
}


