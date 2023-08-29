#include <fs/fs.h>
#include <kernel/sched_os.h>
#include <kernel/string.h>
#include <mm/mm.h>

struct file* first_file;
int nr_files = 0;

int fd_tables[NR_FILE];
struct kmem_cache_struct *filp_cachep;           // file 的结构 

#define alloc_file()      (struct file*)kmem_object_alloc(filp_cachep)

// 扩充 file
void grow_files(void)
{
    struct file* file = alloc_file();

    if(!first_file)
    {
        first_file = file;
        first_file->next = file;
        first_file->prev = file;
    }
    else
    {
        // 添加至尾部
        first_file->prev->next = file;
        file->prev = first_file->prev;
        file->next = first_file;
        first_file->prev = file;
    }

    nr_files += filp_cachep->slab_obj_num;   // 单个 slab 有多少个对象

}


struct file* get_empty_filp()
{
    if(!first_file)
        grow_files();
    
    struct file* f = first_file;

repeat:
    for (int i = 0; i < nr_files; i++)
    {
        if(!f->used)   // 找到空闲的点
        {
            f->prev->next = f->next;
            f->next->prev = f->prev;
            memset(f,0,sizeof(*f));  // 清空内存
            f->used = 1;
            return f;
        }
    }

    if(nr_files < NR_FILE)
    {
        grow_files();
        goto repeat;
    }

    return NULL;
}