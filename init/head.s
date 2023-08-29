; GDT 2kib | idt 2kib | 1个内核页目录�?4个初始页�?


[BITS 32]
; 其实这也是指�?.....
pg_dir    times 1024 dd 0x00
pg0_addr  times 1024 dd 0x00
pg1_addr  times 1024 dd 0x00
pg2_addr  times 1024 dd 0x00
pg3_addr  times 1024 dd 0x00       ; 5kib
; 系统参数 0x5000
; 磁盘定义? 256 mib, 总扇区数 = 524288 �? 一个硬�?
sys_args_area   dw 32     ; 柱面�?         ; 软盘 1kib 的空�?
                db 128    ; 磁头�?/每柱�?
                dw 0 
                dw 0
                db 0
                db 0     ; 控制字节
                db 0 
                db 0
                db 0
                dw 0 
                db 128    ; 每磁道扇区数
                db 0
                ; 显卡信息  VGA 显示标准
                dw  0   ; ORIG_X
                dw  0   ; ORIG_Y    光标列号
                dw  0   ; 初始显示页面
                dw  0x01  ; 显示模式 = 彩色模式
                dw  0   ; 屏幕列数
                dw  0   ; 屏幕行数
                dw  0   ; 
                dd  0x100000   ; 显示内存大小和色彩模�?,字节�? 1 MIB
                dw  0   ; 显卡特性参�?
                times 1024-($-sys_args_area) db 0x00 

; 0x5400
gdt  dq  0x0000000000000000     ; 8 个字节?  
     dq  0x00c09a000000ffff     ; 内核代码?    0x08     256 MIB
     dq  0x00c092000000ffff     ; 内核数据?    0x10     256MIB
     times 253  dq  0x0000000000000000

; 0x5c00
idt times 256 dq 0x0000000000000000 

idt_descr dw  0x7ff
          dd  idt

gdt_descr dw  0x7ff
          dd  gdt
          
                           
;-------------------------------------开始代码段------------------------------------------

PAGE_ATTR equ 7                    ; 页属性

_start:
       mov ax, 0x10               ; 数据段选择�?
       mov ds, ax
       mov es, ax
       mov fs, ax
       mov gs, ax
       mov ss, ax
       mov esp, init_kernel_stack+4096  ; 0x10660

setup_pg_dir:
     mov dword [0],    0x1000 + PAGE_ATTR 
     mov dword [4],    0x2000 + PAGE_ATTR        ; pg_dir is at 0x0000 | P/(R.W)/(U.S)
     mov dword [8],    0x3000 + PAGE_ATTR        ; vaddr is at 3GB
     mov dword [12],   0x4000 + PAGE_ATTR 
     ; 填充页表? 总共 4096 项，4个页? = 16MiB
     mov ebx, 0x1000      ; 页表的物理地址
     xor esi, esi   
     xor edi, edi
     xor eax, eax         ; 映射的物理地址
     
     .b1:
       mov edx, eax
       or  edx, PAGE_ATTR
       mov [ebx + esi * 4], edx
       add eax, 4096     ; 映射的物理地址
       inc esi
       cmp esi, 1024
       jnz .b1           ;  不为0则跳 ?

       mov ebx, 0x2000      ; 页表的物理地址
       xor eax, eax         ; 映射的物理地址
       xor esi, esi
     .b2:
              mov edx, eax
              or  edx, PAGE_ATTR
              mov [ebx + esi * 4], edx
              add eax, 4096     ; 映射的物理地址
              inc esi
              cmp esi, 3072
              jnz .b2     ;  不为0则跳�?

     xor eax, eax
     mov cr3, eax             ; 内核页目录基地址 = 0x00
   
     mov eax,  cr0 
     or  eax,  0x80000011
     mov cr0, eax     ;开启分页机

     lgdt [gdt_descr]
     lidt [idt_descr] ; 中断描述符表  
       mov ax, 0x10               ; 数据段选择�?
       mov ds, ax
       mov es, ax
       mov fs, ax
       mov gs, ax
       mov ss, ax
       
     call main
       
     infi: jmp near infi                 ;无限循环

; EBX = 字符串地址
put_string:                                 ;显示0终止的字符串并移动光�? 
                                            ;输入�? EBX=字符串的线性地址
         push ebx
         push ecx
  .getc:
         mov cl,[ebx]
         or cl,cl                           ;检测串结束标志�?0�? 
         jz .exit                           ;显示完毕，返�? 
         call put_char
         inc ebx
         jmp .getc

  .exit:

         ;sti                                ;硬件操作完毕，开放中�?

         pop ecx
         pop ebx

         ret                               ;段间返回

