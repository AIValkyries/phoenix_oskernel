#include <fs/fs.h>
#include <kernel/sched_os.h>
#include <kernel/string.h>
#include <kernel/kernel.h>

/*
    目录文件系统
    目录是 inode 数组
*/

int match(int len, char *name, struct dir_entry *de)
{
    register int same;

    if (!len && (de->name[0]=='.') && (de->name[1]=='\0'))
		return 1;
    
    if (len < MAX_NAME_LEN && de->name[len])
		return 0;

    __asm__("cld\n\t"
		"fs\n\t" 
		"repe\n\t" 
		"cmpsb\n\t"
		"setz %%al\n\t"
		:"=a" (same)
		:"0" (0),"S" ((long) name),"D" ((long) de->name),"c" (len));

    return same;
}

//  从 inode 中 find entry
static struct buffer_head* find_entry(
    struct m_inode *dir,
    char *entry_name,
    int namelen,
    struct dir_entry **res_dir)
{
    struct buffer_head* bh;
    struct dir_entry* de;

    int entries = 0,i = 0,block = 0;

    *res_dir = NULL;

    // inode 是 block 的数组
    if(!dir->block_area[0])  return NULL;

    bh = bread_block(dir->block_area[0]);
    if(!bh) return NULL;

    entries = dir->size / sizeof(struct dir_entry);
    de = (struct dir_entry*)bh->data;

    while (i <= entries)
    {
        if((char*)de >= (bh->data + BLOCK_SIZE))   // 读取磁盘
        {
            brelse(bh);  bh = NULL;
            
            block = bmap(dir, i/DIR_NO_PRE_BLOCK);
            bh = bread_block(block);

            de = (struct dir_entry *) bh->data;
        }

        // 字符串比较
        if(match(namelen, entry_name, de))
        {
            *res_dir = de;
            return bh;
        }

        de++;
        i++;
    }

    brelse(bh);

    return NULL;
}


static struct buffer_head* add_entry(
    struct m_inode *dir,
    char *name,
    int namelen,
    unsigned short inode,
    struct dir_entry **res_dir)
{
    struct buffer_head* bh;
    struct dir_entry*   de;

    int i = 0, block = 0, j=0;
    int entries = 0;

    if(!dir->block_area[0])  return NULL;

    printk("add entry =%x \\n",(dir->block_area[0] + NR_FS_START) * 2 * 512);
    
    bh = bread_block(dir->block_area[0]);
    if(!bh)
    {
        brelse(bh);
        return NULL;
    }

    de = (struct dir_entry*)bh->data;

    while (1)
    {
        if((char*)de >= (bh->data + BLOCK_SIZE))   // 下一个 block
        {
            brelse(bh);  bh = NULL;

            block = create_block(dir,i / DIR_NO_PRE_BLOCK);   // 如果开没有该 block
            bh = bread_block(block);
            de = (struct dir_entry *) bh->data;
        }

        if(i * sizeof(struct dir_entry) >= dir->size)   // 目录项末尾，没有删除过目录项的痕迹
        {
            // 为目录建立新的目录项
            de->inode = 0;
            dir->size += sizeof(struct dir_entry);
            dir->dirty = 1;
        }

        if(!de->inode)  // 空闲的 目录项
        {
            
            de->inode = inode;
            bh->dirty = 1;

            put_queue(*name);
            con_write();
            printk("   t t\\n");

            for(j = 0; j < namelen; j++)
            {
                de->name[j] = get_fs_byte(name);
                name++;
            }

            *res_dir = de;
            return bh;
        }

        de++;
        i++;   
    }

    brelse(bh);
    return NULL;
}

// 删除目录项
static struct buffer_head* remove_entry(
    struct m_inode *dir,
    struct buffer_head* bh,
    char *name,
    int namelen)
{   
    struct dir_entry *de;
    int i,j,block;

    i = j = 0;
    block = 0;

    de = (struct dir_entry*)bh->data;

    while (1)
    {
        if((unsigned char*)de >= (bh->data + BLOCK_SIZE))
        {
            brelse(bh); bh = NULL;

            block = bmap(dir,i/DIR_NO_PRE_BLOCK);
            bh = bread_block(block);
            de = (struct dir_entry*)bh->data;
        }

        if(i * sizeof(struct dir_entry) >= dir->size )
            return bh;

        if(match(namelen,name,de))
        {
            printk("delete entry=%d \\n", de->inode);

            de->inode = 0;
            for(j = 0;j<MAX_NAME_LEN;j++)
            {
                de->name[j] = 0;
            }
            bh->dirty = 1;
            
            return bh;
        }
        
        de++;
        i++;
    }

}


