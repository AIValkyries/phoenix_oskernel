// 文件系统的系统调用
#include <fs/fs_stat.h>
#include <fs/fs.h>
#include <kernel/sched_os.h>

// 对于文件的读写操作
extern int file_read(
              struct m_inode *inode, 
              struct file *file, 
              char *buf,     // 用户的缓存空间
              int  count     /* 用户缓存空间的大小 */);
extern int file_write(struct m_inode * inode, struct file * file, char * buf, int count);

// 直接对块进行读写, 而非文件
extern int block_write(unsigned long * pos, char * buf, int count);
extern int block_read(unsigned long * pos, char * buf, int count);


// 返回句柄，fd
int do_open(const char *filename, unsigned short mode, int flags)
{
    int fd;
    struct m_inode* inode;
    struct file *f;

    for(fd = 0;fd < NR_FILE; fd++)
    {
        if(!current->filps[fd])  // so 在创建进程的时候，该 filps 数据会被 copy， 因此它是全局的
            break;
    }
    
    f = get_empty_filp();
    if(!f)  return -1; 
    
    inode = open_and_create(filename, mode, flags);
    current->filps[fd] = f;

    f->inode = inode;
    f->mode  = mode;
    f->pos   = 0;
    f->used  = 1;
    
    iput(inode);

    return fd;
}

int do_close(unsigned int fd)
{
    struct file* filp = current->filps[fd];
    if(!filp)  return -1;

    current->filps[fd] = NULL;

    if(--filp->used)
        return 0;

    iput(filp->inode);  // 释放该 i 节点

    return 1;
}

// 返回偏移
int do_lseek(unsigned long fd, unsigned long offset, seek_type origin)
{
    struct file* filp = current->filps[fd];
    if(!filp) return 0;

    switch (origin)
    {
        case SEEK_CUR:    // 基于当前指针的位置
            filp->pos += offset;
            break;
        case SEEK_END:    // 文件的末尾
            filp->pos = filp->inode->size + offset;
            break;
        case SEEK_SET:   // 文件的开头
            filp->pos = offset;
            break;
        default:
            return -1;
    }
    
    return filp->pos;
}

/*
    buf   = 用户空间地址
    count = 长度
*/
int do_read(unsigned long fd,  char* buf, int count)
{
    struct file* file = current->filps[fd];
    if(!file) return 0;

    struct m_inode *inode = file->inode;
    
    if(IS_BLK(inode->mode))
        return block_read(&file->pos, buf, count);

    //  目录文件，或者普通文件
    if(IS_DIR(inode->mode) || IS_REG(inode->mode))
    {
        if(count + file->pos > inode->size)
            count = inode->size - file->pos;

        if(count <= 0) return 0;

        return file_read(inode, file, buf, count);
    }


    return -1;
}


int do_write(unsigned long fd, char* buf, int count)
{
    struct file* file = current->filps[fd];
    if(!file) return 0;

    struct m_inode* inode = file->inode;

    if(IS_BLK(inode->mode))
        block_write(&file->pos, buf, count);

    // 如果是目录文件，以及普通文件
    if(IS_DIR(inode->mode) || IS_REG(inode->mode))
    {
        return file_write(inode, file, buf, count);
    }

    return -1;
}

// 创建文件
int do_create(const char *pathname, int flags)
{
    //常规文件 | 所有人可读写
    return do_open(pathname, (FILE_NORMAL | 0777), flags);
}

// 修改文件宿主
int do_chown(const char *filename,int uid,int gid)
{

}

// 修改文件属性
int do_chmod(const char *filename,int mode)
{

}

// 修改当前进程的工作目录
int do_chdir(const char *filename)
{

}