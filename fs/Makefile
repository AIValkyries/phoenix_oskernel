# -fno-stack-protector
CFLAGS =-Wall -O -m32 -fomit-frame-pointer -fno-stack-protector -nostdinc -no-pie -fno-pic -I../include

CC =gcc -m32
LD =ld -m elf_i386
AS =as -m elf_i386
AR =ar -m elf_i386

## 文件模块编译

.c.s:
	$(CC) $(CFLAGS) \
	-S -o $*.s $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $<

OBJS	= bitmap.o block_dev.o execve.o file_dev.o file_table.o	\
			inode.o name_inode.o super.o sys_file.o 

fs.a: $(OBJS)
	$(LD) -r -o fs.o $(OBJS)
	make clean
	sync

clean:
	@-rm -rf $(OBJS)

