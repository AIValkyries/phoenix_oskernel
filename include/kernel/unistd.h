#ifndef _UNISTD_H
#define _UNISTD_H


#define NR_sys_setup 0
#define NR_sys_fork 1
#define NR_sys_execve   2
#define NR_sys_exit     3
#define NR_sys_pause    4
#define NR_sys_nice     5
#define NR_sys_brk      6
#define NR_sys_open     7       // 打开文件
#define NR_sys_read     8       // 文件读
#define NR_sys_write    9       // 文件写
#define NR_sys_close    10      // 关闭文件
#define NR_sys_creat    11      // 创建文件
#define NR_sys_mkdir    12      // 创建文件夹
#define NR_sys_rmdir    13      // 删除文件夹
#define NR_sys_mount_root   14
#define NR_sys_mount        15
#define NR_sys_umount       16
#define NR_sys_uselib       17      // 加载库文件
#define NR_sys_lseek        18
#define NR_sys_ttyread      19
#define NR_sys_ttywrite     20


#define syscall0(type,name) \
type name(void) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (NR_sys_##name)); \
return (type) __res; \
}

#define syscall1(type, name, atype, a) \
type name(atype a) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (NR_sys_##name),"b" ((long)(a))); \
return (type) __res; \
}

#define syscall2(type, name, atype, a, btype, b) \
type name(atype a,btype b) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (NR_sys_##name),"b" ((long)(a)),"c" ((long)(b))); \
return (type) __res; \
}

#define syscall3(type,name,atype,a,btype,b,ctype,c) \
type name(atype a,btype b,ctype c) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (NR_sys_##name),"b" ((long)(a)),"c" ((long)(b)),"d" ((long)(c))); \
return (type) __res; \
}

extern int printf(char *fmt, ...);


/* extern int pause();
extern int fork(int  prio); */


#endif