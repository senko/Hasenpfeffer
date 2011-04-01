/* bitmap.c - memory allocation bitmap implementation
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
#include <string.h>

#include <memory.h>

static uint8_t bitmap[MEMORY_CAPACITY / (PAGE_SIZE * 8)];

void bitmap_init(void)
{
    /* let the memory detection free page by page of unused memory */
    memset(bitmap, 255, MEMORY_CAPACITY / (PAGE_SIZE * 8));
}

static inline void bitmap_set_page(L4_Word_t page)
{
    L4_Word_t byte = page / 8;
    uint8_t bit = 1 << (page % 8);
    bitmap[byte] |= bit;
}

static inline void bitmap_clear_page(L4_Word_t page)
{
    L4_Word_t byte = page / 8;
    uint8_t bit = 1 << (page % 8);
    bitmap[byte] &= ~bit;
}

static inline uint8_t bitmap_get_page(L4_Word_t page)
{
    L4_Word_t byte = page / 8;
    uint8_t bit = 1 << (page % 8);
    return (bitmap[byte] & bit);
}

void bitmap_set(L4_Word_t page, L4_Word_t size)
{
    while (size--) bitmap_set_page(page++);
}

void bitmap_clear(L4_Word_t page, L4_Word_t size)
{
    while (size--) bitmap_clear_page(page++);
}

L4_Word_t bitmap_find_pages(L4_Word_t size)
{
    L4_Word_t page, cnt;

    page = 0;
    /* search possible starts of memory area */
    while (page < ((MEMORY_CAPACITY / PAGE_SIZE) - size)) {
        /* see how many pages are free */
        for (cnt = 0; cnt < size; cnt++) {
            if (bitmap_get_page(page + cnt)) break;
        }
        /* 'size' pages are free, great, return the start */
        if (cnt == size) return page;
        /* the page needs to be aligned */
        page += size;
    }
    return 0;
}

L4_Word_t bitmap_get_free(void)
{
    L4_Word_t page;
    L4_Word_t count = 0;

    for (page = 0; page < (MEMORY_CAPACITY / PAGE_SIZE); page++) {
        if (!bitmap_get_page(page)) count++;
    }
    return count;
}

