﻿// myfs.h - 自定义文件系统头文件
#ifndef MYFS_H
#define MYFS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define SECTOR_SIZE     512     // 扇区大小
#define SECTORS_PER_CLUSTER 1   // 每簇1扇区
#define RESERVED_SECTORS 1      // 保留扇区数（引导扇区）
#define FAT_SECTORS      256    // FAT表占用的扇区数
#define ROOT_DIR_SECTORS 32     // 根目录占用的扇区数
#define ROOT_DIR_ENTRIES (ROOT_DIR_SECTORS * SECTOR_SIZE / 32)  // 根目录条目数
// 文件属性
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
// 目录项特殊标记
#define DIR_ENTRY_FREE  0x00  // 空闲目录项
#define DIR_ENTRY_DEL   0xE5  // 已删除目录项
#define DIR_ENTRY_DOT   0x2E  // '.' 目录项
// 当前目录和父目录的特殊名字
#define DIR_CURRENT      "."   // 当前目录
#define DIR_PARENT       ".."  // 父目录
// 目录项结构（32字节）
typedef struct {
    char name[8];           // 文件名
    char ext[3];            // 扩展名
    unsigned char attr;     // 文件属性
    unsigned char reserved; // 保留
    unsigned char create_time_ms; // 创建时间（毫秒）
    unsigned short create_time;   // 创建时间
    unsigned short create_date;   // 创建日期
    unsigned short access_date;   // 访问日期
    unsigned short first_cluster_high; // 起始簇号高16位（FAT12/16不用）
    unsigned short modify_time;   // 修改时间
    unsigned short modify_date;   // 修改日期
    unsigned short first_cluster; // 起始簇号
    unsigned long file_size;      // 文件大小
} DirEntry;
// 文件系统信息
typedef struct {
    FILE* disk;                 // 磁盘文件句柄
    unsigned short* fat;        // FAT表内存镜像
    unsigned int total_sectors; // 总扇区数
    unsigned int fat_start;     // FAT表起始扇区
    unsigned int root_start;    // 根目录起始扇区
    unsigned int data_start;    // 数据区起始扇区
    int current_cluster;        // 当前目录的簇号（0表示根目录）
    char current_path[512];     // 当前目录路径
} MyFS;
// API函数
int myfs_mount(MyFS* fs, const char* disk_path);
void myfs_umount(MyFS* fs);
int myfs_format(MyFS* fs);
int myfs_create_file(MyFS* fs, const char* path);
int myfs_delete_file(MyFS* fs, const char* path);
int myfs_create_dir(MyFS* fs, const char* path);
int myfs_delete_dir(MyFS* fs, const char* path);
int myfs_list_dir(MyFS* fs, const char* path);
int myfs_write_file(MyFS* fs, const char* path, const char* data);
int myfs_read_file(MyFS* fs, const char* path);
int myfs_change_dir(MyFS* fs, const char* path);
int myfs_pwd(MyFS* fs);
int myfs_tree(MyFS* fs, const char* path, int level);
int myfs_info(MyFS* fs);
#endif
