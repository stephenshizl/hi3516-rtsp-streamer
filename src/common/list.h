#ifndef __LIST_H__
#define __LIST_H__

#include "hi_type.h"

struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *node,
                               struct list_head *prev,
                               struct list_head *next)
{
    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

static inline void list_add(struct list_head *node, struct list_head *head)
{
    __list_add(node, head, head->next);
}

static inline void list_add_tail(struct list_head *node, struct list_head *head)
{
    __list_add(node, head->prev, head);
}

static inline void list_del(struct list_head *entry)
{
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    entry->next = HI_NULL;
    entry->prev = HI_NULL;
}

static inline HI_BOOL list_empty(const struct list_head *head)
{
    return head->next == head ? HI_TRUE : HI_FALSE;
}

#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define list_for_each(pos, head) \
    for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

#define list_for_each_safe(pos, n, head) \
    for ((pos) = (head)->next, (n) = (pos)->next; \
         (pos) != (head); \
         (pos) = (n), (n) = (pos)->next)

#define list_for_each_entry(pos, head, member)                          \
    for ((pos) = list_entry((head)->next, typeof(*(pos)), member);      \
         &(pos)->member != (head);                                      \
         (pos) = list_entry((pos)->member.next, typeof(*(pos)), member))

#define list_for_each_entry_safe(pos, n, head, member)                  \
    for ((pos) = list_entry((head)->next, typeof(*(pos)), member),      \
         (n) = list_entry((pos)->member.next, typeof(*(pos)), member);  \
         &(pos)->member != (head);                                      \
         (pos) = (n),                                                   \
         (n) = list_entry((n)->member.next, typeof(*(pos)), member))

#endif /* __LIST_H__ */
