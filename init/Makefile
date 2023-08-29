
# -Wall = 输出警告
# -fstrength-reduce    = 执行循环强度缩小优化，并且消除重复变量
# -fomit-frame-pointer = 对于不需要帧指针的函数，不要在寄存器中保存帧指针
# -finline-functions   = 把所有简单的函数集成进调用者 
CFLAGS =-Wall -O -m32 -fomit-frame-pointer -fno-builtin -fno-stack-protector -nostdinc -no-pie -fno-pic -I../include
CC =gcc -m32
LD =ld -m elf_i386

NASM =nasm -f elf

.c.s:
	$(CC) $(CFLAGS) \
	-S -o $*.s $<
.s.o:
	$(NASM) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $<

OBJS  =head.o main.o

init.o: $(OBJS)
	$(LD) -r -o init.o $(OBJS)
	make clean
	sync

clean:
	@-rm -rf $(OBJS)

cleanAll:
	@-rm -rf $(OBJS) init.o

