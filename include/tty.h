#ifndef _TTY_H
#define _TTY_H

/*
    通常使用 tty 来简称各种类型的终端设备，
    tty 是 Teletype 的缩写
*/
#define MAX_CONSOLES 4
#define TTY_BUF_SIZE  1024
#define NR_TTY       4

extern int fg_console;
    

// 字符的缓冲队列
struct tty_queue
{
    unsigned long head_ptr;      // 写入字符，head指针右移
    unsigned long tail_ptr;      // 读取字符，右移指针

    char buf[TTY_BUF_SIZE];
};

// 每个终端设备都对应一个 tty_struct 结构
struct tty_struct
{
    unsigned long pid;                  // 所属进程
    struct tty_queue queue;            // 从键盘或串行终端的数据
};

// 每一个终端设备都对应一个 tty_struct 数据结构,这里暂时只有控制台
extern struct tty_struct con_table[NR_TTY];         // 终端设备结构

#define current_queue con_table[fg_console].queue


extern void put_queue(char c);
extern void read_char(char *c);

#endif