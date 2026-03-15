#include "queue.h"
#include "process.h"

/* 初始化队列 */

void queue_init(Queue* q)
{
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
}

/* 判断队列是否为空 */

int queue_is_empty(Queue* q)
{
    return (q->size == 0);
}

/* 入队 */

void enqueue(Queue* q, PCB* proc)
{
    proc->next = NULL;

    
        if (q->rear == NULL)
        {
            q->front = proc;
            q->rear = proc;
        }
        else
        {
            q->rear->next = proc;
            q->rear = proc;
        }

    q->size++;
    

}

/* 出队 */

PCB* dequeue(Queue* q)
{
    if (queue_is_empty(q))
        return NULL;

    
        PCB* proc = q->front;

    q->front = proc->next;

    if (q->front == NULL)
        q->rear = NULL;

    proc->next = NULL;

    q->size--;

    return proc;
    

}

/* 从队列中移除指定进程 */

void queue_remove(Queue* q, PCB* proc)
{
    if (queue_is_empty(q) || proc == NULL)
        return;

    
        PCB* prev = NULL;
    PCB* curr = q->front;

    while (curr != NULL)
    {
        if (curr == proc)
        {
            if (prev == NULL)
            {
                /* 删除头节点 */

                q->front = curr->next;

                if (q->rear == curr)
                    q->rear = NULL;
            }
            else
            {
                prev->next = curr->next;

                if (q->rear == curr)
                    q->rear = prev;
            }

            curr->next = NULL;

            q->size--;

            return;
        }

        prev = curr;
        curr = curr->next;
    }
    

}
