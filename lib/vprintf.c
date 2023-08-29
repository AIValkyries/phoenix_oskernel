// 格式化输出
#include <kernel/string.h>
#include <kernel/kernel.h>
#include <kernel/stdarg.h>

// 格式化输出标志
#define ZERO_PAD    0x01
#define SIGN        0x02
#define PLUS        0x04
#define SPACE       0x08
#define LEFT        0x10
#define SPECIAL     0x20
#define SMALL       0x40

// [0 == 0x30] ~~~ [9 == 0x39]
#define is_digit(c) (c>='0' && c<='9')
#define tmp_char_len 16

static char buf[1024];
static char tmp[tmp_char_len];

static int char_to_number(char *fmt)
{
    char c = *fmt;
    int number;

    while (is_digit(c))
    {
        number = number * 10 + (c - '0');
    }

    return number;
}

// 将整数转换成指定禁止的字符串
static char* number_to_char(
    char *str,      // 缓冲区地址
    unsigned long num,        // 整数
    int base)       // 进制 = 10,8,16
{
    char *digits = "0123456789ABCDEF";
    
    int i;
    int dx;
	int count;

    i     = 0;
    dx    = 0;
    count = 0;
    memset(tmp,'0',sizeof(tmp));

    while (num > 0)
    {
        dx = num % base;
        num /= base;
        tmp[i] = digits[dx];
        i++;
        if(i == tmp_char_len)
            break;
    }

    if(num == 0)
        tmp[i++] = '0';
    
    if(base == 16)
    {
        tmp[i++] = 'x';
        tmp[i++] = '0';
    }
   
	count = i;
    
    while (i-->0)
    {
        *str = tmp[i];
		str++;
    }

    return str;
}

/*
    args = 指向第一个参数的指针
    buf  = 输出字符串缓冲区， 对 fmt 的字符串进行处理， 然后填充到 buf
    fmt  = 是格式化字符串的地址
*/
int vsprintf(char *buf, char *fmt, char *args)
{
    char *str;
    char *s;

    int flags;
    int field_width;
    int precision;
    int qualifier;

    int len;
    int i;

    for(str = buf; *fmt; ++fmt)
    {
        if(*fmt != '%')         // 格式符号
        {
            *str = *fmt;
            str++;
            continue;
        }

        flags = 0;
repeat:
        fmt++;
        switch (*fmt)
        {
            case '-':   flags |= LEFT; goto repeat;
            case '+':   flags |= PLUS; goto repeat;
            case ' ':   flags |= SPACE; goto repeat;
            case '#':   flags |= SPECIAL; goto repeat;      // 是特殊转换
            case '0':   flags |= ZERO_PAD; goto repeat;
        }
        
        // 要输出字符的最小数目，如果输出短于该数，结果会用空格填充。
        // 如果输出的值长于该数，结果不会被截断
        // width
        field_width = -1;
        if(is_digit(*fmt))
           field_width = char_to_number(fmt);

        // get the precision
        if(*fmt == '.')
        {
            ++fmt;
            precision = char_to_number(fmt);
        }

        /* get the conversion qualifier */
        qualifier = -1;
        if(*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
        {
            qualifier = *fmt;
            ++fmt;
        }

        // 字符串
        switch (*fmt)
        {
        case 'c':              // 单个字符
            if(flags != LEFT)       // 默认是右对齐
            {
                while (--field_width > 0)
                    *str++ = ' ';
            }
            *str++= (unsigned char) va_arg(args, int);

            while (--field_width>0)
                *str++ = ' ';
            break;
        case 's':              // 字符串
            s = va_arg(args, char *);
            len = strlen(s);

            if(flags != LEFT)       // 默认是右对齐
            {
                while (--field_width > 0)
                    *str++ = ' ';
            }

            for(i = 0; i<len; i++)
                *str++ = *s++;

            while (--field_width>0)
                *str++ = ' ';
            break;
        case 'o':	// 有符号8禁止
			str = number_to_char(str, (unsigned long) va_arg(args, unsigned long), 8);
			break;
        case 'p':             // 指针地址输出
            str = number_to_char(str, (unsigned long) va_arg(args, void *), 16);
            break;
        case 'u':             // 以十进制形式输出无符号整数 
        case 'd':             // 以十进制形式输出带符号整数
            str = number_to_char(str, (unsigned long) va_arg(args, unsigned long), 10);
            break;
        case 'x':             // 16 进制，小写
        case 'X':             // 16 进制，大写
            str = number_to_char(str, (unsigned long) va_arg(args, unsigned long), 16);
        break;
        default:
            break;
        }
    }

    *str = '\0';

    return (str-buf);
}

