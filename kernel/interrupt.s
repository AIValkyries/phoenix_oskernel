[BITS 32]
;[CPU 386]
; 设置 idt
; 设置 GDT


USER_CS   equ 0xf
USER_DS   equ 0x17
KERNEL_CS equ 0x08
KERNEL_DS equ 0x10

intr_number  dd   0   ; 中断号


time_tip db "timer idt .....",10, 13, 0; \n\r


; 设备中断
extern idt
extern gdt
extern do_keyboard_fun
extern scheduler_tick
extern hd_timeout
extern do_harddisk
extern need_resched
extern schedule
extern jiffies
extern current
extern init_user_stack


; 异常
extern do_divide_error
extern do_debug
extern do_nmi
extern do_int3
extern do_overflow
extern do_bounds
extern do_invalid_op
extern do_double_fault
extern do_coprocessor_segment_overrun
extern do_invalid_TSS
extern do_segment_not_present
extern do_stack_segment
extern do_general_protection
extern do_coprocessor_error
extern do_reserved
extern do_alignment_check
extern do_page_fault


global SET_SYSTEM_GATE
global interupt_init
global move_to_user_mode
global timer_interrupt

; 构造 ldt 表项,4个参数,使用堆栈
; 输入 : EAX = 门代码所在段内偏移地址
;        BX = 选择子
;        CX = 属性
; 返回 ： EDX:EAX
%macro MAKE_IDT 0
    mov edx, eax
    and edx, 0xffff0000                 ;得到偏移地址高16位 
    or  dx, cx                          ;组装属性部分到EDX

    and eax, 0x0000ffff                 ;得到偏移地址低16位 
    shl ebx, 16                         ; 左移16
    or  eax,ebx                         ;组装段选择子部分
%endmacro

;  ( offset_addr | n 中断号 ) 中断描述符
; 参数 = ( eax = offset_addr ) | ( dx = n 中断号 )
%macro SET_IDT_GATE 0
    ;  DPL(0) | TYPE(14)  | 0x0008 = 段选择符
    mov ecx, 0x8e00       ; 属性
    mov ebx, KERNEL_CS    ; 选择子
    MAKE_IDT              ; 构建 idt 项，返回 EDX:EAX
%endmacro

;  DPL(3) | TYPE (15) 系统调用门
;  (eax = offset_addr) | (dx = n 索引)
SET_SYSTEM_GATE:
    mov ecx, 0xef00
    mov ebx ,KERNEL_CS
    MAKE_IDT
    ret

;  DPL(0) | TYPE(15)  | 0x0008 = 段选择符  陷阱描述符
;  ( eax = offset_addr ) | ( dx = n 索引 )
SET_TRAP_GATE:
    mov ecx, 0x8f00      ; 32位中断门，0特权级
    mov ebx, KERNEL_CS   ; 门代码所在段的选择子 0x08

    ; 输入 : EAX = 门代码所在段内偏移地址
    ;        BX = 选择子
    ;        CX = 属性
    ; 返回 ： EDX:EAX
    MAKE_IDT
    ret

; 移动到用户模式运行
; 使用指令 iret 来实现特权级的控制转移
move_to_user_mode:
    mov ebx, [esp]
    push dword 0x17                 ;   首先将堆栈段选择符 SS 入栈
    push init_user_stack            ;   然后保存的堆栈指针值 esp 入栈
    pushfd                          ;   将标志寄存器 eflags 内容入栈
    push dword 0x0f                 ;   将 task0 代码段选择符 cs 入栈
    push .af                        ;   将下标1的 offset 地址 eip 如栈
    iret                            ; 执行中断返回指令，则会跳转到下面标号 1 处    
    .af:
        mov eax,0x17                ;  初始化段寄存器指向本局部表的数据段
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax

        push ebx
        ret

extern put_string

; 8253/8254 = 100 HZ,平均 10毫秒 一次
;  时钟中断   int 0x20
timer_interrupt:
    push ds
    push es
    push fs
    push edx
    push ecx
    push ebx
    push eax

    mov eax, KERNEL_DS
    mov ds, eax
    mov es, eax

    mov eax, USER_DS
    mov fs, eax
    
    inc dword [jiffies]     ; 系统嘀嗒

    mov al,0x20
    out 0xa0,al     ; 向从片发送
    out 0x20,al     ; 向主片发送

    call scheduler_tick

    mov eax, [need_resched]   ; 是否需要调度
    cmp eax,1
    je schedule

    pop eax
    pop ebx
    pop ecx
    pop edx
    pop fs
    pop es
    pop ds
    
    iret

