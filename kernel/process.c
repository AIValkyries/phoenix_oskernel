#include <kernel/sched_os.h>

extern void eixt_sys_call();
void exit_switch();

#define task_to_user_mode(deip,desp) \
        __asm__("movl %0,%%ebx\n\t"    /*保存 eip */    \
            "pushl $0x17\n\t"       /*SS 的选择子*/     \
            "pushl %1\n\t"          /*用户栈*/      \
            "pushfl\n\t"            /*标志寄存器*/  \
            "pushl $0x0f\n\t"       /*cs的选择子*/  \
            "pushl $1f\n\t"         /*下面的1f地址*/    \
            "iret\n"        \
            "1:\tmovl $0x17,%%eax\n\t"  /*在用户模式下执行*/    \
            "mov %%ax,%%ds\n\t" \
            "mov %%ax,%%es\n\t" \
            "mov %%ax,%%fs\n\t" \
            "mov %%ax,%%gs\n\t" \
            "pushl %%ebx\n\t"   \
    ::"a"(deip),"d"(desp))

/*
    在fork之后，首先跳转到内核态模式
    在从内核态通过 iret 跳转到用户态模式
    如果直接跳转到 用户态模式，则下次通过系统调用进入内核态时，栈不会自动切换，因此会引起bug
*/
void exit_switch()
{
    /*
        if(current->prev != NULL)
        {
            printk("exit_switch pid=%x eip=%x \\n",
                current->prev->pid,current->prev->tss.eip);
        }
    */
    
    current->prev->tss.eip = exit_switch;
    current->tss.cs = USER_CS;
    current->tss.es = USER_DS;
    current->tss.ds = USER_DS;
    current->tss.ss = USER_DS;
    current->tss.gs = USER_DS;
    current->tss.fs = USER_DS;
    current->tss.esp = current->tss.user_stack_page + PAGE_SIZE;
    current->tss.eip = exit_switch;

    task_to_user_mode(current->tss.cache_eip, current->tss.esp);
}

/*
    nr = 进程 PID
*/
void copy_thread(
    int nr, struct task_struct* p, 
    long ebx,long ecx,long edx,
    long fs,long es,long ds, 
    long eip, long cs, long eflags, long esp,long ss)
{
    // 段选择符
    p->tss.cs = KERNEL_CS;
    p->tss.es = KERNEL_DS;
    p->tss.ds = KERNEL_DS;
    p->tss.ss = KERNEL_DS;
    p->tss.gs = KERNEL_DS;
    p->tss.fs = KERNEL_DS;
    p->tss.back_link = 0;
    p->tss.trace_bitmap = 0x80000000;
    p->tss.eflags = eflags;

    p->tss.eip = exit_switch;     // 用户程序的入口点
    p->tss.esp = p->tss.kernel_stack_page + PAGE_SIZE;
    p->tss.ecx = ecx;
    p->tss.edx = edx;
    p->tss.esi = 0;     p->tss.edi = 0;   p->tss.ebp = 0;  p->tss.eax = 0;
        
    // 内核栈
    p->tss.ss0  = KERNEL_DS;
    p->tss.esp0 = p->tss.kernel_stack_page + PAGE_SIZE;   // 内核栈指针
    p->tss.ss1  =   0;
    p->tss.esp1 =   0;
    p->tss.ss2  =   0;
    p->tss.esp2 =   0;

    p->tss.tr  =  __TSS(nr);
    p->tss.ldt =  __LDT(nr);
    p->tss.cache_eip = eip;

    // 在 GDT 全局表中设置 LDT 以及 TSS 段
    set_tss_desc(gdt+(nr<<1)+FIRST_TSS_ENTRY, &p->tss);
    set_ldt_desc(gdt+(nr<<1)+FIRST_LDT_ENTRY, &p->ldt_table);
}


void start_thread(
    struct task_struct *p, 
    unsigned long eip, 
    unsigned long esp)
{
    p->tss.cs = USER_CS;
    p->tss.ds = p->tss.es = p->tss.ss = p->tss.fs = p->tss.gs = USER_DS;
    //p->tss.esp = esp;
    p->tss.eip = eip;
}