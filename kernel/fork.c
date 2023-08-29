#include <kernel/sched_os.h>
#include <kernel/system.h>
#include <mm/mm.h>
#include <kernel/string.h>


struct sysinfo_struct sysinfo;
struct kmem_cache_struct *mm_cachep;             // mm 的结构
struct kmem_cache_struct *task_struct_cachep;    // task的缓存


// 返回空闲的进程
int find_empty_process()
{
    int free_task;
    int i = 0;

    if((++sysinfo.task_last_pid) && 0xffff8000)
        sysinfo.task_last_pid = 1;

    while ( i++ < NR_TASKS)
    {
        if(!task_table[i])
        {
            free_task = i;
            sysinfo.active_task++;
            sysinfo.task_last_pid++;
            break;
        }
    }

    return free_task;    
}


/*
    mm_struct
    vm_area_struct
    页表
*/
void copy_mm(struct task_struct *p)
{
    unsigned long new_data_base;
	unsigned long new_code_base;
    int i;

    struct mm_struct *new_mm;
    struct mm_struct *old_mm;

    // mm
    {
        old_mm = current->mm;
        new_mm = (struct mm_struct*)kmem_object_alloc(mm_cachep);
        *new_mm = *old_mm;
        p->mm = new_mm;
        p->mm->mmap = NULL;
        p->mm->share_mmap = NULL;
    }

    struct vm_area_struct *code_vm;
    code_vm = (struct vm_area_struct*)kmem_object_alloc(vm_area_cachep);
    new_mm->mmap = code_vm;
    
#define TASK_LIMIT 0x4000

    // 64mib 之后
    new_code_base = new_data_base = (TASK_SIZE * p->pid);
    
    code_vm->vm_start = new_code_base;
    code_vm->vm_end = TASK_LIMIT * 4096;
    code_vm->vm_task = p;
    code_vm->vm_file = NULL;
    
    // base=0,limit, GD00,limit-4,PDPL0,TYPE 2000
    // 代码段
    p->ldt_table[1].a = TASK_LIMIT + (new_code_base << 16);
    p->ldt_table[1].b = 0x00c0fa00 + (new_code_base & 0xff000000) + 
                        ((new_code_base>>16) & 0x00ff);
    
    // 数据段
    p->ldt_table[2].a = TASK_LIMIT + (new_data_base << 16);
    p->ldt_table[2].b = 0xc0f200 + (new_data_base & 0xff000000) + 
                        ((new_data_base>>16) & 0x00ff);

    copy_page_tables(p);
}

// 共享 file
void copy_files(struct task_struct *p)
{
    int i;

    for(i = 0; i < NR_FILE; i++)
    {
        if(p->filps[i] != NULL)
            p->filps[i]->used++;
    }   
}

// inode
void copy_fs()
{
    if(current->pwd)
        current->pwd->count++;
    if(current->root)
        current->root->count++;
}

extern void set_nice(struct task_struct *p, int prio);

/*
    系统调用时自动压栈的参数
    | STACK 0x0001160c [0x000066cd]     eip    
    | STACK 0x00011610 [0x0000000f]     code segment    cs      
    | STACK 0x00011614 [0x00000002]     eflags
    | STACK 0x00011618 [0x000115f0]     esp 
    | STACK 0x0001161c [0x00000017]     ss 
    ecx = flags
*/
int do_fork(
    long ebx,long ecx,long edx,
    long fs,long es,long ds, 
    long eip, long cs, long eflags, long esp,long ss)
{
    // 创建新的进程
    struct task_struct* p;
    unsigned long new_kernel_stack;
    unsigned long new_user_stack;
    int nr;
    int flags;

    p = (struct task_struct*)kmem_object_alloc(task_struct_cachep);
    if(!p) return 0;        // 出现错误

    new_kernel_stack = get_free_page();
    new_user_stack   = get_free_page();
    if(!new_kernel_stack)
        goto bad_fork_free;
    
    printk("new_kernel_stack =%x new_user_stack=%x \\n",new_kernel_stack, new_user_stack);
    nr = find_empty_process();

    if(!nr) goto bad_fork_free;

    *p = *current;
    p->time_slice = current->time_slice;
    p->state = TASK_INTERRUPTIBLE;
    p->pid   = nr;
    p->start_time = jiffies;
    p->tss.kernel_stack_page = new_kernel_stack;
    p->tss.user_stack_page = new_user_stack;
    p->next = NULL;
    p->prev = NULL;

    set_nice(p, ebx);
    task_table[nr] = p;

    copy_mm(p);
    copy_thread(nr, p, ebx,ecx,edx,fs,es,
                ds,eip,cs,eflags,esp,ss);       // 复制资源
    copy_fs();                                  // inode
    copy_files(p);
    sched_fork(p);

    return p->pid;

bad_fork_free:
    printk("fork bad_fork_free \\n");
    free_page(new_kernel_stack);
    free_page((long)p);

    return 0;
}