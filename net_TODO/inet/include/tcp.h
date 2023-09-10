#ifndef _TCP_H
#define _TCP_H

#include <dev.h>

#define MAX_SYN_SIZE    MAX_HEADER
#define MAX_FIN_SIZE    MAX_HEADER
#define MAX_ACK_SIZE    MAX_HEADER
#define MAX_RESET_SIZE  MAX_HEADER


// 20 字节
struct tcphdr
{
    unsigned short source;
    unsigned short dest;
    unsigned long seq;
    unsigned long ack_seq;
    
    unsigned short res1:4,
                    doff:4,
                    fin:1,
                    syn:1,
                    rst:1,
                    psh:1,
                    ack:1,
                    urg:1,
                    res2:2;
    unsigned short window;
    unsigned short check;
    unsigned short urg_ptr;
};



#endif