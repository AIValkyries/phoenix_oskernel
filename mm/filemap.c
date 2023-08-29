/*
    文件与虚拟内存映射
*/

#include <mm/mm.h>
#include <fs/fs.h>

// 从文件中读取 vaddr 关联的数据
unsigned long filemap_nopage(
    struct vm_area_struct *vma,
    struct page *page,
    unsigned long vaddr)
{
    unsigned long file_offset;
    unsigned long block_index;
    int nr[4] = {0,0,0,0};
    int i;
    unsigned vaddr_offset;

    struct m_inode *inode;

    vaddr_offset = current->mm->mmap->vm_start;
    vaddr_offset = (vaddr - vaddr_offset) - vma->vm_start;

    inode = vma->vm_file->inode;
    file_offset = vaddr_offset + vma->vm_offset;
    block_index = file_offset >> BLOCK_SIZE_BIT;
    
/*     printk("file_offset=%x,offset=%x,vaddr=%x \\n",
        vma->vm_start, vaddr_offset,vaddr); */

    for(i = 0; i < 4; i++)
    {
        nr[i] = bmap(inode, block_index) + NR_FS_START;
        block_index++;
    }
    
    bread_page(nr, page);
}

/*
    建立映射关系, [ 私有关系映射 | 共享关系映射 ]
*/
int generic_mmap(struct file *file, struct vm_area_struct *vma)
{
    vma->vm_file = file;
    file->inode->count++;

    return 0;
}