;  硬盘中断  int 46
hd_interrupt:      ; 硬盘中断
    push eax
    push ecx
    push edx
    push ds
    push es
    push fs

    ; 内核段
    mov eax, KERNEL_DS
    mov ds, ax
    mov es, ax

    mov eax, USER_DS
    mov fs,  ax

    ; 发送 EOI
    mov al,0x20
    out 0xa0,al ; 向从片发送
    out 0x20,al     ; 向主片发送
    
    jmp a_1
    jmp a_1     ; 这里 jmp 起到延时作用
a_1:  xor  edx, edx
    mov  edx, [hd_timeout]
    call do_harddisk
a_2:  out 0x20, al              ; 送 8259A 主芯片 EOI 指令(结束硬件中断)
    pop fs
    pop es
    pop ds
    pop edx
    pop ecx
    pop eax
    iret

; 键盘中断, al = 扫描码
keyboard_interrupt:     
    push eax
    push ecx
    push edx
    push ds
    push es
    push fs

    ; 内核段
    mov eax, KERNEL_DS
    mov ds, ax
    mov es, ax

    ; 用户段
    mov eax, USER_DS
    mov fs, ax

    xor eax, eax
    in  al, 0x60            ; 读取扫描码
    ; 如果 al 最高位是1， 表示松开
    push ax                 ; 扫描码入栈 al = 扫描码
    call do_keyboard_fun    ; 键盘按键处理函数
    
    ; 发送 EOI
    mov al,0x20
    out 0xa0,al ; 向从片发送
    out 0x20,al     ; 向主片发送
    
    pop fs
    pop es
    pop ds
    pop edx
    pop ecx
    pop eax
    iret


; 中断初始化 -mcmodel=large
interupt_init:
    ; 时钟中断
    mov edx, 0x20
    mov eax, timer_interrupt
    SET_IDT_GATE
    
    mov [idt+0x20*8],eax
    mov [idt+0x20*8+4],edx

    ; 硬盘中断
    mov edx, 0x76
    mov eax, hd_interrupt
    SET_IDT_GATE

    mov [idt+0x3B0],eax
    mov [idt+0x3B4],edx

    ; 键盘中断
    mov edx, 0x21
    mov eax, keyboard_interrupt
    SET_IDT_GATE

    mov [idt+0x108],eax
    mov [idt+0x10c],edx

    ret


%macro SAVE_ALL 0
    cld
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    push eax
    push ds
    push es
    push fs
    push gs
    pushfd      ; 标志寄存器入栈
    push esp
    push ss 
    ;lea eax, [esp+48]   ; 指向第一行 push 的地址
%endmacro

%macro RESET_ALL 0
    pop ss
    pop esp
    popfd
    pop gs
    pop fs
    pop es
    pop ds
    pop eax
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
%endmacro


divide_error:
    SAVE_ALL
    call do_divide_error
    RESET_ALL
    iret

debug:                          ; int1--debug调试中断入口点，处理过程同上
    SAVE_ALL
    call do_debug
    RESET_ALL
    iret

nmi:                            ; int2---非屏蔽中断调用入口点
    SAVE_ALL
    call do_nmi
    RESET_ALL
    iret

break_point:                    ; int3--断点指令引起中断的入口点
    SAVE_ALL
    call do_int3
    RESET_ALL
    iret

over_flow:                      ; int4--溢出出错处理中断入口点
    SAVE_ALL
    call do_overflow
    RESET_ALL
    iret

bounds_check:                   ; int5 寻址到有效地址以外时引起
    SAVE_ALL
    call do_bounds
    RESET_ALL
    iret

invalid_opcode:                 ; int6 CPU执行一条无效指令引起
    SAVE_ALL
    call do_invalid_op
    RESET_ALL
    iret

double_fault:                   ; int8--双出错故障
    SAVE_ALL
    call do_double_fault
    RESET_ALL
    iret

coprocessor_segment_overrun:    ; int9 协处理器的异常
    SAVE_ALL
    call do_coprocessor_segment_overrun
    RESET_ALL
    iret

invalid_TSS:                    ; int10 CPU切换时发觉TSS无效
    SAVE_ALL
    call do_invalid_TSS
    RESET_ALL
    iret

segment_not_present:            ; int11 描述符所指的段不存在
    SAVE_ALL
    call do_segment_not_present
    RESET_ALL
    iret

stack_segment:                  ; int12 堆栈段不存在，或寻址越出堆栈段
    SAVE_ALL
    call do_stack_segment
    RESET_ALL
    iret

general_protection:             ; int13 没有复合 80386 保护机制 (特权级) 的操作引起
    push esp
    call do_general_protection
    pop esp
    iret

coprocessor_error:              ; 16 协处理器发出的出错信号
    SAVE_ALL
    call do_coprocessor_error
    RESET_ALL
    iret

