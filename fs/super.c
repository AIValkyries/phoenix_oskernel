#include <fs/fs.h>
#include <kernel/string.h>
#include <kernel/kernel.h>

struct m_super_block super_block;
extern void con_write();

/*
    BTS nr, addr, 测试并设置
    bt  将指定位(bitnr)复制到进位标志位,目的操作数为包含指定位的数值
        源操作数为该位在目的操作数中的位置
    返回原位值    
*/
#define set_bit(bitnr,addr) ({ \
register int __res; \
__asm__("bt %2,%3 \n\t setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \
__res; })



struct m_super_block* read_super()
{
    struct m_super_block *super;
    struct buffer_head *bh;
    int i;
    int block;
    char *tmpdata;

    super = &super_block;
    bh = bread_block(ROOT_INO);

    *((struct s_super *) super) = *((struct s_super*) (bh->data));

    printk("imap_blocks=%x,zmap_blocks=%x \\n",super->imap_blocks,super->zmap_blocks);
    printk("inode_nos=%d,block_nos=%d \\n",super->inode_nos,super->block_nos);

    brelse(bh);
    
    // i 节点位图
    for(i = 0; i < super->imap_blocks; i++)
    {
       super->inode_map[i] = bread_block(ROOT_INO + 1 + i);
    }
    
    // 逻辑块位图
    for(i = 0; i < super->zmap_blocks; i++)
    {
        block = ROOT_INO + super->imap_blocks + 1 + i;
        super->zblock_map[i] = bread_block(block);
    }

    return super;
}


static void show_free_block(struct m_super_block *p)
{
    int i;
    int free;

    free = 0;
    i = p->block_nos;       // 总的逻辑块数

    while (--i >= 0)
    {
       	if (!set_bit(i&8191,p->zblock_map[i>>13]->data))
			free++;
    }

    printk("free_block=%d,block_map=%d \\n",free, p->zmap_blocks);
}


static void show_free_inode(struct m_super_block *p)
{
    int i;
    int free;

    free = 0;
	i = p->inode_nos + 1;

    while (--i >= 0)
    {
        if(!set_bit(i&8191,p->inode_map[i>>13]->data))
            free++;
    }

    printk("free_inode=%d,inode_map=%d \\n",free, p->zmap_blocks);
}

/*
    读取根文件系统,从磁盘上
*/
int do_mount_root()
{
    struct m_super_block *p;
    struct m_inode *root;

    int i;
    int free;

    p    = read_super();
    root = iget(ROOT_INO);

    p->root = root;

    current->root = root;
    current->pwd  = root;
    
    // 创建文件夹？删除目录?
    // 创建文件？
    /*
        . 1
        .. 1
        etc 2
        dev 3
        bin 4
        usr 5
        usr/bin 6
        var 7
    */
    
    return 1;
}



void fs_init()
{
    first_file = ((void*)0);
    nr_files = 0;
    head_cachep = ((void*)0);
}
