#ifndef _MM_H
#define _MM_H


#include <mm/pagetable.h>
#include <kernel/sched_os.h>

/*
    刷新页变换高速缓冲宏函数 
    为了提高地址转换的效率，CPU将最近使用的页表数据存放在芯片中高速缓冲中，在修改页表信息之后
    就需要刷新该缓冲区，这里使用重新加载页目录基地址寄存器 cr3 的方法来进行刷新 。下面 eax = 0
    是页目录基址
*/
#define invalidate()    \
    __asm__("movl %%eax,%%cr3"::"a" (0))

#define PAGE_SHIFT 12
#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE-1))
#define PAGING_MEMORY (32*1024*1024)    // 总内存大小
#define NR_PAGES (PAGING_MEMORY >> PAGE_SHIFT)
#define PTRS_PER_PAGE			(PAGE_SIZE/sizeof(void*))
#define USED 100         // 页面被占用标志

#define PAGE_ATTR 7

/*
    16777217 / 4096 = 4097
    20971521 / 4096 = 5121
    物理内存分配的时候，要把这一块内存去除，它是 buffer 分配的 
*/
#define BUFFER_START_PFN 4097
#define BUFFER_END_PFN   5121


/*
    16384 个页框 = 2048 byte = 2kib 总共 4kib
    2^8 = 256 * 4kib   = 1 MiB       64  [256 个 page 数组，64 个 2^8 块的数组]
    2^7 = 128*4kib     = 512  kib    128
    2^6 = 64*4kib      = 256  kib    256 
    2^5 = 32*4kib      = 128  kib    512					
    2^4 = 16*4kib      = 64 kib      1024					
    2^3 = 8*4kib       = 32 kib      2048					
    2^2 = 4*4kib       = 16 kib      4096					
    2^1 = 2*4kib       = 8  kib      8192					
    2^0 = 1*4kib       = 4  kib      16384	个页框				
*/
#define MAX_ORDER 9  // 伙伴阶数,8 阶，1MiB的大小
#define MAX_ZONES 3

#define GFP_KERNEL 0
#define GFP_BUFFER 1
#define GFP_USER 2

#define SLAB_END 65535
#define WORD_BYTES sizeof(void*)    // 字的大小

/* ---------------------------- 线性区的标志 -------------------------- */
// 线性区的读写权限
#define VM_READ    0x0001
#define VM_WRITE   0x0002
#define VM_EXEC    0x0004          //  执行权限

// 映射行为标志
#define VM_MAP_SHARED  0x08        // 可以被共享
#define VM_MAP_PRIVATE 0x10        // 不可以被共享
#define VM_MAP_FIXED   0x20        // 新区间必须使用指定的地址 vaddr
// 好像有点多余....
#define VM_MAP_DENYWRITE    0x40    // ETXTBSY 区域映射一个不可写的文件,TODO? 
#define VM_MAP_EXECUTABLE   0x80    // 映射一个可执行文件，TODO


// ---- 对应页表的权限 -----
#define PROT_READ       0x01
#define PROT_WRITE      0x02
#define PROT_EXEC       0x04
#define PROT_NONE       0x00


// 页框描述符
/*
    16384 个页框, [内核区 = 4096 个页框] [高速缓存区 = 2048] [主内存区 = 10240个]
    16 字节
*/
struct page
{
    /*
        如果在 slab 中，则 next 指向 cachep ； prev 指向 slab
    */
    struct page *next;
    struct page *prev;

    unsigned long flags;

    unsigned short count;      // 被页表引用的次数
    unsigned short order;      // 所在阶 ，[max = 65535] 表示没有
};

// 空闲链表 == 伙伴阶的链表
struct free_area_struct
{
    struct page *free_list;                 // 首指针
    unsigned long nr_free;                  // 链表的数量
};


/*
    slab 一个内存块
    obj_mem 返回内核空间的线性地址
*/
struct slab_struct
{
    struct slab_struct *next;
    struct slab_struct *prev;
    void *obj_mem;   // 第一个对象的地址
    unsigned short free_idx;  // 下一个空闲对象的下标
    unsigned short free_num;  // 空闲对象的数量
};


// 缓存信息,4kib的页框，可以装载 1百多个 kmem_cache_struct
struct kmem_cache_struct
{
    // slab 链表（只分配在内部），因此 slab 链表地址 = 虚拟地址
    struct slab_struct *slabs_partial;   // 半满 or 全空闲的
    struct slab_struct *slabs_full;      // 全满
    struct kmem_cache_struct *next;         // 缓存的链表
    struct kmem_cache_struct *slabp_cache;  // slab 的缓存
    char *name;                             // 对象类型名称

    //unsigned long  start_vaddr;
    unsigned short obj_size;
    unsigned short slab_obj_num;  // 单个 slab 有多少个对象
    unsigned short slab_size;
    unsigned short slab_order;    // 连续的页框 

