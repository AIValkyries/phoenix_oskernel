
#include <mm/mm.h>
#include <mm/pagetable.h>


static void add_free_area(struct page *p, unsigned long new_order)
{
    p->order = new_order;

    struct free_area_struct *area = &free_area[new_order];
    
    // 增加 free_area 
    if(area->free_list == NULL)
    {
        area->free_list = p;
        area->free_list->next = p;
        area->free_list->prev = p;
    }
    else
    {
        area->free_list->next->prev = p;
        p->next = area->free_list->next;
        p->prev = area->free_list;
        area->free_list->next = p;
    }

    area->nr_free++;
}

static void remove_free_area(
    struct page *p, unsigned long old_order)
{
    struct free_area_struct *area = &free_area[old_order];

    struct page *tmp;
    tmp = area->free_list;

    while (tmp != NULL)
    {
        if(tmp == p)
        {
            tmp->prev->next = tmp->next;
            tmp->prev = tmp->prev;
            area->nr_free--;
            break;
        }
        tmp = tmp->next;
        if(tmp == area->free_list)
            break;
    }
}


/*
    将 high 降阶    [paddr = 指向 high 阶的地址]
*/
void expand(
    unsigned long low_order,        // 源阶
    unsigned long high_order,       // 高阶
    struct page *page)
{
    struct page *p;
    int i;
    struct free_area_struct *tmp_area;

    // 局部的索引
    unsigned long nr_pages = 1 << high_order;

    while (low_order < high_order)
    {
        high_order--;
        nr_pages >>= 1;
        p = (page + nr_pages);

        i = nr_pages-1;

        while (i > 0)
        {
            (p+i)->order = high_order;
            i--;
        }
        add_free_area(p, high_order);
    }
}

// 从伙伴系统中分配页框
struct page* __alloc_pages(unsigned long order)
{
    struct free_area_struct *free_list;   
    struct page* p;
    struct page *tmp;
    int i;
    unsigned long new_order;
    unsigned long p_count;

    new_order = order;
    i = 0;

    while (new_order < MAX_ORDER)
    {
       free_list = &free_area[new_order];

       if(free_list->nr_free > 0)
       {
            p = free_list->free_list->next != NULL ? free_list->free_list->next:free_list->free_list;

            if(p != NULL)
            {
                free_list->nr_free--;

                p->prev->next = p->next->next;
                p->next->prev = p->prev;
                
                p->order = order;
                p_count = 1 << order;

                for (tmp = p;i<p_count;i++)
                {
                    tmp->count = 1;
                    tmp++;
                }

                if(new_order > order)
                    expand(order, new_order, p);

                return p;
            }
       }

       new_order++;
    }

    return 0;
}


struct page* alloc_one_page()
{
    struct page *p;
    p = __alloc_pages(0);
    return p;
}

struct page* alloc_pages(unsigned short order)
{
    return __alloc_pages(order);
}

// 返回虚拟地址
unsigned long get_free_pages___(unsigned short order)
{
    struct page *page = __alloc_pages(order);
    if(!page)   return 0;

    page->count++;

    unsigned long pfn = __page_to_pfn(page);

    return __va(pfn);
}


unsigned long get_free_page()
{
    return get_free_pages___(0);
}


// 释放 page， 填充至伙伴系统
int __free_pages___(struct page *page, unsigned short order)
{
    unsigned short current_idx;
    unsigned short page_pfn;
    unsigned short neighbor_idx;
    unsigned short count;
    int i;

    struct page *coalesced;

    page_pfn = __page_to_pfn(page);
    neighbor_idx = page_pfn;
    current_idx  = page_pfn;
    count = 0;

    while (order < (MAX_ORDER-1))
    {
        int start_idx;
        if(current_idx & 0x01)
            neighbor_idx -= 1;      // 左边的邻居
        else
            neighbor_idx += 1;

        count = (1 << order);
        start_idx = neighbor_idx * count;

        // 相邻的所有页框是否都是空闲的
        for(i = start_idx; i < (start_idx+count); i++)
        {
            if(mem_map[i].count > 0)
                goto merge;
            mem_map[i].order = order;
        }

        // 合并之后取最小的 page index
        page_pfn = (neighbor_idx < current_idx ? neighbor_idx : current_idx) * count;
        coalesced = __pfn_to_page(page_pfn);
        // 可以合并，移除当前阶
        remove_free_area(coalesced, order);
        
        current_idx >>= 1;                // 下一阶的 index
        order++;
        neighbor_idx = current_idx;
    }

merge:
    // 升阶
    // page 的起始地址合并
    coalesced = __pfn_to_page(page_pfn);
    add_free_area(coalesced, order);

    count *= 2;
    // 设置该 阶 所有的page的信息
    struct page *p;
    for(i = page_pfn + 1; i < (page_pfn + count); i++)
    {
        p = __pfn_to_page(i);
        p->order = order;
        p->count = 0;
    }

    return count;
}


/*
    释放内存, 找到和 vaddr 相邻的 page ，将其合并
    vaddr = 虚拟地址
*/
void free_page(unsigned long vaddr)
{
    struct page* page = __virt_to_page(vaddr);
    __free_pages___(page, page->order);
}


// 初始化伙伴系统
void init_buddy_pages(unsigned long start_pfn, unsigned long end_pfn)
{
    int count;
    int i;
    int j;

    for(i = 0; i < MAX_ORDER; i++)
    {
        free_area[i].nr_free = 0;
    }

    i = start_pfn;

    // 主存区域的页框全部清零 [16MiB - 64MiB]
    while (start_pfn < end_pfn)
    {
        count = __free_pages___(&mem_map[i], 0);
        start_pfn += count;
        i += count;
        j++;
    }
    
}


void show_all_page()
{
    int i;
    for(i = 0;i < NR_PAGES; i++)
    {
        if(&mem_map[i] == 0x86DA0 || &mem_map[i] == 0x86DB0)
            printk("index = %x \\n",i);
    }
}

void show_buddy_area()
{
    unsigned long free;
    struct page *p;
    struct page *tmp;

    int j;
    int i;
    j = 0;
    i = 3;

    for (j = 0; j < MAX_ORDER; j++)
    {
        free = free_area[j].nr_free;
        p = free_area[j].free_list;

        printk("%x area free_area = %x, free_list=%p, free: %x \\n", j,&free_area[j], p, free);
    }

}