#include <kernel/system.h>
#include <kernel/sched_os.h>
#include <kernel/kernel.h>


/*
	中断出错的打印信息
	出错的名称处理 = str | 进程 PID
	中断名称 | 调用程序 EIP，ESP，EFLAGS | fs 段寄存器值 10字节指令码，
	如果堆栈在用户数据段，则还打印16字节的堆栈内容
*/
static void die(char *str, struct pt_regs *regs)
{
	printk("%s: \\n", str);
	printk("PID:%d \\n",current->pid);
	printk("ESP:%x- \\n",regs->esp);

	do_exit();
}

// 当/0的时候产生
void do_divide_error(struct pt_regs *regs)
{
	//die("divide error", regs);

	printk("divide error \\n");
}

// int1--debug调试中断入口点，处理过程同上
void do_debug(struct pt_regs *regs)
{
	//die("debug", regs);

	printk("debug \\n");
}

// 由不可屏蔽中断产生
void do_nmi(struct pt_regs *regs)
{
	//die("nmi", regs);

	printk("nmi \\n");
}

// 由断点3产生
void do_int3(struct pt_regs *regs)
{
	//die("int3", regs);

	printk("int3 \\n");
}

// eflags 的溢出标志 OF引起
void do_overflow(struct pt_regs *regs)
{
	//die("overflow", regs);

	printk("overflow \\n");
}

// 寻址到有效地址以外时
void do_bounds(struct pt_regs *regs)
{
	//die("bounds", regs);

	printk("bounds \\n");
}

// 无效的指令操作码
void do_invalid_op(struct pt_regs *regs )
{
	//die("invalid_op", regs);

	printk("invalid_op \\n");
}

// 设备不存在
void do_coprocessor_segment_overrun(struct pt_regs *regs )
{
	//die("coprocessor segment overrun", regs);

	printk("coprocessor segment overrun \\n");
}

// 
void do_reserved(struct pt_regs *regs )
{
	//die("reserved", regs);

	printk("reserved \\n");
}

// 协处理器段超出
void do_coprocessor_error(struct pt_regs *regs )
{
	//die("coprocessor error", regs);

	printk("coprocessor error \\n");
}

// 双故障出错
void do_double_fault(struct pt_regs *regs)
{
	//die("double fault", regs);

	printk("double fault \\n");
}

// TSS 无效，任务切换时发生
void do_invalid_TSS(struct pt_regs *regs)
{
	//die("invalid TSS", regs);

	printk("invalid TSS \\n");
}

// int11--段不存在
void do_segment_not_present(struct pt_regs *regs)
{
	//die("segment not present", regs);

	printk("segment not present \\n");
}

// int12 堆栈段不存在，或寻址越出堆栈段
void do_stack_segment(struct pt_regs *regs)
{
	//die("stack segment", regs);
	printk("stack segment \\n");
}

// int13 --一般保护性出错
void do_general_protection(unsigned long esp)
{
	// die("general protection", regs);
	// printk("general protection esp=%x \\n", esp);
}

// 对齐边界检查
void do_alignment_check(unsigned long esp, long error)
{
	printk("PID=%x,ESP=%x \\n",current->pid, esp);
}