#ifndef _PAGETABLE_H
#define _PAGETABLE_H

#include <mm/mm.h>

/*
    两级目录
*/
#define PAGE_P   0x0001  // 存在位
// 如果=1，表示可读，可写可执行；如果=0，表示只读可执行
#define PAGE_RWE 0x0002
#define PAGE_US 0x0004      // 
#define PAGE_ACCESSED 0x008
#define PAGE_DIRTY 0x0010
#define PAGE_LOCK  0x20
#define PG_SLAB    0x40
#define PG_SHARE   0x80    // 页是共享的

#define clear_page_slab(p) (p->flags &= ~(PG_SLAB))
#define set_page_slab_flag(p) (p->flags |= PG_SLAB)

#define set_page_slab(p,slabp) (p->prev = slabp)
#define set_page_cachep(p,cachep) (p->next = cachep)

#define get_page_slab(p) (struct slab_struct*)(p->prev)
#define get_page_cachep(p) (struct kmem_cache_struct*)(p->next)


// 写保护 0 页表目录项
#define ZERO_PAGE 0x00000000
#define PAGE_ENTRY_NUMBER 1024  // 页表项的数量


#define PGD_SHIFT 22
#define PTE_SHIFT 12
#define PGD_SIZE (1 << PGD_SHIFT)


#define pgd_index(vaddr) ((vaddr>>PGD_SHIFT)*4)
#define pte_index(vaddr) (((vaddr>>PTE_SHIFT)&0x3ff)*4)

#define page_start_paddr (unsigned long*)(&mem_map)

// 内核起始虚拟地址
#define PAGE_STRUCT_SIZE sizeof(struct page)

// 根据物理地址转换成虚拟地址--内核模式使用
#define __va(pfn) ((pfn << PTE_SHIFT))
#define __pa(vaddr) (vaddr)
#define __pa_to_pfn(vaddr) ((vaddr) >> PTE_SHIFT)

#define __page_to_pfn(page) (page - &mem_map[0])
#define __pfn_to_page(pfn)  (&mem_map[0] + (pfn))

#define __virt_to_page(vaddr) (&mem_map[0] + ( __pa_to_pfn(vaddr)))


#define copy_page(from,to)({ \
    __asm__("cld\n\t" \
    "rep\n\t" \
    "movsb\n\t" \
    ::"S" (from),"D" (to),"c" (4096)); \
})

#define copy_blk(from,to)({ \
    __asm__("cld\n\t" \
    "rep\n\t" \
    "movsb\n\t" \
    ::"S" (from),"D" (to),"c" (1024)); \
})

#endif