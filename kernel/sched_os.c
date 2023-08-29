#include <kernel/sched_os.h>
#include <kernel/atomic.h>
#include <kernel/kernel.h>
#include <kernel/io.h>
#include <kernel/unistd.h>
#include <kernel/string.h>


int need_resched = 0;
unsigned long jiffies = 0;

// 运行队列
struct runqueue arrays[2];       // 活跃的队列，时间片用完的队列
struct runqueue *active = &arrays[0];
struct runqueue *expired = &arrays[1];

/*
    4kib
*/
unsigned long init_kernel_stack[1024];    // init 进程的 内核态栈
unsigned long init_user_stack[1024];
struct timer_list timer_head;
struct timer_list timer_lists[TIME_REQUESTS];


struct vm_area_struct init_mmap = 
{
    &init_task, 0, 0x40000000, VM_READ | VM_WRITE | VM_EXEC     
};

struct mm_struct init_mm = 
{
    .start_code = 0,.end_code = 0,.end_data = 0,.start_data = 0,
    .start_brk = 0,.brk = 0,.start_stack = 0,.start_mmap = 0,
    .arg_start = 0,.arg_end = 0,.env_start = 0,.env_end = 0,
    .size = 0, .pgd_base = 0,.mmap = &init_mmap,.share_mmap=NULL,
    .map_count = 0
};

struct task_struct init_task = 
{
    .state = TASK_RUNNING,
    .pid = 0,
    .umask = 0,.euid=0,.egid=0,.tty = -1,
    .time_slice = 5,.start_time = 0,.stime = 0,.utime = 0,.policy = SCHED_RR,.prio = 15,
    .next = 0,.prev=0,
    .root = 0,.pwd = 0,
    .filps = {(void*)0,},
    .mm = &init_mm,
    .tss = 
    {
        .back_link = 0,
        .esp0 = sizeof(init_kernel_stack) + (long)(&init_kernel_stack),
        .ss0 = KERNEL_DS,
        .esp1 = 0 ,.ss1=KERNEL_DS,.esp2=0,.ss2=KERNEL_DS,
        .cr3=0,.eip=0,.eflags=0,.eax=0,.ecx=0,.edx=0,.ebx=0,
        
        .esp= sizeof(init_user_stack) + (long)(&init_user_stack),
        .ebp=0,.esi=0,.edi=0,
        .es = USER_DS,.cs = USER_DS,.ss = USER_DS,.ds=USER_DS,.fs=USER_DS,.gs = USER_DS,
        .ldt=__LDT(0),.trace_bitmap = 0x80000000,.tr = __TSS(0),
        .kernel_stack_page = sizeof(init_kernel_stack) + (long)(&init_kernel_stack)
    },
    .ldt_table = 
    {
        {0, 0},
        {0x4000, 0xc0fa00},   // 代码长 64mib, base=0x0, G=1, D=1, DPL=3, P=1, TYPE=0xa
        {0x4000, 0xc0f200}    // 数据长 64mib, base=0x0, G=1. D=1, DPL=3, P=1, TYPE=0x2
    }
};

struct task_struct *current  = &init_task;
struct task_struct *task_table[NR_TASKS] = {&init_task};
struct timer_list   timer_head = {NULL,0,0,NULL};


// 计算时间片
static int task_timeslice(struct task_struct *task)
{
    if(task->prio < 120)
        return (MAX_PRIO - task->prio) * 20;
   return (MAX_PRIO-task->prio) * 5;
}


void set_links(struct task_struct *p)
{
    if(init_task.next == NULL)
    {
        init_task.next = p;
        init_task.prev = p;
        p->prev = &init_task;
        return;
    }

    init_task.prev->next = p;
    p->prev = init_task.prev;
    init_task.prev = p;
}

void remove_links(struct task_struct *p)
{
    if(p->next == NULL)  // 队列尾
    {
        p->prev->next = NULL;
        init_task.prev = p->prev;
        return;
    }

    p->next->prev = p->prev;
    p->prev->next = p->next;
}


