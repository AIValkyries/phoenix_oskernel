#include <fs/fs.h>
#include <kernel/atomic.h>
#include <kernel/kernel.h>


struct m_inode *head_cachep;         // inode 缓存的头指针
struct kmem_cache_struct *inode_cachep;          // inode 的结构

/*
    可以将 inode 看做是块数组
*/

//　等待该节点
static void test_and_wait_inode(struct m_inode *inode) 
{
    if(atomic_read(&inode->sem.count) < 0)
    {
        sem_down(&inode->sem);
    }
}


static void lock_inode(struct m_inode* inode)
{
    sem_down(&inode->sem);
}


static void unlock_inode(struct m_inode* inode)
{
    sem_up(&inode->sem);
}


static void remove_list(struct m_inode* inode)
{
    if(head_cachep->prev == inode)  // 处于 tail
    {
        head_cachep->prev = inode->prev;
        inode->prev->next = head_cachep;
    }
    else
    {
        inode->prev->next = inode->next;
        inode->next->prev = inode->prev;
    }
}

/*
    释放 i 节点的所有 block, inode 是 block 数组
*/
extern void free_inode_block(struct m_inode* inode)
{
    int i;
    struct buffer_head *bh;
    struct buffer_head *twice_bh;
    unsigned short *twice_p;
    unsigned short *p;

    for(i = 0; i < (INODE_BLOCK_AREA_LEN-2); i++)
    {
        if(inode->block_area[i])
            free_block(inode->block_area[i]);
    }

    // 一次间接块
    if(inode->block_area[7])
    {
        bh = bread_block(inode->block_area[7]);
        p  = (unsigned short*) bh->data;

        for(i = 0 ;i < 512; i++)  // 一个逻辑块 有 2048 个 block 号
        {
            if(*p)
            {
                free_block(*p);
                bh->dirty = 1;
            }

            p++;
        }

        free_block(inode->block_area[7]);

        brelse(bh);
    }

    // 二次间接块
    if(inode->block_area[8])
    {
        bh = bread_block(inode->block_area[8]);
        p = (unsigned short*)bh->data;

        for(i = 0;i<512;i++)
        {
            if(*p)
            {
                twice_bh = bread_block(*p);
                twice_p = (unsigned short*)twice_bh->data;

                for(int j=0;j<512;j++)
                {
                    if(*twice_p)
                    {
                        free_block(*twice_p);
                        twice_bh->dirty = 1;
                    }

                    twice_p++;
                }

                brelse(twice_bh);
                free_block(*p);
                bh->dirty = 1;
            }

            p++;      
        }

        free_block(inode->block_area[8]);
        brelse(bh);
    }

    inode->dirty = 1;
}


struct m_inode* get_minode_cachep()
{
    struct m_inode *inode = (struct m_inode*)kmem_object_alloc(inode_cachep);
    inode->count++;

    if(head_cachep == NULL)
    {
        head_cachep = inode;
        head_cachep->next = inode;
        head_cachep->prev = inode;
    }
    else
    {        
        // 插入尾部
        head_cachep->prev->next = inode;
        inode->prev = head_cachep->prev;

        head_cachep->prev = inode;
        inode->next = head_cachep;
    }

    return inode;
}

// 其实是将 inode 的数据填充到 inode 对应的 buffer
void write_inode(struct m_inode* inode)
{
    //lock_inode(inode);

    if(!inode->dirty)
    {
        unlock_inode(inode);
        return;
    }

    unsigned short inode_block;
    unsigned short inode_buffer_i;
    struct buffer_head *bh;

    unsigned long inode_start_block = 
        super_block.imap_blocks + super_block.zmap_blocks + ROOT_INO;

    inode_block = inode_start_block + (inode->inode - 1 + INODES_PER_BLOCK) / INODES_PER_BLOCK;
   
    printk("write_inode block=%x \\n",inode_block);

    bh = bread_block(inode_block);

    inode_buffer_i = (inode->inode - 1) % INODES_PER_BLOCK;

    // 在缓冲块中存入节点
    ((struct s_inode*)bh->data)[inode_buffer_i] = *(struct s_inode*) inode;

    bh->dirty = 1;    // 缓冲区变脏
    inode->dirty = 0;

    brelse(bh);

    //unlock_inode(inode);
}


