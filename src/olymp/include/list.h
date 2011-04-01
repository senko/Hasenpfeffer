#ifndef _LIST_H_
#define _LIST_H_

#include <l4/types.h>

#include <stdint.h>

struct list
{
    struct list *next;
    uint8_t data[1];
};

typedef struct list list_t;

#define LIST_TYPE(ptr) ((list_t *) ptr)
#define LIST_SIZE(size) (sizeof(list_t) + (size) - 1)
#define LIST_FROM_DATA(ptr) ((list_t *) (((uint8_t *) ptr) - sizeof(list_t) + 1))

void list_push(list_t **li, list_t *obj);
list_t *list_pop(list_t **li);
list_t *list_find(list_t *li, void *data, L4_Word_t size);
list_t *list_remove(list_t **li, list_t *obj);

#endif

