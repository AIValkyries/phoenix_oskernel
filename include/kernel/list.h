#ifndef _LIST_H
#define _LIST_H

#include <mm/mm.h>


struct list_head
{
    struct list_head *next;
    struct list_head *prev;
};


static inline void page_list_add(struct page **head, struct page **next)
{
    if((*head) == NULL)
    {
        (*head) = *next;
        (*head)->next = *head;
        (*head)->prev = *head;
    }
    else
    {
        (*head)->next->prev = *next;
        (*next)->next = (*head)->next;
        (*next)->prev = (*head);
        (*head)->next = *next;
    }
}

static inline void page_list_del(struct page *entry)
{
    entry->prev->next = entry->next->next;
    entry->next->prev = entry->prev;
    entry->next = 0;
    entry->prev = 0;
}


static inline void list_add(struct list_head *head, struct list_head *next)
{
    head->next->prev = next;
    next->next = head->next;
    next->prev = head;
    head->next = next;
}


static inline void list_del(struct list_head *entry)
{
    entry->prev->next = entry->next->next;
    entry->next->prev = entry->prev;
    entry->next = 0;
    entry->prev = 0;
}



#endif