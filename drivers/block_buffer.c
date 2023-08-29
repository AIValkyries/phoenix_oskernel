/*
    页高速缓存
*/
#include <fs/fs.h>
#include <kernel/kernel.h>

static struct buffer_head *start_buffer;

// 活跃 的链表,400 个，大约 3 kib左右
struct buffer_head *active_hash_table[BUFFER_HASH_NO];
struct buffer_head *free_list_table;
// 缓冲是否为空，是否有的 wait
static struct wait_queue_head bh_wait;
static int nr_buffers;
static int active_buffer_count;


#define hash_block(block) active_hash_table[block%BUFFER_HASH_NO]

// 输出高速缓存的信息
void show_active_buffer_head()
{
    int i;
    struct buffer_head *bh;

    printk("buffer count %d \n",nr_buffers);

    for(i = 0; i < BUFFER_HASH_NO; i++)
    {
        if(active_hash_table[i] != NULL)
        {
            printk("count: %d - lock: %d - block: %d \n",active_hash_table[i]->count,
                active_hash_table[i]->lock,active_hash_table[i]->block);

            bh = active_hash_table[i]->hash_next;

            while (bh != NULL && bh != active_hash_table[i])
            {
                printk("hash_next count: %d - lock: %d - block: %d \n",bh->count, bh->lock, bh->block);
                bh = bh->hash_next;
            }
        }
    }
}


void update_active_hash(struct buffer_head *buffer)
{
    // 移除空闲链表
    buffer->free_prev->free_next = buffer->free_next;
    buffer->free_next->free_prev =  buffer->free_prev;

    if(free_list_table == buffer)
        free_list_table = buffer->free_next;

    unsigned long hash_index;

    hash_index = buffer->block%BUFFER_HASH_NO;
    if(active_hash_table[hash_index] == NULL)
        active_hash_table[hash_index] = buffer;
    else
    {
        if(active_hash_table[hash_index]->hash_next != NULL)
        {
            active_hash_table[hash_index]->hash_next->hash_prev = buffer;
            buffer->hash_next = active_hash_table[hash_index]->hash_next;
        }

        active_hash_table[hash_index]->hash_next = buffer;
        buffer->hash_prev = active_hash_table[hash_index];
    }
}


void free_active_hash(struct buffer_head *buffer)
{
    if(buffer->hash_next)
        buffer->hash_next->hash_prev = buffer->hash_prev;
    if(buffer->hash_prev)
        buffer->hash_prev->hash_next = buffer->hash_next;

    // 移除 hash 表
    unsigned long hash_index;

    hash_index = buffer->block%BUFFER_HASH_NO;
    if(active_hash_table[hash_index] == buffer)
    {
        active_hash_table[hash_index] = buffer->hash_next;
    }
    else
    {
        buffer->hash_next->hash_prev = buffer->hash_prev;
        buffer->hash_prev->hash_next = buffer->hash_next;
    }
}

int sync_buffer(struct buffer_head *bh)
{
    if(!bh) return -1;
    
    read_write_block(FILE_WRITE, bh);
}

int sync_dev()
{
    struct buffer_head *bh;
    int i;
    
    bh = start_buffer;

    for (i = 0; i < nr_buffers; i++,bh++)
    {
        if(!mutex_lock_test(&bh->lock))
            continue;
        if(!bh->dirty)
            continue;

        read_write_block(FILE_WRITE, bh);
    }

}

static struct buffer_head* get_hash_buffer(unsigned short block)
{
    struct buffer_head *temp;

repeat:
    for(temp = hash_block(block); temp!=NULL; temp = temp->hash_next)
    {
        if(temp->block != block)
            continue;
        
        if(atomic_read(&temp->lock))
        {
            sleep_on(&temp->lock_wait);
            goto repeat;
        }

        atomic_inc(&temp->count);
        return temp;
    }
    return NULL;
}

