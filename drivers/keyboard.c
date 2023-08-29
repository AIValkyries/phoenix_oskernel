#include <keyboard.h>
#include <tty.h>
#include <kernel/kernel.h>

/* ------------------ 83键盘 扫描码对应的字符码 ------------------ */

unsigned char mode;     // 是键盘特殊键按下时的状态标志
unsigned char leds;     // 用于表示键盘指示灯的状态标志
unsigned char e0;

extern void do_tty_interrupt();
extern void con_write();

// 单键,索引=扫描码
char ascii_map[] = 
{
    NUL,ESC,NUM_1,NUM_2,NUM_3,NUM_4,NUM_5,NUM_6,NUM_7,NUM_8,NUM_9,NUM_0,0x2D,0x3D, Backspace, HT_TAB,
    c_q,c_w,c_e,c_r,c_t,c_y,c_u,c_i,c_o,c_p, 0x5B/*[*/,0x5D/*]*/, CR , NUL,
    c_a,c_s,c_d,c_f,c_g,c_h,c_j,c_k,c_l,0x3B/*;*/, 0x27/*'*/, 0x60/*<*/, 
    NUL,0x5C/*\*/,c_z,c_x,c_c,c_v,c_b,c_n,c_m,0x2C/*,*/,0x2E/*.*/,0x2F/*/*/
};

// shift 对应的字符 "!@#$",,"^&*()_+"
char ascii_shift_map[] = 
{
    NUL,ESC,0x21/*!*/,0x40/*@*/,0x23/*#*/,0x24/*$*/,0x25/*%*/,0x5E/*^*/,
    0x26/*&*/,0x2A/*`*/,0x28/*(*/,0x29/*)*/,0x2D/*-*/,0x2B/*+*/, Backspace, HT_TAB,
    C_Q,C_W,C_E,C_R,C_T,C_Y,C_U,C_I,C_O,C_P, 0x7B/*{*/,0x7D/*}*/, CR , NUL,
    C_A,C_S,C_D,C_F,C_G,C_H,C_J,C_K,C_L,':', 0x22/*"*/, 0x7E/*~*/, NUL,0x2F/*/*/,
    C_Z,C_X,C_C,C_V,C_B,C_N,C_M, 0x3C/*<*/,0x3E/*>*/,0x3F/*?*/
};


/* ------------------ 扫描码对应的例程 ------------------ */

typedef void (key_handfn)(unsigned char scan_code);
typedef void (*key_hand)(unsigned char scan_code);

// 有 down 和 up 
static key_handfn do_self,  do_esc,      // 主处理函数
                  do_enter, do_space/*空格键*/,do_back_space/*回退键盘*/,
                  do_shift, do_ctrl, do_alt, do_tab, do_caps,       // 
                  do_home,  do_end, do_page_up, do_page_down,         // 方向功能键,处理光标和滚屏之类的功能
                  do_up_arrow, do_left_arrow, do_right_arrow, do_down_arrow, do_del,
                  do_func /*F1-F10*/,  do_ignore;
                

/*
    扫描码对应的例程 83键盘
    索引=扫描码 索引0=没有对应，因此 do_ignore
*/
static key_hand key_table_func[] = 
{
    do_ignore, do_esc, 
    
    // 第二排
    /*1 and!*/ do_self, do_self, do_self, do_self, do_self,do_self, 
    do_self, do_self, do_self, do_self, do_self, do_self/*0D = and +*/,do_back_space/*0E*/, 

    // 第三排
    do_tab/*0xF*/,do_self, do_self, do_self, do_self, do_self, do_self, 
    do_self, do_self, do_self, do_self/*p=0x19*/, do_self, do_self, 
    do_enter /* enter 0x1C=28.index */,

    // 第四排
    do_ctrl/* 0x1D */,  do_self/*a  0x1e*/, do_self, do_self, do_self, do_self, 
    do_self, do_self, do_self, do_self, do_self, do_self/*0x28='" 40.index*/, 
    
    do_self/*0x29=`~*/, do_shift/*0x2A shift_left*/,do_self/*\|0x2B*/, 

    // 第五排
    do_self/*z*/, do_self, do_self, do_self, do_self,do_self,do_self,
    do_self/*,<*/, do_self/*0x34.>*/,do_self/*0x35 /?*/, 

    do_shift/*0x36 right*/, do_ignore/*PrtScreen*/, do_alt, do_space/*0x39*/,
    do_caps/*0x3A=58.index*/,

    // F1~F10
    do_func,do_func,do_func,do_func,do_func,do_func,do_func,do_func,do_func,do_func,

    // 数字键盘
    do_ignore/*69.index=Numlock*/,do_ignore/*ScrollLock 0x46*/,
    do_home/*0x47*/,do_up_arrow/*0x48*/,do_page_up/*0x49*/,do_ignore/*-法*/,
    do_left_arrow/*0x4B*/,do_self,do_right_arrow/*->60x4D*/,do_ignore/*+*/,
    do_end/*0x4F*/,do_down_arrow,do_page_down/*0x51 & 3*/,
    do_self/*0 and ins 0x52*/, do_del/*0x53*/
};


