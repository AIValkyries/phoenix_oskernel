#include <kernel/system.h>
#include <tty.h>
#include <console.h>
#include <kernel/io.h>
#include <keyboard.h>
#include <kernel/kernel.h>

int fg_console = 0;     // 当前控制台
int nr_console = 0;     // 控制台总数
struct tty_struct con_table[4];         // 终端设备结构

extern void keyboard_interrupt(void);

// 设置显示光标
static void con_set_cursor(int currcons)
{
    if(currcons != fg_console)
        return;

    unsigned long pos;
    pos = vc_cur_pos(currcons) - video_mem_base;

    // 高字节
    outb(CURSOR_CURRENT_14, VIDEO_PORT_REG);
    // 16 位端口 video_mem_base >> 8 / 2 ; /2表示一个像素2字节.
    outb(0xff & (pos >> 9), VIDEO_PROT_VAL);

    outb(CURSOR_CURRENT_15, VIDEO_PORT_REG);
    outb(0xff & (pos >> 1), VIDEO_PROT_VAL);
}

// 隐藏光标
static void hide_cursor(int currcons)
{
    if(currcons != fg_console)
        return;
    
    outb(CURSOR_CURRENT_14, VIDEO_PORT_REG);
    outb(0, VIDEO_PROT_VAL);

    outb(CURSOR_CURRENT_15, VIDEO_PORT_REG);
    outb(0, VIDEO_PROT_VAL);
}

// 起始显存位置
static void set_origin(int currcons)
{
    if(currcons != fg_console)
        return;

    unsigned long pos;

    pos = vc_origiin(currcons) - video_mem_base;

    outb(VIDEO_MEM_BASE_PORT_12, VIDEO_PORT_REG);
    outb((pos >> 9) & 0xff, VIDEO_PROT_VAL);

    outb(VIDEO_MEM_BASE_PORT_13, VIDEO_PORT_REG);
    outb((pos >> 1) & 0xff, VIDEO_PROT_VAL);
}

/*
	向上卷动一行
	将分配给 console 的内存下移一行
*/
static void scrup(int currcons)
{

}

/*
   	向下卷动一行
	将屏幕滚动窗口向上移动一行，相应屏幕滚动区域内容向下移动1行。 
*/
static void scrdown(int currcons)
{

}

// 更新光标的位置
static void update_cursor_xy(int currcons, int new_x, int new_y)
{
	if (new_x > video_num_columns || new_y >= video_num_lines)
		return;

    vc_cur_x(currcons) = new_x;
    vc_cur_y(currcons) = new_y;
    vc_cur_pos(currcons) = vc_video_mem_start(currcons) + (new_y * video_size_row) + (new_x << 1); 
}

// 光标上移
static void down_cursor(int currcons)
{ 
    vc_cur_y(currcons)--;
    vc_cur_pos(currcons) -= video_size_row;
}

// 光标下移
static void up_cursor(int currcons)
{
    if(vc_cur_y(currcons)+1 < video_num_lines)
    {
        vc_cur_y(currcons)++;
        vc_cur_pos(currcons) += video_size_row - (vc_cur_x(currcons)<<1);
    }
}

// 左移
static void left_cursor(int currcons)
{
    if(vc_cur_x(currcons)-1>0)
    {
        vc_cur_x(currcons)--;
        vc_cur_pos(currcons)-=2;
    }

}

// 右移
static void right_cursor(int currcons)
{
    if(vc_cur_x(currcons)+1<video_num_columns)
    {
        vc_cur_x(currcons)++;
        vc_cur_pos(currcons)+=2;
    }
}

// 回到列头
static void cr(int currcons)
{
    vc_cur_pos(currcons) -= (vc_cur_x(currcons)<<1);    // 一个字符2字节
    vc_cur_x(currcons)   = 0;
}

// 在光标处插入一个字符
static void insert_char(int currcons, char c)
{
    int i;
    unsigned short *base;
    unsigned short tmp, old;

    i = vc_cur_x(currcons);
    old = c;
    base = (unsigned short*)vc_cur_pos(currcons);
    
    // 把光标开始出的所有字符右移一格
    while (i++ < video_num_columns)
    {
        tmp=*base;
		*base=old;
		old=tmp;
		base++;
    }
}

// 在光标处插入 nr 个字符
static void instert_char_num(int currcons,int nr, char c)
{
    if(nr > video_num_columns)
        nr = video_num_columns;
    
    if(nr <= 0)
        nr = 1;
    while (nr--)
    {
        insert_char(currcons, c);
    }
}