// 从高速缓存中分配一个 缓冲块
struct buffer_head* alloc_buffer(int block)
{
    struct buffer_head *bh;

repeat:
    bh = get_hash_buffer(block);
    if(bh != NULL) return bh;

    bh = free_list_table;
    free_list_table = free_list_table->free_next;

    if(bh == NULL)           // 没有空闲的 buffer，缓冲区被用完了
    {
        sleep_on(&bh_wait);  // current 需要睡眠
        goto repeat;         // 等待睡眠进程被唤醒时，则重新 getblk
    }

    if(atomic_read(&bh->lock))       // 被锁定，重新 find
        goto repeat;

    // 回写
    if(bh->dirty)
    {
        sync_dev();     // 同步

        if(mutex_lock_test(&bh->lock))   // 写入磁盘的时候，buffer 会被锁定
        {
            sleep_on(&bh->lock_wait);
            goto repeat;
        }
    }

    atomic_add(&bh->count,1);
    bh->dirty = 0;
    bh->block = block;
    active_buffer_count++;

    // 更新活跃链表
    update_active_hash(bh);
    
    return bh;
}


void brelse(struct buffer_head *bh)
{
    // 被锁定
    if(atomic_read(&bh->lock))
        sleep_on(&bh->lock_wait);

    atomic_dec_and_test(&bh->count);

    if(bh->count.counter <= 0)
    {
        if(bh->dirty)
            sync_buffer(bh);
        
        active_buffer_count--;
        free_active_hash(bh);   // 添加至空闲链表
        wake_up(&bh_wait);
    }
}


struct buffer_head *bread_block(int block)
{
    struct buffer_head *bh;
    int tmp_block;
    
    tmp_block = block + NR_FS_START;

    bh = alloc_buffer(tmp_block);
    read_write_block(FILE_READ, bh);   // 发送读取请求，以及上锁
    
    // 所以 磁盘 I/O 目前是同步操作
/*     if(atomic_read(&bh->lock))
        sleep_on(&bh->lock_wait); */

    return bh;
}


// 读取磁盘数据至 page
long bread_page(int block[4], struct page *page)
{
    int i;
    unsigned long paddr;
    struct buffer_head *bh[4];      // page = 4 个 buffer_head

    for(i = 0; i < 4; i++)
    {
        if(bh[i] = alloc_buffer(block[i]))
        {
            read_write_block(FILE_READ, bh[i]);
        }
    }

    // 0x812000
    paddr = (__page_to_pfn(page) << PAGE_SHIFT);
    // 从 buffer_head 中 copy 数据至 page (paddr)
    for(i = 0; i < 4; i++)
    {
        if(bh[i])
        {
            if(atomic_read(&bh[i]->lock))
                sleep_on(&bh[i]->lock_wait);        // 等待解锁

            copy_blk(bh[i]->data, paddr);
            paddr += BLOCK_SIZE;
            
            brelse(bh[i]);                          // 释放缓冲区
        }
    }
    
}

/*
    end = 0x1000000 |  [ 1MiB ~ 8MiB ] , [ 7168 个 buffer ] [0x700000 / 1024 = 7168]
    可以通过内核虚拟地址直接映射,到 16MiB 结束，内核有4个初始页表，可以直接映射 16MiB 的物理空间
    ERROR = physical address not available
*/
void buffer_init(unsigned long buffer_memory_end)
{
    struct buffer_head *b;

    start_buffer = (struct buffer_head*)(0x100000);
    b = start_buffer;
    nr_buffers = 0;

    // 1024
    while ((buffer_memory_end -= BLOCK_SIZE)>=(b+1))
    {
        b->dirty = 0;
        b->count.counter = 0;
        b->lock.counter = 0;
        b->free_next = b+1;
        b->free_prev = b-1;
     
        b->data = (char*)(buffer_memory_end);
        
        nr_buffers++;
        b++;
    }

    b--;
    free_list_table = start_buffer;
    free_list_table->free_prev = b;

    b->free_next = free_list_table;
    active_buffer_count = 0;
}