// 将字符放入 读缓冲队列中
void put_queue(char c)
{
    struct tty_queue *r_q;

    r_q = &(current_queue);

    r_q->buf[r_q->head_ptr++] = c;
    r_q->head_ptr &= (TTY_BUF_SIZE - 1);

    if(r_q->head_ptr == r_q->tail_ptr)      // 缓冲区满了
    {
        r_q->head_ptr--;        // 如果满了，则放弃该字符
        return;
    }
}

void read_char(char *c)
{
    *c = current_queue.buf[current_queue.tail_ptr++];
    current_queue.tail_ptr &= (TTY_BUF_SIZE - 1);
}


static void do_ignore(unsigned char scan_code)
{
    // TODO
    printk("do_ignore TODO \\n");
}


static void do_page_up(unsigned char scan_code)
{
    printk("do_page_up TODO \\n");
}

static void do_page_down(unsigned char scan_code)
{
    printk("do_page_down  TODO \\n");
}

// 是按下还是松开
void do_keyboard_fun(unsigned char scan_code)
{
    if(scan_code > 0x53)    //  松开
        scan_code -= 0x53;

    key_hand hand  =  key_table_func[scan_code];
    if(hand == NULL)
    {
        printk("key_handfn is null, scan_code = %x \\n",scan_code);
        return;
    }
    // 调用键盘处理函数
    hand(scan_code);
    con_write();
}

// --------------------- 模式函数 ---------------------

static void do_shift(unsigned char scan_code)
{
    if(scan_code == LEFT_SHIFT)        // left OR right
    {
        mode |= MODE_LEFT_SHIFT;
    }
    else if(scan_code == RIGHT_SHIFT)
    {
        mode |= MODE_RIGHT_SHIFT;
    }
    else if(scan_code == (LEFT_SHIFT + 0x53))
    {
        mode |= ~MODE_LEFT_SHIFT;
    }
    else if(scan_code == (RIGHT_SHIFT + 0x53))
    {
        mode |= ~MODE_RIGHT_SHIFT;
    }
}


static void do_ctrl(unsigned char scan_code)
{
    if(scan_code == CTRL)
    {
        mode |= MODE_LEFT_CTRL;
    }
    else if(scan_code == (CTRL + 0x53))
    {
        mode |= ~MODE_LEFT_CTRL;
    }
}

static void do_alt(unsigned char scan_code)
{
    if(scan_code == ALT)
    {
        mode |= MODE_LEFT_ALT;
    }
    else if(scan_code == (CTRL + 0x53))
    {
        mode |= ~MODE_LEFT_ALT;
    }
}


static void do_caps(unsigned char scan_code)
{
    if(mode == CAPS)
    {
        mode |= MODE_DOWN_CAPS;
    }
    else if(mode == (CAPS + 0x53))
    {
        mode |= ~MODE_UP_CAPS;
    }
}

// --------------------- 处理方向键和光标 ---------------------

static void do_home(unsigned char scan_code)
{
    // 转换成回车符，回到头列
    put_queue(CR);
}

static void do_end(unsigned char scan_code)
{
    // 头尾部
    put_queue(EOT);
}

// 光标上移
static void do_up_arrow(unsigned char scan_code)
{
    put_queue(ESC);
    put_queue('W');  // 光标上移动
}

static void do_left_arrow(unsigned char scan_code)
{
    put_queue(ESC);
    put_queue('A');  // left
}

static void do_right_arrow(unsigned char scan_code)
{
    put_queue(ESC);
    put_queue('D');  // 右边
}

static void do_down_arrow(unsigned char scan_code)
{
    put_queue(ESC);
    put_queue('S');  // down
}


static void do_del(unsigned char scan_code)
{
    put_queue(0x7F);
}


// --------------------- 主处理函数 ---------------------


static void do_self(unsigned char scan_code)
{
    // 检测 alt | shift | 等标志是否 down
    char c;

    c = ascii_map[scan_code];

    if(mode == MODE_LEFT_SHIFT || 
        mode == MODE_RIGHT_SHIFT || mode == MODE_DOWN_CAPS)
    {
        // 转换大小写
        if(c >= 0x61 && c <= 0x7D)
            c = ascii_shift_map[scan_code];
    }

    put_queue(c);
}

// 回车键
static void do_enter(unsigned char scan_code)
{
    put_queue(LF_NL);
}

// 空格字符
static void do_space(unsigned char scan_code)
{
    put_queue(Space);
}

// 删除一个字符
static void do_back_space(unsigned char scan_code)
{
    put_queue(Backspace);
}

static void do_tab(unsigned char scan_code)
{
    // 4个空格字符
    put_queue(HT_TAB);
}


// --------------------- 子功能处理键 ---------------------

// [F1-F10]
static void do_func(unsigned char scan_code)
{
    printk("F1-F10 TODO \\n");
}


static void do_esc(unsigned char scan_code)
{
    printk("esc TODO \\n");
}