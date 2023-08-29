/*
    硬盘请求队列
*/
#include <kernel/atomic.h>
#include <kernel/flags_bit.h>
#include <fs/fs.h>
#include <hd.h>

// 队列是否空闲
static struct wait_queue_head wait_for_request;   //等待请求队列

static struct hd_request* get_request()
{
    struct hd_request *req;
    int i;
   
repeat: 
    for(i = 0;i < NR_REQUEST; i++)
    {
        if(!atomic_read(&request[i].lock))
        {
            req = &request[i];
            break;
        }
    }

    if(req == (void*)0)
    {
        sti();  // 开中断
        sleep_on(&wait_for_request);
        goto repeat;  // 重新获取
    }

    return req;
}


// 结束请求
void end_request(struct hd_request *req)
{
    struct hd_request *prev;

    // 需要解锁缓冲区？
    mutex_unlock(&req->lock);  // 释放锁
    mutex_unlock(&req->bh->lock);

    prev       = first_req;
    first_req  = first_req->next;

    wake_up(&req->bh->lock_wait);
    wake_up(&wait_for_request);
}

// 发送 I/O 请求
void read_write_block(int rw, struct buffer_head *bh)
{
    //cli();      // 关闭中断

    struct hd_request *req;
    struct hd_request *tmp; 
    struct hd_request *prev;

    req = get_request();
    
    mutex_lock(&req->lock);   // 标识为被占用
    mutex_lock(&bh->lock);
    
    req->cmd    = rw;
    req->errors = 0;
    req->bh     = bh;
    req->nr_sectors = 2;    // 一次读取2个扇区 一个的 block 大小
    
    // [扇区号 = 一个 block 2 个扇区号]
    req->sector = bh->block * 2;   
    req->buffer = bh->data;
    
    // 队列进行排序
    if(first_req == (void*)0)
    {
        first_req = req;
        do_hd_request();  // 发出请求
    }
    else
    {
        tmp = first_req;

        // tmp 进行移动
        while (tmp)
        {
            if(req->sector > tmp->sector)
                break;
            
            prev = tmp;
            tmp = tmp->next;
        }

        req->next = tmp;
        prev->next = req;
    }

    //sti();
}