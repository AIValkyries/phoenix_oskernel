#include <mm/mm.h>
#include <kernel/string.h>
#include <fs/fs.h>

#define MALLOC_COUNT 6

// kmem_cache_struct 缓存的缓存,同时也是 init kmem_cache
// 107个缓存
static struct kmem_cache_struct cache_cache = 
{
    .obj_size = sizeof(struct kmem_cache_struct),
    .slab_order = 0,
    .name = "cache_cache",
    .slabp_cache = NULL,
    .slabs_full = NULL
};
static struct cache_sizes_struct malloc_sizes[MALLOC_COUNT];

// 返回虚拟地址
static void* page_address(struct page *page)
{
    unsigned long pfn;
    unsigned long vaddr;

    pfn = __page_to_pfn(page);
    vaddr = (void*)__va(pfn);

    return vaddr;
}

static unsigned short* slab_bufct1(struct slab_struct* slabp)
{
    return (unsigned short*)(slabp+1);
}

#define SLAB_BUF_SIZE sizeof(unsigned short)
#define SLAB_SLAB_STRUCT_SIZE sizeof(struct slab_struct)
#define MAX_SLAB_ORDER 4


static void slab_add_list(struct slab_struct **head,struct slab_struct *slabp)
{
    if(*head == (void*)0)
    {
        (*head ) = slabp;
        (*head)->next = slabp;
        (*head)->prev = slabp;
    }
    else
    {
        (*head)->next->prev = slabp;
        slabp->next = (*head)->next;
        slabp->prev = (*head);
        (*head)->next = slabp;        
    }
}


static void slab_del_head(struct slab_struct **head)
{
    (*head)->next->prev = (*head)->prev;
    (*head)->prev->next = (*head)->next;
    *head = (*head)->next;
}


static void slab_del_list(struct slab_struct **head,struct slab_struct *slabp)
{
    if(head == slabp)
    {
        slab_del_head(head);
    }
    else
    {
        slabp->next->prev = slabp->prev;
        slabp->prev->next = slabp->next;
    }
}


static void slab_move(struct slab_struct **from, struct slab_struct **to)
{
    slab_del_head(from);
    slab_add_list(to, *from);
}


static struct kmem_cache_struct* kmem_find_general_cachep(unsigned long size)
{
    struct cache_sizes_struct *cache_sizes = malloc_sizes;

    for(;cache_sizes->cs_size;cache_sizes++)
    {
        if(size < cache_sizes->cs_size)
            continue;
       return cache_sizes->cs_cachep;
    }

    return NULL;
}

/*
    计算缓存的 slab_order and num
*/
static unsigned short cache_estimate(
    unsigned long slab_order,
    unsigned short size,
    unsigned long *left_over)
{
    unsigned long page_size;
    int i;

    page_size = (PAGE_SIZE << (slab_order));
    
    i = 1;
    while (i * size + (SLAB_SLAB_STRUCT_SIZE + i * SLAB_BUF_SIZE) <= page_size)
    {
        i++;
    }
    
    i--;
    *left_over = page_size - ((size * i) + (i* SLAB_BUF_SIZE) + SLAB_SLAB_STRUCT_SIZE);

    return i;
}

/*
    name = "对象名称";
    size = 字节
*/
struct kmem_cache_struct* kmem_cache_create(char *name, unsigned int size)
{
    // 为对象分配一个缓存信息
    struct kmem_cache_struct *cachep;   // 给对象分配一个缓存对象
    unsigned long left_over;            // 内存碎片
    unsigned long slab_size;

    // 字节对齐
    if(size & (WORD_BYTES-1))
    {
        size += WORD_BYTES;
        size &= ~(WORD_BYTES-1);
    }

    left_over = 0;
    slab_size = 0;

    cachep = (struct kmem_cache_struct*)kmem_object_alloc(&cache_cache);
  
    cachep->slab_order = 0;
    cachep->slab_obj_num = 0;

    if(size < PAGE_SIZE)    //  slab 同时也存放在slab内部
    {
        // 计算缓存的 slab_order and num
        cachep->slab_obj_num = cache_estimate(0, size, &left_over);
    }
    else
    {
        while (1)
        {
            cachep->slab_obj_num = cache_estimate(cachep->slab_order, size, &left_over);
            
            if(cachep->slab_obj_num == 0)
                goto next;

            // 最大 64 kib
            if(cache_cache.slab_order >= MAX_SLAB_ORDER)
                break;
            
            // 页框的8分之一的空洞??
            if(left_over * 8 >= (PAGE_SIZE << cachep->slab_order))
                goto next;
            
            break;
next:
            cachep->slab_order++;
        }
    }

