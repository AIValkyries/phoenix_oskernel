#ifndef _FS_STAT_H
#define _FS_STAT_H

/*
    文件类型和属性(rwx位)
    15-12文件类型
        {
            001-FIFO 文件（八进制）
            002-字符设备文件
            004-目录文件
            006-块设备文件
            010-常规文件
        }
        11-9执行文件时设置的信息
        {
            01-执行时设在用户ID （set-user-ID）
            02-执行时设在组ID
            04-对于目录，寿险删除标志
        }
        8-0表示文件的访问权限
        {
            0-2(RWX) 其他人
            3-5(RWX) 组员
            6-8(RWX) 宿主
        }
*/
#define FILE_PIPE (0x0001<<12)     // 管道设备文件
#define FILE_CHR  (0x0002<<12)     // 字符设备文件
#define FILE_DIR  (0x0004<<12)     // 目录文件
#define FILE_BLK  (0x0006<<12)     // 块设备文件
#define FILE_NORMAL  (0x008<<12)  // 常规文件

#define SET_UID_ID  (0x01<<9)
#define SET_GID_ID  (0x02<<9)
#define SET_ISVTX   (0x04<<9)       // 需要root权限

// 其他人(0-2)
#define IRWXO   0x07
#define IROTH   0x01
#define IWOTH   0x02
#define IXOTH   0x04

// 组员(3-5)
#define IRWXG   0x38
#define IRGRP   0x20
#define IWGRP   0x10
#define IXGRP   0x08

// 宿主(6-8)
#define IRWXU   0x1c0     
#define IRUSR   0x100
#define IWUSR   0x080
#define IXUSR   0x040


#define IS_CHR(type)  (type & FILE_CHR)
#define IS_BLK(type)  (type & FILE_BLK)
#define IS_DIR(type)  (type & FILE_DIR)
#define IS_PIPE(type) (type & FILE_PIPE)
#define IS_REG(type)  (type & FILE_NORMAL)


/*
	SEEK_SET  文件的开头
	SEEK_END  文件的末尾
	SEEK_CUR  基于当前指针的位置
*/
typedef unsigned long seek_type;

#define SEEK_SET 0
#define SEEK_END 1
#define SEEK_CUR 2


// 标志
#define FILE_READ  0x01
#define FILE_WRITE 0x02
#define FILE_RW  (FILE_READ | FILE_WRITE)
#define FILE_CREATE 0x04
#define FILE_APPEND 0x08  // 添加至文件末尾


#define IS_READ(mode)  (mode & FILE_READ)
#define IS_WRITE(mode) (mode & FILE_WRITE)
#define IS_RW(mode)    (mode & FILE_RW)
#define IS_APPEND(mode) (mode & FILE_APPEND)


#endif