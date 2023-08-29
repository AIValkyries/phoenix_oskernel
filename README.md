### 1.设备中断类型
8259 主片 IR0～IR7  主片端口号  0x20-0x21
从片      IR0～IR7  从片端口号  0xa0-0xa1
| 中断请求号 |  中断号   |  名称    |
| :----     |  :---     |  :----  |
| IRQ1      |    0x21   | 键盘中断 |
| IRQ8      |    0x28   | 时钟中断 |
| IRQ14     |    0x2e   | 硬盘中断 |
一旦用ICW编程了8259A，操作命令字(OCW)就用于控制8259A的操作。
[A0][M7][M6][M5][M4][M3][M2][M1][M0]  用于设置和读取中断屏蔽寄存器
状态寄存器
    中断请求寄存器 IRR
    服务寄存器 ISR
    中断屏蔽寄存器 IMR  用于指示那些中断被屏蔽了。0表示允许，1表示阻止.
    IRR 和 ISR 均通过 OCW3 读出，而IMR通过OCW1读出。为了读 IMR，需使 A0=1
    为了读 IRR和ISR，需A0=0

### 2.IBM PC I/O端口地址分配
| 端口地址范围  |  分配说明  |
| :----        | :-------  |
| 0x00-0x01F   | 8337A DMA 控制器1 |
| 0x20-0x03F   | 8259A 可编程中断控制器1 |
| 0x40-0x05F   | 8253/8254A 定时计数器 |
| 0x60-0x06F   | 8042 键盘控制器 |
| 0x70-0x07F   | CMOS RAM/RTC 端口  |
| 0x80-0x09F   | DMA 页面寄存器访问端口 |
| 0xA0-0x0BF   | 8259A 可编程中断控制器2 |
| 0xC0-0x0DF   | 8237A DMA 控制器2 |
| 0xF0-0x0FF   | 协处理器访问端口 |
| 0x1F0-0x01F7   | IDE硬盘控制器0 |
| 0x278-0x01F   | 并行打印机端口2 |
| 0x2F8-0x027F   | 串行控制器2 |
| 0x378-0x02FF   | 并行打印机端口1 |
| 0x3B0-0x037F   | 单色MDA显示控制器 |
| 0x3C0-0x03BF   | 彩色CGA显示控制器 |
| 0x3D0-0x03CF   | 彩色EGA/VGA显示控制器 |
| 0x3F0-0x03DF   | 软盘控制器 |
| 0x3F8-0x03F7   | 串行控制器1 |
| 0x170-0x0177   | IDE硬盘控制器1 |

#### 3.实现的控制器设备
    中断控制器
    定时/计数器
    键盘控制器？
    显示控制器
    硬盘控制器

### 4.系统调用过程

system_call:  
    调用具体的函数  
ret_from_sys_call:  
    reschedule // 进程是否需要调度


#### 5.进程系统调用
    sys_fork
    sys_execve
    sys_exit
    sys_pause
    sys_kill
    sys_wait
    sys_nice    调整优先级

#### 内存系统调用
    sys_brk()

#### 文件系统调用
    sys_open
    sys_read
    sys_write
    sys_close
    sys_creat 创建文件
    sys_mkdir()  创建目录
    sys_rmdir()  删除目录
#### MINIX 文件系统
    引导块 | 超级块 | inode bit map | block bit map | inodes | 内存程序[system] | rootfs 
#### root fs
    dev/ 设备文件
    bin/ 系统执行程序，例如 sh、mkfs、fdisk
    usr/ 存放库函数，手册和其他一些文件
    etc/ 目录主要含有一些系统配置文件


#### 内核内存结构视图


#### 编译流程
    预处理      cpp hello.c -o hello.i
    编译成汇编   cc -S hello.i -o hello.S
    汇编编译器   as hello.s -o hello.o      可重定位目标文件
    链接        ld  hello.o -o hello -lc    /usr/lib/crtl.o ...
    







