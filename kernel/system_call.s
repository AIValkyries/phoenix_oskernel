[BITS 32]

[section .data]

USER_CS equ 0xf
USER_DS equ 0x17
KERNEL_CS equ 0x08
KERNEL_DS equ 0x10


[section .text]

extern task_table
extern do_fork
extern do_execve
extern do_exit
extern do_pause
extern do_nice
extern set_brk
extern do_open
extern do_read
extern do_write
extern do_close
extern do_create
extern do_mkdir
extern do_rmdir
extern do_mount_root
extern do_mount
extern do_umount
extern do_uselib
extern do_lseek
extern SET_SYSTEM_GATE
extern current
extern need_resched
extern schedule
extern idt

extern tty_read
extern tty_write

global system_call_init
global eixt_sys_call
global exit_switch
global sys_fork
global sys_execve
global sys_exit
global sys_pause
global sys_nice
global sys_brk
global sys_open
global sys_read
global sys_write
global sys_close
global sys_mkdir
global sys_rmdir
global sys_mount_root
global sys_mount
global sys_umount
global sys_uselib
global sys_lseek
global sys_setup
global sys_creat


; 系统调用出错码
bad_sys_call:  
    push 38 
    jmp ret_from_sys_call

reschedule:        ; 任务调度
    push eixt_sys_call
    jmp  schedule

; eax = 功能号, 最多三个参数 ebx,ecx,edx
do_system_call:
    push ds
    push es
    push fs
    
    ; 入栈顺序是 GNU gcc 规定的
    push edx
    push ecx
    push ebx
    
    mov edx, KERNEL_DS
    mov ds, edx
    mov es, edx
    
    mov edx, USER_DS
    mov fs, edx      ; 指向用户空间

    ;cmp eax, 19
    ;jae bad_sys_call        ;高于或等于跳转

    cmp eax, 0
    je sys_setup

    cmp eax,1
    je sys_fork

    cmp eax, 2
    je sys_execve

    cmp eax, 3
    je sys_exit

    cmp eax, 4
    je sys_pause

    cmp eax, 5
    je sys_nice

    cmp eax, 6
    je sys_brk

    cmp eax, 7
    je sys_open

    cmp eax, 8
    je sys_read

    cmp eax, 9
    je sys_write

    cmp eax, 10
    je sys_close

    cmp eax, 11
    je sys_creat

    cmp eax, 12
    je sys_mkdir

    cmp eax, 13
    je sys_rmdir

    cmp eax, 14
    je sys_mount_root

    cmp eax, 15
    je sys_mount

    cmp eax, 16
    je sys_umount

    cmp eax, 17
    je sys_uselib

    cmp eax, 18
    je sys_lseek

    cmp eax, 19
    je sys_ttyread

    cmp eax, 20
    je sys_ttywrite

; 以下这段代码执行从系统调用C函数返回后 | 对信号进行识别处理。
; 其他中断服务程序退出时也将跳转到这里进行处理后才退出中断过程
; 内核态执行时不可抢占   
ret_from_sys_call:
    push eax
    mov eax, current
    mov ebx, task_table
    cmp ebx, eax
    je eixt_sys_call           ; 是否是进程0
    
    ; 是用户任务吗？通过代码段
    ;cmp cs, USER_CS
    ;jne a_3              ; 如果是在内核中发生系统调用

    cmp eax,  1                 ; 如何当前进程不为 TASK_RUNNING ，则进行调度
    je schedule

    mov eax, [need_resched]       ; 是否需要调度
    cmp eax, 1
    je schedule

eixt_sys_call:
    pop eax
    pop ebx
    pop ecx
    pop edx
    pop fs
    pop es
    pop ds
    
    iret

; 进程系统调用
sys_fork:           ; 创建进程
    call do_fork
    jmp ret_from_sys_call

sys_execve:
    lea eax,[esp+24]
    push eax
    call do_execve
    pop eax
    jmp ret_from_sys_call

sys_exit:
    call do_exit
    jmp ret_from_sys_call

sys_pause:
    call do_pause
    jmp ret_from_sys_call

; 优先级更改
sys_nice:
    call do_nice
    jmp ret_from_sys_call

sys_brk:
    call set_brk
    jmp ret_from_sys_call

; 文件模块的系统调用
sys_open:
    call do_open
    jmp ret_from_sys_call

sys_read:
    call do_read
    jmp ret_from_sys_call

sys_write:
    call do_write
    jmp ret_from_sys_call

sys_close:
    call do_close
    jmp ret_from_sys_call

sys_creat:
    call do_create
    jmp ret_from_sys_call

sys_mkdir:
    call do_mkdir
    jmp ret_from_sys_call

sys_rmdir:
    call do_rmdir
    jmp ret_from_sys_call

sys_mount_root:         ; 安装跟文件系统
    call do_mount_root
    jmp ret_from_sys_call

sys_mount:
    call do_mount
    jmp ret_from_sys_call

sys_umount:
    call do_umount
    jmp ret_from_sys_call

sys_uselib:
    call do_uselib
    jmp ret_from_sys_call

sys_lseek:
    call do_lseek
    jmp ret_from_sys_call

sys_ttywrite:
    call tty_write
    jmp ret_from_sys_call

sys_ttyread:
    call tty_read
    jmp  ret_from_sys_call

sys_setup:
    jmp ret_from_sys_call

; 1110 1100 0000 0000
system_call_init:
    ; 系统调用
    mov dx, 0x80 
    mov eax, do_system_call     ;0x6d7e
    call dword SET_SYSTEM_GATE

    mov [idt+0x400],eax         ; 86d7e
    mov [idt+0x404],edx         ; ef00
    
    ret