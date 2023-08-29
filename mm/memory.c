#include <mm/mm.h>
#include <mm/pagetable.h>



struct page mem_map[NR_PAGES];
struct free_area_struct free_area[MAX_ORDER];
unsigned long HIGH_MEMORY;

extern struct vm_area_struct* find_vm_area(
    struct mm_struct *mm, 
    unsigned long vaddr);

static struct vm_area_struct *get_vm_area(struct mm_struct *mm, unsigned long vaddr)
{
    struct vm_area_struct* vm = mm->mmap;
    
    while (vm)
    {
        if(vaddr > vm->vm_start && vaddr < vm->vm_end)
            return vm;

        vm = vm->vm_next;
    }

    return 0;   
}

/*
    页共享试图找到一个进程， 他可以与当前进程共享页面
    从 share_vm 中复制页表至 vm 中
*/
static int try_share_page(
    struct vm_area_struct *vm,
    struct vm_area_struct *share_vm,
    unsigned long vaddr)
{
    unsigned long from_vaddr;

    unsigned long *from_pgd_entry;
    unsigned long *from_pte_entry;

    unsigned long *to_pgd_entry;
    unsigned long *to_pte_entry;

    struct page *page;

    // share_vm 所指向的页框能共享吗？
    if(share_vm->vm_flags & VM_MAP_PRIVATE)
        return -1;

    printk("not map \\n");

    from_vaddr   = share_vm->vm_start + (vaddr - vm->vm_start);

    // 复制页表映射
    from_pgd_entry = (unsigned long*)(share_vm->vm_task->mm->pgd_base + pgd_index(from_vaddr));
    if(!(*from_pgd_entry & PAGE_P))      // 不存在、不能共享
        return 0;
    
    from_pte_entry = (unsigned long*)(*from_pgd_entry + pte_index(from_vaddr));
    if(!(*from_pte_entry & PAGE_P))
        return 0;

    printk("from_pte_entry \\n");

    // 目标页表
    to_pgd_entry = (unsigned long*)(vm->vm_task->mm->pgd_base + pgd_index(vaddr));
    if(!(*to_pgd_entry & PAGE_P))
    {
        // 创建一个页表
        page = alloc_one_page();
        *to_pgd_entry = (__page_to_pfn(page) << PAGE_SHIFT) | 7;
    }

    printk("to_pgd_entry \\n");

    to_pte_entry = (unsigned long*)(*to_pgd_entry + pte_index(vaddr));
    *to_pte_entry = *from_pte_entry | ~PAGE_RWE;    // 共享页不能写

    // 页共享记数
    mem_map[*to_pte_entry >> PAGE_SHIFT].count++;

    return 1;
}

static int share_page(
    struct vm_area_struct *vm,
    unsigned long vaddr)
{
    int i;
    struct vm_area_struct *share_vm;

    share_vm = &init_mmap;
    
    while (share_vm)
    {
        if(share_vm->vm_file->inode == vm->vm_file->inode
            && share_vm->vm_task != current)
        {
            if(try_share_page(vm, share_vm, vaddr))
                return 1;
        }
        
        share_vm = share_vm->vm_next_share;

        if(share_vm == &init_mmap || share_vm == NULL)
        {
            share_vm = NULL;
            return -1;
        }
    }

    return -1;   
}

/*
    写保护异常==访问共享页面的时候，会发生该异常;
    页权限异常==用户访问内核页表也会有这个问题
*/
static void do_wp_page(unsigned long vaddr)
{
    unsigned long *pgd_entry;
    unsigned long *pte_entry;
    struct page *page;
    struct page *new_page;

    unsigned long *old_addr;
    unsigned long new_addr;

    unsigned long pte_index = pte_index(vaddr);

    pgd_entry = (unsigned long*)(pgd_index(vaddr));
    pte_entry = (unsigned long*)((*pgd_entry & 0xfffff000) + pte_index);

    printk("pgd_entry=%x \\n",pgd_entry);

    page = &mem_map[(*pte_entry >> PAGE_SHIFT)];

    if(page->count == 1)            // 引用只有1，可以直接设置为可写
    {
        *pte_entry |= PAGE_RWE;      // 设置可写
        return;
    }

    // 复制页框
    page->count--;
    
    new_page = alloc_one_page();
    new_page->count++;

    new_addr = (unsigned long*)(__page_to_pfn(new_page) << PAGE_SHIFT);

    *pte_entry = new_addr | PAGE_ATTR;   // 设置可读可写标志以及存在标志

    old_addr = __page_to_pfn(page) << PAGE_SHIFT;
    
    copy_page(old_addr, new_addr);
    
    invalidate();
}



