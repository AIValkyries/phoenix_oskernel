#include <kernel/sched_os.h>
#include <mm/mm.h>


struct kmem_cache_struct *vm_area_cachep;


// 解除页映射
void unmap_page_range(
    struct mm_struct *mm, struct vm_area_struct *vm)
{
    struct page     *page;
    unsigned long   *pgd_addr;
    unsigned long   *pte_addr;

    unsigned long end_vaddr;
    unsigned long pte_num;

    unsigned long size;
    unsigned long start_vaddr;

    start_vaddr = vm->vm_start;
 
    pgd_addr = (unsigned long*)(mm->pgd_base);
    size = (vm->vm_end - vm->vm_start) >> PGD_SHIFT;

    for(;size-->0;pgd_addr++)
    {
        if(!(*pgd_addr & 1))
            continue;

        pte_addr = (unsigned long*)(*pgd_addr & 0xfffff000);
        pte_num = 0;

        while (pte_num < PAGE_ENTRY_NUMBER)
        {
            if(*pte_addr)
            {
                if((*pte_addr) & 1)
                {
                    page = &mem_map[*pte_addr >> PTE_SHIFT];
                    __free_pages___(page, page->order);
                }

                *pte_addr = ZERO_PAGE;
            }
            pte_addr++;
            pte_num++;
        }
    }

    printk("DOing \\n");

}


// 建立范围内的页表映射
int zeromap_page_range(
    unsigned long start_vaddr, 
    unsigned long size,
    unsigned long page_prot)
{
    unsigned long end_vaddr;
    unsigned long *pgd_addr;
    struct page *page;
    unsigned long *pte_addr;    // 页表地址指针

    // unsigned long pgd_base;     // TODO 方便测试

    int c_pte;

    start_vaddr += current->mm->mmap->vm_start;

    end_vaddr = start_vaddr + size;
    pgd_addr = (unsigned long*)pgd_index(start_vaddr);
    
    // 构建页目录
    while (start_vaddr < end_vaddr)
    {
        // 建立页表
        page = alloc_one_page();  // 页表项
        pte_addr = (unsigned long*)(__page_to_pfn(page) << PAGE_SHIFT);

        // 将 page 转换成物理地址
        (*pgd_addr) = (unsigned long)pte_addr;      // 填充页目录项
        (*pgd_addr) |= PAGE_ATTR;

        c_pte = 0;
        
        // 填充页表
        while (c_pte <= PAGE_ENTRY_NUMBER)  // 填充页表项
        {
            (*pte_addr) = ZERO_PAGE;

            pte_addr++;
            c_pte++;
        }
        
        start_vaddr += PGD_SIZE;
        pgd_addr++;
    }

    return 1;
}


// 释放进程所有的页表和页目录
void free_page_tables(struct task_struct *task)
{
    unsigned long size;

    struct vm_area_struct *mmap;

    mmap = task->mm->mmap;
    if(mmap == NULL)
        return;
    //pgd_addr = (unsigned long*)task->mm->pgd_base;

    while (mmap)
    {
        //size = (mmap->vm_end - mmap->vm_start);
        unmap_page_range(task->mm, mmap);

        mmap = mmap->vm_next;
    }
}