    slab_size = cachep->slab_obj_num * SLAB_BUF_SIZE + SLAB_SLAB_STRUCT_SIZE;
    if(slab_size & (WORD_BYTES-1))
    {
        slab_size += WORD_BYTES;
        slab_size &= ~(WORD_BYTES-1);
    }

    if(left_over < slab_size)   // 存放在 slab 外部
    {
        cachep->slabp_cache = kmem_find_general_cachep(slab_size);
        left_over -= slab_size;
    }
    
    cachep->left_over = left_over;
    cachep->name      = name;
    cachep->obj_size  = size;
    cachep->slab_size = slab_size;
    cachep->slabp_cache = NULL;
    cachep->slabs_partial = NULL;
    cachep->next = NULL;
    cachep->slabs_full = NULL;

    return cachep;
}


int slab_create(struct kmem_cache_struct *cachep)
{
    struct page *p;
    struct slab_struct *slabp;
    int page_count;
    int i;
    void* obj_vaddr;

    // object 的容器 
    p = alloc_pages(cachep->slab_order);

    // 返回虚拟地址
    obj_vaddr  = (void*)page_address(p);
    page_count = (1 << cachep->slab_order);

    slabp = (struct slab_struct*)obj_vaddr; 

    if(cachep->slabp_cache != NULL)
    {
        slabp = kmem_object_alloc(cachep->slabp_cache);
        slabp->obj_mem = obj_vaddr;
    }
    else
    {
        slabp->obj_mem = obj_vaddr + cachep->slab_size;
    }

    while (page_count)
    {
    /*     set_page_slab_flag(p); */
      /*   set_page_slab(p,  slabp);       // 记录 page 的 slab
        set_page_cachep(p,cachep);      // 记录 page 的 kmem */

        p->flags |= PG_SLAB;
        p->prev = slabp;
        p->next = cachep;
        
        page_count--;
        p++;
    }
    
    slabp->free_num = cachep->slab_obj_num;

    for( i = 0; i < cachep->slab_obj_num; i++)
    {
        slab_bufct1(slabp)[i] = i+1;
    }

    slab_bufct1(slabp)[i-1] = SLAB_END;
    slabp->free_idx = 0;

    slab_add_list(&cachep->slabs_partial, slabp);

    return 1;
}


void* kmem_object_alloc(struct kmem_cache_struct *cachep)
{
    // 从链表中分配
    struct slab_struct *slabp;  // 空闲的 slab
    void  *obj_addr;
    unsigned short next;

    if(cachep->slabs_partial == NULL)
        slab_create(cachep);

    obj_addr = NULL;
    slabp = cachep->slabs_partial;
    cachep->slabs_partial = cachep->slabs_partial->next;

    if(slabp->free_num)    // 还有空闲对象
    {
        obj_addr = (void*)(slabp->obj_mem + slabp->free_idx * cachep->obj_size);
        slabp->free_num--;

        next = slab_bufct1(slabp)[slabp->free_idx];
        slabp->free_idx = next;
    }

    if(slabp->free_idx == SLAB_END)   // 链表满了
        slab_move(&cachep->slabs_partial, &cachep->slabs_full);

    return obj_addr;
}

// 缓存中的对象释放
void  kmem_object_free(struct kmem_cache_struct *cachep, void *objp)
{
    unsigned long pfn;
    struct page *p;
    struct slab_struct *slabp; 

    pfn = ((unsigned long)objp) >> PAGE_SHIFT;
    p =  mem_map + pfn;

    slabp = get_page_slab(p);
    
    unsigned short objnr = (objp - slabp->obj_mem ) / cachep->obj_size;

    slab_bufct1(slabp)[slabp->free_idx] = objnr;
    slabp->free_idx = objnr;

    if(slabp->free_num == 0)
        slab_move(&cachep->slabs_full, &cachep->slabs_partial);

    slabp->free_num++;
}


// slab 销毁，还给伙伴系统
static void slab_destroy(struct kmem_cache_struct *cachep, struct slab_struct *slabp)
{
    // slab 的指针
    void *vaddr;
    struct page *p;
    unsigned long pfn;

    vaddr = (slabp->obj_mem - cachep->slab_size);
    pfn   = ((unsigned long)vaddr >> PAGE_SHIFT);
    p     =  mem_map + pfn;

    int i = 1 << cachep->slab_order;

    while (i-->0)
    {
        clear_page_slab(p);
        p++;
    }

    if(slabp->free_num == 0)  // 是满的
        slab_del_list(&cachep->slabs_full, slabp);
    else
        slab_del_list(&cachep->slabs_partial,slabp);

    __free_pages___(p, cachep->slab_order);
}


