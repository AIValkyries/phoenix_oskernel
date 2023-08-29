#include <tty.h>
#include <fs/fs.h>
#include <keyboard.h>


extern void put_queue(char c);
extern void read_char(char *c);
extern void con_write();


/*
    将终端的数据 copy 到用户数据区 buf
*/
int tty_read(char *buf, int nr)
{
    char c;
    char *b = buf;

    while (nr > 0)
    {
        read_char(&c);
        put_fs_byte(c, b++);
        nr--;
    }
    
    return b-buf;
}

// 将用户下的数据(buf) copy 到 con_write 的队列中
int tty_write(char *buf, int nr)
{
    int fd;
    int i;
    char c, *b = buf;
    
    while (nr>0)
    {
        c = get_fs_byte(b++);
        put_queue(c);
        nr--;
    }
    
    put_queue(LF_NL);
    con_write();

    return b-buf;
}

void tty_init()
{
    int i;

    for(i = 0;i < NR_TTY;i++)
    {
        con_table[i].queue.tail_ptr = 0;
        con_table[i].queue.head_ptr = 0;
    }
}