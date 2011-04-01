/* slab.c - fast slab object allocation support
 *
 * MIT X11 license, Copyright (c) 2005 by Senko Rasic <senko@senko.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <slab.h>
#include <assert.h>

static slab_t *slab_mem_init(L4_Word_t size, L4_Word_t count, void *buffer)
{
    int i;
    uint8_t *ptr;

    if (!buffer) return NULL;

    ptr = buffer;
    for (i = 0; i < (count - 1); i++) {
        ((slab_t *) ptr)->next = (slab_t *) (ptr + SLAB_SIZE(size));
        ptr += SLAB_SIZE(size);
    }
    ((slab_t *) ptr)->next = NULL;

    return (slab_t *) buffer;
}

void slab_init(L4_Word_t size, L4_Word_t count, slab_pool_t *pool, void *buffer, slab_allocator_t more_core, L4_Word_t more_count)
{
    assert(pool != NULL);

    pool->size = size;
    pool->more_core = more_core;
    pool->more_count = more_count;
    if (!buffer) {
        pool->first = NULL;
        return;
    }

    pool->first = slab_mem_init(size, count, buffer);
    return;
}

slab_t *slab_alloc(slab_pool_t *pool)
{
    slab_t *tmp;

    assert(pool != NULL);

    if (!pool->first) {
        void *newbuf = pool->more_core(pool->more_count * SLAB_SIZE(pool->size));
        if (!newbuf) return NULL;
        pool->first = slab_mem_init(pool->size, pool->more_count, newbuf);
    }

    tmp = pool->first;
    pool->first = pool->first->next;
    return tmp;
}

void slab_free(slab_pool_t *pool, slab_t *object)
{
    assert(pool != NULL);

    object->next = pool->first;
    pool->first = object;
}


