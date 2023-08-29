#ifndef _KERNEL_H
#define _KERNEL_H

#define NR_TASKS        64             // 任务最多数量
#define NULL 0


struct tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};


/* ---------------------------------------- 字符串定义 ---------------------------------------- */
extern char interrupt_tips[];
extern char mem_tips[];


/* ---------------------------------------- 驱动设备 ---------------------------------------- */

// 因为具有模块化性质，因此在此处声明
extern int hd_timeout;
extern void do_harddisk();


/* ---------------------------------------- 外部接口 ---------------------------------------- */
extern int printk(char * fmt, ...);
extern int panic(char *fmt, ...);
extern void console_print(char * b);


extern void show_task_tss(struct task_struct *p);
extern void sys_sti();                  // 关闭中断
extern void sys_cli();                  // 开启中断


// ---------------------------------------- 动态内存 ----------------------------------------











#endif