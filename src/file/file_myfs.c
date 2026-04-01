// myfs.c - 完整版支持目录操作和路径切换
#include "../../inc/file_myfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../inc/cmd.h"
// 读取扇区
static int read_sector(MyFS* fs, unsigned int sector, void* buffer) {
    fseek(fs->disk, sector * SECTOR_SIZE, SEEK_SET);
    return fread(buffer, 1, SECTOR_SIZE, fs->disk) == SECTOR_SIZE ? 0 : -1;
}
// 写入扇区
static int write_sector(MyFS* fs, unsigned int sector, const void* buffer) {
    fseek(fs->disk, sector * SECTOR_SIZE, SEEK_SET);
    return fwrite(buffer, 1, SECTOR_SIZE, fs->disk) == SECTOR_SIZE ? 0 : -1;
}
// 挂载文件系统
int myfs_mount(MyFS* fs, const char* disk_path) {
    fs->disk = fopen(disk_path, "r+b");
    if (!fs->disk) return -1;
    // 计算磁盘布局
    fseek(fs->disk, 0, SEEK_END);
    fs->total_sectors = ftell(fs->disk) / SECTOR_SIZE;
    fs->fat_start = RESERVED_SECTORS;
    fs->root_start = fs->fat_start + FAT_SECTORS;
    fs->data_start = fs->root_start + ROOT_DIR_SECTORS;
    // 分配FAT表内存
    fs->fat = (unsigned short*)malloc(FAT_SECTORS * SECTOR_SIZE);
    if (!fs->fat) return -1;
    // 读取FAT表
    read_sector(fs, fs->fat_start, fs->fat);
    // 初始化当前目录为根目录
    fs->current_cluster = 0;
    strcpy(fs->current_path, "/");
    return 0;
}
// 卸载文件系统
void myfs_umount(MyFS* fs) {
    if (fs->fat) {
        // 写回FAT表
        write_sector(fs, fs->fat_start, fs->fat);
        free(fs->fat);
        fs->fat = NULL;
    }
    if (fs->disk) {
        fclose(fs->disk);
        fs->disk = NULL;
    }
}
// 格式化磁盘
int myfs_format(MyFS* fs) {
    unsigned char sector[SECTOR_SIZE];
    self_printf("正在格式化磁盘...\n");
    // 1. 首先清空所有扇区（使用全0）
    memset(sector, 0, SECTOR_SIZE);
    // 2. 清空FAT表区域
    for (int i = 0; i < FAT_SECTORS; i++) {
        write_sector(fs, fs->fat_start + i, sector);
    }
    // 3. 初始化FAT表的前两个特殊条目
    unsigned short* fat = (unsigned short*)sector;
    fat[0] = 0xFFF8;  // 媒体描述符
    fat[1] = 0xFFFF;  // FAT表结束标志
    write_sector(fs, fs->fat_start, sector);  // 只写第一个扇区
    // 4. 清空根目录区（确保全部是0）
    memset(sector, 0, SECTOR_SIZE);
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        write_sector(fs, fs->root_start + i, sector);
    }
    // 5. 重新读取FAT表到内存
    read_sector(fs, fs->fat_start, fs->fat);
    // 6. 重置当前目录
    fs->current_cluster = 0;
    strcpy(fs->current_path, "/");
    self_printf("格式化完成\n");
    return 0;
}
// 查找空闲簇
static int find_free_cluster(MyFS* fs) {
    unsigned short* fat = fs->fat;
    int total_clusters = (fs->total_sectors - fs->data_start) / SECTORS_PER_CLUSTER;
    for (int i = 2; i < total_clusters; i++) {
        if (fat[i] == 0) return i;
    }
    return -1;
}
// 分配簇
static int allocate_cluster(MyFS* fs) {
    int cluster = find_free_cluster(fs);
    if (cluster < 0) return -1;
    fs->fat[cluster] = 0xFFF;  // 文件结束标志
    return cluster;
}
// 文件名转换函数
static void name_to_fat(const char* path, char* name8, char* ext3) {
    char filename[256];
    char filepart[256] = {0};
    char extpart[256] = {0};
    strcpy(filename, path);
    // 去掉路径
    char* slash = strrchr(filename, '/');
    if (slash) strcpy(filename, slash + 1);
    slash = strrchr(filename, '\\');
    if (slash) strcpy(filename, slash + 1);
    // 分离文件名和扩展名
    char* dot = strchr(filename, '.');
    if (dot) {
        // 有扩展名
        int name_len = dot - filename;
        strncpy(filepart, filename, name_len);
        filepart[name_len] = '\0';
        strcpy(extpart, dot + 1);
    } else {
        // 无扩展名
        strcpy(filepart, filename);
        extpart[0] = '\0';
    }
    // 填充文件名到8个字符（不足补空格）
    memset(name8, ' ', 8);
    int name_len = strlen(filepart);
    if (name_len > 8) name_len = 8;
    memcpy(name8, filepart, name_len);
    // 填充扩展名到3个字符（不足补空格）
    memset(ext3, ' ', 3);
    int ext_len = strlen(extpart);
    if (ext_len > 3) ext_len = 3;
    memcpy(ext3, extpart, ext_len);
}
// 分割路径为目录路径和文件名
void split_path(const char* fullpath, char* dir_path, char* file_name) {
    char temp[256];
    strcpy(temp, fullpath);
    // 查找最后一个路径分隔符
    char* last_slash = strrchr(temp, '/');
    char* last_backslash = strrchr(temp, '\\');
    // 取更靠后的那个
    if (last_backslash > last_slash) {
        last_slash = last_backslash;
    }
    if (last_slash) {
        *last_slash = '\0';
        strcpy(dir_path, temp);
        strcpy(file_name, last_slash + 1);
    } else {
        strcpy(dir_path, "");
        strcpy(file_name, temp);
    }
}
// 查找目录的起始簇号（支持相对路径）
int find_dir_cluster(MyFS* fs, const char* path) {
    // 如果是空路径或根目录
    if (strlen(path) == 0 || strcmp(path, "/") == 0) {
        return 0;  // 根目录簇号为0
    }
    // 如果是当前目录
    if (strcmp(path, ".") == 0) {
        return fs->current_cluster;
    }
    // 如果是父目录
    if (strcmp(path, "..") == 0) {
        if (fs->current_cluster == 0) {
            return 0;  // 根目录的父目录还是根目录
        }
        // 查找当前目录的父目录
        unsigned char sector[SECTOR_SIZE];
        int sector_num = fs->data_start + (fs->current_cluster - 2) * SECTORS_PER_CLUSTER;
        read_sector(fs, sector_num, sector);
        DirEntry* entries = (DirEntry*)sector;
        // ".." 条目是第二个
        if (entries[1].name[0] == '.' && entries[1].name[1] == '.') {
            return entries[1].first_cluster;
        }
        return 0;
    }
    // 处理相对路径（如 "web/html"）
    char temp_path[256];
    strcpy(temp_path, path);
    // 从当前目录开始查找
    int current = fs->current_cluster;
    // 使用 strtok 分割路径
    char* token = strtok(temp_path, "/\\");
    char* contexts[10];
    int count = 0;
    while (token != NULL && count < 10) {
        contexts[count++] = token;
        token = strtok(NULL, "/\\");
    }
    // 逐级查找
    int result_cluster = current;
    for (int i = 0; i < count; i++) {
        char* part = contexts[i];
        unsigned char sector[SECTOR_SIZE];
        char name8[8] = {0}, ext3[3] = {0};
        // 跳过空的部分
        if (strlen(part) == 0) continue;
        // 处理特殊目录
        if (strcmp(part, ".") == 0) {
            continue;  // 当前目录，保持不变
        }
        if (strcmp(part, "..") == 0) {
            // 返回上一级
            if (result_cluster == 0) {
                continue;  // 根目录的上一级还是根目录
            }
            // 查找父目录
            int sector_num = fs->data_start + (result_cluster - 2) * SECTORS_PER_CLUSTER;
            read_sector(fs, sector_num, sector);
            DirEntry* entries = (DirEntry*)sector;
            if (entries[1].name[0] == '.' && entries[1].name[1] == '.') {
                result_cluster = entries[1].first_cluster;
            }
            continue;
        }
        // 普通目录名转换
        name_to_fat(part, name8, ext3);
        memset(ext3, ' ', 3);  // 目录的扩展名为空格
        int found = 0;
        if (result_cluster == 0) {
            // 在根目录中查找
            for (int i = 0; i < ROOT_DIR_SECTORS && !found; i++) {
                read_sector(fs, fs->root_start + i, sector);
                DirEntry* entries = (DirEntry*)sector;
                for (int j = 0; j < SECTOR_SIZE / 32; j++) {
                    if (entries[j].name[0] != 0 && entries[j].name[0] != 0xE5 &&
                        memcmp(entries[j].name, name8, 8) == 0 &&
                        memcmp(entries[j].ext, ext3, 3) == 0 &&
                        (entries[j].attr & ATTR_DIRECTORY)) {
                        result_cluster = entries[j].first_cluster;
                        found = 1;
                        break;
                    }
                }
            }
        } else {
            // 在子目录中查找
            int sector_num = fs->data_start + (result_cluster - 2) * SECTORS_PER_CLUSTER;
            read_sector(fs, sector_num, sector);
            DirEntry* entries = (DirEntry*)sector;
            // 跳过 . 和 .. 条目
            for (int j = 2; j < SECTOR_SIZE / 32 && !found; j++) {
                if (entries[j].name[0] != 0 && entries[j].name[0] != 0xE5 &&
                    memcmp(entries[j].name, name8, 8) == 0 &&
                    memcmp(entries[j].ext, ext3, 3) == 0 &&
                    (entries[j].attr & ATTR_DIRECTORY)) {
                    result_cluster = entries[j].first_cluster;
                    found = 1;
                    break;
                }
            }
        }
        if (!found) {
            return -1;  // 目录不存在
        }
    }
    return result_cluster;
}
// 在指定目录中查找空闲目录项
static int find_free_entry_in_dir(MyFS* fs, int dir_cluster, int* sector_index, int* entry_index) {
    unsigned char sector[SECTOR_SIZE];
    if (dir_cluster == 0) {
        // 在根目录中查找
        for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
            read_sector(fs, fs->root_start + i, sector);
            DirEntry* entries = (DirEntry*)sector;
            for (int j = 0; j < SECTOR_SIZE / 32; j++) {
                if (entries[j].name[0] == 0 || entries[j].name[0] == 0xE5) {
                    *sector_index = i;
                    *entry_index = j;
                    return fs->root_start + i;
                }
            }
        }
    } else {
        // 在子目录中查找
        int sector_num = fs->data_start + (dir_cluster - 2) * SECTORS_PER_CLUSTER;
        read_sector(fs, sector_num, sector);
        DirEntry* entries = (DirEntry*)sector;
        for (int j = 2; j < SECTOR_SIZE / 32; j++) {  // 跳过 . 和 ..
            if (entries[j].name[0] == 0 || entries[j].name[0] == 0xE5) {
                *sector_index = sector_num;
                *entry_index = j;
                return sector_num;
            }
        }
    }
    return -1;  // 目录已满
}
// 检查目录是否为空
static int is_dir_empty(MyFS* fs, int cluster) {
    unsigned char sector[SECTOR_SIZE];
    if (cluster == 0) {
        // 根目录永远不为空（有 . 和 ..）
        return 0;
    }
    int sector_num = fs->data_start + (cluster - 2) * SECTORS_PER_CLUSTER;
    read_sector(fs, sector_num, sector);
    DirEntry* entries = (DirEntry*)sector;
    // 跳过 . 和 .. 条目（前两个）
    for (int i = 2; i < SECTOR_SIZE / 32; i++) {
        // 检查是否有有效条目（不是空且不是已删除）
        if (entries[i].name[0] != 0 && entries[i].name[0] != 0xE5) {
            return 0;  // 目录不为空
        }
    }
    return 1;  // 目录为空
}
// 更新目录大小
static void update_dir_size(MyFS* fs, int dir_cluster) {
    unsigned char sector[SECTOR_SIZE];
    int entry_count = 0;
    if (dir_cluster == 0) {
        // 根目录特殊处理
        return;
    }
    // 读取目录的第一个扇区
    int sector_num = fs->data_start + (dir_cluster - 2) * SECTORS_PER_CLUSTER;
    read_sector(fs, sector_num, sector);
    DirEntry* dir_entries = (DirEntry*)sector;
    // 计算目录中有效的条目数
    for (int i = 0; i < SECTOR_SIZE / 32; i++) {
        if (dir_entries[i].name[0] != 0 && dir_entries[i].name[0] != 0xE5) {
            entry_count++;
        }
    }
    // 这里需要更新父目录中这个目录的目录项
    // 由于实现复杂，暂时只打印信息
}
// 创建文件
int myfs_create_file(MyFS* fs, const char* path) {
    unsigned char sector[SECTOR_SIZE];
    char dir_path[256] = {0};
    char file_name[256] = {0};
    char name8[8] = {0}, ext3[3] = {0};
    // 分割路径
    split_path(path, dir_path, file_name);
    // 转换文件名
    name_to_fat(file_name, name8, ext3);
    // 查找目标目录的簇号
    int dir_cluster;
    if (strlen(dir_path) == 0) {
        dir_cluster = fs->current_cluster;  // 使用当前目录
    } else {
        dir_cluster = find_dir_cluster(fs, dir_path);
    }
    if (dir_cluster < 0) {
        self_printf("错误: 目录不存在: %s\n", dir_path);
        return -1;
    }
    // 在目标目录中查找空闲位置
    int target_sector, target_index;
    int sector_num = find_free_entry_in_dir(fs, dir_cluster, &target_sector, &target_index);
    if (sector_num < 0) {
        self_printf("错误: 目录已满\n");
        return -1;
    }
    // 读取目标扇区
    if (dir_cluster == 0) {
        read_sector(fs, fs->root_start + target_sector, sector);
    } else {
        read_sector(fs, sector_num, sector);
    }
    DirEntry* entries = (DirEntry*)sector;
    // 初始化目录项
    memset(&entries[target_index], 0, 32);
    memcpy(entries[target_index].name, name8, 8);
    memcpy(entries[target_index].ext, ext3, 3);
    entries[target_index].attr = ATTR_ARCHIVE;
    // 分配第一个簇
    int cluster = allocate_cluster(fs);
    if (cluster < 0) return -1;
    entries[target_index].first_cluster = cluster;
    // 设置时间
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    entries[target_index].create_date = ((tm_info->tm_year - 80) << 9) | 
                                         ((tm_info->tm_mon + 1) << 5) | 
                                         tm_info->tm_mday;
    entries[target_index].create_time = (tm_info->tm_hour << 11) | 
                                         (tm_info->tm_min << 5) | 
                                         (tm_info->tm_sec / 2);
    // 写回扇区
    if (dir_cluster == 0) {
        write_sector(fs, fs->root_start + target_sector, sector);
    } else {
        write_sector(fs, sector_num, sector);
        // 更新目录大小
        update_dir_size(fs, dir_cluster);
    }
    // 写回FAT表
    write_sector(fs, fs->fat_start, fs->fat);
    self_printf("文件创建成功: %s (起始簇: %d)\n", path, cluster);
    return 0;
}
// 创建目录
int myfs_create_dir(MyFS* fs, const char* path) {
    unsigned char sector[SECTOR_SIZE];
    char dir_path[256] = {0};
    char dir_name[256] = {0};
    char name8[8] = {0}, ext3[3] = {0};
    // 分割路径
    split_path(path, dir_path, dir_name);
    // 目录名转换
    name_to_fat(dir_name, name8, ext3);
    memset(ext3, ' ', 3);  // 目录的扩展名为空格
    // 查找父目录的簇号
    int parent_cluster;
    if (strlen(dir_path) == 0) {
        parent_cluster = fs->current_cluster;
    } else {
        parent_cluster = find_dir_cluster(fs, dir_path);
    }
    if (parent_cluster < 0) {
        self_printf("错误: 父目录不存在: %s\n", dir_path);
        return -1;
    }
    // 在父目录中查找空闲位置
    int target_sector, target_index;
    int sector_num = find_free_entry_in_dir(fs, parent_cluster, &target_sector, &target_index);
    if (sector_num < 0) {
        self_printf("错误: 父目录已满\n");
        return -1;
    }
    // 分配一个簇用于存储目录内容
    int cluster = allocate_cluster(fs);
    if (cluster < 0) {
        self_printf("错误: 无法分配簇\n");
        return -1;
    }
    // 读取父目录扇区
    if (parent_cluster == 0) {
        read_sector(fs, fs->root_start + target_sector, sector);
    } else {
        read_sector(fs, sector_num, sector);
    }
    DirEntry* entries = (DirEntry*)sector;
    // 初始化目录项
    memset(&entries[target_index], 0, 32);
    memcpy(entries[target_index].name, name8, 8);
    memcpy(entries[target_index].ext, ext3, 3);
    entries[target_index].attr = ATTR_DIRECTORY;
    entries[target_index].first_cluster = cluster;
    // 设置时间
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    entries[target_index].create_date = ((tm_info->tm_year - 80) << 9) | 
                                         ((tm_info->tm_mon + 1) << 5) | 
                                         tm_info->tm_mday;
    entries[target_index].create_time = (tm_info->tm_hour << 11) | 
                                         (tm_info->tm_min << 5) | 
                                         (tm_info->tm_sec / 2);
    // 设置目录大小（初始为2个目录项：. 和 ..）
    entries[target_index].file_size = 2 * 32;
    // 写回父目录扇区
    if (parent_cluster == 0) {
        write_sector(fs, fs->root_start + target_sector, sector);
    } else {
        write_sector(fs, sector_num, sector);
        update_dir_size(fs, parent_cluster);
    }
    // 初始化新目录的内容（添加 . 和 .. 条目）
    unsigned char dir_sector[SECTOR_SIZE];
    memset(dir_sector, 0, SECTOR_SIZE);
    DirEntry* dir_entries = (DirEntry*)dir_sector;
    // 添加 "." 条目（当前目录）
    memset(&dir_entries[0], 0, 32);
    memset(dir_entries[0].name, ' ', 8);
    dir_entries[0].name[0] = '.';
    dir_entries[0].attr = ATTR_DIRECTORY;
    dir_entries[0].first_cluster = cluster;
    dir_entries[0].file_size = 0;
    // 添加 ".." 条目（父目录）
    memset(&dir_entries[1], 0, 32);
    memset(dir_entries[1].name, ' ', 8);
    dir_entries[1].name[0] = '.';
    dir_entries[1].name[1] = '.';
    dir_entries[1].attr = ATTR_DIRECTORY;
    dir_entries[1].first_cluster = parent_cluster;
    dir_entries[1].file_size = 0;
    // 写入目录的第一个扇区
    int dir_sector_num = fs->data_start + (cluster - 2) * SECTORS_PER_CLUSTER;
    write_sector(fs, dir_sector_num, dir_sector);
    // 写回FAT表
    write_sector(fs, fs->fat_start, fs->fat);
    self_printf("目录创建成功: %s (起始簇: %d, 大小: %lu字节)\n", 
           path, cluster, entries[target_index].file_size);
    return 0;
}
// 删除文件
int myfs_delete_file(MyFS* fs, const char* path) {
    unsigned char sector[SECTOR_SIZE];
    char dir_path[256] = {0};
    char file_name[256] = {0};
    char name8[8] = {0}, ext3[3] = {0};
    // 分割路径
    split_path(path, dir_path, file_name);
    // 转换文件名
    name_to_fat(file_name, name8, ext3);
    // 查找目标目录的簇号
    int dir_cluster;
    if (strlen(dir_path) == 0) {
        dir_cluster = fs->current_cluster;
    } else {
        dir_cluster = find_dir_cluster(fs, dir_path);
    }
    if (dir_cluster < 0) {
        self_printf("错误: 目录不存在: %s\n", dir_path);
        return -1;
    }
    // 在目标目录中查找文件
    if (dir_cluster == 0) {
        // 在根目录中查找
        for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
            read_sector(fs, fs->root_start + i, sector);
            DirEntry* entries = (DirEntry*)sector;
            for (int j = 0; j < SECTOR_SIZE / 32; j++) {
                // 比较文件名和扩展名
                int name_match = 1;
                for (int m = 0; m < 8; m++) {
                    if (entries[j].name[m] != name8[m]) {
                        name_match = 0;
                        break;
                    }
                }
                int ext_match = 1;
                for (int m = 0; m < 3; m++) {
                    if (entries[j].ext[m] != ext3[m]) {
                        ext_match = 0;
                        break;
                    }
                }
                if (name_match && ext_match && !(entries[j].attr & ATTR_DIRECTORY)) {
                    // 释放簇链
                    int cluster = entries[j].first_cluster;
                    while (cluster >= 2 && cluster < 0xFFF8) {
                        int next = fs->fat[cluster];
                        fs->fat[cluster] = 0;
                        cluster = next;
                    }
                    // 标记目录项为已删除
                    entries[j].name[0] = 0xE5;
                    write_sector(fs, fs->root_start + i, sector);
                    // 写回FAT表
                    write_sector(fs, fs->fat_start, fs->fat);
                    self_printf("文件删除成功: %s\n", path);
                    return 0;
                }
            }
        }
    } else {
        // 在子目录中查找
        int sector_num = fs->data_start + (dir_cluster - 2) * SECTORS_PER_CLUSTER;
        read_sector(fs, sector_num, sector);
        DirEntry* entries = (DirEntry*)sector;
        for (int j = 2; j < SECTOR_SIZE / 32; j++) {  // 跳过 . 和 ..
            // 比较文件名和扩展名
            int name_match = 1;
            for (int m = 0; m < 8; m++) {
                if (entries[j].name[m] != name8[m]) {
                    name_match = 0;
                    break;
                }
            }
            int ext_match = 1;
            for (int m = 0; m < 3; m++) {
                if (entries[j].ext[m] != ext3[m]) {
                    ext_match = 0;
                    break;
                }
            }
            if (name_match && ext_match && !(entries[j].attr & ATTR_DIRECTORY)) {
                // 释放簇链
                int cluster = entries[j].first_cluster;
                while (cluster >= 2 && cluster < 0xFFF8) {
                    int next = fs->fat[cluster];
                    fs->fat[cluster] = 0;
                    cluster = next;
                }
                // 标记目录项为已删除
                entries[j].name[0] = 0xE5;
                write_sector(fs, sector_num, sector);
                // 更新目录大小
                update_dir_size(fs, dir_cluster);
                // 写回FAT表
                write_sector(fs, fs->fat_start, fs->fat);
                self_printf("文件删除成功: %s\n", path);
                return 0;
            }
        }
    }
    self_printf("文件不存在: %s\n", path);
    return -1;
}
// 删除目录
int myfs_delete_dir(MyFS* fs, const char* path) {
    unsigned char sector[SECTOR_SIZE];
    char dir_path[256] = {0};
    char dir_name[256] = {0};
    char name8[8] = {0}, ext3[3] = {0};
    // 分割路径
    split_path(path, dir_path, dir_name);
    // 目录名转换
    name_to_fat(dir_name, name8, ext3);
    memset(ext3, ' ', 3);
    // 查找父目录的簇号
    int parent_cluster;
    if (strlen(dir_path) == 0) {
        parent_cluster = fs->current_cluster;
    } else {
        parent_cluster = find_dir_cluster(fs, dir_path);
    }
    if (parent_cluster < 0) {
        self_printf("错误: 父目录不存在: %s\n", dir_path);
        return -1;
    }
    // 查找要删除的目录
    int target_cluster = -1;
    int target_sector_num = 0, target_index = 0;
    if (parent_cluster == 0) {
        // 在根目录中查找
        for (int i = 0; i < ROOT_DIR_SECTORS && target_cluster < 0; i++) {
            read_sector(fs, fs->root_start + i, sector);
            DirEntry* entries = (DirEntry*)sector;
            for (int j = 0; j < SECTOR_SIZE / 32; j++) {
                // 比较目录名
                int name_match = 1;
                for (int m = 0; m < 8; m++) {
                    if (entries[j].name[m] != name8[m]) {
                        name_match = 0;
                        break;
                    }
                }
                if (name_match && (entries[j].attr & ATTR_DIRECTORY)) {
                    target_cluster = entries[j].first_cluster;
                    target_sector_num = fs->root_start + i;
                    target_index = j;
                    break;
                }
            }
        }
    } else {
        // 在子目录中查找
        int sector_num = fs->data_start + (parent_cluster - 2) * SECTORS_PER_CLUSTER;
        read_sector(fs, sector_num, sector);
        DirEntry* entries = (DirEntry*)sector;
        for (int j = 2; j < SECTOR_SIZE / 32 && target_cluster < 0; j++) {  // 跳过 . 和 ..
            // 比较目录名
            int name_match = 1;
            for (int m = 0; m < 8; m++) {
                if (entries[j].name[m] != name8[m]) {
                    name_match = 0;
                    break;
                }
            }
            if (name_match && (entries[j].attr & ATTR_DIRECTORY)) {
                target_cluster = entries[j].first_cluster;
                target_sector_num = sector_num;
                target_index = j;
                break;
            }
        }
    }
    if (target_cluster < 0) {
        self_printf("目录不存在: %s\n", path);
        return -1;
    }
    // 检查目录是否为空
    if (!is_dir_empty(fs, target_cluster)) {
        self_printf("错误: 目录不为空\n");
        return -1;
    }
    // 释放目录占用的簇
    int cluster = target_cluster;
    while (cluster >= 2 && cluster < 0xFFF8) {
        int next = fs->fat[cluster];
        fs->fat[cluster] = 0;
        cluster = next;
    }
    // 在父目录中标记目录项为已删除
    read_sector(fs, target_sector_num, sector);
    DirEntry* entries = (DirEntry*)sector;
    entries[target_index].name[0] = 0xE5;
    write_sector(fs, target_sector_num, sector);
    // 更新父目录大小
    update_dir_size(fs, parent_cluster);
    // 写回FAT表
    write_sector(fs, fs->fat_start, fs->fat);
    self_printf("目录删除成功: %s\n", path);
    return 0;
}
// 写入文件
int myfs_write_file(MyFS* fs, const char* path, const char* data) {
    unsigned char sector[SECTOR_SIZE];
    char dir_path[256] = {0};
    char file_name[256] = {0};
    char name8[8] = {0}, ext3[3] = {0};
    // 分割路径
    split_path(path, dir_path, file_name);
    // 转换文件名
    name_to_fat(file_name, name8, ext3);
    // 查找目标目录的簇号
    int dir_cluster;
    if (strlen(dir_path) == 0) {
        dir_cluster = fs->current_cluster;
    } else {
        dir_cluster = find_dir_cluster(fs, dir_path);
    }
    if (dir_cluster < 0) {
        self_printf("错误: 目录不存在: %s\n", dir_path);
        return -1;
    }
    int found = 0;
    int target_sector_num = 0, target_index = 0;
    DirEntry target_entry;
    // 查找文件
    if (dir_cluster == 0) {
        // 在根目录中查找
        for (int i = 0; i < ROOT_DIR_SECTORS && !found; i++) {
            read_sector(fs, fs->root_start + i, sector);
            DirEntry* entries = (DirEntry*)sector;
            for (int j = 0; j < SECTOR_SIZE / 32; j++) {
                // 比较文件名和扩展名
                int name_match = 1;
                for (int m = 0; m < 8; m++) {
                    if (entries[j].name[m] != name8[m]) {
                        name_match = 0;
                        break;
                    }
                }
                int ext_match = 1;
                for (int m = 0; m < 3; m++) {
                    if (entries[j].ext[m] != ext3[m]) {
                        ext_match = 0;
                        break;
                    }
                }
                if (name_match && ext_match && !(entries[j].attr & ATTR_DIRECTORY)) {
                    found = 1;
                    target_sector_num = fs->root_start + i;
                    target_index = j;
                    memcpy(&target_entry, &entries[j], 32);
                    break;
                }
            }
        }
    } else {
        // 在子目录中查找
        int sector_num = fs->data_start + (dir_cluster - 2) * SECTORS_PER_CLUSTER;
        read_sector(fs, sector_num, sector);
        DirEntry* entries = (DirEntry*)sector;
        for (int j = 2; j < SECTOR_SIZE / 32 && !found; j++) {  // 跳过 . 和 ..
            // 比较文件名和扩展名
            int name_match = 1;
            for (int m = 0; m < 8; m++) {
                if (entries[j].name[m] != name8[m]) {
                    name_match = 0;
                    break;
                }
            }
            int ext_match = 1;
            for (int m = 0; m < 3; m++) {
                if (entries[j].ext[m] != ext3[m]) {
                    ext_match = 0;
                    break;
                }
            }
            if (name_match && ext_match && !(entries[j].attr & ATTR_DIRECTORY)) {
                found = 1;
                target_sector_num = sector_num;
                target_index = j;
                memcpy(&target_entry, &entries[j], 32);
                break;
            }
        }
    }
    if (!found) {
        self_printf("文件不存在: %s\n", path);
        return -1;
    }
    // 先释放原有的簇链
    int cluster = target_entry.first_cluster;
    while (cluster >= 2 && cluster < 0xFFF8) {
        int next = fs->fat[cluster];
        fs->fat[cluster] = 0;
        cluster = next;
    }
    // 重新分配第一个簇
    cluster = allocate_cluster(fs);
    if (cluster < 0) {
        self_printf("错误: 无法分配簇\n");
        return -1;
    }
    target_entry.first_cluster = cluster;
    // 写入数据
    int data_len = strlen(data);
    int bytes_written = 0;
    int current_cluster = cluster;
    while (bytes_written < data_len) {
        int sector_num = fs->data_start + (current_cluster - 2) * SECTORS_PER_CLUSTER;
        memset(sector, 0, SECTOR_SIZE);
        int to_write = data_len - bytes_written;
        if (to_write > SECTOR_SIZE) to_write = SECTOR_SIZE;
        memcpy(sector, data + bytes_written, to_write);
        write_sector(fs, sector_num, sector);
        bytes_written += to_write;
        if (bytes_written < data_len) {
            int next_cluster = allocate_cluster(fs);
            if (next_cluster < 0) break;
            fs->fat[current_cluster] = next_cluster;
            current_cluster = next_cluster;
        } else {
            fs->fat[current_cluster] = 0xFFF;
        }
    }
    // 更新文件大小
    target_entry.file_size = data_len;
    // 更新修改时间
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    target_entry.modify_date = ((tm_info->tm_year - 80) << 9) | 
                                ((tm_info->tm_mon + 1) << 5) | 
                                tm_info->tm_mday;
    target_entry.modify_time = (tm_info->tm_hour << 11) | 
                                (tm_info->tm_min << 5) | 
                                (tm_info->tm_sec / 2);
    // 写回目录项
    read_sector(fs, target_sector_num, sector);
    ((DirEntry*)sector)[target_index] = target_entry;
    write_sector(fs, target_sector_num, sector);
    // 写回FAT表
    write_sector(fs, fs->fat_start, fs->fat);
    self_printf("写入 %d 字节到文件: %s\n", data_len, path);
    return 0;
}
// 改变当前目录
int myfs_change_dir(MyFS* fs, const char* path) {
    if (strcmp(path, "/") == 0) {
        fs->current_cluster = 0;
        strcpy(fs->current_path, "/");
        return 0;
    }
    if (strcmp(path, "..") == 0) {
        // 返回上一级目录
        if (fs->current_cluster == 0) {
            return 0;  // 已经在根目录
        }
        // 查找当前目录的父目录
        unsigned char sector[SECTOR_SIZE];
        int sector_num = fs->data_start + (fs->current_cluster - 2) * SECTORS_PER_CLUSTER;
        read_sector(fs, sector_num, sector);
        DirEntry* entries = (DirEntry*)sector;
        // ".." 条目是第二个
        if (entries[1].name[0] == '.' && entries[1].name[1] == '.') {
            fs->current_cluster = entries[1].first_cluster;
            // 更新路径
            char* last_slash = strrchr(fs->current_path, '/');
            if (last_slash && last_slash != fs->current_path) {
                *last_slash = '\0';
            } else {
                strcpy(fs->current_path, "/");
            }
        }
        return 0;
    }
    if (strcmp(path, ".") == 0) {
        return 0;  // 留在当前目录
    }
    // 查找目标目录
    int cluster = find_dir_cluster(fs, path);
    if (cluster < 0) {
        self_printf("目录不存在: %s\n", path);
        return -1;
    }
    fs->current_cluster = cluster;
    // 更新路径
    if (strcmp(fs->current_path, "/") == 0) {
        sprintf(fs->current_path, "/%s", path);
    } else {
        sprintf(fs->current_path, "%s/%s", fs->current_path, path);
    }
    return 0;
}
// 显示当前目录
int myfs_pwd(MyFS* fs) {
    self_printf("%s\n", fs->current_path);
    return 0;
}
// 计算目录总大小（包括所有子目录和文件）
static unsigned long calculate_dir_total_size(MyFS* fs, int cluster) {
    unsigned char sector[SECTOR_SIZE];
    unsigned long total = 0;
    if (cluster == 0) {
        // 根目录
        for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
            read_sector(fs, fs->root_start + i, sector);
            DirEntry* entries = (DirEntry*)sector;
            for (int j = 0; j < SECTOR_SIZE / 32; j++) {
                if (entries[j].name[0] != 0 && entries[j].name[0] != 0xE5) {
                    if (entries[j].attr & ATTR_DIRECTORY) {
                        // 递归计算子目录
                        total += calculate_dir_total_size(fs, entries[j].first_cluster);
                    } else {
                        total += entries[j].file_size;
                    }
                }
            }
        }
    } else {
        // 子目录
        int sector_num = fs->data_start + (cluster - 2) * SECTORS_PER_CLUSTER;
        read_sector(fs, sector_num, sector);
        DirEntry* entries = (DirEntry*)sector;
        // 跳过 . 和 .. 条目（前两个）
        for (int j = 2; j < SECTOR_SIZE / 32; j++) {
            if (entries[j].name[0] != 0 && entries[j].name[0] != 0xE5) {
                if (entries[j].attr & ATTR_DIRECTORY) {
                    total += calculate_dir_total_size(fs, entries[j].first_cluster);
                } else {
                    total += entries[j].file_size;
                }
            }
        }
    }
    return total;
}
// 列出目录
int myfs_list_dir(MyFS* fs, const char* path) {
    unsigned char sector[SECTOR_SIZE];
    int found = 0;
    int entry_count = 0;
    unsigned long total_size = 0;
    // 确定要列出的目录
    int list_cluster;
    if (path == NULL || strlen(path) == 0) {
        list_cluster = fs->current_cluster;
    } else {
        list_cluster = find_dir_cluster(fs, path);
        if (list_cluster < 0) {
            self_printf("目录不存在: %s\n", path);
            return -1;
        }
    }
    self_printf("\n%-15s %-10s %-8s %-8s %s\n", "文件名", "大小", "类型", "簇", "总大小(含子目录)");
    self_printf("---------------------------------------------------------------------\n");
    if (list_cluster == 0) {
        // 列出根目录
        for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
            read_sector(fs, fs->root_start + i, sector);
            DirEntry* entries = (DirEntry*)sector;
            for (int j = 0; j < SECTOR_SIZE / 32; j++) {
                if (entries[j].name[0] != 0 && entries[j].name[0] != 0xE5) {
                    // 格式化文件名
                    char filename[13] = {0};
                    int k = 0;
                    // 复制文件名（只复制非空格字符）
                    for (int m = 0; m < 8; m++) {
                        unsigned char ch = entries[j].name[m];
                        if (ch != ' ' && ch != 0) {
                            filename[k++] = ch;
                        }
                    }
                    // 检查是否有扩展名
                    int has_ext = 0;
                    for (int m = 0; m < 3; m++) {
                        unsigned char ch = entries[j].ext[m];
                        if (ch != ' ' && ch != 0) {
                            has_ext = 1;
                            break;
                        }
                    }
                    // 如果有扩展名且不是目录，添加扩展名
                    if (has_ext && !(entries[j].attr & ATTR_DIRECTORY)) {
                        filename[k++] = '.';
                        for (int m = 0; m < 3; m++) {
                            unsigned char ch = entries[j].ext[m];
                            if (ch != ' ' && ch != 0) {
                                filename[k++] = ch;
                            }
                        }
                    }
                    filename[k] = '\0';
                    char type[8] = "文件";
                    unsigned long dir_total = entries[j].file_size;
                    if (entries[j].attr & ATTR_DIRECTORY) {
                        strcpy(type, "目录");
                        dir_total = calculate_dir_total_size(fs, entries[j].first_cluster);
                    }
                    self_printf("%-15s %-10lu %-8s %-8d %lu\n", 
                           filename, 
                           entries[j].file_size,
                           type,
                           entries[j].first_cluster,
                           dir_total);
                    found = 1;
                    entry_count++;
                    total_size += entries[j].file_size;
                }
            }
        }
    } else {
        // 列出子目录
        int sector_num = fs->data_start + (list_cluster - 2) * SECTORS_PER_CLUSTER;
        read_sector(fs, sector_num, sector);
        DirEntry* entries = (DirEntry*)sector;
        // 跳过 . 和 .. 条目
        for (int j = 2; j < SECTOR_SIZE / 32; j++) {
            if (entries[j].name[0] != 0 && entries[j].name[0] != 0xE5) {
                // 格式化文件名
                char filename[13] = {0};
                int k = 0;
                // 复制文件名（只复制非空格字符）
                for (int m = 0; m < 8; m++) {
                    unsigned char ch = entries[j].name[m];
                    if (ch != ' ' && ch != 0) {
                        filename[k++] = ch;
                    }
                }
                // 检查是否有扩展名
                int has_ext = 0;
                for (int m = 0; m < 3; m++) {
                    unsigned char ch = entries[j].ext[m];
                    if (ch != ' ' && ch != 0) {
                        has_ext = 1;
                        break;
                    }
                }
                // 如果有扩展名且不是目录，添加扩展名
                if (has_ext && !(entries[j].attr & ATTR_DIRECTORY)) {
                    filename[k++] = '.';
                    for (int m = 0; m < 3; m++) {
                        unsigned char ch = entries[j].ext[m];
                        if (ch != ' ' && ch != 0) {
                            filename[k++] = ch;
                        }
                    }
                }
                filename[k] = '\0';
                char type[8] = "文件";
                unsigned long dir_total = entries[j].file_size;
                if (entries[j].attr & ATTR_DIRECTORY) {
                    strcpy(type, "目录");
                    dir_total = calculate_dir_total_size(fs, entries[j].first_cluster);
                }
                self_printf("%-15s %-10lu %-8s %-8d %lu\n", 
                       filename, 
                       entries[j].file_size,
                       type,
                       entries[j].first_cluster,
                       dir_total);
                found = 1;
                entry_count++;
                total_size += entries[j].file_size;
            }
        }
    }
    if (!found) {
        self_printf("(空目录)\n");
    } else {
        self_printf("\n总计: %d 个文件/目录, 本层总大小: %lu 字节\n", entry_count, total_size);
    }
    return 0;
}
// 树形显示目录结构
int myfs_tree(MyFS* fs, const char* path, int level) {
    unsigned char sector[SECTOR_SIZE];
    int tree_cluster;
    if (path == NULL || strlen(path) == 0) {
        tree_cluster = fs->current_cluster;
    } else {
        tree_cluster = find_dir_cluster(fs, path);
        if (tree_cluster < 0) return -1;
    }
    if (tree_cluster == 0) {
        // 根目录
        for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
            read_sector(fs, fs->root_start + i, sector);
            DirEntry* entries = (DirEntry*)sector;
            for (int j = 0; j < SECTOR_SIZE / 32; j++) {
                if (entries[j].name[0] != 0 && entries[j].name[0] != 0xE5) {
                    // 格式化文件名
                    char filename[13] = {0};
                    int k = 0;
                    for (int m = 0; m < 8; m++) {
                        unsigned char ch = entries[j].name[m];
                        if (ch != ' ' && ch != 0) {
                            filename[k++] = ch;
                        }
                    }
                    int has_ext = 0;
                    for (int m = 0; m < 3; m++) {
                        unsigned char ch = entries[j].ext[m];
                        if (ch != ' ' && ch != 0) {
                            has_ext = 1;
                            break;
                        }
                    }
                    if (has_ext && !(entries[j].attr & ATTR_DIRECTORY)) {
                        filename[k++] = '.';
                        for (int m = 0; m < 3; m++) {
                            unsigned char ch = entries[j].ext[m];
                            if (ch != ' ' && ch != 0) {
                                filename[k++] = ch;
                            }
                        }
                    }
                    filename[k] = '\0';
                    for (int s = 0; s < level; s++) {
                        self_printf("  ");
                    }
                    if (entries[j].attr & ATTR_DIRECTORY) {
                        self_printf("+-- [%s] (目录)\n", filename);
                        myfs_tree(fs, filename, level + 1);
                    } else {
                        self_printf("+-- %s (%lu 字节)\n", filename, entries[j].file_size);
                    }
                }
            }
        }
    } else {
        // 子目录
        int sector_num = fs->data_start + (tree_cluster - 2) * SECTORS_PER_CLUSTER;
        read_sector(fs, sector_num, sector);
        DirEntry* entries = (DirEntry*)sector;
        for (int j = 2; j < SECTOR_SIZE / 32; j++) {  // 跳过 . 和 ..
            if (entries[j].name[0] != 0 && entries[j].name[0] != 0xE5) {
                // 格式化文件名
                char filename[13] = {0};
                int k = 0;
                for (int m = 0; m < 8; m++) {
                    unsigned char ch = entries[j].name[m];
                    if (ch != ' ' && ch != 0) {
                        filename[k++] = ch;
                    }
                }
                int has_ext = 0;
                for (int m = 0; m < 3; m++) {
                    unsigned char ch = entries[j].ext[m];
                    if (ch != ' ' && ch != 0) {
                        has_ext = 1;
                        break;
                    }
                }
                if (has_ext && !(entries[j].attr & ATTR_DIRECTORY)) {
                    filename[k++] = '.';
                    for (int m = 0; m < 3; m++) {
                        unsigned char ch = entries[j].ext[m];
                        if (ch != ' ' && ch != 0) {
                            filename[k++] = ch;
                        }
                    }
                }
                filename[k] = '\0';
                for (int s = 0; s < level; s++) {
                    self_printf("  ");
                }
                if (entries[j].attr & ATTR_DIRECTORY) {
                    self_printf("+-- [%s] (目录)\n", filename);
                    myfs_tree(fs, filename, level + 1);
                } else {
                    self_printf("+-- %s (%lu 字节)\n", filename, entries[j].file_size);
                }
            }
        }
    }
    return 0;
}
// 显示文件系统信息
int myfs_info(MyFS* fs) {
    int total_clusters = (fs->total_sectors - fs->data_start) / SECTORS_PER_CLUSTER;
    int free_clusters = 0;
    for (int i = 2; i < total_clusters; i++) {
        if (fs->fat[i] == 0) free_clusters++;
    }
    self_printf("\n文件系统信息:\n");
    self_printf("  总扇区数: %u\n", fs->total_sectors);
    self_printf("  总簇数: %d\n", total_clusters);
    self_printf("  空闲簇数: %d\n", free_clusters);
    self_printf("  已用簇数: %d\n", total_clusters - free_clusters);
    self_printf("  总空间: %lu MB\n", (fs->total_sectors * SECTOR_SIZE) / (1024 * 1024));
    self_printf("  可用空间: %lu MB\n", (free_clusters * SECTOR_SIZE) / (1024 * 1024));
    self_printf("  当前目录: %s\n", fs->current_path);
    return 0;
}
// 读取文件（简单实现）
int myfs_read_file(MyFS* fs, const char* path) {
    self_printf("读取文件功能暂未实现\n");
    return -1;
}