// 复制页表信息, [current - to]
int  copy_page_tables(struct task_struct *to)
{
    unsigned long *from_pgd_addr;
    unsigned long *from_pte_addr;
    unsigned long *to_pgd_addr;
    unsigned long *to_pte_addr;

    unsigned long size;
    int nr;

    struct page *to_page;
    current->mm->pgd_base = 0;

    from_pgd_addr = (unsigned long*)current->mm->pgd_base;
    size  = 3;      // 12mib， 3项
    to_pgd_addr = pgd_index(to->mm->mmap->vm_start);

    while (size-->0)
    {
        if(!(*from_pgd_addr & PAGE_P))
            goto add_addr;

        // 创建一个页表
        if(!(to_page = alloc_one_page()))
            return -1;

        // 设置页目录项
        to_pte_addr = (unsigned long*)((__page_to_pfn(to_page) << PAGE_SHIFT));
          
        *to_pgd_addr  = (unsigned long)to_pte_addr | PAGE_P | PAGE_RWE | PAGE_US;
        from_pte_addr = *from_pgd_addr & 0xfffff000;

        // 复制页表项
        for(nr = 0; nr < PAGE_ENTRY_NUMBER; nr++)
        {
            unsigned long from_pte_value;

            // 页表项
            from_pte_value = *from_pte_addr;
            //from_pte_value &= (~PAGE_RWE);     // 只读共享=不可写
            *to_pte_addr = from_pte_value ;

            from_pte_addr++;
            to_pte_addr++;
        }

    add_addr:
        from_pgd_addr++;
        to_pgd_addr++; 
    }

    invalidate();

    return 1;
}

 struct vm_area_struct* find_share_vm_area(
     struct vm_area_struct *head,
     unsigned long vaddr)
 {
    struct vm_area_struct *mpnt;
    struct m_inode *inode;
    
    mpnt  = head->vm_next_share;
    inode = head->vm_file->inode;

    while (mpnt != NULL && mpnt != head)
    {
        if(mpnt->vm_file->inode == inode)   // 表示找到了相同的文件
            break;
        
        mpnt = mpnt->vm_next_share;
    }

    return mpnt;
 }

// 找到某个包含 vaddr 的线性区
struct vm_area_struct* find_vm_area(
    struct mm_struct *mm, 
    unsigned long vaddr)
{
    struct vm_area_struct* vm = mm->mmap->vm_next;
    unsigned long tmp_vaddr;

    tmp_vaddr = vaddr - mm->mmap->vm_start;
    while (vm)
    {
        if(tmp_vaddr >= vm->vm_start && tmp_vaddr <= vm->vm_end)
            return vm;

        vm = vm->vm_next;
    }

    return NULL;   
}


void remove_vm_struct(struct mm_struct* mm, struct vm_area_struct* vm)
{
    struct vm_area_struct* tmp = mm->mmap;

    while (tmp)
    {
        if(tmp == vm)
        {
            tmp->vm_next = vm->vm_next;
            break;
        }
        
        tmp = tmp->vm_next;
    }
}


static void insert_vm_struct(
    struct mm_struct *mm, 
    struct vm_area_struct *vm)
{
    struct vm_area_struct *tmp;
    tmp          = mm->mmap;

    vm->vm_next = tmp->vm_next;
    tmp->vm_next = vm;
}


static void insert_share_vm_struct(struct vm_area_struct* new_vm)
{
    struct vm_area_struct *tmp = &init_mmap;

    while (tmp)
    {
        if(tmp == new_vm)
            return;
        tmp = tmp->vm_next_share;
    }
    
    new_vm->vm_next_share = tmp->vm_next_share;
    new_vm->vm_prev_share = tmp;
    tmp->vm_next_share    = new_vm;
}


int remove_share_vm_struct(struct vm_area_struct* vm)
{
    if(!vm->vm_file)        return -1;
    if(vm == &init_mmap)    return -1;

    vm->vm_prev_share->vm_next_share = vm->vm_next_share;
    vm->vm_next_share->vm_prev_share = vm->vm_prev_share;

    return 1;
}

