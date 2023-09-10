#ifndef _SKBUFF_H
#define _SKBUFF_H

#include <dev.h>

// sk_buff is 队列
struct sk_buff
{
    struct sk_buff *next;
    struct sk_buff *prev;
    struct sock *sk;

    struct ethhdr  *eth;
    struct iphdr   *iph;
    struct tcphdr  *tph;

    unsigned long mem_len;      // sk 包的长度
    void* mem_addr;     // 内存地址

    // 需要吗？
    unsigned long daddr;        // 目标地址
    unsigned long saddr;        // 源地址
    unsigned long dest;         // 目标端口
    unsigned long source;       // 源端口

    unsigned char data[0];
};


extern struct sk_buff *alloc_skb(unsigned long size);
extern void free_skb(struct *sk_buff sk);


extern void skb_queue_head(struct sk_buff *list, struct sk_buff *buf);
extern void skb_queue_tail(struct sk_buff *list, struct sk_buff *buf);
extern struct sk_buff * skb_peek(struct sk_buff *list);
extern struct sk_buff * skb_dequeue(struct sk_buff *list);


#endif