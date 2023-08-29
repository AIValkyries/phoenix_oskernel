nasm -f bin boot.asm -o boot.bin -l boot.lst
dd if=/media/lonely/新加卷/工程实践/OS/boot/boot.bin of=/opt/bochs/system.img bs=512 count=1 conv=notrunc