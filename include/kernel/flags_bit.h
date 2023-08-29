// 标志

#ifndef _FLAGS_BIT_H
#define _FLAGS_BIT_H

/*
    INT 中断 跳转之前，清除中断标志位，并将标志寄存器、CS和IP入栈

    lock 锁定系统总线

    NEG 取反 计算目的操作数的补数，并将结果保存到目的操作数 NEG reg
    NOP 空指令 
    NOT 非，对操作数进行逻辑 NOT 操作，按每一位取反 NOT reg | NOT mem
    OR  或，每一位按 OR 操作，OR reg ,reg | 

    TEST 测试
        TEST REG,REG 对目的操作数和源操作数中的每组对应位进行测试，执行 AND 操作，只影响标志位，不影响目的操作数
*/

#define hlt() __asm__("hlt"::)  // 停机 暂停CPU直到出现一个硬件中断
#define clc() __asm__("clc"::)  // 清除进位标志位
#define cld() __asm__("cld"::)  // 清除方向标志位
#define cmc() __asm__("cmc"::)  // 进位标志位求补,取反进位标志位的值

#define stc() __asm__("stc"::)
#define std() __asm__("std"::)


#define cli() __asm__("cli"::)   // 禁止中断
#define sti() __asm__("sti"::)   // 中断位 = 1，允许中断
#define iret() __asm__("iret"::) // 中断返回，从中断处理器返回，将栈顶内容送入 IP/CS和标志寄存器


/*
;    位扫描 BSF / BSR reg16,r/m16 
;    搜索第一个等于1的位。若存在，则清除零标志位，并把该位的位号（索引）赋给目的操作数
;        若不存在，则 ZF=1，BSF = 从0位到高位；BSR = 从最高位到0位
; 
;    字节交换(x86) BSWAP reg32
;         反转32位目的寄存器的字节顺序
;     
;     BT 位测试
      BTC 位测试并取反
      BTR 位测试并复位
      BTS 位测试并设置
      影响 CF 标志位
      BT r/m16,imm8 将指定位(n)赋值到进位标志位，
                        目的操作为包含指定位的数值        imm8 = 数值
;                       源操作数为该位在目的操作数中的位置 r/m16
*/
   

/*
    将 1024 字节内存清零
    stosb 将al复制到内存
    stosw 将ax复制到内存
    stosd 将eax复制到内存
*/
#define clear_block(addr)({ \
__asm__("cld\n\trep\n\tstosb"::"a"(0),"c"(1024),"D"((long)(addr)));    \
})

/*
    从 addr 处开始寻找第一个 0 值位
    loadsl = 将字符串加载到累加器,将 DS：SI 指定的内存字节或字加载到累加器（AL，AX，EAX）
    notl 对操作数进行逻辑 NOT 操作，每一位取反
    BSF  扫描操作数，搜索第一个等于 1的位，若发现这样的位，则清除零标志位
        并把改位的位号赋予给目的操作数。 从位0到最高位
*/


#endif