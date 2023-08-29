#ifndef _STR_H
#define _STR_H
/*
;    1.字符串传送  MOVSB dest,source
;        MOVS,MOVSB,MOVSW,MOVSD
;            源地址 DS:SI 目标地址 = ES:DI
    
;    2.字符串扫描 SCAS,SCASB,SCASW,SCASD
;        SCASB ES:dest SCASB dest 由 ES:DI 指定的内存字符串，寻找与累加器匹配的数值
;        方向标志位 = 1 DI 递减，否则递增

;    3.将字符加载到累加器 LODS,LODSB,LODSW,LODSD
;        LODS mem 将DS:SI 指定的内存字节或字加载到 累加器（AL,AX,EAX）
;            方向位

    4.字符串比较  CMPS,CMPSB,CMPSW,CMPSD
        内存地址由 DS:SI 和 ES：DI 指定，从源串减去目的的串
        CMPSB 比较字节
        CMPSW 比较字
        CMPSD 比较双字
        根据操作数大小和方向标志位，若方向标志位 = 1，递减，否则 递增

     5.保存字符串 STOS....STOSB,STOSW,STOSD
        将累加器存入 ES:DI 指定的内存位置
*/


// rep MOVSB b = 字节
// 内存块复制，从源地址 src 到目的地址 dest，复制 n 个字节
#define memcopy(dest,src,n) ({ \
void * _res = dest; \
__asm__ ("cld;  \
rep;  \
movsb" \
	::"D" ((long)(_res)),"S" ((long)(src)),"c" ((long) (n))); \
_res; \
})


#define strlen(str1)({\
register int __res __asm__("cx");   \
    __asm__("cld\n\t"   \
        "repne\n\t"     \
        "scasb\n\t"     \
        "not %0\n\t"    \
        "dec %0"        \
        :"=c" (__res):"D" (str1),"a" (0),"0" (0xffffffff)); \
__res;  \
})

#define strcmp(cs,ct)({\
register int __res __asm__("ax");   \
    __asm__("cld\n"                 \
        "1:\tlodsb\n\t"             \
        "scasb\n\t"                 \
        "jne 2f\n\t"                \
        "testb %%al,%%al\n\t"       \
        "jne 1b\n\t"                \
        "xorl %%eax,%%eax\n\t"      \
        "jmp 3f\n"                  \
        "2:\tmovl $1,%%eax\n\t"     \
        "jl 3f\n\t"                 \
        "negl %%eax\n"              \
        "3:"                        \
        :"=a" (__res):"D" (cs),"S" (ct));   \
__res;  \
})

#define strncmp(str1,str2,count)({      \
     register int __res __asm__("ax");  \
    __asm__("mov eax,0\n\t"             \
            "cld\n\t"                   \
            "repe\n\t"                  \
            "cmpsd\n\t"                 \
            "jne L1 \n\t"               \
            "L1:\tmov eax,1\n\t"        \
            "jl L2\n\t"                 \
            "\tL2:neg eax\n\t"          \
            :"=a"(__res)                \
            :"D"(str1),"S"(str2),"c"(count));   \
__res;  \
})


#define strcopy(dest,src,count) memcopy(dest,src,count)

#define memset(src,c,count)({\
    __asm__("cld\n\t"       \
	"rep\n\t"               \
	"stosb"                 \
	::"a" (c),"D" (src),"c" (count));   \
src;    \
})


// 用字符串填充指定长度内存块
/* extern inline void* memset(void* src, char c, int count)
{
__asm__("cld\n\t"
	"rep\n\t"
	"stosb"
	::"a" (c),"D" (src),"c" (count));
return src;
}
 */
/*
    返回 ： str1>str2 = 1 | str1<str2 = -1 | str1==str2 = 0
*/
/* extern inline int strncmp(const char *str1,const char *str2, int count)
{
    register int __res __asm__("ax");

    __asm__("mov eax,0\n\t"
            "cld\n\t"
            "repe\n\t"
            "cmpsd\n\t"
            "jne L1 \n\t"   
            "L1:\tmov eax,1\n\t"
            "jl L2\n\t"
            "\tL2:neg eax\n\t"
            :"=a"(__res)
            :"D"(str1),"S"(str2),"c"(count));
    return __res;
} */

/*
    字符串比较
*/
/* extern inline int strcmp(const char * cs,const char * ct)
{
register int __res __asm__("ax");
__asm__("cld\n"
	"1:\tlodsb\n\t"
	"scasb\n\t"
	"jne 2f\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n\t"
	"xorl %%eax,%%eax\n\t"
	"jmp 3f\n"
	"2:\tmovl $1,%%eax\n\t"
	"jl 3f\n\t"
	"negl %%eax\n"
	"3:"
	:"=a" (__res):"D" (cs),"S" (ct));
return __res;
} */

/* // 字符串长度
extern inline int strlen(const char* str1)
{
register int __res __asm__("cx");
__asm__("cld\n\t"
	"repne\n\t"
	"scasb\n\t"
	"not %0\n\t"
	"dec %0"
	:"=c" (__res):"D" (str1),"a" (0),"0" (0xffffffff));
return __res;
} */


#endif