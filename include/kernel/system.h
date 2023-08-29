#ifndef _SYSTEM_H
#define _SYSTEM_H

struct sysinfo_struct
{
	unsigned long task_last_pid;
    unsigned short active_task;  // 活跃的进程数量
	unsigned long dynamic_start;  // 动态内存开始的物理地址
};

typedef struct desc_struct 
{
	unsigned long a,b;
} desc_table[256];

extern desc_table idt,gdt;

// 寄存器组
struct pt_regs
{
	unsigned long ss;
	long esp;
	long eflags;
	unsigned long cs;
	unsigned long gs;
	unsigned long fs;
	unsigned long es;
	unsigned long ds;
	long eax;
	long ebp;
	long edi;
	long esi;
	long edx;
	long ecx;
	long ebx;
	// long eip;
};

/*
    GDT 布局

    0 - NULL
    1 - kernel code  0x08
    2 - kernel data  0x10
    3-TSS  #0        0x18
    4-LDT  #0        0x20
*/
#define KERNEL_CS   0x08
#define KERNEL_DS   0x10
#define USER_CS     0xf
#define USER_DS     0x17

#define FIRST_TSS_ENTRY 3
#define FIRST_LDT_ENTRY 4

#define GDT_ADDR 0x5400

// 计算在全局表中第n个任务的 TSS 段描述符的选择符索引号
#define __TSS(nr) ((nr<<4)+(FIRST_TSS_ENTRY<<3))
#define __LDT(nr) ((nr<<4)+(FIRST_LDT_ENTRY<<3))

// 任务切换 ecx = tsk, eax= tss.tr
#define switch_to(tsk) \
__asm__("cmpl %%ecx,current\n\t" \
	"je 1f\n\t" \
	"xchgl %%ecx,current\n\t" \
	"movw %1,%%ax \n\t"	\
	"ljmp %0\n\t" \
	"1:" \
	: /* no output */ \
	:"m" (*(((char *)&tsk->tss.tr)-4)), \
	 "m" (tsk->tss.eip),				\
	 "c" (tsk))

// type = [P|DPL|0][TYPE] = '89'
#define _set_tss_ldt_desc(addr,base,type)\
__asm__("movw $104,%1\n\t"\
			"movw %%ax,%2\n\t"\
			"rorl $16,%%eax\n\t"\
			"movb %%al,%3\n\t"\
			"movb $" type ",%4\n\t"\
			"movb $0x00,%5 \n\t"\
			"movb %%ah,%6 \n\t"\
			"rorl $16,%%eax \n\t"\
		::"a"(base),"m"(*(addr)),"m"(*(addr+2)),"m"(*(addr+4)),"m"(*(addr+5)),\
		"m"(*(addr+6)),"m"(*(addr+7)))

#define set_tss_desc(addr,base) _set_tss_ldt_desc(((char *) (addr)),base,"0x89")
#define set_ldt_desc(addr,base) _set_tss_ldt_desc(((char *) (addr)),base,"0x82")

// 把第n个任务的TSS段 选择符 加载到任务寄存器TR中
#define ltr(n) __asm__("ltr %%ax"::"a" (__TSS(n)))
#define lldt(n) __asm__("lldt %%ax"::"a" (__LDT(0)))
 
/* ------------------------------ fs 段--------------------------- */
#define set_fs(val)	__asm__("mov fs,%0"::"a" (val))

#define get_ds()({	\
	unsigned short _v; \
	__asm__("mov ax,ds":"=a"(_v));	\
	_v;	\
})

#define get_fs()({\
	unsigned short _v; \
	__asm__("mov ax,fs":"=a"(_v));	\
	_v;	\
})

#define put_fs_byte(val,vaddr)	__asm__("movb %0,%%fs:%1"::"r" (val),"m" (*vaddr))
#define put_fs_word(val,vaddr)	__asm__("movw %0,%%fs:%1"::"r" (val),"m" (*vaddr))
#define put_fs_long(val,vaddr)  __asm__("movl %0,%%fs:%1"::"r" (val),"m" (*vaddr))

#define get_fs_byte(vaddr)({	\
	unsigned char _v;	\
	__asm__("movb %%fs:%1,%0":"=r" (_v):"m" (*vaddr));	\
_v;	\
})

#define get_fs_word(vaddr)({ \
	unsigned short _v;	\
	__asm__("movw %%fs:%1, %0":"=r" (_v):"m" (*vaddr));	\
_v;	\
})

#define get_fs_long(vaddr)({\
	unsigned long _v;	\
	__asm__("movl %%fs:%1, %0":"=r" (_v):"m" (*vaddr)); 	\
 _v;	\
})






#endif