reserved:                       ; int15---qita intel 保留中断入口点
    SAVE_ALL
    call do_reserved
    RESET_ALL
    iret

; int17---边界对齐检查出错
alignment_check:
    push 0
    push esp
    call do_alignment_check
    add esp, 8
    iret

; 页出错异常
page_fault:     
    xchg eax, [esp]
    push ecx
    push edx
    push ds
    push es
    push fs
 
    ; 内核段
    mov edx, KERNEL_DS
    mov ds,dx
    mov es,dx
    mov fs,dx

    mov edx, cr2
    push edx
	push eax

    call do_page_fault

    pop eax
    pop edx
    pop fs
    pop es
    pop ds
    pop edx
    pop ecx
    pop eax

    iret


global exception_init

; 异常初始化 注册中断表
exception_init: 
        ;设置8259A中断控制器,由 ICW1 编程为级联方式，那么还必须编程ICW3
        ; 如果单个8259A用于系统中，则ICW1,ICW2,ICW4必须被编程
        ; 如果是级联的方式，则4个ICW都需要被编程
        ; 从 0x20/0xA0端口接受命令字 ICW1后，期待从0x21/0xA1端口接受命令字ICW2
        ; 但是他是否期待ICW3和ICW4,还要看ICW1的内容
         mov al,0x11                        ; 期待ICW4                   
         out 0x20,al                        ; ICW1：边沿触发/级联方式
         
         mov al,0x20                        ; 设置起始向量，0x20
         out 0x21,al                        ; ICW2:起始中断向量
         
         mov al,0x04
         out 0x21,al                        ;ICW3:从片级联到IR2
         
         mov al,0x01
         out 0x21,al                        ;ICW4:非总线缓冲，全嵌套，正常EOI

         ; 从片设置
         mov al,0x11
         out 0xa0,al                        ;ICW1：边沿触发/级联方式
         mov al,0x70                        ;起始向量 0x70
         out 0xa1,al                        ;ICW2:起始中断向量
         mov al,0x04
         out 0xa1,al                        ;ICW3:从片级联到IR2
         mov al,0x01
         out 0xa1,al                        ;ICW4:非总线缓冲，全嵌套，正常EOI

         ; 屏蔽所有的中断
         mov al,0xFF
         out 0x21,al
         out 0xa1,al

    mov edx,0
    mov eax, divide_error
    call SET_TRAP_GATE

     ; 初始化中断
     mov ebx, idt
     xor esi,esi
   .idt0:
       mov [ebx+esi*8],eax
       mov [ebx+esi*8+4],edx
       inc esi
       cmp esi,256
       jle .idt0     

    mov edx,0
    mov eax, divide_error
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt],eax
    mov [idt+4],edx

    mov edx,1
    mov eax, debug
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+8],eax
    mov [idt+12],edx

    mov edx,2
    mov eax, nmi
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+16],eax
    mov [idt+20],edx

    mov edx,3
    mov eax, break_point
    call SET_SYSTEM_GATE

    ; 设置中断描述符
    mov [idt+24],eax
    mov [idt+28],edx

    mov edx,4
    mov eax, over_flow
    call SET_SYSTEM_GATE

    ; 设置中断描述符
    mov [idt+32],eax
    mov [idt+36],edx

    mov edx,5
    mov eax, bounds_check
    call SET_SYSTEM_GATE

    ; 设置中断描述符
    mov [idt+40],eax
    mov [idt+44],edx

    mov edx,6
    mov eax, invalid_opcode
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+48],eax
    mov [idt+52],edx

    ;mov dx, 7
    ;mov eax, device_not_available
    ;call set_trap_gate_os
    

    mov edx, 8
    mov eax, double_fault
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+64],eax
    mov [idt+68],edx

    mov edx, 9
    mov eax, coprocessor_segment_overrun
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+72],eax
    mov [idt+76],edx

    mov edx, 10
    mov eax, invalid_TSS
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+80],eax
    mov [idt+84],edx

    mov edx, 11
    mov eax, segment_not_present
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+88],eax
    mov [idt+92],edx

    mov edx, 12
    mov eax, stack_segment
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+96],eax
    mov [idt+100],edx
	
    mov edx, 13
    mov eax, general_protection
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+104],eax
    mov [idt+108],edx

    ; 页异常处理
    mov edx,14
    mov eax,page_fault
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+112],eax
    mov [idt+116],edx

    mov edx, 15
    mov eax, reserved
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+120],eax
    mov [idt+124],edx

    mov edx, 16
    mov eax, coprocessor_error
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+128],eax
    mov [idt+132],edx

    mov edx, 17
    mov eax, alignment_check
    call SET_TRAP_GATE

    ; 设置中断描述符
    mov [idt+136],eax
    mov [idt+140],edx

    ret