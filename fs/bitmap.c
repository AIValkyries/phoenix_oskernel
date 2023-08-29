// i 节点位图，逻辑块位图
#include <kernel/flags_bit.h>
#include <fs/fs.h>
#include <kernel/string.h>
#include <kernel/kernel.h>

/*
    bts 将位n复制到进位标志位，且将目的操作数中的该位置1
    返回原位值
*/
#define set_bit(nr,addr) ({\
register int res; \
__asm__ volatile("btsl %2,%3\n\tsetb %%al": \
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})

/*
    将指定位复制到进位标志位，并且清除目的操作数中的该位
*/
#define  clear_bit(nr,addr) ({ \
register int res;  \
__asm__ volatile("btrl %2,%3\n\t setnb %%al":"=a"(res):"0"(0),"r"(nr),"m"(*(addr))); \
res;})

static int find_first_zero(char* addr)
{
int __res;
__asm__ volatile("cld\n" 
	"1:\tlodsl\n\t" 
	"notl %%eax\n\t" 
	"bsfl %%eax,%%edx\n\t" 
	"je 2f\n\t" 
	"addl %%edx,%%ecx\n\t" 
	"jmp 3f\n" 
	"2:\taddl $32,%%ecx\n\t" /*lodsl=eax*/ 
	"cmpl $8192,%%ecx\n\t" 
	"jl 1b\n" 
	"3:" 
	:"=c" (__res):"c" (0),"S" (addr)); 
return __res;
}


void free_block(unsigned int block)
{
    struct buffer_head *bh, *mp;
    int zblock_map_i;
    int block_i;

    struct m_super_block* super = &super_block;
    bh = alloc_buffer(block);

    zblock_map_i = (block) / BLOCK_SIZE;  // 清空位图
    block_i = (block) % BLOCK_SIZE;
    
    mp = super->zblock_map[zblock_map_i];
    mp->dirty = 1;
    clear_bit(block_i, mp->data);             //重新设置 block 位标志

    brelse(mp);

    bh->dirty = 1;
    brelse(bh);
}


int new_block()
{
    struct m_super_block *super = &super_block;

    volatile int index = -1;
    int i = 0;
    struct buffer_head *bh;
    unsigned short block;
    volatile char *data;

    bh = NULL;

    for(i = 0; i < super->zmap_blocks; i++)
    {
        bh    = super->zblock_map[i];
        index = find_first_zero(bh->data);
        if(index > 0)   break;
    }

    if(i > 8 || index >= 8192 || bh == NULL)
        return -1; 
    data = bh->data;

    if(set_bit(index, data))
        printk("new_block: bit already set \\n");

    // 获得空闲的 block
    block = (i * BLOCK_SIZE + index);

    printk("new_block: %d,block_addr=%x----- \\n", 
            block,(bh->block * 2 * 512));

    bh->dirty = 1;
    brelse(bh);

    // 在高缓存中申请一块缓冲块给逻辑块使用，用指定的逻辑块号
    bh = alloc_buffer(block);
    clear_block(bh->data);
    bh->dirty = 1;

    brelse(bh);
    
    return block;
}


struct m_inode* new_inode()
{
    struct m_inode* inode; 
    struct m_super_block* super;
    struct buffer_head* bh;

    // 获取空闲的 i—map
    volatile int index = -1;
    volatile char *data;

    int i = 0;

    bh = NULL;
    inode = get_minode_cachep();
    super = &super_block;

    for(i = 0; i < super->imap_blocks; i++)
    {
        bh = super->inode_map[i];
        index = find_first_zero(bh->data);
        if(index>0) break;
    }
    
    if(i > 8 || index >= 8192 || bh == NULL)
        return NULL;
    data = bh->data;
    if(set_bit(index, data))
        printk("new_inode: bit already set \\n");

    bh->dirty = 1;

    // inode 节点在磁盘中的索引
    inode->inode = i * (BLOCK_SIZE) + index;
    inode->dirty = 1;
    inode->count = 1;
    inode->uid = current->euid;
    inode->gid = current->egid;

    printk("new_inode: %d,inode_addr=%x \\n", 
        inode->inode, (bh->block * 2 * 512));

    for(i = 0; i < INODE_BLOCK_AREA_LEN; i++)
    {
        inode->block_area[i] = 0;
    }

    brelse(bh);

    return inode;
}


void free_inode(struct m_inode *inode)
{
    struct buffer_head *bh;
    struct m_super_block *super;
    int inode_map_i;
    int block_i;

    super = &super_block;

    // 清空标志位
    inode_map_i = (inode->inode) /  BLOCK_SIZE;
    block_i = (inode->inode) % BLOCK_SIZE;

    bh = super->inode_map[inode_map_i];
    
    clear_bit(block_i, bh->data);
    bh->dirty = 1;
    
    // 清空内存中的 inode 数据
    memset(inode, 0, sizeof(struct m_inode));

    brelse(bh);
}