/*
    建立映射  【 页对齐 】  【 匿名映射  |  文件映射 】
*/
int do_mmap(
    unsigned long file_szie,
    struct file  *file, 
    unsigned long vaddr,
    unsigned long len,      // end
    unsigned long prot,     // 对应的页表标志
    unsigned long flags,    // 线性区本身的标志
    unsigned long off       /* 文件中的偏移位置 */ )
{
    // 段对齐 【 页对齐 ， 4kib 】
    struct vm_area_struct *cur_vm;
    unsigned long start_vaddr;

    if(len & (PAGE_SIZE-1))
    {
        len += PAGE_SIZE;
        len &= ~(PAGE_SIZE-1);
    }

    cur_vm = (struct vm_area_struct*)kmem_object_alloc(vm_area_cachep);
    start_vaddr = vaddr;

    cur_vm->vm_task  = current;

    cur_vm->vm_start = start_vaddr;
    cur_vm->vm_end   = len; 
    cur_vm->vm_page_prot = prot;
    cur_vm->vm_offset    = off;         // 本段在文件中的偏移，开始位置
    cur_vm->vm_flags     = flags;
    
    if(flags & VM_MAP_PRIVATE)  // 非共享的
    {
        cur_vm->vm_flags |= VM_READ | VM_EXEC | VM_WRITE;
    }
    else if(flags & VM_MAP_SHARED)
    {
        cur_vm->vm_flags |= VM_READ | VM_EXEC;      // 共享是不可写的。
    }

    current->mm->size += len;
    current->mm->map_count++;

    /* ---------------------------- 下面建立映射关系 ----------------------------*/
    if(file && file_szie > 0)        // 文件关联映射
    {
        cur_vm->vm_file->pos = file_szie;
        cur_vm->vm_file = file;
        file->inode->count++;

        insert_share_vm_struct(cur_vm);      // 共享链表
    }
    else
    {
        // 创建页表，与物理地址建立映射关系
        zeromap_page_range(start_vaddr, (len-start_vaddr), prot);
    }

    insert_vm_struct(current->mm, cur_vm);

    return 1;
}

/*
    解除映射
*/
int do_munmap(unsigned long vaddr, unsigned long len)
{
    struct vm_area_struct* vm;

    vm = find_vm_area(current->mm, vaddr);
    if(!vm) return -1;

    current->mm->size -= len;
    current->mm->map_count--;
    
    if(vm->vm_file)
        remove_share_vm_struct(vm);

    remove_vm_struct(current->mm, vm);
    
    kmem_object_free(vm_area_cachep, (void*)vm);

    //unmap_page_range(current->mm, vm->vm_start, (vm->vm_end - vm->vm_start));

    return 1;
}

// 消除整个进程的线性区
void exit_mmap(struct task_struct* task)
{
    struct vm_area_struct* vm = task->mm->mmap;
    task->mm->mmap = NULL;

    while (vm)
    {
        if(vm->vm_file) // 是文件映射
        {
            vm->vm_file->file_op->close(vm->vm_file);
            remove_share_vm_struct(vm);
        }

        kmem_object_free(vm_area_cachep, (void*)vm);
        vm = vm->vm_next;
    }
}

/*
    堆的扩充或缩减
*/
unsigned long set_brk(unsigned long brk)
{
    if(brk < current->mm->end_data)
        return current->mm->brk;
    
    unsigned long newbrk, oldbrk;

    newbrk = 0;
    oldbrk = 0;

    // 页对齐
    if(brk & PAGE_MASK)
    {
        newbrk = brk + (PAGE_SIZE - 1);
        newbrk &= ~PAGE_MASK;
    }

    if(current->mm->brk & PAGE_MASK)
    {
        oldbrk = current->mm->brk + (PAGE_SIZE - 1);
        oldbrk &= ~PAGE_MASK;
    }

    if(newbrk == oldbrk)
        return current->mm->brk;

    // 释放堆空间
    if(brk < current->mm->brk)
    {
        current->mm->brk = brk;
        do_munmap(newbrk, oldbrk - newbrk);
        return brk;
    }

    // 扩张堆空间
    current->mm->brk = brk;

    if(!find_vm_area(current->mm, brk))
    {
        // 建立新的映射,私有的
        do_mmap(NULL, 
            0,
            brk,        // vaddr
            newbrk - oldbrk,
            PROT_READ | PROT_WRITE | PROT_EXEC,           // 对应的页表标志
            VM_MAP_FIXED | VM_MAP_PRIVATE,                // 线性区自身的标志
            0);
    }

    return brk;
}