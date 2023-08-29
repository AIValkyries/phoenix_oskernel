
cd init;make
cd ..
cd kernel;make
cd ..
cd drivers;make
cd ..
cd mm;make
cd ..
cd fs;make
cd ..

make

cd tools
./build

cd ..


dd if=/media/lonely/新加卷/工程实践/OS/boot/boot.bin of=/opt/bochs/system.img bs=512 count=1 conv=notrunc
dd if=/media/lonely/新加卷/工程实践/OS/tools/system.bin of=/opt/bochs/system.img bs=512 seek=1 count=200 conv=notrunc