/* Copyright 2003, 2004 by Senko Rasic <senko@senko.net>.
 * You may use and/or distribute this code under the terms
 * of a new-style BSD license. */

#include <stdio.h>
#include <string.h>
#include <mutex/mutex.h>

static unsigned char *heap_start = (unsigned char *) 0x80000000UL; /* 2GB - start of heap area */
static void *sbrk(size_t size)
{
    printf("sbrk: requested %ld bytes, touching memory at %p\n", size, heap_start);

    /* touch it now */
    volatile unsigned char x = *heap_start; x++;

    void *tmp = (void *) heap_start;
    heap_start += size;
    printf("sbrk returning: %p\n", tmp);
    return tmp;
}

#define MINIMAL_BLOCK_SIZE 4

#define blk_size sizeof(block_t)
#define block_size_round(x) ((x + blk_size - 1) / blk_size)
#define block_size(x) (*x >> 1)
#define block_set(x, s, u) *x = ((s) << 1) | u

#define block_next(x) (x + block_size(x))
#define block_data(x) ((void *) (x + 1))
#define block_set_used(x) *x |= 1
#define block_clear_used(x) *x &= ~1
#define block_is_used(x) (*x & 1)

typedef size_t block_t;

static block_t *floor = NULL;
static block_t *ceiling = NULL;

static mutex_t lock;

static void *block_create(size_t size)
{
	block_t *tmp = sbrk(size * blk_size);
    printf("sbrk returned: %p\n", tmp);
	if (!tmp) return NULL;

	block_set(tmp, size, 1);
	ceiling = tmp + size;
    printf("returning block of data: %p\n", tmp);
	return block_data(tmp);
}

static void *block_extract(block_t *block, size_t size)
{
	size_t tmp;

	if ((block_size(block) - size) < (MINIMAL_BLOCK_SIZE * blk_size)) {
		block_set_used(block);
		return block_data(block);
	}

	tmp = block_size(block) - size; block_set(block, size, 1);
	block_set(block_next(block), tmp, 0);
	return block_data(block);
}

static void block_merge(void)
{
	block_t *block;

	for (block = floor; block_next(block) < ceiling; block = block_next(block)) {
		if (block_is_used(block) || block_is_used(block_next(block))) continue;
		block_set(block, block_size(block) + block_size(block_next(block)), 0);
	}
}

static void *malloc_t(size_t size)
{
	block_t *tmp;
	if (!size) return NULL;

	size = 1 + block_size_round(size);
	if (size < (MINIMAL_BLOCK_SIZE)) size = MINIMAL_BLOCK_SIZE;

	for (tmp = floor; tmp < ceiling; tmp = block_next(tmp)) {
		if (block_is_used(tmp)) continue;
		if (block_size(tmp) < size) continue;
		return block_extract(tmp, size);
	}
	return block_create(size);
}

void *malloc(size_t size)
{
	void *x;

    printf("i'm here\n");
    /* FIXME - init stuf is not thread-safe */
	if (!floor) {
        floor = sbrk(0);
        printf("mutex init\n");
        mutex_init(&lock);
        printf("mutex init done\n");
    }

    mutex_lock(&lock);
	x = malloc_t(size);
    mutex_unlock(&lock);
	return x;
}

static void free_t(void *ptr)
{
	block_t *block = (block_t *) ptr;

	if (!ptr) return;

	block -= 1;
	block_clear_used(block);
	block_merge();
}

void free(void *ptr)
{
	mutex_lock(&lock);
	free_t(ptr);
    mutex_unlock(&lock);
}

void *realloc(void *ptr, size_t size)
{
	void *ptr2;
	block_t *block = (block_t *) ptr;

	if (!size) {
		free(ptr);
		return NULL;
	}

	ptr2 = malloc(size);
	if (ptr) {
		block -= 1;
		size_t old_size = (block_size(block) - 1) * blk_size;
		memcpy(ptr2, ptr, (old_size < size) ? old_size : size);
		free(ptr);
	}
	return ptr2;
}

void *calloc(size_t nmemb, size_t size)
{
	void *ptr = malloc(nmemb * size);
	memset(ptr, 0, nmemb * size);
	return ptr;
}

