#include <fs/fs.h>
#include <kernel/system.h>

/*
    文件读取
    {
        目录文件
        ELF
    }
*/
int file_read(
            struct m_inode *inode, 
            struct file *file, 
            char *buf,    // 用户空间的虚拟地址指针
            int count     /* 用户缓存空间的大小 */)
{
    int block_index = file->pos >> BLOCK_SIZE_BIT;
    int offset      = file->pos & (BLOCK_SIZE - 1);

    int read;
    int chars;
    int block;

    struct buffer_head *bh;
    char* p;    // 内核空间的虚拟地址指针

    read = 0;
    bh = NULL;

    while (count > 0)
    {
        chars = BLOCK_SIZE - offset;
        if(chars > count) chars = count;

        if((block = bmap(inode, block_index)))
            bh = bread_block(block);
        
        block_index++;
        p = offset + bh->data;
        offset = 0;

        count -= chars;
        file->pos += chars;
        read += chars;

        while (chars-->0)
        {
            put_fs_byte(*(p),buf);    // fs 段，用来作为 kernel 与 user 之间的数据传输
            p++;
            buf++;            
        }

        brelse(bh);
    }

    return read;
}


int file_write(
    struct m_inode * inode, 
    struct file * file, 
    char * buf, 
    int count)
{
    int pos    = inode->size;
    int offset = pos & (BLOCK_SIZE-1);
    
    int written;
    int chars;
    int block, block_index;

    struct buffer_head *bh;
    char *p;

    //if(IS_APPEND(file->mode))   // 添加至文件末尾
        //pos = inode->size;
    
    written = 0;

    while (count > 0)
    {
        chars = BLOCK_SIZE - offset;
        if(chars > count ) chars = count;

        block_index = pos / BLOCK_SIZE;
        if(!(block = bmap(inode, block_index)))
            block = create_block(inode,block_index);

        bh = bread_block(block);

        p = offset + bh->data;
        offset = 0;

        written += chars;
        count -= chars;
        pos += chars;

        if(pos > inode->size)  // 重新设定 size
        {
            inode->size = pos;
            inode->dirty = 1;
        }

        while (chars-->0)
        {
            *p = get_fs_byte(buf);
            
            p++; buf++;
            bh->dirty = 1;
        }

    }

    return written;
}