#ifndef _SLAB_H_
#define _SLAB_H_

#include <l4/types.h>

#include <stdint.h>

typedef void* (*slab_allocator_t)(L4_Word_t);

struct slab
{
    struct slab *next;
    uint8_t data[1];
};

typedef struct slab slab_t;

struct slab_pool
{
    L4_Word_t size;
    slab_allocator_t more_core;
    L4_Word_t more_count;
    slab_t *first;
};
typedef struct slab_pool slab_pool_t;

#define SLAB_SIZE(size) (sizeof(slab_t) + (size) - 1)
#define SLAB_FROM_DATA(ptr) ((slab_t *) (((uint8_t *) ptr) - sizeof(slab_t) + 1))

void slab_init(L4_Word_t size, L4_Word_t count, slab_pool_t *pool, void *buffer, slab_allocator_t more_core, L4_Word_t more_count);
slab_t *slab_alloc(slab_pool_t *pool);
void slab_free(slab_pool_t *pool, slab_t *object);

#endif

