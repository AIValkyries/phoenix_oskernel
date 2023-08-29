// 加载 elf 格式

#include <fs/elf.h>
#include <fs/fs.h>


extern void unmap_page_range(
    struct mm_struct *mm, struct vm_area_struct *vm);

/*
    装载库
*/
int  do_uselib(const char *library)
{
    

    
    return 0;
} 

/*
    装载进程，暂时不包含 Shell 解析
    do_execve 是在当前进程下执行的 ？

    // 三个段
    LOAD           0x000000 0x00fff000 0x00fff000 0x000d4 0x000d4 R   0x1000
    LOAD           0x001000 0x01000000 0x01000000 0x00290 0x00290 R E 0x1000
    LOAD           0x002000 0x01001000 0x01001000 0x001c0 0x001c0 R   0x1000
    LOAD           0x000000 0x01003000 0x01003000 0x00000 0x00410 RW  0x1000
*/
int do_execve(unsigned long *eip, const char *filename)
{
    struct elf32_header *elf_head;
    struct elf32_program_header_table *elf_phdata;
    struct elf32_program_header_table *elf_ppnt;
    int fd = 0;
    int i = 0;
    int size_count = 0;
    int page_prot = 0;
    int phnum = 0;
    
    unsigned long start_code = 0x00;
    unsigned long end_code  = 0;
    unsigned long end_data  = 0;
    unsigned long start_brk = 0;
    unsigned long brk       = 0;
    unsigned long elf_entry   = 0;
    unsigned long start_stack = 0x00;

    size_count = sizeof(struct elf32_header);
    elf_head   = kmalloc(size_count);

    // 加载 inode
    fd = do_open(filename, FILE_NORMAL, 0);

    // 读取 elf 头
    if(!do_read(fd, (char*)elf_head, size_count))
        return -1;

    if( elf_head->ident[0] != 0x7f || elf_head->ident[1] != 0x45 || 
        elf_head->ident[2] != 0x4c || elf_head->ident[3] != 0x46)
        return -1;

    if(elf_head->ident[4] != ELF_32)    // 不支持 64位
        return -1;

    // 暂时只支持 intel
    if(elf_head->type != ET_EXEC || elf_head->machine != EM_386)
        return -1;

    elf_entry = elf_head->entry;
    // 文件读取，以块为单位，每块 4kib，因此需要调整 pos
    do_lseek(fd, size_count, SEEK_SET);

    size_count = elf_head->phnum * elf_head->phentsize;
    phnum = elf_head->phnum;
    kfree(elf_head);

    elf_phdata = kmalloc(size_count);

    if(!do_read(fd, (char*)elf_phdata, size_count))
        return -1;

    elf_ppnt = elf_phdata;

    printk("elf_entry=%x \\n",elf_entry);

    // 遍历程序头表,建立映射
    for(i = 0; i < phnum; i++)
    {
        page_prot = 0;
        if(elf_ppnt->type == PT_LOAD)   // 可装载的 segment
        {
            // 根据段的权限，设置 page 的权限
            if(elf_ppnt->flags & PF_R)  page_prot |= PROT_READ;
            if(elf_ppnt->flags & PF_W)  page_prot |= PROT_WRITE;
            if(elf_ppnt->flags & PF_X)  page_prot |= PROT_EXEC;

            do_mmap(
                elf_ppnt->filesz,
                current->filps[fd],
                elf_ppnt->vaddr,                            // 本段在内存中的虚拟起始地址
                elf_ppnt->memsz + elf_ppnt->vaddr,         // 本段内容在文件中的长度
                page_prot,
                // 线性区本身的标志映射标志
                VM_MAP_FIXED | VM_MAP_SHARED | VM_MAP_DENYWRITE | VM_MAP_EXECUTABLE,
                elf_ppnt->offset);

            if(start_code  == 0x00 && elf_ppnt->vaddr == 0)
                start_code = elf_ppnt->vaddr;
            if(elf_ppnt->flags == PF_RE)
                end_code = elf_ppnt->vaddr + elf_ppnt->memsz;

            if(elf_ppnt->flags == PF_RW)
            {
                end_data = elf_ppnt->vaddr + elf_ppnt->memsz;
                // .bss 在目标文件中不占空间
                // 它在段中，即在进程空间中却会占有一席之地
                brk = start_brk = elf_ppnt->vaddr + elf_ppnt->memsz;
            }
        }

        elf_ppnt++;
    }

    current->mm->start_code = start_code;
    current->mm->end_code = end_code;
    current->mm->end_data = end_data;
    current->mm->start_brk = start_brk;
    current->mm->brk = brk;
    current->mm->start_stack = current->tss.user_stack_page + PAGE_SIZE;

    set_brk(brk);
    current->tss.eip = *eip;
    kfree(elf_phdata);
    do_close(fd);

    unsigned long *esp_addr = (unsigned long*)current->tss.esp;

    //printk("eip=%x \\n", current->tss.eip);
    //printk("esp=%x,eip=%x \\n",(current->tss.esp),current->tss.eip);

    current->tss.cache_eip = elf_entry;
    
    current->tss.esp-=4;
    *esp_addr = current->tss.eip;
    
    eip[0] = elf_entry;
    return 1;
}