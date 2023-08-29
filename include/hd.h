#ifndef _HD_H
#define _HD_H

#include <kernel/atomic.h>
#include <kernel/wait.h>

// 1.硬盘的硬件信息.如果获取信息
struct hd_struct
{
    unsigned long  max_size;       // 总大小，字节
    unsigned long  max_block;      // 总块数
    unsigned short cyl;            // 柱面数
    unsigned char  head;           // 磁道数
    unsigned char  sectors;        // 每磁道扇区数量
    unsigned short cyl_sectors;    // 每柱面扇区数量
    unsigned long  total_sectors;  // 扇区总数
};


// 请求队列
struct hd_request
{
    int cmd;
    int errors;      // 产生错误的次数
    int nr_sectors;  // 扇区数量
    int sector;      // [起始扇区号 = 全局性的]
    struct atomic lock;     // 是否被使用
    char *buffer;
    struct wait_queue_head waits;
    struct buffer_head *bh;
    struct hd_request  *next;
};


// 硬盘中断
extern void hd_interrupt(void);
extern void end_request(struct hd_request *req); 
extern void do_hd_request();


#define MAR 3       // 硬盘号
#define NR_REQUEST	32
#define MAX_ERRORS  7

extern struct hd_request request[];
extern struct hd_request *first_req;




// ----------------------------------- 端口 --------------------------------
#define HD_DATA 0x1f0
#define HD_ERROR 0x1f1
#define HD_NSECTOR 0x1f2    // 读写扇区的数量
#define HD_SECTOR 0x1f3     // 起始扇区号
#define HD_LCYL 0x1f4
#define HD_HCYL 0x1f5
#define HD_CURRENT 0x1f6
#define HD_STATUS 0x1f7       // 主状态寄存器
#define HD_COMMAND 0x1f7      // 命令寄存器


// ----------------------------------- 主状态寄存器 HD_STATUS --------------------------------
#define ERR_STAT 0x01
#define INDEX_STAT 0x02
#define ECC_STAT 0x04
#define DRQ_STAT 0x08      // 表示驱动器已经准备好和内核传输一个字或者一个字节的数据
#define SEEK_STAT 0x10     // 驱动器寻道结束
#define WRERR_STAT 0x20    // 写出错
#define READY_STAT 0x40    // 表示准备好了接收命令
#define BUSY_STAT 0x80     // 控制器忙碌，此时主机不能发送命令块



// ------------------------------- 硬盘控制器命令列表 HD_COMMAND -------------------------------

#define WIN_RESTORE 0x10  // 复位
#define WIN_READ    0x20     // 读扇区
#define WIN_WRITE   0x30    // 写扇区
#define WIN_VERIFY  0x40   // 扇区校验
#define WIN_FORMAT  0x50   // 格式化磁道
#define WIN_INIT    0x60    // 控制器初始化
#define WIN_SEEK    0x70    // 寻道
#define WIN_DIAGNOSE  0x90 // 控制器诊断
#define WIN_SPECIFY   0x91 // 建立驱动器参数



#endif