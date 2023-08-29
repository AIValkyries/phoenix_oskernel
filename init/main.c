// �? init 中调用系统调�? setup，用来收集硬盘设备分区表信息并安装根文件系统
#include <fs/fs.h>
#include <mm/mm.h>
#include <kernel/kernel.h>
#include <kernel/unistd.h>
#include <keyboard.h>
#include <kernel/io.h>
#include <kernel/stdarg.h>


static inline syscall0(int, pause);
static inline syscall1(int, fork, int, prio);
static inline syscall0(int, mount_root);
static inline syscall3(int, open,const char*,filename,int,mode,int,flags);
static inline syscall3(int, read,long,fd, char*,buf, int,count);
static inline syscall1(int, close,int,fd);
static inline syscall1(int, execve,const char*,filename);


extern void reset_pg_dir();

extern unsigned long paging_init(unsigned long start_mem, unsigned long end_mem);
// 内存
extern void mem_init(unsigned long start_mem, unsigned long end_mem);
// extern void kmem_cache_init();      // 缓存初始?
extern void buffer_init(unsigned long buffer_memory_end);

// 中断 | 异常 | 系统调用
extern void exception_init();
extern void interupt_init();
extern void system_call_init();

// 设备接口
extern void con_init();
extern void tty_init();
extern void hd_init();
extern int init(void);
extern void fs_init();

// ---------- test ----------------//
extern void timer_interrupt();
extern void do_keyboard_fun();
extern void hd_interrupt();


#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)
/*
    0x70 and 0x74 = 索引端口
    0x71 and 0x75 = 数据端口
    端口0x70的最高bit，是控制 NMI 中断的开关，当他为0时允许 NMI 中断达到处理器，否则不允许
*/
#define CMOS_READ(addr)({   \
    outb(0x80|addr,0x70);    \
    inb(0x71);  \
})

/*
    0x00    sec             0x07    日
    0x01    闹钟秒           0x08    月
    0x02    分              0x09    年
    0x03    闹钟分           0x0A    寄存器A 
    0x04    时              0x0B    寄存器B
    0x05    闹钟时           0x0C    C
    0x06    星期             0x0D   D
*/
void timer_init()
{
    struct tm time;
    time.tm_sec = CMOS_READ(0);
    time.tm_min = CMOS_READ(2);
    time.tm_hour = CMOS_READ(4);
    time.tm_mday = CMOS_READ(7);
    time.tm_mon = CMOS_READ(8);
    time.tm_year = CMOS_READ(9);

    printk("sec=%x,min=%x,hour=%x,mday=%x,mon=%x,year=%x \\n",
        time.tm_sec,time.tm_min,time.tm_hour,time.tm_mday,time.tm_mon,time.tm_year);
}


extern int printf(char *fmt, ...);
extern int printbuffer(char *buf, int count);

static char buffer[2048];

// 在分页模式下运行
void main(void)
{
    reset_pg_dir();

    con_init();         // 键盘和显示器 init
    printk(" ------------------- entry kernel ------------------- \\n");
    
    // 10630 栈
    // 8MiB~4个4kib存放内核页表
    unsigned long start_mem = 0x800000+4096*4;

    // 中断 | 异常 | 陷阱
    exception_init();
    interupt_init();
    system_call_init();
    
    open_page_tips();   

    mem_init(start_mem, PAGING_MEMORY);     // 内存初始�?
    buffer_init(0x7fffff);

    sched_init();

    tty_init();
    // 驱动设备
    hd_init();
    fs_init();

    timer_init();
    do_mount_root();

    //sti();  // IF 标志
    // 进入task0的用户模式
    move_to_user_mode();
    printf("----------------task0 user mode ---------------- ");
    
    unsigned long pid;
    fork(120);

    const char *pathname1 = "/usr/OS_test_task2";
    execve(pathname1);
    //pause();
        
    printf("---------------- task1 user mode ---------------- ");

    task0_init();
}


// 该函数运行在 任务0第一次创建? 任务1
int task0_init()
{
    fork(110);
    //pause();
    
    const char *pathname = "/usr/OS_test_task";
    execve(pathname);
    
    int a = 10;
    int b = 20;
    int c;

    while (1)
    {
        /* code */
        c = a+b;
    }
    
    int d = c;
}

