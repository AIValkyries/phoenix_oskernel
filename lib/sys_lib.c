#include <kernel/unistd.h>
#include <kernel/stdarg.h>

/*
    注意：原本这些参数都是从 BIOS 中获取，但目前我没有这么做。使用硬性定�?
    系统参数的位?
*/



static inline syscall0(int, exit);
static inline syscall0(int, pause);
static inline syscall0(int, setup);


static inline syscall1(int, uselib,char*,library);
static inline syscall1(int, nice,int,increment);
static inline syscall1(int, brk,int,brk);

// 文件
static inline syscall2(int, creat,const char*,pathname,int,type);
static inline syscall1(int, rmdir,const char*,pathname);
static inline syscall2(int, mkdir,const char*,pathname,int,mode);
static inline syscall3(int, open,const char*,filename,int,mode,int,flags);
static inline syscall3(int, read,long,fd,char*,buf,int,count);
static inline syscall3(int, write,long,fd,char*,buf,int,count);
static inline syscall3(int, lseek,long,fd,long,offset,int,origin);
static inline syscall1(int, close,int,fd);

// 装载文件系统和卸载文件系统
static inline syscall0(int, umount);
static inline syscall0(int, mount);
static inline syscall0(int, mount_root);

// 装载进程
static inline syscall1(int, execve,const char*,filename);


static inline syscall2(int,ttywrite, char*, buf, int,nr);
static inline syscall2(int,ttyread, char*, buf, int, nr);



/* --------用户模式下的常用接口--------- */

static char buf[1024];
extern int vsprintf(char *buf, char *fmt, char *args);

int printf(char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);

    i = vsprintf(buf, fmt, args);
	va_end(args);

    ttywrite(buf, i);
	
	return i;
}

void printbuffer(char *buf, int count)
{
	ttywrite(buf, count);
}