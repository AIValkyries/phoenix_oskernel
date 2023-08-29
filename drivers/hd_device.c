/*
    磁盘
*/
#include <kernel/io.h>
#include <fs/fs.h>
#include <hd.h>
#include <kernel/kernel.h>

struct hd_request request[NR_REQUEST];
struct hd_request *first_req = NULL;
static int reset = 0;

int hd_timeout;
void (*do_hd)(void);
// 磁盘信息
struct hd_struct hd_info = {0,0,0,0,0};


#define SET_INTR(x) (do_hd = x,hd_timeout = 200)


void show_hd_request(struct hd_request *req)
{
    printk("cmd: %d, errors: %d, nr_sectors: %d, sector: %d, lock: %d, buffer_addr: %p \\n",
            req->cmd, req->errors, req->nr_sectors, req->sector, req->lock, req->buffer);
}

// 读写硬盘失败处理调用函数
static void bad_rw_intr()    
{
    if(++first_req->errors > MAX_ERRORS)
        end_request(first_req);
    if(first_req->errors > MAX_ERRORS/2)
        reset = 1;   // 复位硬盘控制器操作
}


/*
    检测硬盘执行命令后的状态
    返回0，表示正常；返回1，表示出错。
*/
static int win_result()
{
    int state = inb(HD_STATUS);

    int flags = BUSY_STAT | WRERR_STAT | ERR_STAT | READY_STAT | SEEK_STAT;
    int ok_state = READY_STAT | SEEK_STAT;

    if((state & flags) == ok_state)
        return 0;  //表示 ok

    if(state & ERR_STAT)
        return inb(HD_ERROR);

    return 1;
}

static int drive_busy()
{
    int state = inb(HD_STATUS);
    if(state & BUSY_STAT)
        return 1;
    return 0;
}

// READY_STAT = 1,  BUSY_STAT = 0
// 硬盘控制器是否准备就绪
static int controller_ready()
{
   	int retries = 100000;

	while (--retries && (inb(HD_STATUS)&0xc0)!=READY_STAT);
	return (retries);
}


// 读操作中断调用函数
static void read_intr()
{
    if(win_result())   // 控制器忙碌，或者读写错误
    {
        bad_rw_intr();
        do_hd_request();
        return;
    }
    volatile long sector = first_req->sector;
    volatile char *tmpdata = first_req->buffer;

    //printk("sector=%d buffer=%x \\n", sector,first_req->buffer);
    
    // 读取命令
    read_disk(sector);

    while (!(inb_p(HD_STATUS) & DRQ_STAT))
    {
        
    }
    
    // 一次读 512 字节 至 请求结构缓冲区
    port_read(HD_DATA, tmpdata, 256);
    first_req->errors = 0;
    first_req->buffer += 512;
    first_req->sector++;

    first_req->nr_sectors--;

    if(first_req->nr_sectors > 0)
    {
        read_intr();
        return;
    }

    end_request(first_req);
    do_hd_request();          // 继续读取请求队列
}

// 写扇区中断调用函数,写入磁盘
static void write_intr()
{
    if(win_result())   // 控制器忙碌，或者读写错误
    {
        bad_rw_intr();
        do_hd_request();
        return;
    }

    volatile long sector = first_req->sector;
    volatile char *tmpdata = first_req->buffer;

    write_disk(sector);

    // 使用轮询等待结果
    while (!(inb_p(HD_STATUS) & DRQ_STAT))
    {
        
    }
    
    port_write(HD_DATA, tmpdata, 256);
    first_req->sector++;        // 起始扇区数量 add
    first_req->buffer += 512;
    first_req->errors = 0;
    first_req->nr_sectors--;

    if(first_req->nr_sectors > 0)
    {
        write_intr();
        return;
    }

    end_request(first_req);
    do_hd_request();          // 继续读取请求队列
}

/*
    硬盘控制器编程
*/
static void hd_out(
    unsigned short cyl,    // 柱面数
    unsigned short head,   // 磁道数
    unsigned short sec,    // 起始扇区
    unsigned short cmd,    // 硬盘控制命令
    unsigned short nsect)  // 读写扇区总数
{
    register int port asm("dx");

    printk("hd_out cyl: %x, head: %x, sec: %x, nsect: %x, cmd: %x \\n",
            cyl, head, sec, nsect, cmd);

    port = HD_DATA;                        // 置dx为数据寄存器端口
    ++port;
	outb(nsect, ++port);          // 读写扇区总数
	outb(sec, ++port);            // 起始扇区
	outb(cyl, ++port);            // 柱面号 低8位
	outb(cyl>>8, ++port);         // 柱面号 高8位
	outb(0xA0|(0)|head, ++port);  // 驱动器号+磁头号
	outb(cmd, ++port);            // 硬盘控制命令
}

// 硬盘超时
void hd_times_out(void)
{
    if(first_req == NULL) return;

    if (++first_req->errors >= MAX_ERRORS)
        end_request(first_req);

    do_hd = NULL;
    reset = 1;
    do_hd_request();
}

// 读写磁盘的接口
void do_hd_request()
{
    unsigned short cyl;
    unsigned short heads;
    unsigned short head;
    unsigned short sec;

    if(first_req == NULL) return;

    int rw;
    rw = first_req->cmd;
    
    if(rw == FILE_READ)
    {
        read_intr();            // 读取
    }
    else if(rw == FILE_WRITE)   // 写入
    {
        write_intr();
    }
}


void do_harddisk()
{
    printk("do_harddisk \\n");
    if(do_hd != NULL)
        do_hd();
}

void hd_init()
{
    hd_info.cyl     = 520;
    hd_info.head    = 16;       // 磁道数
    hd_info.sectors = 63;       // 每磁道扇区数

    // 扇区总大小
    hd_info.total_sectors = hd_info.cyl * hd_info.head  * hd_info.sectors;
    hd_info.cyl_sectors   = hd_info.head * hd_info.sectors;
    hd_info.max_size  = hd_info.total_sectors * 512;         // 总字节数
    hd_info.max_block = hd_info.max_size >> BLOCK_SIZE_BIT;  // 总块数
    
    outb(inb(0x21)&0xfb, 0x21);           // 复位接联的主 8259A int2的屏蔽位
	outb(inb(0xA1)&0xbf, 0xA1);           // 复位硬盘中断请求屏蔽位

    //do_mount_root();
}