/*
    从磁盘中读取 inode,一个 block 120 左右 inode
    65536 / 120 = [547 个 block] 大概需要 547个 block
*/
void read_inode(struct m_inode *inode)
{
    //lock_inode(inode);

    unsigned short block;
    struct buffer_head* bh;
    unsigned short inode_buffer_i;

    unsigned long inode_start_block = 
        super_block.imap_blocks + super_block.zmap_blocks + ROOT_INO;

    block = inode_start_block + (inode->inode-1 + INODES_PER_BLOCK) / INODES_PER_BLOCK;
    
    bh = bread_block(block);
    inode_buffer_i = (inode->inode - 1) % INODES_PER_BLOCK;
    
    *(struct s_inode*) inode = ((struct s_inode*)bh->data)[inode_buffer_i];

    brelse(bh);

    //unlock_inode(inode);
}


struct m_inode* iget(int inode)
{
    struct m_inode *tmp = head_cachep;
    
    // 从现有之中进行加载
    while (tmp)
    {
        if(tmp->inode == inode)
        {
            test_and_wait_inode(tmp);

            if(tmp->inode != inode)
            {
                tmp = head_cachep;
                continue;
            }
            
            tmp->count++;
            break;
        }

        if(tmp->next == head_cachep)
        {
            tmp = NULL;
            break;
        }

        tmp = tmp->next;
    }

    // 如果没有则从磁盘中加载
    if(tmp == NULL)
    {
        tmp = get_minode_cachep();
        tmp->inode = inode;
        read_inode(tmp);
    }

    return tmp;
}


// 将一个i节点写回至设备
void iput(struct m_inode *inode)
{
    if(inode == NULL)
        return;
    
    test_and_wait_inode(inode);
        
    if(inode->count == 1)
    {
        //cli();  // 关闭中断
        kmem_object_free(inode_cachep,  (void*)inode);
        remove_list(inode);
        inode->count = 0;
        
        if(inode->dirty)
            write_inode(inode);
        
        //sti();

        return;
    }

    inode->count--;    
}


// 映射根据 i 节点的索引
int __bmap(struct m_inode *inode, int block_index, int create)
{
    struct buffer_head* bh;
    unsigned short block;
    unsigned short i;

    if(block_index < 7)
    {
        if(create && !inode->block_area[block_index])
        {
            inode->block_area[block_index] = new_block();
            inode->dirty = 1;
        }
        
        return inode->block_area[block_index];
    }

    block_index -= 7;

    if(block_index < 512)   // 第一个间接块
    {
        if(create && !inode->block_area[7]) // 首次使用间接块
        {
            inode->block_area[7] = new_block();
            inode->dirty = 1;
        }

        bh = bread_block(inode->block_area[7]);
        if(!bh) return 0;

        block = ((unsigned short*)bh->data)[block_index];
        if(create && !block)
        {
            if(block = new_block())
            {
                ((unsigned short*)bh->data)[block_index] = block;
                bh->dirty = 1;
            }
        }

        brelse(bh);
        return block;
    }

    block -= 512;       // 二次间接块

    if(create && inode->block_area[8])
    {
        inode->block_area[8] = new_block();
        inode->dirty = 1;
    }

    bh = bread_block(inode->block_area[8]);
    if(bh == NULL)  return 0;

    // 间接块的索引
    i = ((unsigned short*)bh->data)[block_index>>9];     // 和/512相同的结果

    if(create && !i)
    {
        if(i = new_block())
        {
            ((unsigned short*)bh->data)[block_index>>9] = i;
            bh->dirty = 1;
        }
    }

    brelse(bh);

    if(!(bh = bread_block(i)))
        return 0;

    block = ((unsigned short*)bh->data)[block_index & 511];
    if(create && !block)
    {
        if(block = new_block())
        {
            ((unsigned short*)bh->data)[block_index & 511] = block;
            bh->dirty = 1;
        }
    }
    brelse(bh);
    return block;
}


/*
    给 m_inode 创建 block
    一个 inode 最多 8+2048 = 2056 个块
*/
int create_block(struct m_inode *inode, int block_index)
{
    return __bmap(inode,block_index, 1);
}

int bmap(struct m_inode * inode, int block_index)
{
    return __bmap(inode,block_index, 0);
}

void sync_inodes()
{
    if(head_cachep == NULL)
        return;

    struct m_inode *inode = head_cachep;

    while (inode)
    {
        write_inode(inode);
 
        if(inode->next == head_cachep)
            break;
        
        inode = inode->next;
    }
    
}