void kmem_cache_destroy(struct kmem_cache_struct* cachep)
{
    struct slab_struct *slabp;

    if(cachep->slabs_partial != NULL)
    {
        slabp = cachep->slabs_partial;

        while (slabp && slabp != cachep->slabs_partial)
        {
            slab_destroy(cachep, slabp);
            slabp = slabp->next;
        }
        cachep->slabs_partial = NULL;
    }
    
    if(cachep->slabs_full != NULL)
    {
        slabp = cachep->slabs_full;
        while (slabp && slabp != cachep->slabs_full)
        {
            slab_destroy(cachep, slabp);
            slabp = slabp->next;
        }
        cachep->slabs_full = NULL;
    }

    cachep->slab_order = 0;
}


// 通用对象的分配
void* kmalloc(unsigned short size)
{
    int i;
    
    for(i = 0; i < MALLOC_COUNT; i++)
    {
        if(malloc_sizes[i].cs_size < size)
            continue;
        return kmem_object_alloc(malloc_sizes[i].cs_cachep);
    }

    return NULL;
}

void kfree(void *objp)
{
    struct page *page;
    struct kmem_cache_struct *cache;
    unsigned long pfn;

    pfn = ((unsigned long)(objp)) >> PAGE_SHIFT;
    page  = __pfn_to_page(pfn);
    cache = get_page_cachep(page);

    kmem_object_free(cache, objp);
}

// 创建缓存的缓存器
static void init_kmem_cache_cache()
{
    unsigned long left_over;

    cache_cache.obj_size = sizeof(struct kmem_cache_struct);
    cache_cache.slab_order = 0;
    cache_cache.name = "cache_cache";
    cache_cache.slabp_cache = NULL;
    cache_cache.slabs_full = NULL;

    cache_cache.slab_obj_num = cache_estimate(
            cache_cache.slab_order,
            cache_cache.obj_size,
            &left_over);
    cache_cache.slab_size = cache_cache.slab_obj_num * SLAB_BUF_SIZE + SLAB_SLAB_STRUCT_SIZE;

    // slab_addr = 0x001FF000
    slab_create(&cache_cache);
    //printk("init_kmem_cache_cache cache_cache =%x,size%d \\n",&cache_cache, cache_cache.obj_size);
}


void kmem_cache_init()
{   
    //printk(" ------- kmem init -------\\n");

    struct cache_sizes_struct *sizes;
    int i;

    init_kmem_cache_cache();

    malloc_sizes[0].cs_size = 32;  malloc_sizes[0].name   = "size-32";malloc_sizes[0].cs_cachep = (void*)0;
    malloc_sizes[1].cs_size = 64;  malloc_sizes[1].name   = "size-64";malloc_sizes[1].cs_cachep = (void*)0;
    malloc_sizes[2].cs_size = 128; malloc_sizes[2].name  = "size-128";malloc_sizes[2].cs_cachep = (void*)0;
    malloc_sizes[3].cs_size = 256; malloc_sizes[3].name  = "size-256";malloc_sizes[3].cs_cachep = (void*)0;
    malloc_sizes[4].cs_size = 512; malloc_sizes[4].name  = "size-512";malloc_sizes[4].cs_cachep = (void*)0;
    malloc_sizes[5].cs_size = 1024;malloc_sizes[5].name = "size-1024";malloc_sizes[5].cs_cachep = (void*)0;

    for(i=0; i < MALLOC_COUNT; i++)
    {
        malloc_sizes[i].cs_cachep = kmem_cache_create(malloc_sizes[i].name, malloc_sizes[i].cs_size);
    }

    mm_cachep = kmem_cache_create("mm_cachep",            sizeof(struct mm_struct));
    vm_area_cachep = kmem_cache_create("vm_area_cachep",  sizeof(struct vm_area_struct));
    task_struct_cachep = kmem_cache_create("task_cachep", sizeof(struct task_struct));
    filp_cachep = kmem_cache_create("file_cachep", sizeof(struct file));
    inode_cachep = kmem_cache_create("inode_cachep", sizeof(struct m_inode));
    

    if(mm_cachep == NULL 
        || vm_area_cachep == NULL 
        || task_struct_cachep == NULL 
        || filp_cachep == NULL
        || inode_cachep == NULL)
    {
        printk("error mm_cachep or vm_area_cachep or task_struct_cachep \\n");
        printk("mm_cachep=%x,vm_area_cachep=%x,task_struct_cachep=%x \\n",mm_cachep, vm_area_cachep, task_struct_cachep);
    }

    // -------- 通用对象测试 -------- //
    /*   
        void *test_addr;
        // 0xC0036D40
        test_addr = kmalloc(33);
        //printk("kmalloc =%x \\n",test_addr);
        //show_buddy_area();

        //printk("test_addr =%c \\n",test_addr);
        kfree(test_addr); 
    */
}