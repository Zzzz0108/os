﻿#ifndef OS_SYNC_H
#define OS_SYNC_H
/* * 平台无关的互斥锁句柄。
 * 使用 void* 可以隐藏底层具体实现（如 Windows 的 CRITICAL_SECTION 或 HANDLE）。
 */
typedef void* os_mutex_t;
/* 创建并初始化一个互斥锁 */
os_mutex_t os_mutex_create(void);
/* 获取锁（阻塞等待） */
void os_mutex_lock(os_mutex_t mutex);
/* 释放锁 */
void os_mutex_unlock(os_mutex_t mutex);
/* 销毁互斥锁，释放系统资源 */
void os_mutex_destroy(os_mutex_t mutex);
#endif // OS_SYNC_H
