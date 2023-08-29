
CFLAGS =-Wall -g -O -m32 -nostdinc -fno-stack-protector -no-pie -fno-pic -I../include
CC =gcc -m32
LD =ld -e main -m elf_i386 -Ttext 0x00000000
# -Ttext
OBJS  = init/init.o kernel/kernel.o drivers/drivers.o mm/mm.o \
			fs/fs.o lib/lib.o

# all: os_system

#init/init.o:
#	(cd init;make)

#kernel/kernel.o:
#	(cd kernel;make)

#drivers/drivers.o:
#	(cd drivers;make)

#mm/mm.o:
#	(cd mm;make)

#fs/fs.o:
#	(cd fs;make)  >System.map
Image: $(OBJS)
	$(LD) $(OBJS) -o tools/system
	objcopy --remove-section .eh_frame tools/system
	objdump -S tools/system > tools/system.txt
	sync

clean:
	@-rm -rf os_system.o