// 缺页异常
static void do_no_page(unsigned long vaddr)
{
    struct vm_area_struct *vm;
    struct page *page;

    unsigned long *pgd_entry;
    unsigned long *pte_entry;
    unsigned long pte_idx;

    pgd_entry = (unsigned long*)pgd_index(vaddr);   // 页目录索引地址
    vm = find_vm_area(current->mm, vaddr);

    if(vm == NULL)
    {
        printk("not find vm \\n");
    }

    if(!(*pgd_entry & PAGE_P))   //  页目录项 = 页表
    {
        // 创建页表的框
        page = alloc_one_page();
        *pgd_entry = (__page_to_pfn(page) << PAGE_SHIFT) | PAGE_ATTR;
    }

    pte_idx  = pte_index(vaddr);
    pte_entry = (unsigned long*)((*pgd_entry & 0xfffff000) + pte_idx);

    if(!vm->vm_file)    // 匿名映射
    {
        if(!(*pte_entry & PAGE_P))   // 分配页框
        {
            // 映射页表项
            page = alloc_one_page();
            *pte_entry = (__page_to_pfn(page) << PAGE_SHIFT) | PAGE_ATTR;
        }
        return;
    }

    /* ----------------------- 文件映射 ------------------------- */
    // 存在同一个可执行文件

/*     if(share_page(vm, vaddr))                            // 有共享的可执行文件
        return; */

    page = alloc_one_page();
    
    // 从文件中加载数据至 page
    filemap_nopage(vm, page, vaddr);

    // 页表映射
    *pte_entry = (__page_to_pfn(page) << PAGE_SHIFT) | PAGE_ATTR;
    page->count++;
    
    invalidate();
}


/*
    [缺页异常 | 写保护异常]
    error_code = 4字节，只使用了最后3位
    {
        位2 (U/S) = 0 表示在超级用户下执行，1表示在用户模式下执行
        位1 (W/R) = 0 表示读操作，1 表示写操作
        位0 (P) = 0表示页不存在， 1 表示页保护
        111 = ?
    }
*/
void do_page_fault(
    unsigned long error_code,
	unsigned long address)
{
    printk("do_page_fault vaddr=%x,%d \\n", address, error_code);
    unsigned long user_mode;

    user_mode = 0;

    if(error_code & 0x4)    // user mode access
        user_mode = 1;

    if(error_code & 0x01)   // 页保护
    {
        do_wp_page(address);
        return;
    }

    if(!(error_code & 0x01)) // 页不存在
    {
        do_no_page(address);
        return;
    }
}


// 从 8MiB ~ [32mib or 64MiB]
void mem_init(unsigned long start_mem, unsigned long end_mem)
{
    int i;
    int start_pfn;

    HIGH_MEMORY = start_mem;    // 高端地址
    start_pfn = (start_mem / PAGE_SIZE);
    end_mem >>= PAGE_SHIFT;

    //printk("start_Pfn = %d,NR_PAGES=%d end_mem%d \\n",start_pfn, NR_PAGES,end_mem);

    for(i = 0; i < start_pfn; i++)
    {
        mem_map[i].count  =  1;
        mem_map[i].flags  =  PAGE_RWE | PAGE_P | PAGE_LOCK;
        mem_map[i].order  = (MAX_ORDER);   // 表示不属于伙伴系统
    }

    for (i = start_pfn; i < end_mem; i++)
    {
        mem_map[i].count  =  0;
        mem_map[i].flags  =  0;
        mem_map[i].order  = (MAX_ORDER);   // 表示不属于伙伴系统
    }
    
    init_buddy_pages(start_pfn, end_mem);       // 伙伴系统 init

    //show_buddy_area();
    kmem_cache_init();
    //show_buddy_area();
}


// [ 0x800000 ~ 0x2000000 ]
unsigned long paging_init(unsigned long start_mem, unsigned long end_mem)
{
    unsigned long *pg_dir;
    unsigned long *pg_table;

    unsigned long address;
    unsigned long tmp;

    pg_dir = (unsigned long*)(768+4);       // head.S 中映射了 4个页目录=16MiB
    address = 0x1000000;

    while (address < end_mem)
    {
        tmp = *pg_dir;
        // 页目录
        if(!tmp)
        {
            tmp = start_mem | PAGE_RWE | PAGE_P;
            *pg_dir = tmp;
            start_mem += PAGE_SIZE;
        }

        pg_dir++;
        pg_table = (unsigned long*)tmp;

        // 页表 1024 项
        for(tmp = 0; tmp < PTRS_PER_PAGE; tmp++, pg_table++)
        {
            if(address < end_mem)
            {
                *pg_table = address | PAGE_RWE | PAGE_P;
                address += PAGE_SIZE;
            }
        }
    }
    
    return start_mem;
}