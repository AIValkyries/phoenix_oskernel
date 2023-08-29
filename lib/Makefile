## 生成静态库 .a

## -Wall 警告选项
# -O 编译器会试图减少目标代码的大小和执行时间
# -fstrength-reduce 执行循环强度缩小优化，并且消除重复变量
# -fomit-frame-pointer 对不需要帧指针的函数，不要在寄存器中保存帧指针
# -finline-functions	把所有简单的函数集成进调用者，编译器探索地决定哪些函数足够简单，值得这种集成
# -nostdinc 不要在标准系统目录中找到头文件，只搜索 '-I' 选项指定的目录
CFLAGS = -m32 -Wall -O -fstrength-reduce -fomit-frame-pointer -no-pie -fno-pic		\
		-fno-builtin -nostdinc -I../include
CC =gcc -m32
LD =ld -m elf_i386

OBJS  = sys_lib.o

.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $<

lib.o: $(OBJS)
	$(LD) -r -o lib.o $(OBJS)
	make clean
	sync

clean:
	@-rm -rf sys_lib.o vprintf.o
