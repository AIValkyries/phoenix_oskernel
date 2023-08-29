#include <kernel/wait.h>
#include <kernel/sched_os.h>

struct wait_queue w_queues[NR_TASKS];

struct wait_queue *get_free_wait_queue()
{
    int i;

    for(i = 0;i < NR_TASKS; i++)
    {
        if(w_queues[i].count == 0)
        {
            w_queues[i].count = 1;
            return &w_queues[i];
        }
    }

    return NULL;
}

struct wait_queue *init_waitqueue(struct task_struct *task)
{
    struct wait_queue *wait = get_free_wait_queue();
    wait->task = task;
    current->state = TASK_INTERRUPTIBLE;
    
    return wait;
}

// 等待队列的函数
void add_wait_queue(struct wait_queue_head *head, struct wait_queue *wait)
{
    cli();      // 禁止中断

    if(head->waits == NULL)
    {
        head->waits = wait;
        head->waits->next = wait;
    }
    else
    {
        wait->next = head->waits->next;
        head->waits->next = wait;
    }

    sti();      // 开中断
}

void remove_wait_queue(struct wait_queue_head *head,struct wait_queue *wait)
{
    cli();      // 禁止中断

    if(head->waits == wait)
    {
        head->waits = head->waits->next;
    }
    else
    {
        struct wait_queue *q = head->waits;
        struct wait_queue *prev;

        while (!q)
        {
            if(q == wait)
            {
                prev = q->next;
                prev->count=0;
                break;
            }
            prev = q;
            q = q->next;
        }
    }
    
    sti();      // 开中断
}