/*
    获得目录上一级的目录 inode
    例如 ：/linux/bin/src/ test
    返回   /linux/bin/src/ 的i节点
*/
struct m_inode* get_last_dir(const char *pathname)
{
    char c;
    char* thisname = 0;
    
    int namelen = 0;
    int nr = 0;

    struct m_inode *dir;
    struct m_inode *inode;
    struct dir_entry *entry;
    struct buffer_head *bh;

    c = get_fs_byte(pathname);

    if(c == '/')   // 绝对路径
    {
        inode = current->root;
        pathname++;
    }
    else
    {
        inode = current->pwd;  // 相对路径
    }

    while (1)
    {
        thisname = pathname;
repeat:
        c = get_fs_byte(pathname);
        
        if(!c) return inode;
        if(c == '/')
        {
            pathname++;
            goto find_inode;
        }

        pathname++;
        namelen++;

        goto repeat;
find_inode:
        bh = find_entry(inode, thisname, namelen, &entry);

        if(bh == NULL)
        {
            iput(inode);
            return NULL;
        }

        nr = entry->inode;
        brelse(bh);

        dir = inode;
        inode = iget(nr);

        printk("root =%x, find nr=%d \\n",inode->inode,nr);
        
        if(!inode)
        {
            iput(dir);
            return NULL;
        }

        namelen  = 0;
    }

    return inode;
}

static void get_last_name(const char *pathname,char **name, int *namelen)
{
    char* tmp_name; 
    char c;

    pathname++;
    tmp_name = pathname;

    while (1)
    {
        c = get_fs_byte(pathname);
        if(!c)  break;
        if(c == '/')
        {
            pathname++;
            tmp_name = pathname;
        }

        pathname++;
    }

    *namelen = pathname - tmp_name;
    *name = tmp_name;
}


struct m_inode* dir_namei(char ** name, int* namelen, const char* pathname)
{
    struct m_inode *dir;
    dir = get_last_dir(pathname);
    get_last_name(pathname, name, namelen);
    return dir;
} 

/*
    打开 and 创建文件
    flag：是打开文件标志（只读/只写/读写/创建/被创建的文件必须不存在/在文件未添加数据）
    例如 ：/linux/bin/src/ test  获得 test
*/
struct m_inode* open_and_create(
            const char *pathname, 
            unsigned short mode,
            int flags)
{
    struct m_inode *parent_dir, *inode; 
    struct dir_entry* de;
    struct buffer_head *bh;

    char* last_name; 
    int namelen = 0;

    parent_dir = dir_namei(&last_name, &namelen, pathname);
    bh = find_entry(parent_dir, last_name, namelen, &de);

    if(!de) printk("  no dir =%x ,paren_dir_inode=%x\\n",de,parent_dir->inode);
    
    if(de)
    {
        printk("has dir=%x,inode=%d \\n",de,de->inode);
        return iget(de->inode);
    }
    
    if(!(flags & FILE_CREATE))
        return NULL;
    
    // 创建 文件？
    inode = new_inode();
    if(!inode)
    {
        iput(parent_dir);
        return NULL;
    }
    inode->dirty = 1;

    // ROOT 权限 = SET_ISVTX
    inode->mode = (SET_ISVTX | (mode & ~current->umask));

    // 向 parent_dir 增加一项 目录项
    bh = add_entry(parent_dir, last_name, namelen, inode->inode, &de);

    printk("parent_dir.inode=%x \\n",parent_dir->inode);
    
    if(!bh)
    {
        iput(parent_dir);
        iput(inode);
        return -1;
    }

    brelse(bh);
    iput(parent_dir);

    return inode;
}


