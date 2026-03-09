#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CMD 1024

void help() {
    printf("可用命令:\n");
    printf("  help       查看帮助\n");
    printf("  clear      清屏\n");
    printf("  echo ...   输出文字\n");
    printf("  dir        显示目录\n");
    printf("  sysinfo    系统信息\n");
    printf("  exit       退出终端\n");
}

void clear() {
    system("cls");
}

void echo(const char *msg) {
    printf("%s\n", msg);
}

void dir() {
    printf("  目录: C:\\MiniOS\\\n");
    printf("  kernel.sys\n");
    printf("  shell.exe\n");
    printf("  user.cfg\n");
    printf("  boot.ini\n");
}

void sysinfo() {
    printf("=== 系统信息 ===\n");
    printf("OS    : MiniOS 1.0\n");
    printf("Build : 2026\n");
    printf("Shell : Terminal\n");
    printf("Compiler: MSVC (C)\n");
}

void welcome() {
    clear();
    printf("========================================\n");
    printf("       Mini OS Terminal (纯 C/VS)       \n");
    printf("          输入 help 查看命令            \n");
    printf("========================================\n");
}

int main() {
    char cmd[MAX_CMD];
    welcome();

    while (1) {
        printf("\n[MiniOS] > ");
        if (!fgets(cmd, MAX_CMD, stdin))
            break;

        // 去掉换行
        cmd[strcspn(cmd, "\n")] = 0;

        // 分割第一个单词
        char op[MAX_CMD] = {0};
        sscanf(cmd, "%s", op);

        if (strcmp(op, "help") == 0) {
            help();
        } else if (strcmp(op, "clear") == 0) {
            clear();
        } else if (strcmp(op, "echo") == 0) {
            char *msg = cmd + strlen("echo");
            while (*msg == ' ') msg++;
            echo(msg);
        } else if (strcmp(op, "dir") == 0) {
            dir();
        } else if (strcmp(op, "sysinfo") == 0) {
            sysinfo();
        } else if (strcmp(op, "exit") == 0) {
            printf("退出终端...\n");
            break;
        } else if (strlen(op) > 0) {
            printf("不支持的命令: %s\n", op);
        }
    }
    return 0;
}