#ifndef QUEUE_H
#define QUEUE_H
#include <stdio.h>
#include "process_config.h"
/* 前向声明 */
struct PCB;
/* 队列结构 */
typedef struct Queue
{
	struct PCB* front;
	struct PCB* rear;
	int size;
} Queue;
/* 队列操作 */
void queue_init(Queue* q);
void enqueue(Queue* q, struct PCB* proc);
struct PCB* dequeue(Queue* q);
int queue_is_empty(Queue* q);
void queue_remove(Queue* q, struct PCB* proc);
#endif