/*
    mode = 使用权限, 创建一个目录 
    例如 ：/linux/bin/src/ test
         : linux/bin/src/ 相对目录 ，pwd 下为准
*/
int do_mkdir(const char *pathname, unsigned short mode)
{
    char* last_name;  int namelen = 0;
    struct m_inode *parent_dir, *inode; 
    struct dir_entry   *de;
    struct buffer_head *bh;

    // 获得 父目录
    parent_dir = dir_namei(&last_name, &namelen, pathname);   // 返回 src 的i节点

    if(!parent_dir)
    {
        printk("not find parent \\n");
        return -1;
    }

    if(!namelen)
    {
        iput(parent_dir);
        return -1;
    }

    bh = find_entry(parent_dir, last_name, namelen, &de);

    if(de)  // 判断 test 是否已经存在了
    {
        brelse(bh);
        iput(parent_dir);
        return 0;
    }

    // 例如：创建 test 的 inode
    inode = new_inode();
    if(!inode)
    {
        iput(parent_dir);
        return -1;
    }

    inode->size = 32;
    inode->dirty = 1;

    if(!(inode->block_area[0] = new_block()))
    {
        iput(inode);
        iput(parent_dir);
        return -1;
    }
        

    if(!(bh = bread_block(inode->block_area[0])))
    {
        iput(inode);
        iput(parent_dir);
        return -1;
    }

    de = (struct dir_entry*)bh->data;
    strcopy(de->name, ".",1);
    de->inode = inode->inode;
    de++;

    strcopy(de->name, "..",2);            // 设置 '..' 目录项
    de->inode = parent_dir->inode;

    inode->nlinks = 2;
    // 0777 = 111111
    inode->mode = (FILE_DIR | SET_ISVTX | (mode & 0777 & ~current->umask));
    
    de = NULL;

    bh->dirty = 1;

    brelse(bh);

    // parent_dir add entry
    bh = add_entry(parent_dir, last_name, namelen, inode->inode, &de);

    if(!bh)
    {
        iput(inode);
        iput(parent_dir);
        return -1;
    }

    printk("create dir=%x,inode_block_addr=%x \\n",
        de->inode,(inode->block_area[0]+NR_FS_START)*2*512);
  
    parent_dir->nlinks++;

    brelse(bh);
    iput(parent_dir);
    iput(inode);

    return 1;
}

/*
    例如 ：/linux/bin/src/ test
*/
int do_rmdir(const char *pathname)
{
    char* last_name;  int namelen = 0;
    struct m_inode *parent_dir, *inode; 
    struct dir_entry *de;
    struct buffer_head *bh;
    
    // 获得 父目录
    parent_dir = dir_namei(&last_name, &namelen, pathname);   // 返回 src 的i节点

    if(!parent_dir) return -1;

    if(!namelen)
    {
        iput(parent_dir);
        return -1;
    }
 
    bh = find_entry(parent_dir, last_name, namelen, &de);
    if(!de)
    {
        iput(parent_dir);
        return -1;
    }

    inode = iget(de->inode);   // 找到对应的i节点,释放i节点所占用的 block

    if(inode->count > 1)
        return -1;

    if(!(inode->mode & SET_ISVTX))
    {
        printk("not root ! \\n");
        return -1;
    }

    if(current->euid != inode->uid)
    {
        printk("euid \\n");
        return -1;
    }

    if(inode->inode == parent_dir->inode)       // 不能删除.目录
    {
        iput(parent_dir);
        iput(inode);
        brelse(bh);
        return -1;
    }

    printk("rmdir file, inode index: %d \n", inode->inode);

    free_inode_block(inode);
    free_inode(inode);

    parent_dir->dirty = 1;
    parent_dir->nlinks--;

    remove_entry(parent_dir, bh, last_name, namelen);
    brelse(bh);     // 

    iput(parent_dir);
    iput(inode);

    return 1; 
}

int do_mount()
{
    // TODO
    return -1;
}

int do_umount()
{
    // TODO
    return -1;
}

extern void con_write();

#include <keyboard.h>

void show_dir_entry(struct buffer_head* bh)
{
    int i,j=0;
    int entries;
    struct dir_entry *entry;

    entries = 1024 / sizeof(struct dir_entry);
    entry = (struct dir_entry*)bh->data;

    while (j <= entries)
    {
        if(entry->inode)
        {
            printk("\\n SHOW inode=%d,j=%d  ",entry->inode,j);
            for(int k=0;k<MAX_NAME_LEN;k++)
            {
                put_queue(entry->name[k]);
            }
            put_queue('  ');
            con_write();
        }

        entry++;
        j++;
    }
}

// 显示所有的目录
void show_all_dir_entry(struct m_inode *inode)
{
    int i,j=0;
    struct buffer_head *bh;
    int entries;

    struct dir_entry *entry;

    entries = inode->size / sizeof(struct dir_entry);

    printk("size=%x \\n",sizeof(struct dir_entry));

    for(i = 0;i < INODE_BLOCK_AREA_LEN;i++)
    {
        if(!inode->block_area[i])
            continue;
        
        bh = bread_block(inode->block_area[i]);

        if(bh == NULL)
            continue;
        
        entry = (struct dir_entry*)bh->data;

        while (j <= entries)
        {
            printk("\\n SHOW inode=%d,j=%d  ",entry->inode,j);
            for(int k=0;k<MAX_NAME_LEN;k++)
            {
                put_queue(entry->name[k]);
            }
            put_queue('  ');
            con_write();

            entry++;
            j++;
        }
    }


}