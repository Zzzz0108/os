// create_disk.c - 创建虚拟磁盘
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISK_SIZE 100 * 1024 * 1024  // 100MB
#define SECTOR_SIZE 512

int main() {
    FILE* disk = fopen("mydisk.img", "wb");
    if (!disk) {
        printf("创建磁盘文件失败\n");
        return 1;
    }
    
    printf("正在创建虚拟磁盘文件 mydisk.img (100MB)...\n");
    
    // 分配100MB空间，初始化为0
    char* buffer = (char*)calloc(1, SECTOR_SIZE);
    for (long long i = 0; i < DISK_SIZE / SECTOR_SIZE; i++) {
        fwrite(buffer, 1, SECTOR_SIZE, disk);
    }
    
    free(buffer);
    fclose(disk);
    printf("虚拟磁盘创建成功!\n");
    return 0;
}
