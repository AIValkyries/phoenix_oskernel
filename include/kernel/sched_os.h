#ifndef _SCHED_H
#define _SCHED_H

#include <mm/mm.h>
#include <kernel/atomic.h>
#include <fs/fs.h>
#include <kernel/system.h>
#include <kernel/flags_bit.h>
#include <tty.h>
#include <kernel/wait.h>
#include <kernel/kernel.h>

#define TASK_OFFSET     0x1000000
#define TASK_SIZE       0x4000000      // 32mib
#define LIBRARY_SIZE    0x00400000      // 动态加载库，4MiB

#define HZ 100
#define TIME_REQUESTS 32  // 最多32个定时器

#define SCHED_FIFO    1 
#define SCHED_RR      2   // 时间片轮转的实时进程
#define SCHED_NORMAL  3   // 普通时分进程

#define NOT_CLONES_VM 0x01

/*
    0-99 表示实时进程
    100-139 表示普通进程
*/
#define MAX_PRIO 140   // 优先级数量
#define MAX_RT_PRIO 100  // 

#define TASK_RUNNING        1   // 运行
#define TASK_INTERRUPTIBLE  2   // 阻塞
#define TASK_STOPPED        3   // 停止



// 28 + 2 = 30 个字段
struct thread_struct
{
    // 16 high bits zero
    long back_link;
    // 特权级栈
    long esp0;      
    long ss0;   // 0 特权级堆栈选择子
    long esp1;
    long ss1;
    long esp2;
    long ss2;
    long cr3;          // 页目录基地址

    long eip;          // 程序指针
    long eflags;
    long eax;
    long ecx;
    long edx;
    long ebx;
    long esp;          // 栈指针
    long ebp;
    long esi;
    long edi;

    // 段选择子
    long es;
    long cs;
    long ss;
    long ds;
    long fs;
    long gs;
    long ldt;

    long trace_bitmap;
    long tr;                   // tss 段选择符？

    // 任务内核态栈
    unsigned long kernel_stack_page;    // 内核态栈
    unsigned long user_stack_page;      // 用户态的栈
    long cache_eip;
};

/*
    elf 程序头 LOAD 标志的段会被映射至内存
*/
struct mm_struct
{
    unsigned long start_code, end_code, start_data, end_data;
    unsigned long start_brk,  brk;      // 堆区 brk = 长度

    unsigned long start_stack;          // 栈区，8 kib 的大小
    unsigned long start_mmap;           // 文件映射区

    unsigned long arg_start,  arg_end,  env_start,  env_end;

    unsigned long  size;             // 线性内存总大小
    unsigned long  pgd_base;

    struct vm_area_struct *mmap;
    struct vm_area_struct *share_mmap;  // 共享链表，都是文件映射

    unsigned short map_count;        // 线性区数目
};

/*
    状态模型
    身份标识
    地址空间
    文件信息
    信号信息
    运行时间片

    484 字节
*/
struct task_struct
{
    /*
        运行
        阻塞
        停止
    */
    long state;
    unsigned long pid;  // 进程的标识符

    unsigned short umask;
    unsigned short euid;
    unsigned short egid;
    int tty;

    /* -------------------- 进程调度 -------------------- */
    int time_slice;   // 时间片 = 嘀嗒为单位
    unsigned long start_time;   // 进程开始运行时刻
    unsigned long stime;        // 内核运行了多少时间
    unsigned long utime;        // 用户运行了多少时间
    unsigned long policy;       // 进程的类型 [SCHED_FIFO]--[SCHED_RR]--[SCHED_NORMAL]
    unsigned char prio;         // 优先级

    /* -------------------- 进程之间的关系 -------------------- */
    struct task_struct *next;
    struct task_struct *prev;
    
    /* ---------------------------- 资源信息 ------------------------------ */
    // 关于文件
    struct m_inode* root;        // 系统根节点
    struct m_inode* pwd;         // 进程的工作目录

    // 指针数组
    struct file *filps[64];

    // 虚拟内存
    struct mm_struct    *mm;                

    // 线程信息
    struct thread_struct tss;
    struct desc_struct   ldt_table[3];
};


struct run_task_head
{
    unsigned long task_count;
    struct task_struct *task;
};

// 运行队列
struct runqueue
{
    unsigned long nr_running;
    // 优先级队列
    struct run_task_head task_list[MAX_PRIO];
    unsigned char bitmap[MAX_PRIO];
};


/* -----------------------------  定时器  ------------------------------ */

struct timer_list
{
    struct timer_list* next;

    unsigned long jiffies;
    unsigned long data;

    void (*fn)(void);
};


extern void add_timer(void (*fn)(void), unsigned long jiffies);

/* -----------------------------  定时器 end  ------------------------------ */

// 调度,100HZ
extern unsigned long jiffies;
extern unsigned long startup_time;      // 开始时间,从 1970:0:0 开始
extern int need_resched;

// 当前时间
#define CURRENT_TIME    ((startup_time+jiffies)/HZ)
#define FIRST_TASK task_table[0]            // 第一项任务，0任务比较特殊
#define LAST_TASK task_table[NR_TASKS-1]    // 最后一项任务

extern struct vm_area_struct init_mmap;  // 同时也是共享线性区的链表
extern struct task_struct    init_task;
extern struct task_struct    *current;

extern struct task_struct *task_table[];


extern struct runqueue *active;
extern struct runqueue *expired;


// ---------------------------------------- 队列相关 ----------------------------------------
// 将 current 进程加入到 等待队列中
extern void sleep_on(struct wait_queue_head *head);
// 唤醒加入运行队列
extern void wake_up(struct wait_queue_head *head);  

extern void sem_down(struct semaphore* sem);  // 等待信号
extern void sem_up(struct semaphore* sem);    // 唤醒信号 

// ---------------------------------------- 进程调度 ----------------------------------------
extern void schedule(void);
extern void sched_init(void);
extern void scheduler_tick();    // 时钟中断 
extern void sched_fork(struct task_struct *p);

// ---------------------------------------- 进程周期 ----------------------------------------
extern int  do_fork(
    long ebx,long ecx,long edx,
    long fs,long es,long ds, 
    long eip, long cs, long eflags, long esp,long ss);

extern int  do_exit();
extern int  do_execve(unsigned long *eip, const char *filename);
extern int  do_uselib(const char *library);
extern void sched_exit(struct task_struct *p);


// 进程 copy
extern void copy_thread(
    int nr, struct task_struct* p, 
    long ebx,long ecx,long edx,
    long fs,long es,long ds, 
    long eip, long cs, long eflags, long esp,long ss
);
extern void start_thread(struct task_struct *p, unsigned long eip, unsigned long esp);


// 增加至 进程的队列尾部
extern void set_links(struct task_struct *p);
extern void remove_links(struct task_struct *p);




#endif