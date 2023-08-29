#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <tty.h>

#define VIDEO_TYPE_MDA		0x10	/* 	单色文本	*/
#define VIDEO_TYPE_CGA		0x11	/*  CGA 显示器	*/
#define VIDEO_TYPE_EGAM		0x20	/* 	EGA/VGA 单色	*/
#define VIDEO_TYPE_EGAC		0x21	/* 	EGA/VGA 彩色	*/

// -------------------- 端口索引 --------------------

#define CURSOR_R10      10        // 光标开始位置端口
#define CURSOR_R11      11        // 结束位置端口
#define CURSOR_CURRENT_14   14          // 光标当前位置,高字节
#define CURSOR_CURRENT_15   15          // 低位
#define VIDEO_MEM_BASE_PORT_12   12       // 高位置
#define VIDEO_MEM_BASE_PORT_13   13       // 低位置
#define VIDEO_R0    0       // 水平字符总数
#define VIDEO_R1    1       // 水平显示字符
#define VIDEO_R2    2       // 水平同步位置
#define VIDEO_R3    3       

#define NORMAL_CHAR 0       // 普通字符
#define SESC_CHAR   1       // 转义字符
#define SQUARE_CHAR 2       // 控制字符序列


// #define cursor_x       (*(unsigned char*)0x5010)    // 初始光标号 x
// #define cursor_y       (*(unsigned char*)0x5012)    // 初始光标号 y
#define screen_max_x   (*(unsigned short*)(0x5018))  // 屏幕宽度
#define screen_max_y   (*(unsigned short*)(0x501A))  // 屏幕高度
#define video_mode     (*(unsigned short*)(0x5016))             // 显示模式
#define video_ega_bx   (*(unsigned short*)(0x501E))    // 显示内存大小和色彩模式
#define video_ega_cx   (*(unsigned short*)(0x5022))             // 显卡特性参数


static unsigned long video_mem_base;        // 显存的 起始位置
static unsigned long video_mem_term;        // 显存的 结束位置
static unsigned char video_type;

// 屏幕每行字节数
static unsigned long video_size_row;
static unsigned long video_num_columns;     //  屏幕文本列数
static unsigned long video_num_lines;       //  屏幕文本行数

#define VIDEO_PORT_REG 0x3d4             // 索引寄存器端口
#define VIDEO_PROT_VAL 0x3d5             // 数据控制器端口


/*
    光标    [start-end]
    背景色
    字体属性颜色
    行列数
    行字符数数

    显示内存 start - end 的位置
    当前光标 current x - y
    滚屏位置
*/
struct window
{
    // 关于光标的数值
    unsigned long cursor_x;
    unsigned long cursor_y;
    unsigned long cursor_pos;       // x+y的值

    // 关于window 在显存中的位置
    unsigned long video_mem_start;
    unsigned long video_mem_end;

    // 关于滚屏
    unsigned long origin;
    unsigned long scr_end;
    unsigned long top;
    unsigned long bottom;

    // 关于字符
    unsigned char attr;         // 字符属性
    unsigned char def_attr;     // 默认字符属性
    unsigned char bold_atrr;    // 粗体字符属性
};


static struct window vc_windows[MAX_CONSOLES];


#define vc_attr(currcons)             vc_windows[currcons].attr
#define vc_def_attr(currcons)         vc_windows[currcons].def_attr
#define vc_bold_attr(currcons)        vc_windows[currcons].bold_atrr
#define vc_video_mem_start(currcons)  vc_windows[currcons].video_mem_start
#define vc_video_mem_end(currcons)    vc_windows[currcons].video_mem_end
#define vc_origiin(currcons)          vc_windows[currcons].origin
#define vc_scr_end(currcons)          vc_windows[currcons].scr_end
#define vc_top(currcons)              vc_windows[currcons].top
#define vc_bottom(currcons)           vc_windows[currcons].bottom

#define vc_cur_pos(currcons)          vc_windows[currcons].cursor_pos
#define vc_cur_x(currcons)            vc_windows[currcons].cursor_x
#define vc_cur_y(currcons)            vc_windows[currcons].cursor_y


#define video_space_char    0x0720      // 0x20 是字符，0x07 是属性


#endif