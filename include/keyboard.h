#ifndef _KEYBOARD_H
#define _KEYBOARD_H

// ASCII 码

#define NUL 0x00
#define DEL 0x7F            // 删除
#define SOH 0x01            // 标题开始
#define STX 0x02            // 正文开始
#define ETX 0x03            // 正文结束
#define EOT 0x04            // 传输结束
#define ENQ 0x05            // 请求
#define ACK 0x06            // 回应/响应/收到通知
#define BEL 0x07            // 响铃
#define Backspace 0x08      // 退格键
#define HT_TAB 0x09         // tab 水平制表符
#define LF_NL 0x0A          // 换行键
#define VT 0x0B             // 垂直制表符
#define FF_NP 0x0C          // 换页键
#define CR 0x0D             // 回车键
#define SO 0x0E             // 不用切换
#define SI 0x0F             // 	启用切换

#define DLE 0x10        // 数据链路转义
#define DC1 0x11        // 设备控制1、传输开始
#define DC2 0x12        // 设备控制2
#define DC3 0x13        // 设备控制3、传输中断
#define DC4 0x14        // 设备控制4
#define NAK 0x15        // 无响应/非正常响应/拒绝接收
#define SYN 0x16        // 同步空闲
#define ETB 0x17        // 传输块结束/块传输终止
#define CAN 0x18        // 取消
#define EM  0x19        // 已到介质末端/介质存储已满/介质中断
#define SUB 0x1A        // 替补/替换
#define ESC  0x1B        // 逃离/取消
#define FS  0x1C         // 组分隔符/分组符
#define GS  0x1D         // 记录分离符
#define RS  0x1E         // 单元分隔符
#define US  0x1F         // 空格

#define Space 0x20

#define NUM_0   0x30
#define NUM_1   0x31
#define NUM_2   0x32
#define NUM_3   0x33
#define NUM_4   0x34
#define NUM_5   0x35
#define NUM_6   0x36
#define NUM_7   0x37
#define NUM_8   0x38
#define NUM_9   0x39


// 大写
#define C_Q     'Q'
#define C_W     'W'
#define C_E     'E' 
#define C_R     'R'
#define C_T     'T'
#define C_Y     'Y'
#define C_U     'U'
#define C_I     'I'    
#define C_O     'O'
#define C_P     'P'

#define C_A     'A'
#define C_S     'S'
#define C_D     'D'
#define C_F     'F'
#define C_G     'G'
#define C_H     'H'
#define C_J     'J'    
#define C_K     'K'
#define C_L     'L'

#define C_Z     'Z'
#define C_X     'X'
#define C_C     'C'
#define C_V     'V'
#define C_B     'B'    
#define C_N     'N'
#define C_M     'M'


// 小写
#define c_q     'q'
#define c_w     'w'
#define c_e     'e' 
#define c_r     'r'
#define c_t     't'
#define c_y     'y'
#define c_u     'u'
#define c_i     'i'    
#define c_o     'o'
#define c_p     'p'

#define c_a     'a'
#define c_s     's'
#define c_d     'd'
#define c_f     'f'
#define c_g     'g'
#define c_h     'h'
#define c_j     'j'   
#define c_k     'k'
#define c_l     'l'

#define c_z     'z'
#define c_x     'x'
#define c_c     'c'
#define c_v     'v'
#define c_b     'b'    
#define c_n     'n'
#define c_m     'm'

//  --------------- 键盘状态 --------------------
#define MODE_LEFT_SHIFT     0x01
#define MODE_RIGHT_SHIFT    0x02
#define MODE_LEFT_CTRL      0x04
#define MODE_RIGHT_CTRL     0x08
#define MODE_LEFT_ALT       0x10
#define MODE_RIGHT_ALT      0x20
#define MODE_UP_CAPS        0x40
#define MODE_DOWN_CAPS      0x80

#define LEDS_CAPS   0x04
#define LEDS_NUM     0x02
#define LEDS_SCROLL 0x01


// --------------- 部分扫描码 --------------------
#define LEFT_SHIFT      0x2A
#define RIGHT_SHIFT     0x36
#define ALT             0x38
#define CTRL            0x1D
#define CAPS            0x3A
#define TAB             0x0F
#define HOME            0x47
#define END             0x4F
#define PAGE_UP         0x49
#define PAGE_DOWN       0x51

#define DN_ARROW        0x50
#define LEFT_ARROW      0x4B
#define RIGHT_ARROW     0x4D
#define UP_ARROW        0x48

#define BACK_SPACE      0x0E
#define ENTER           0x1C
#define SPACE           0x39

#endif