// 入运行队列
static void enqueue_task(struct task_struct *p, struct runqueue *rq)
{
    struct run_task_head *task_head;
    struct task_struct *last_task;

    task_head = &rq->task_list[p->prio];

    p->state = TASK_RUNNING;
    task_head->task_count++;

    if(task_head->task == NULL)
    {
        task_head->task = p;
        task_head->task->prev = p;
        return;
    }

    // 添加至队列尾部
    last_task = task_head->task->prev;
    last_task->next = p;
    task_head->task->prev = p;
}

// 从运行队列中移除
extern void dequeue_task(struct task_struct* p,  struct runqueue *rq)
{
    struct run_task_head *task_head;

    task_head = &rq->task_list[p->prio];

    task_head->task_count--;
    if(task_head->task_count <= 0)
        rq->bitmap[p->prio] = 0;

    task_head->task = task_head->task->next;    
}

// 激活队列
static void activate_task(struct task_struct *p)
{
    enqueue_task(p, active);
    active->nr_running++;           // 可执行进程数量
    active->bitmap[p->prio] = 1;        // 该优先级队列不为NULL
}

// 过期队列
static void expired_task(struct task_struct *p)
{
    enqueue_task(p, expired);
    expired->nr_running++;           // 可执行进程数量
    expired->bitmap[p->prio] = 1;        // 该优先级队列不为NULL
}

// 从 激活队列中移除当前进程
static void deactivate_task(struct task_struct *p)
{
    dequeue_task(p, active);
    active->nr_running--;
}

// 从过期队列中移除当前进程
static void deexpired_task(struct task_struct *p)
{
    dequeue_task(p, expired);
    expired->nr_running--;
}

// 睡眠
void sleep_on(struct wait_queue_head *head)
{
    struct wait_queue *wait = init_waitqueue(current);
    add_wait_queue(head, wait);
    deactivate_task(current);
    schedule();
    remove_wait_queue(head, wait);
}

// 唤醒
void wake_up(struct wait_queue_head *head)
{
    struct wait_queue *temp;
    temp = head->waits;

    while (temp)
    {
        temp->task->state = TASK_RUNNING;
        activate_task(temp->task);
        temp = temp->next;

        if(temp == head->waits)
            break;
    }
}

// 等待信号
void sem_down(struct semaphore *sem)
{
    struct wait_queue *wait = init_waitqueue(current);
    add_wait_queue(sem->head, wait);
    deactivate_task(current);
    schedule();
    remove_wait_queue(sem->head, wait);
}

// 唤醒某个信号量的睡眠进程
void sem_up(struct semaphore* sem)
{
    atomic_inc(&sem->count);
    wake_up(sem->head);
}

void add_timer(void (*fn)(void), unsigned long jiffies)
{
    struct timer_list *p = &timer_lists[0];

    for(; p < timer_lists + TIME_REQUESTS; p++)
    {
        if(p->jiffies)
            continue;
        p->fn       = fn;
        p->jiffies  = jiffies;

        p->next = timer_head.next;
        timer_head.next = p;

        break;
    }
}

/*
    100HZ 的频率,  时钟中断
    非抢占式调度
*/
void scheduler_tick(void)
{
    struct timer_list *p = &timer_head;
    struct timer_list *prev;

    printk("scheduler_tick \\n");

    // 定时器执行
    while (p)
    {
        void (*fn)(void);

        p->jiffies--;   // hz
        if(p->jiffies <= 0 && p->fn != NULL)
        {
            fn = p->fn;
            p->fn = NULL;
            prev->next = p->next;
            (fn)();
        }
        
        prev = p;
        p = p->next;
    }

    if(current->prio < 100)  // 实时调度进程
    {
        // 时间片用完了，并且是 SCHED_RR 类型进程
        if(current->policy == SCHED_RR && (--current->time_slice)<=0)
        {
            current->time_slice = task_timeslice(current);
            need_resched = 1;

            deactivate_task(current);           // 从激活队列中移除
            expired_task(current);              // 过期队列
        }

       return;
    }

    if(!(--current->time_slice))   // 普通进程的时间片用完了
    {
        current->time_slice = task_timeslice(current);
        need_resched = 1;

        // 更新动态优先级
        // 交互式进程 or 批处理式进程？
        // 平均睡眠时间
        deactivate_task(current);
        expired_task(current);
    }

}

