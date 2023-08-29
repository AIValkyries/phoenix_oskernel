#ifndef _WAIT_H
#define _WAIT_H

#include <kernel/atomic.h>
#include <kernel/kernel.h>

// 信号量
struct semaphore
{
    struct atomic count;
    struct wait_queue_head *head;
};

// 等待队列
struct wait_queue
{
    struct task_struct *task;
    // 等待队列的双链表
    struct wait_queue *next;
    unsigned char count;
};

// 等待队列头
struct wait_queue_head
{
    struct atomic lock;
    struct wait_queue *waits;
};

// 所有等待队列列表
extern struct wait_queue w_queues[NR_TASKS];

extern struct wait_queue *get_free_wait_queue();
extern struct wait_queue* init_waitqueue(struct task_struct *task);
// 向等待队列头添加等待队列
extern void add_wait_queue(struct wait_queue_head *head, struct wait_queue *wait);
extern void remove_wait_queue(struct wait_queue_head *head,struct wait_queue *wait);


#endif