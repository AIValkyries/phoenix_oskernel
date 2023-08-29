#ifndef _FS_H
#define _FS_H

#include <mm/mm.h>
#include <kernel/atomic.h>
#include <kernel/wait.h>
#include <kernel/sched_os.h>
#include <fs/fs_stat.h>

// #define NULL ((void *) 0)
#define NR_FS_START 128         // 文件系统的偏移 block
#define ROOT_INO 1              // 根 i 节点
#define NR_INODES 65536         // i 节点总数
#define NR_FILE 64              // 系统中可以存在最大的 file 数量
//#define INODE_START_BLOCK 6     // i 节点起始块
#define MAX_NAME_LEN  14        // 名字的最大char长度
#define CHAR_LEN 8192  


/*----------------- 高速缓存(磁盘中的结构) buffer 中的结构 --------------------*/

#define BLOCK_SIZE 1024       // 每块 1kib 的 大小
#define BLOCK_SIZE_BIT 10     // 每块 = 多少 bit
#define INODE_BLOCK_AREA_LEN 9   // 所有的i节点数需要 512 个块 = 2MiB
#define INODES_PER_BLOCK    (BLOCK_SIZE / sizeof(struct s_inode))    // 每块能有多少个 inode 
#define DIR_NO_PRE_BLOCK    (BLOCK_SIZE / sizeof(struct dir_entry)) // 每个逻辑块可以存储多少个 目录项
#define NR_BLOCK_PRE_INODE  (INODE_BLOCK_AREA_LEN-1)+(BLOCK_SIZE/2)   // 一个 inode 能容纳的 block 总数
#define BUFFER_HASH_NO 400                  // hash number

// 高速缓存头文件,一个缓存块 1kib 大小
struct buffer_head
{
    // 高速缓存中的虚拟地址
    char *data;   // 1 kib size | 1024
    unsigned char dirty;
    // 被引用次数
    struct atomic count; 
    /*
        [lock = 0] 没有锁定 [lock = 1] 锁定
        Lock when reading or writeing blocks from disk
    */
    struct atomic lock;  

    // 磁盘数据中的索引块号
    unsigned long block;                // 磁盘的块号,
    struct wait_queue_head lock_wait;   // 读取和写入的锁定 等待队列

    // 空闲链表
    struct buffer_head *free_next;
    struct buffer_head *free_prev;

    // 忙碌的 buffer 链表
    struct buffer_head *hash_next;
    struct buffer_head *hash_prev;
};


// 磁盘中的超级快
struct s_super
{
    unsigned short inode_nos;             // i 节点总数
    unsigned short block_nos;             // 总逻辑块
    unsigned short imap_blocks;           // i 节点所占块数
    unsigned short zmap_blocks;    
    unsigned short first_data_block;     // 第一个数据块
    unsigned short log_zone_size;        // log（数据块数/逻辑块）
	unsigned long max_size;             // 文件最大长度
	unsigned short magic;                // 文件系统魔数
};

/*
    磁盘中的i节点，1块 4kib
*/
struct s_inode
{
    /*
        文件类型和属性(rwx位)
        15-12文件类型
        {
            001-FIFO 文件（八进制）
            002-字符设备文件
            004-目录文件
            006-块设备文件
            010-常规文件
        }
        11-9执行文件时设置的信息
        {
            01-执行时设在用户ID （set-user-ID）
            02-执行时设在组ID
            04-对于目录，寿险删除标志
        }
        8-0表示文件的访问权限
        {
            0-2(RWX) 其他人
            3-5(RWX) 组员
            6-8(RWX) 宿主
        }
    */
    unsigned short mode;
    unsigned short uid;    // 用户ID
    unsigned long size;    // 文件大小
    unsigned long time;   // 文件创建时间
    unsigned char gid;      // 组ID
    unsigned char nlinks;   // 连接数

    /*
        0-7 直接块，8 * 4kib = 32 kib
        8 = 间接块 8 MiB?
    */
    unsigned short block_area[INODE_BLOCK_AREA_LEN];
};

// 目录项 16 个 char
struct dir_entry
{
    unsigned short inode;    // ｉ 节点号
    char name[MAX_NAME_LEN];
};


// ----------------------------------- 内存中的结构 -----------------------------------
/*
    引导块 | 超级块 | i 节点位图块 | 逻辑块位图 | i节点数组 | 根目录
*/
// 所有文件可以看做 inode 的数组
// inode 可以看做是 block 的数组
struct m_inode
{
    unsigned short mode;    // flags create | wirte | read ...
    unsigned short uid;     // 用户ID
    unsigned long size;     // 文件大小
    unsigned long time;    // 文件创建时间
    unsigned char gid;      // 组ID
    unsigned char nlinks;   // 连接数
    