/*
    调度程序，选择哪个进程进行调度？
*/
void schedule(void)
{
    // 计算机运行时间，记录进程开始运行的时间点
    // 如果该运行队列中没有可运行进程，则从其他运行队列中移过来。SMP模式
    // 检测运行队列中的进程？
    struct runqueue *rq;
    struct run_task_head *task_head = NULL;
    struct task_struct *next;
    unsigned long pid;
    int i;

    rq = active;

    if(!rq->nr_running)      // 活动队列与非活动队列进行更换
    {
        rq = expired;
        expired = active;
        active = rq;
    }
    
    for(i = 0; i < MAX_PRIO; i++)
    {
        if(rq->bitmap[i])
        {
            task_head = &rq->task_list[i];
            break;
        }
    }

    next = task_head->task;
    pid = next->pid;

    printk("next =%x next.tr=%x,next.eip=%x,current.tss=%x \\n", 
        next, next->tss.tr,next->tss.eip, &current->tss);

    if(next == NULL)
        next = &init_task;

    if(next == current)
        return;
    next->prev = current;
    need_resched = 0;
    // ...1f011bc
    switch_to(next);
}

// 更新优先级
int do_nice(int increment)
{
    int newprio;

    newprio = current->prio - increment;

    if(newprio < 1)
        newprio = 1;
    if(newprio > 139)
        newprio = 139;

    return newprio;
}

void set_nice(struct task_struct *p, int prio)
{
    p->prio = prio;
    if(prio < 100)
        p->policy = SCHED_RR;
    else
        p->policy = SCHED_NORMAL;
}

// 暂停进程
void do_pause()
{
    struct task_struct *p = (struct task_struct*)(0x806020);
    // 0x806020
    printk("pause eip =%x \\n",p->tss.eip);

    current->state = TASK_INTERRUPTIBLE;     // 
    deactivate_task(current);                // 进行调度
    need_resched = 1;
}

// 任务进入 and exit
void sched_fork(struct task_struct *p)
{
    p->state      = TASK_RUNNING;
    need_resched = 1;
    activate_task(p);
}


void sched_exit(struct task_struct *p)
{
    p->state = TASK_STOPPED;
    deactivate_task(p);
    //expired_task(p);              // 过期队列
    need_resched = 1;
}

#define CMOS_READ(addr,value)({   \
    outb(0x80|addr,0x70);    \
    outb(value,0x71);       \
})

// PC8253 定时芯片的输入时钟频率约为 1.193180Hz
// 也即每10ms发出一次时钟中断。
#define LATCH (1193180/HZ)

/*
    [SC1][SC0][RL1][RL0][M2][M1][M0][BCD]
    BCD = 计数制选择 1-BCD,0-二进制
    [M0-M2]=工作方式
    {
        000=计数结束正跳变
        001=单脉冲发生器
        010=周期性负脉冲发生器
        x11=方波信号发生器
        100=4
        101=5
    }
    [RL1-RL0]
    {
        00 将计数器中的数据锁存于缓冲器
        01-只读/写计数器低8位
        10-只读/写计数器高8位
        11-先读/写计数器低8位，再读/写计数器高8位
    }
    [SC1-SC0]
    {
        00-计数器0,01-计数器1,10-计数器2,11-非法
    }
    端口 0x40~0x43
*/
void sched_init(void)
{
    // 进程0的 tss 段 和 ldt 段
    set_tss_desc(gdt+FIRST_TSS_ENTRY, &init_task.tss);
    set_ldt_desc(gdt+FIRST_LDT_ENTRY, &init_task.ldt_table);

    ltr(0);
    lldt(0);

    // 36=11 011 0 0x43=1000011
    outb(0x36,0x43);		/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */
	outb(LATCH >> 8 , 0x40);	/* MSB */

    // IRQ0  从 0x21 中读取 ICW1,设置 IRQ0 为可中断
    outb(inb_p(0x21)&~0x01,0x21);

    memset(arrays,'0',sizeof(struct runqueue) * 2);
    active  = &arrays[0];
    expired = &arrays[1];

    //activate_task(&init_task);
}
