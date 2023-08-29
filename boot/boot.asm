; 被 BIOS 读入到内存绝对地址  0x7C00 (31 KB) 处
; 读取内核


BOOTSEG   equ 0x7c0
INITSEG   equ 0x5000           ; 将自己移动到 0x50000 (512 kib) 处
SYSTEMSEG equ 0x00000000        ; 加载到 [addr = 0] 处
CORE_TOTAL_NSEC equ 118

CORE_START_SECTOR equ 0x00000001

start:
    cli
    mov ax, 3
    int 0x10

    mov ax, INITSEG
    mov ss, ax
    mov sp, 0xffff

    ; 将自己 copy 到 0x50000 (320kib)处
    mov ax, BOOTSEG
    mov ds, ax
    mov ax, INITSEG
    mov es, ax
    sub si,si
    sub di,di
    mov cx, 256
    rep 
    movsw
    
    jmp dword INITSEG:go               

; 加载 system 模块,处于 0x50000 处
go:
    ; 提示信息
    mov si, loading_sys_msg
    call print

    mov ecx, CORE_TOTAL_NSEC      ; 总磁盘数量 = 127
    xor edx, edx
    mov ds,  edx
    
    ; 加载内核代码
    ; mov ecx, eax
    mov eax, CORE_START_SECTOR  ; 起始扇区号
    mov ebx, SYSTEMSEG          ; 内核起始位置
    .b1:
        call load_system
        inc  eax
        loop .b1
    
    ; 将自己 data(e000-1000=0xd000) copy to 0x11000 处
    ; data 位于 0x11000 物理地址处
    mov ax, 0xe00
    mov ds, ax
    mov ax, 0x1100
    mov es, ax
    sub si,si
    sub di,di
    mov cx, 2048
    rep 
    movsw

    jmp dword ok_load_system

; 加载内核代码,eax=逻辑扇区号
load_system:
    push eax 
    push ecx
    push edx

    push eax    ; 存储了逻辑扇区号

    ; 写入扇区数, 逻辑扇区号
    mov dx, 0x1f2
    mov al, 1
    out dx, al

    ; 起始扇区号 = 0x1f3
    inc dx
    pop eax
    out dx,al

    inc dx
    mov cl,8
    shr eax,cl
    out dx,al                          ;LBA地址15~8

    inc dx                             ;0x1f5
    shr eax,cl
    out dx,al                          ;LBA地址23~16

    inc dx                             ;0x1f6
    shr eax,cl
    or al,0xe0                         ;第一硬盘  LBA地址27~24
    out dx,al

    inc dx                             ;0x1f7
    mov al,0x20                        ;读命令
    out dx,al

    .waits:
        in al,dx
        and al,0x88
        cmp al,0x08
        jnz .waits                       ; 不忙，且硬盘已准备好数据传输 

        mov ecx,256                      ; 总共要读取的字数
        mov dx,0x1f0                     ; 数据端口
    .readw:
        in ax,dx
        mov [ebx],ax
        add ebx,2
        loop .readw

    pop edx
    pop ecx
    pop eax
      
    ret


ok_load_system:
    mov ax, INITSEG
    mov ds, ax
    lgdt [ds:pgdt]

    in al, 0x92                         ;南桥芯片内的端口 
    or al, 0000_0010B
    out 0x92, al                        ;打开A20

    mov eax, cr0                  
    or  eax, 1
    mov cr0, eax                        ;设置PE位

    ;hlt
    ;infi: jmp near infi 
    jmp dword 0x0008:0x640c             ; head 代码开始的地方

print:
    mov ah, 0x0e
.next:
    mov al, [si]
    cmp al, 0
    jz .done
    int 0x10
    inc si
    jmp .next
.done:
    ret

gdt  dq  0x0000000000000000     ; 8 个字节  
     dq  0x00cf9a0000000fff     ; 内核代码段
     dq  0x00cf920000000fff     ; 内核数据段
     ;dq  0x00cb9a000000ffff     ; 用户代码?    0x18 f
     ;dq  0x00cb92000000ffff     ; 用户数据?    0x20 f

pgdt dw 0x800       ; 限长 
     dd 0x50000+gdt   ; 全局符号表的地址  


loading_sys_msg db 'loading system start....',10, 13, 0; \n\r
; prepare entry protection mode....
protection_mode db 'prepare entry protection mode....',10, 13, 0; \n\r
end:

times 510-($-$$) db 0
      db 0x55,0xaa