// 删除一个字符
static void delete_char(int currcons)
{
    unsigned short *base;
    int i;

    if(vc_cur_x(currcons)  >= video_num_columns)
        return;

    i = vc_cur_x(currcons);
    base = (unsigned short*)vc_cur_pos(currcons);

    while (i++ < video_num_columns)
    {
        *base = *(base+1);
        base++;
    }

    *base = (char)video_space_char;
}

// 删除多个字符
static void delete_char_num(int currcons, int  nr)
{
     if(nr > video_num_columns)
        nr = video_num_columns;
    
    if(nr <= 0)
        nr = 1;

    while (nr--)
    {
        delete_char(currcons);
    }
}

// -------------------- 关于字符的函数 --------------------

// 擦除光标前一个字符（用空格替代）
static void del(int currcons)
{
    if(vc_cur_x(currcons))
    {
        vc_cur_x(currcons)--;
        vc_cur_pos(currcons) -= 2;
        *(unsigned short*)vc_cur_pos(currcons) = video_space_char;
    }
}

enum
{
    BG_BLACK = 0x00,       // 0x00
    BG_BLUE = 0x10,        // 0x10
    BG_GREEN = 0x20,       // 0x20
    BG_CYAN = 0x30,        // 0x30
    BG_RED = 0x40,         // 0x40
    BG_MAGENTA = 0x50,     // 0x50
    BG_BROWN = 0x60,       // 0x60
    BG_WHITE = 0x70        // 0x70
};


/*
    设置显示字符属性
    color_type = 上面的 enum
*/
static void set_bg_color(int currcons, int color_type)
{
    unsigned short attr;
    attr = color_type;

    vc_attr(currcons) = (vc_attr(currcons) & 0xf0 & attr);
}


// 字符属性
static void set_char_color(int currcons, int color_type)
{
    unsigned short attr;
    attr = color_type;

    vc_attr(currcons) = (vc_attr(currcons) & 0x0f & attr);
}


static void set_default_attr(int currcons)
{
    vc_attr(currcons) = vc_def_attr(currcons);
}


static void update_screen()
{
    set_origin(fg_console);
    con_set_cursor(fg_console);
}

enum
{
    ESnormal=1,
    ESesc=2,          // 控制序列
};

// 在屏幕中显示字符
void con_write()
{
    struct tty_struct *tty;
    char c;
    int  nr;     // 队列中的字符数
    int state;

    tty = con_table + fg_console;
    state = ESnormal;

    // 队列中字符的长度
    nr = tty->queue.head_ptr - tty->queue.tail_ptr;

    while (nr>0)
    {
        nr--;
        // 在光标处显示字符
        read_char(&c);
        
        switch (state)
        {
            case ESnormal:
            {
                if(c == ESC)            // 多个字符组成的控制序列
                {
                    state = ESesc;
                    continue;
                }
                
                if(c > 31 && c < 127)   // 普通显示字符
                {
                    if(vc_cur_x(fg_console) >= video_num_columns)
                    {
                        vc_cur_x(fg_console)   -= video_num_columns;
                        vc_cur_pos(fg_console) -= video_size_row;
                        up_cursor(fg_console);
                    }
                    
                    __asm__("movb %2,%%ah\n\t"
						"movw %%ax,%1\n\t"
						// 32 个控制字符 ，不需要显示
						::"a"(c),
						"m" (*(short *)vc_cur_pos(fg_console)),
						"m" (vc_attr(fg_console)));

                    vc_cur_pos(fg_console) += 2;
                    vc_cur_x(fg_console)++;

                    continue;
                }

                // 单个控制字符
                switch (c)
                {
                    case Backspace:      // 退格
                        del(fg_console);
                        break;
                    case HT_TAB:        // 水平制表符
                        {
                            vc_cur_x(fg_console) += 4;
                            vc_cur_pos(fg_console) += 8;

                            if(vc_cur_x(fg_console)>video_num_columns)
                            {
                                vc_cur_x(fg_console)-=video_num_columns;
                                vc_cur_pos(fg_console)-=video_size_row;
                                up_cursor(fg_console);
                            }
                        }
                        break;
                    case LF_NL:         // 换行
                        cr(fg_console);           // 回到头列
                        up_cursor(fg_console);  // 光标下移一行
                        break;
                    case CR:            // 回车,将光标移动到头
                        cr(fg_console);
                        break;
                    case DEL:           // 删除字符
                        del(fg_console);
                        break;
                    default:
                        break;
                }
            }
            break;
            // 控制字符
            case ESesc:
            {
                state = ESnormal;
                switch (c)
                {
                case 'W':           // up   光标上移
                    down_cursor(fg_console);
                    break;      
                case 'A':           // left
                    left_cursor(fg_console);
                    break;
                case 'D':           // right
                    right_cursor(fg_console);
                    break;  
                case 'S':           // down
                    up_cursor(fg_console);
                    break;
                default:
                        break;
                }
            }
            break;
            default: state = ESnormal;
                    break;
        }
    }
    
    con_set_cursor(fg_console);
}

