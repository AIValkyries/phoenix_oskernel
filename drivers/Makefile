
# -Wall = 输出警告
# -fstrength-reduce    = 执行循环强度缩小优化，并且消除重复变量
# -fomit-frame-pointer = 对于不需要帧指针的函数，不要在寄存器中保存帧指针
# -finline-functions   = 把所有简单的函数集成进调用者
CFLAGS =-Wall -O -m32 -fomit-frame-pointer -fno-stack-protector -nostdinc -no-pie -fno-pic -I../include
CC =gcc -m32
LD =ld -m elf_i386
AS =as -m elf_i386
AR =ar -m elf_i386

.c.s:
	$(CC) $(CFLAGS) \
	-S -o $*.s $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $<

# block_buffer.o console.o hd_device.o hd_request_queue.o keyboard.o
OBJS	= block_buffer.o console.o hd_device.o hd_request_queue.o keyboard.o tty.o

fs.a: $(OBJS)
	$(LD) -r -o drivers.o $(OBJS)
	make clean
	sync

clean:
	@-rm -rf $(OBJS)