    /*
 	    0-6是直接逻辑块号
		7是一次间接块号
		8是二次间接块号
    */
    unsigned short block_area[INODE_BLOCK_AREA_LEN];
    
    unsigned short inode;       // inode 的索引号
    unsigned short dirty;        // 已经修改的脏标志
    unsigned short count;        // 被引用
    
    struct atomic lock;                // 被锁定

    struct m_inode *next;
    struct m_inode *prev;
    struct semaphore sem;          // 信号量
    struct vm_area_struct *mmap;   // 映射的线性区
};

struct m_super_block
{
    unsigned short inode_nos;  // i 节点总数
    unsigned short block_nos;  // 总逻辑块
    unsigned short imap_blocks;  // i 节点所占块数
    unsigned short zmap_blocks;    
    unsigned short first_data_block;     // 第一个数据块
    unsigned short log_zone_size;        // log（数据块数/逻辑块）
	unsigned long  max_size;             // 文件最大长度
	unsigned short magic;                // 文件系统魔数

    struct buffer_head *inode_map[8];       // inode 块位图
    //  磁盘分成 1kib [65536]  的块数组
    struct buffer_head *zblock_map[8];      // 逻辑块位图

    struct m_inode *root;                   // 文件系统的根节点
};

struct file
{
    //unsigned short count;  // 被引用的次数
    unsigned short used;     // 是否被使用
    unsigned short mode;          // 读写权限的标志
    struct m_inode *inode;
    struct file_operations *file_op;

    struct file *next;
    struct file *prev;

    unsigned long pos;  // indoe 文件位置
};

// 文件操作函数列表
struct file_operations
{
    int (*lseek)();
    int (*mmap)(struct file *file, struct vm_area_struct* vm);
    int (*open)(const char *filename);
    int (*read)(const char *filename);
    int (*write)(const char *filename);
    int (*close)(struct file *file);
    int (*nopage)(    
        struct vm_area_struct *vma,
        struct page *page,
        unsigned long vaddr);
};


// ------------------------  block_buffer.c  ------------------------
extern struct buffer_head* bread_block(int block);
extern long bread_page(int block[4], struct page *page);          // 读取磁盘数据-to page
extern void read_write_block(int rw, struct buffer_head *buffer); 
extern struct buffer_head* alloc_buffer(int block);  // 分配缓冲区
extern void brelse(struct buffer_head *buffer);
extern int sync_buffer(struct buffer_head *bh);
extern int sync_dev();

// ------------------------  (bitmap and inode)   ------------------------
extern int new_block();
extern struct m_inode* new_inode();          // 从位图中分配 inode
extern void free_block(unsigned int block);
extern void free_inode(struct m_inode *inode);

// 该函数获得在 s_inode 在内存中的副本
extern struct m_inode* get_minode_cachep();
extern struct m_inode* iget(int inode);
extern void iput(struct m_inode *inode);
extern void free_inode_block(struct m_inode *inode);  // 释放 inode 的所有 block

// 为 inode 创建一个 block 
extern int create_block(struct m_inode *inode, int block_index);
extern int bmap(struct m_inode *inode, int block_index);
extern void sync_inodes();          // 同步所有 i 节点信息

// ------------------------  iname (路径转换)  ------------------------
extern struct m_inode* open_and_create(
        const char *pathname, 
        unsigned short mode,
        int flags);
// 获取文件的最后一级目录
extern struct m_inode* get_last_dir(const char *pathname);
extern struct m_inode* dir_namei(char **name, int *namelen, const char *pathname);


// ------------------------  super (超级块)  ------------------------
extern int do_mount_root();     // 安装根文件系统
extern int do_umount();         // 卸载文件系统
extern int do_mount();          // 安装文件系统
extern struct m_super_block* read_super();


// ------------------------  file_table.c  ------------------------
extern struct file* get_empty_filp();
extern int fd_tables[];
extern struct file *first_file;
extern int nr_files;


// ------------------------  系统调用  ------------------------
extern int do_read(unsigned long fd,  char *buf, int count);
extern int do_write(unsigned long fd, char *buf, int count);
extern int do_open(
                const char *filename, 
                unsigned short mode,
                int flags);  // 返回句柄，fd
extern int do_close(unsigned int fd);
extern int do_lseek(unsigned long fd, unsigned long offset, seek_type origin);
extern int do_create(const char *pathname,int flags);

// 目录系统调用
extern int do_mkdir(const char *pathname, unsigned short mode);  // 构建目录
 // 删除目录 or 文件？
extern int do_rmdir(const char *pathname);


// ------------------------ 打印调试函数 ------------------------
extern void show_active_buffer_head();
extern void show_all_dir_entry(struct m_inode *inode);

// 超级块
extern struct m_super_block super_block;
extern struct m_inode *head_cachep; 



/*
    根文件系统
    {
        etc
        dev
        bin
        usr
        usr/bin
        var
    }
*/

#endif