static void change_console(long  new_console)
{
    if(new_console > MAX_CONSOLES || new_console == fg_console)
        return;

    fg_console  = new_console;
    update_screen();
}


void con_init()
{
    register unsigned char a;
    long video_memory;
    int  currcons;
    char *display_desc = "????";
    char *display_ptr;

    // set 显存地址, 32kib 大小
    video_mem_base = 0xb8000;       // 显存起始位置
    video_mem_term = 0xc0000;  
    video_type = VIDEO_TYPE_EGAC;
    display_desc   = "EGAc";       // 在屏幕右上角显示描述字符串

    // 屏幕行列
    video_num_lines   = 25;
    video_num_columns = 80;
    video_size_row    = video_num_columns * 2;           // 屏幕行列字节数
    
    video_memory = video_mem_term - video_mem_base;     // 显存大小
    // 显存大小可以容纳多少个 屏幕 32768/4000=8个，大概
    nr_console = video_memory / (video_size_row * video_num_lines);

    if(nr_console > MAX_CONSOLES)
        nr_console = MAX_CONSOLES;
    // 32768/4=8kib左右
    video_memory /= nr_console;         // 每个屏幕占用的现存大小

    vc_top(0) = 0;
    vc_bottom(0) = video_num_lines;     // 字符行数

    // 初始化控制台的变量
    for(currcons = 0; currcons < nr_console; currcons++)
    {
        // 0xb8000 + currcons * video_memory; 
        vc_video_mem_start(currcons) = video_mem_base + currcons * video_memory;  // 屏幕开始位置
        vc_video_mem_end(currcons)   = vc_video_mem_start(currcons) + video_memory;
        vc_attr(currcons)            = 0x07;
        vc_def_attr(currcons)        = 0x07;
        
        vc_origiin(currcons)         = vc_video_mem_start(currcons);
        vc_scr_end(currcons)         = vc_origiin(currcons) + video_num_lines * video_size_row;

        update_cursor_xy(currcons, 0, 0);
    }
    
    //update_screen();

    // 中断处理,向 OCW1 写入，允许 键盘中断 11111101， 1是屏蔽，0是允许, 允许IR1的中断信号
    outb(inb(0x21) & 0xfd, 0x21);
    a = inb(0x61);							// 读取键盘端口 0x61
	outb(a|0x80, 0x61);						// 设置禁止键盘工作	
	outb(a, 0x61);								// 再允许键盘工作，用于复位键盘
}

// 内核输出信息的接口
// '\'是转义前缀字符
void console_print(char *b)
{
	int currcons = fg_console;
    char c;
    unsigned long vc_addr;
    unsigned long test_addr;
    // 栈异常？
    struct window *vc_cur_win;

    vc_cur_win = &vc_windows[currcons];

    while (c = *(b++))
    {
        if(c == 0x5C)       // 如果是转义字符
        {
            c = *(b++);
            if(c == 'r')     // 回车键 'r'
            {
                cr(currcons);
                continue;
            }

            if(c == 'n')              // 换行
            {
                cr(currcons);           // 回到头列
                up_cursor(currcons);  // 光标下移一行
                continue;
            }

            if(c == 'b')            // 退格(BS) ，将当前位置移到前一列
            {
                left_cursor(currcons);
                continue;
            }

            if(c == 't')        // 水平制表(HT) 
            {
                // 4个空格字符
                instert_char_num(currcons, 4, (char)video_space_char);
                continue;
            }
        }

        if(vc_cur_x(currcons) >= video_num_columns)
        {
            vc_cur_x(currcons) -= video_num_columns;
            vc_cur_pos(currcons) -= video_size_row;
            down_cursor(currcons);
        }

        vc_addr = vc_cur_win->cursor_pos;

        // 这里出现了 page 异常，why？ 80170000 | 17740
        // al是需要显示的字符, ah是属性, 将 ax 的内容存放至 pos （显存） 之处
        __asm__(
            "movb %2,%%ah\n\t"              //  属性字节放到 ah 中
            "movw %%ax,%1\n\t"              //  ax 内容放到 pos 处
			::"a" (c),							//  字符存入 ax
			"m" (*(short *)vc_addr),
			"m" (vc_cur_win->attr));
        
        vc_cur_pos(currcons) += 2;
        vc_cur_x(currcons)++;
    }

    // 最后设置光标内存位置，设置显示控制器中光标的位置
	con_set_cursor(currcons);
}

