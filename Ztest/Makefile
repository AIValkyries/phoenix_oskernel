CFLAGS =-Wall -O -m32 -fomit-frame-pointer -fno-builtin -fno-stack-protector \
	-ffunction-sections -fdata-sections -nostdinc -no-pie -fno-pic -I../include
CC =gcc -m32
LD =ld --gc-sections -e main -m elf_i386 -Ttext 0x1000000

.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $<

OBJS  = ../lib/lib.o OS_test_task.o
OBJS2 = ../lib/lib.o OS_test_task2.o

all.o: $(OBJS) $(OBJS2)
	$(LD) $(OBJS0) $(OBJS) -o OS_test_task
	$(LD) $(OBJS0) $(OBJS2) -o OS_test_task2
	make clean
	sync

clean:
	@-rm -rf $(OBJS) $(OBJS2)