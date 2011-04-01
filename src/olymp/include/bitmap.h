#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <l4/types.h>

#include <stdint.h>

void bitmap_init(void);
void bitmap_set(L4_Word_t page, L4_Word_t size);
void bitmap_clear(L4_Word_t page, L4_Word_t size);
L4_Word_t bitmap_find_pages(int32_t size);
L4_Word_t bitmap_get_free(void);

#endif

