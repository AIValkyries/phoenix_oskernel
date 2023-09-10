#ifndef _IP_H
#define _IP_H

// 20 字节
struct iphdr
{
    unsigned char ihl:4,
                version:4;
    unsigned char tos;
    unsigned short tot_len;

    // 数据包分片相关
    unsigned short id;
    unsigned short frag_off;

    unsigned char ttl;      // 寿命，跳转
    unsigned char protocol;
    unsigned short check;

    unsigned long saddr;    // 源ip地址
    unsigned long daddr;    // 目标ip地址
};




#endif