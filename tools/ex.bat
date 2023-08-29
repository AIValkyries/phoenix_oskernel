gcc -m32 build.c -o build
./build

dd if=../boot/boot.bin of=/opt/bochs/system.img bs=512 count=1 conv=notrunc
dd if=../tools/system.bin of=/opt/bochs/system.img bs=512 seek=1 count=200 conv=notrunc
dd if=../tools/rootram.img of=/opt/bochs/system.img bs=512 seek=255 count=2048 conv=notrunc