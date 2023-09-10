#ifndef _SOCK_H
#define _SOCK_H


struct sock
{
    // 关于缓存空间，接收端 | 发送端

    unsigned long wmem_alloc;       // send
    unsigned long rmem_alloc;
    unsigned short sndbuf;
    unsigned short rcvbuf;
    

    /* ----------------  接收端变量 ---------------- */

   // 接收端队列
   void *wback = NULL;
   void *wfront = NULL;

   // 拥塞控制
   unsigned long max_window;
   unsigned long cong_window;       // 窗口增加值
   unsigned long cong_count;        // 拥塞避免的长度值
   unsigned long ssthresh;          // 阈值
   unsigned short packets_out;      // 本地已发送但尚未得到应答的数据包数目

    // 接收端序号


   /* ----------------  发送端变量 ---------------- */

    // 发送端序号
    unsigned long write_seq;        // 发送序号的来源
    unsigned long sent_seq;         // 发送的序号
    unsigned long window_seq;       // 上次接收到 seq + tcphead 中的 window
    unsigned long rcv_ack_seq;      // 对端最后确认的一个序列号
};



#endif