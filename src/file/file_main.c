// main.c - 主程序
#include "../../inc/file_myfs.h"
#include <stdio.h>
#include <string.h>
#include "../../inc/cmd.h"

void print_help() {
    self_printf("\n可用命令:\n");
    self_printf("  create <文件名>      - 创建文件\n");
    self_printf("  delete <文件名>      - 删除文件\n");
    self_printf("  mkdir <目录名>       - 创建目录\n");
    self_printf("  rmdir <目录名>       - 删除空目录\n");
    self_printf("  write <文件名> <内容> - 写入文件\n");
    self_printf("  ls [目录]            - 列出文件/目录\n");
    self_printf("  cd <目录>            - 切换目录\n");
    self_printf("  pwd                  - 显示当前目录\n");
    self_printf("  tree                 - 树形显示目录结构\n");
    self_printf("  info                 - 显示文件系统信息\n");
    self_printf("  format               - 格式化磁盘\n");
    self_printf("  exit                 - 退出\n");
    self_printf("  help                 - 显示帮助\n\n");
}

int main() {
    MyFS fs;
    char cmd[512];
    
    self_printf("========================================\n");
    self_printf("     我的FAT12文件系统 v2.0\n");
    self_printf("========================================\n");
    
    // 挂载磁盘
    if (myfs_mount(&fs, "mydisk.img") != 0) {
        self_printf("挂载磁盘失败！请先运行 create_disk.exe 创建磁盘文件\n");
        return 1;
    }
    
    // 询问是否格式化
    self_printf("是否格式化磁盘？(y/n): ");
    char choice = getchar();
    while (getchar() != '\n'); // 清空输入缓冲区
    
    if (choice == 'y' || choice == 'Y') {
        myfs_format(&fs);
    }
    
    print_help();
    
    while (1) {
        self_printf("\033[32m%s\033[0m> ", fs.current_path);  // 彩色显示当前路径
        fflush(stdout);
        
        if (!self_fgets(cmd, sizeof(cmd))) break;
        
        // 去掉换行符
        size_t len = strlen(cmd);
        if (len > 0 && cmd[len-1] == '\n') {
            cmd[len-1] = '\0';
        }
        
        if (strlen(cmd) == 0) continue;
        
        // 解析命令
        if (strcmp(cmd, "ls") == 0) {
            myfs_list_dir(&fs, NULL);
        }
        else if (strncmp(cmd, "ls ", 3) == 0) {
            myfs_list_dir(&fs, cmd + 3);
        }
        else if (strcmp(cmd, "pwd") == 0) {
            myfs_pwd(&fs);
        }
        else if (strcmp(cmd, "tree") == 0) {
            myfs_tree(&fs, NULL, 0);
        }
        else if (strcmp(cmd, "info") == 0) {
            myfs_info(&fs);
        }
        else if (strncmp(cmd, "cd ", 3) == 0) {
            myfs_change_dir(&fs, cmd + 3);
        }
        else if (strcmp(cmd, "cd") == 0) {
            myfs_change_dir(&fs, "/");
        }
        else if (strncmp(cmd, "create ", 7) == 0) {
            myfs_create_file(&fs, cmd + 7);
        }
        else if (strncmp(cmd, "delete ", 7) == 0) {
            myfs_delete_file(&fs, cmd + 7);
        }
        else if (strncmp(cmd, "mkdir ", 6) == 0) {
            myfs_create_dir(&fs, cmd + 6);
        }
        else if (strncmp(cmd, "rmdir ", 6) == 0) {
            myfs_delete_dir(&fs, cmd + 6);
        }
        else if (strncmp(cmd, "write ", 6) == 0) {
            char* space = strchr(cmd + 6, ' ');
            if (space) {
                *space = '\0';
                myfs_write_file(&fs, cmd + 6, space + 1);
            } else {
                self_printf("用法: write <文件名> <内容>\n");
            }
        }
        else if (strcmp(cmd, "format") == 0) {
            self_printf("确定要格式化吗？(y/n): ");
            char confirm = getchar();
            while (getchar() != '\n');
            if (confirm == 'y' || confirm == 'Y') {
                myfs_format(&fs);
            }
        }
        else if (strcmp(cmd, "exit") == 0) {
            break;
        }
        else if (strcmp(cmd, "help") == 0) {
            print_help();
        }
        else if (strlen(cmd) > 0) {
            self_printf("未知命令: %s (输入 help 查看帮助)\n", cmd);
        }
    }
    
    myfs_umount(&fs);
    self_printf("再见！\n");
    return 0;
}
