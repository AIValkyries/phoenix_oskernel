#include <fs/fs.h>
#include <kernel/system.h>

/*
    @pos  = 读取的位置 file 中的 pos 字段
    char* = 数据来源
    count = 数据长度
*/
int block_write(
    unsigned long * pos, 
    char * buf, 
    int count)
{
    int block  = *pos >> BLOCK_SIZE_BIT;
    int offset = *pos & (BLOCK_SIZE - 1);
    int written;
    int chars;

    char* p;

    struct buffer_head* bh;

    written = 0;

    while (count > 0)
    {
        chars = BLOCK_SIZE - offset;
        if(chars > count) chars = count;

        bh = bread_block(block);
        block++;

        p = offset + bh->data;

        offset = 0;
        count -= chars;
        written += chars;
        *pos += chars; // ?? 写入和 read 需要两个不同的 file？
        
        while (chars-->0)
        {
            *p = get_fs_byte(buf);      // 从用户空间写入数据
            p++;
            buf++;
        }

        bh->dirty = 1;
    }

    return written;
}

/*
    直接从 block 中读取
*/
int block_read(
    unsigned long * pos, 
    char * buf, 
    int count)
{
    // 直接对块进行读取
    int block  = *pos >> BLOCK_SIZE_BIT;
    int offset = *pos & (BLOCK_SIZE-1);
    int read;
    int chars;
    char* p;

    struct buffer_head* bh;

    read = 0;

    while (count > 0)
    {
        chars = BLOCK_SIZE - offset;

        if(chars > count)  chars = count;

        bh = bread_block(block);
        block++;

        *pos += chars;
        read += chars;
        count -= chars;

        p = offset + bh->data;
        offset = 0;

        while (chars-->0)
        {
            put_fs_byte(*p, buf);    // 向用户空间写入数据
            buf++;
            p++;
        }
    }

    return read;
}