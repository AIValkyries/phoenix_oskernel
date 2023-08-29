// out 输出到端口，向端口输出一个字或者字节
// out dx,accum  dX 包含端口号
// outS,outSB,OUTSW,OUTSD | rep OUTSW dest,DX
// 将ES:DI 指向的字符串输出到端口

// in 从端口输入一个字节到AL，或一个字到 AX，双字 EAX
#define inb(port)({\
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al":"=a" (_v):"d" (port)); \
_v; \
})


#define outb(value, port) __asm__ ("outb %%al,%%dx"::"a" (value),"d" (port)) 

/*
带延迟的硬件端口字节输出函数，使用两条跳转语句来延迟一会
*/
#define outb_p(value,port) \
__asm__ ("outb %%al,%%dx\n" \
		"\tjmp 1f\n" \
		"1:\tjmp 1f\n" \
		"1:"::"a" (value),"d" (port))

// 从指定设备端口读取指定数据
#define inb_p(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al\n" \
	"\tjmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:":"=a" (_v):"d" (port)); \
_v; \
})


// INS | INSB | INSW | INSD  rep INSW dest,dx
//  从端口输入字符串到 ES:DI,DX指定端口号，可以和rep一起使用 ，rep 和 ecx一起使用


// req insw dest,dx 从端口输入一个字符串到ES:DI ，dx指定端口号
// cld = 清除方向标志 rep：ecx = 0 且标志位条件为真,则重复
#define port_read(port,buf,nr) __asm__("cld\n\trep\n\tinsw"::"d"(port),"D"(buf),"c"(nr))
// req outsw dest, dx；端口号由 dx 指定，将ES:DI指向的字符串输出到端口
#define port_write(port,buf,nr) __asm__("cld\n\trep\n\toutsw"::"d"(port),"S"(buf),"c"(nr))
