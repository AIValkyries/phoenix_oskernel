//销毁进程

#include <kernel/sched_os.h>
#include <kernel/system.h>


int do_exit()
{
    // 虚拟内存释放？
    int i;

    // 页表释放
    free_page_tables(current);

    printk("free_page_tables last \\n");

    // 文件句柄释放？
    for(i = 0; i < NR_FILE; i++)
    {
        if(current->filps[i] != NULL)
            do_close(i);
    }
    
    // 文件释放？
    iput(current->pwd);
    current->pwd = NULL;
    iput(current->root);
    current->root = NULL;
    current->state = TASK_STOPPED;

    // 进程之间的关系调整
    if(current->prev!=NULL)
    {
        current->prev->next = current->next;
    }
    if(current->next!=NULL)
    {
        current->next->prev = current->prev;
    }
    
	sched_exit(current);
    schedule();

    return 1;
}