    unsigned long left_over;
};


/*
    通用对象, 分配数组 ？ or 字符串 ？
    32-131072 字节
    131072 = 2^17 = 128 kib?
*/
struct cache_sizes_struct
{
    unsigned int cs_size;
    char *name;
    struct kmem_cache_struct *cs_cachep;
};


/*
    【 对齐 4096 】 线性区是虚拟地址空间的分段结果

    通常有4个段
    {
        02  [.interpret    .dynsym        .dynstr     .rela.plt]    // 动态符号表，动态字符串表
        03  [.init         .text          .plt        .plt.got]     // 可读可执行
        04  [.rodata       .eh_frame_hdr  .eh_frame]              // 只读数据
        05  [.init_array  .fini_array .dynamic .got .data .bss]  // 读写数据 	
    }
    【 CODE VMA | DATA VMA | READ_DATA VMA 】
    【 HEAP VMA | ...多个文件映射... | STACK VMA 】
*/
struct vm_area_struct
{
    struct task_struct *vm_task;
    unsigned long vm_start;
    unsigned long vm_end;
    unsigned long vm_file_size;
    unsigned long vm_flags;          // 本身的标志

    unsigned long vm_page_prot;      // 页框的属性
    unsigned long vm_offset;         // 在文件中的偏移量，在文件中的开始位置

    // 文件映射
    struct file *vm_file;  

    struct vm_area_struct *vm_next;
    //struct vm_area_struct* vm_prev;

    // 所有的可执行文件都用相关联
    struct vm_area_struct *vm_next_share;
    struct vm_area_struct *vm_prev_share;
};

// 所有的页框，总 16384 个
extern struct page mem_map[];
// 伙伴阶
extern struct free_area_struct free_area[];

extern unsigned long HIGH_MEMORY;

extern void mem_init(unsigned long start_mem, unsigned long end_mem);

/*  -------------------------- 文件映射 ------------------------------------ */
unsigned long filemap_nopage(
    struct vm_area_struct *vma,
    struct page *page,
    unsigned long vaddr);
int generic_mmap(struct file *file, struct vm_area_struct *vma);

/* -------------------------------- 伙伴系统 -------------------------------- */
extern struct page* alloc_one_page();
extern struct page* alloc_pages(unsigned short order);
// 返回虚拟地址
extern unsigned long get_free_page();
extern unsigned long get_free_pages(unsigned long order);

extern void free_page(unsigned long vaddr);
// 返回合并了多少个 page
extern int __free_pages___(struct page* page,unsigned short order);
extern void init_buddy_pages(unsigned long start_pfn,unsigned long end_pfn);              // 初始化伙伴系统


/* -------------------------------- slab 系统 -------------------------------- */
extern void kmem_cache_init();
extern void kmem_cache_destroy(struct kmem_cache_struct *cachep);

// 分配对象
extern void* kmem_object_alloc(struct kmem_cache_struct *cachep);
// 缓存对象的释放
extern void  kmem_object_free(struct kmem_cache_struct *cachep, void *objp);
// 通用对象分配
extern void* kmalloc(unsigned short size);
extern void kfree(void *objp);


/* -------------------------------- vm_area 线性区 -------------------------------- */

//extern int insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vm);
//extern int insert_share_vm_struct(struct vm_area_struct *new_vm);
extern int remove_share_vm_struct(struct vm_area_struct *vm);

// 堆的扩充或者缩减
extern unsigned long set_brk(unsigned long brk);

// 地址建立映射
extern int do_mmap(
    unsigned long file_szie,
    struct file* file, 
    unsigned long vaddr,
    unsigned long len,
    unsigned long prot,
    unsigned long flags,
    unsigned long off);

extern int do_munmap(unsigned long vaddr, unsigned long len);
extern void exit_mmap(struct task_struct* task);

// 页表
extern void free_page_tables (struct task_struct *task);  // 释放进程所有的页表和页目录
extern int  copy_page_tables (struct task_struct *to);    // 复制页表信息
// 页表填充为0
extern int zeromap_page_range(
    unsigned long from, 
    unsigned long size,
    unsigned long page_prot);


#define INIT_SLAB_LIST(head) (head->next = (head);head->prev = (head))

/* -------------------------------- 缓存 -------------------------------- */

extern struct kmem_cache_struct *filp_cachep;           // file 的结构
extern struct kmem_cache_struct *mm_cachep;
extern struct kmem_cache_struct *task_struct_cachep;
// 内存的结构，还有 slab，slab 存储在 kmem_cachep_struct 中
extern struct kmem_cache_struct *vm_area_cachep;        // vm 虚拟内存结构
extern struct kmem_cache_struct *inode_cachep;          // inode 的结构


/* -------------------------------- 调试 -------------------------------- */
extern void show_buddy_area();
extern void show_all_page();


#define invalidate() \
__asm__("movl %%eax,%%cr3"::"a" (0))

#endif