;-------------------------------------------------------------------------------
put_char:                                   ;在当前光标处显示一个字�?,并推�?
                                            ;光标。仅用于段内调用 
                                            ;输入：CL=字符ASCII�? 
         push ebx
         ;以下取当前光标位�?
         mov dx,0x3d4
         mov al,0x0e
         out dx,al
         inc dx                             ;0x3d5
         in al,dx                           ;高字
         mov ah,al

         dec dx                             ;0x3d4
         mov al,0x0f
         out dx,al
         inc dx                             ;0x3d5
         in al,dx                           ;低字
         mov bx,ax                          ;BX=代表光标位置�?16位数
         and ebx,0x0000ffff                 ;准备使用32位寻址方式访问显存 
         
         cmp cl,0x0d                        ;回车符？
         jnz .put_0a                         
         
         mov ax,bx                          ;以下按回车符处理 
         mov bl,80
         div bl
         mul bl
         mov bx,ax
         jmp .set_cursor

  .put_0a:
         cmp cl,0x0a                        ;换行符？
         jnz .put_other
         add bx,80                          ;增加一行
         jmp .roll_screen

  .put_other:                               ;正常显示字符
         shl bx,1
         mov [0x000b8000+ebx], cl            ;在光标位置处显示字符 ,ebx = 光标位置

         ;以下将光标位置推进一个字�?
         shr bx,1
         inc bx

  .roll_screen:
         cmp bx,2000                        ;光标超出屏幕？滚�?
         jl .set_cursor

         cld
         mov esi,0x000b80a0                 ;小心�?32位模式下movsb/w/d 
         mov edi,0x000b8000                 ;使用的是esi/edi/ecx 
         mov ecx,1920
         rep movsd
         mov bx,3840                        ;清除屏幕最底一�?
         mov ecx,80                         ;32位程序应该使用ECX
  .cls:
         mov word [0x000b8000+ebx],0x0720
         add bx,2
         loop .cls

         mov bx,1920

  .set_cursor:
         mov dx, 0x3d4
         mov al, 0x0e
         out dx, al
         inc dx                             ;0x3d5
         mov al, bh
         out dx, al
		 
         dec dx                             ;0x3d4
         mov al, 0x0f
         out dx, al
         inc dx                             ;0x3d5
         mov al, bl
         out dx, al
         pop ebx
         ret  

open_page_tips:
       ; 激活分?
       mov ebx, active_page
       call put_string
       ret

reset_pg_dir:
       ;mov dword [0],      0x1000 + PAGE_ATTR          ; pg_dir is at 0x0000 | P/(R.W)/(U.S)
       ;mov dword [4],      0x2000 + PAGE_ATTR 
       ;mov dword [8],      0x3000 + PAGE_ATTR         ; 12 kib       
       ;mov dword [12],     0x4000 + PAGE_ATTR           ; 16 kib  0x4000     
       mov dword [16],    0x800000 + PAGE_ATTR 
       mov dword [20],    0x801000 + PAGE_ATTR 
       mov dword [24],    0x802000 + PAGE_ATTR 
       mov dword [28],    0x803000 + PAGE_ATTR      
                  
       ; 填充页表? 总共 4096 项，4个页? = 16MiB
       mov ebx, 0x00001000      
       xor esi, esi   
       xor edi, edi
       xor eax, eax         ; 映射的物理地址
       .b1:
                     mov edx, eax
                     or  edx, PAGE_ATTR
                     mov [ebx + esi * 4], edx
                     add eax, 4096       ; 映射的物理地址
                     inc esi
                     cmp esi, 4096
                     jnz .b1             ;  不为0则跳�?

       mov ebx, 0x00800000      
       xor esi, esi   
       xor edi, edi
       .b2:
                     mov edx, eax
                     or  edx, PAGE_ATTR
                     mov [ebx + esi * 4], edx
                     add eax, 4096       ; 映射的物理地址
                     inc esi
                     cmp esi, 4096
                     jnz .b2             ;  不为0则跳?
       ret

; eax=存储了逻辑扇区号
read_disk:
    push eax 

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
      
    ret

write_disk:
    push eax 

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
    mov al,0x30                        ;写命令
    out dx,al
      
    ret


active_page db "active page .....",10, 13, 0; \n\r
end_str:

extern main
extern init_kernel_stack
global gdt
global idt
global _start
global reset_pg_dir
global put_string
global open_page_tips
global read_disk
global write_disk