# -Wall = 输出警告
# -fstrength-reduce    = 执行循环强度缩小优化，并且消除重复变量
# -fomit-frame-pointer = 对于不需要帧指针的函数，不要在寄存器中保存帧指针
# -finline-functions   = 把所有简单的函数集成进调用者 -finline-functions -fstrength-reduce
CFLAGS =-Wall -O -m32 -fomit-frame-pointer -fno-stack-protector -nostdinc -no-pie -fno-pic -I../include
CC =gcc -m32
LD =ld -m elf_i386
AS =as

NASM =nasm -f elf

## nasm -f elf -o foo.o foo.s
## nasm -f elf -o exception.o exception.asm
## nasm -f elf -o system_call.o system_call.asm
## 内核模块编译

.c.s:
	$(CC) $(CFLAGS) \
	-S -o $*.s $<
.s.o:
	$(NASM) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $<

#  interrupt.o exception.o system_call.o
# process.o sched_os.o trap.o vprintf.o wait.o atomic.o exit.o fork.o
OBJS  = interrupt.o system_call.o	\
	process.o sched_os.o trap.o vprintf.o wait.o atomic.o exit.o fork.o

kernel.o: $(OBJS)
	$(LD) -r -o kernel.o $(OBJS)
	make clean
	sync

clean:
	@-rm -rf $(OBJS)

cleanAll:
	@-rm -rf $(OBJS) kernel.o