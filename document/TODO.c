文件搜索 name_inode.c find_entry 中还有问题 TODO


find_share_vm_struct 函数未实现


echo "/tmp/corefile-%e">/proc/sys/kernel/core_pattern
cat /proc/sys/kernel/core_pattern

dd if=../tools/system_minix.img of=/opt/bochs/system.img bs=4096 seek=2048 count=65520 conv=notrunc

http://petpwiuta.github.io/2020/05/09/Bochs%E8%B0%83%E8%AF%95%E5%B8%B8%E7%94%A8%E5%91%BD%E4%BB%A4/

bochs -f ./bochsrc
bochs-gdb -q -f bochsrcgdb
 
352 个 inode
1023 个块
首个数据区=15 (15)
区大小=1024
最大尺寸=268966912


文件系统        大小  已用  可用 已用% 挂载点
/dev/loop16    1008K  7.0K 1001K    1% /media/lonely/disk

// 5000265
// 10100 0000000000 001001100101
 

https://blog.csdn.net/leacock1991/article/details/113575966


dd bs=1024 if=/dev/zero of=rootram.img count=1024

sudo losetup /dev/loop50 system.img
sudo fdisk /dev/loop50

sudo losetup -f -o 512 --sizelimit 1048064 rootram.img
sudo losetup -f -o 68157440 --sizelimit 135265792 system.img
sudo losetup -f -o 135266304 --sizelimit 202374656 system.img
sudo losetup -f -o 202375168 --sizelimit 268369408 system.img

sudo mkfs.minix /dev/loop50
sudo losetup -d /dev/loop50

Project Manager
gdb system
{
    break _start
    target remote localhost:1234
    c
}
dd if=/media/lonely/新加卷/工程实践/OS/system_img.bin of=/opt/bochs/system.img bs=512 seek=1 count=115 conv=notrunc


{
    67a2
    110=6 011110100010

}

core_duo_t2400_yonah
gdbstub: enable=1, port=1234, text_base=0, data_base=0, bss_base=0
// 非 GDB的
./configure --with-x11 --with-wx --enable-debugger --enable-disasm --enable-all-optimizations --enable-readline --enable-long-phy-address --enable-ltdl-install --enable-idle-hack --enable-plugins --enable-a20-pin --enable-x86-64 --enable-smp --enable-cpu-level=6 --enable-large-ramfile --enable-repeat-speedups --enable-fast-function-calls  --enable-handlers-chaining  --enable-trace-linking --enable-configurable-msrs --enable-show-ips  --enable-debugger-gui --enable-iodebug --enable-logging --enable-assert-checks --enable-fpu --enable-vmx=2 --enable-svm --enable-3dnow --enable-alignment-check  --enable-monitor-mwait --enable-avx  --enable-evex --enable-x86-debugger --enable-pci

./configure --with-x11 --with-wx --enable-disasm --enable-all-optimizations --enable-readline --enable-long-phy-address --enable-ltdl-install --enable-idle-hack --enable-plugins --enable-a20-pin --enable-x86-64 --enable-cpu-level=6 --enable-large-ramfile --enable-repeat-speedups --enable-fast-function-calls  --enable-handlers-chaining  --enable-trace-linking --enable-configurable-msrs --enable-show-ips  --enable-debugger-gui --enable-iodebug --enable-logging --enable-assert-checks --enable-fpu --enable-vmx=2 --enable-svm --enable-3dnow --enable-alignment-check  --enable-monitor-mwait --enable-avx  --enable-evex --enable-x86-debugger --enable-pci --enable-usb -enable-gdb-stub
b8000 = 10111000 000000000000
        184*4+4096=4832=0x12e0

export PATH=/os_gdb/bochs/bin:$PATH
export PATH=/os_gdb/bochsgdb/bin:$PATH

B8                   B800      mov ax, #BOOTSEG

sudo apt remove bochs-gdb

https://blog.csdn.net/zxb4221v/article/details/38701151

void main()
{
    mem_init(memory_arg.main_memory_start, memory_arg.memory_end);
    buffer_init(memory_arg.buffer_memory_end);

    // 中断 | 异常 | 陷阱
    exception_init();
    interupt_init();
    system_call_init();

    // 驱动设备
    con_init();             // 键盘和显示器 init
    hd_init((void*)sys_arg_addr);
    
    sched_init();

    // 在用户空间中执行
    if(fork() != 0)
        init();	
}

内存接口
{
    mem_init()  // 0x08051e00
    {
        
    }
    伙伴系统
    {
        init_buddy_pages()  // 
        __free_pages___
        __alloc_pages
    }
    Slab 
    {
        kmem_cache_init
        kfree
        kmalloc

        kmem_cache_destroy
        slab_destroy
        kmem_object_free
        kmem_object_alloc
    }
    内存映射
    {
        物理内存映射
        {
            do_mmap     // 关联映射
            do_munmap   // 解开映射
            exit_mmap
            set_brk
        }
        filemap - 文件映射
        {
            generic_mmap
            filemap_nopage
        }
    }

    execve.c    // 加载可执行文件
    {
        do_execve
    }
}

文件
{
    do_open();
    do_close();
    do_lseek();
    do_read();
    do_write();

    do_create();    // 创建文件
    do_rmdir();     // 删除文件夹
    do_mkdir();     // 创建文件夹    


    inode and block alloc and free 
    {
        free_block()
        new_block()
        new_inode()
        free_inode()
    }

    do_mount_root();
}

drivers
{
    console
    keyboard
}

kernel
{
    do_fork()
    exit();
    do_nice();

    // 睡眠队列，唤醒
    sleep_on();
    wake_up();
    sem_down();
    sem_up();

    // 过期对列，运行队列

    调度
    {
        schedule();         // 
        scheduler_tick();   // real time tick
        add_timer();
    }
}