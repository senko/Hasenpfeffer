/* memory.c - core memory management implementation
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
#include <mutex/mutex.h>
#include <bitmap.h>
#include <slab.h>
#include <list.h>
#include <l4/sigma0.h>
#include <string.h>
#include <memory.h>
#include <l4/schedule.h>
#include <capability.h>
#include <assert.h>
#include <debug.h>

static mutex_t memlock;

#define MEMPAGE_SLAB_BUFFER_COUNT 1024
static uint8_t mempage_slab_buffer[MEMPAGE_SLAB_BUFFER_COUNT * SLAB_SIZE(LIST_SIZE(sizeof(mempage_t)))];
static slab_pool_t mempool;

#define ADDRSPACE_SLAB_BUFFER_COUNT 256
static uint8_t addrspace_slab_buffer[ADDRSPACE_SLAB_BUFFER_COUNT * SLAB_SIZE(LIST_SIZE(sizeof(AddrSpace_t)))];
static slab_pool_t aspool;

static list_t *addrspace_list;

L4_Word_t memory_init(void)
{
    L4_Fpage_t fp;
    L4_Word_t size;
    L4_Word_t total = 0;

    mutex_init(&memlock);
    mutex_lock(&memlock);

    slab_init(LIST_SIZE(sizeof(mempage_t)), MEMPAGE_SLAB_BUFFER_COUNT, &mempool, mempage_slab_buffer, kmalloc, MEMPAGE_SLAB_BUFFER_COUNT);
    slab_init(LIST_SIZE(sizeof(AddrSpace_t)), ADDRSPACE_SLAB_BUFFER_COUNT, &aspool, addrspace_slab_buffer, kmalloc, ADDRSPACE_SLAB_BUFFER_COUNT);
    addrspace_list = NULL;

    bitmap_init();

    for (size = sizeof(L4_Word_t) * 8 - 1; size >= 10; size--) {
        while (1) {
            fp = L4_Sigma0_GetAny(L4_nilthread, size, L4_CompleteAddressSpace);
            if (L4_IsNilFpage(fp)) break;

            bitmap_clear(L4_Address(fp) / PAGE_SIZE, L4_Size(fp) / PAGE_SIZE);
            total += L4_Size(fp);
        }
    }

    assert (total == memory_get_free());

    /* don't ever allocate 1M of memory, there be dragons */
    /* FIXME - make this a constant */
    bitmap_set(0, (1024 * 1024) / PAGE_SIZE);

    mutex_unlock(&memlock);
    return total;
}

L4_Word_t memory_get_free(void)
{
    return bitmap_get_free() * PAGE_SIZE;
}

/* note: have size be multiple of PAGE_SIZE to avoid memory waste */
void *kmalloc(L4_Word_t size)
{
    L4_Word_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE; /* round up */
    L4_Word_t start = 0;

    mutex_lock(&memlock);
    start = bitmap_find_pages(pages);

    if (start == 0) {
        mutex_unlock(&memlock);
        return NULL;
    }

    bitmap_set(start, pages);
    mutex_unlock(&memlock);

    return (void *) (start * PAGE_SIZE);
}

void kfree(L4_Word_t start, L4_Word_t size)
{
    L4_Word_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE; /* round up */

    mutex_lock(&memlock);
    bitmap_clear(start / PAGE_SIZE, pages);
    mutex_unlock(&memlock);
}

AddrSpace_t *address_space_new(L4_ThreadId_t tid, L4_ThreadId_t pager)
{
    list_t *li;
    slab_t *sb;
    AddrSpace_t *this;

    sb = slab_alloc(&aspool);
    if (!sb) return NULL;

    L4_Word_t old_control = 0;
    if (FALSE == (L4_SpaceControl(tid, 0, L4_Fpage(KIP_AREA_LOCATION, KIP_AREA_SIZE),
            L4_Fpage(UTCB_AREA_LOCATION, UTCB_AREA_SIZE), L4_anythread, &old_control))) {
        slab_free(&aspool, sb);
        return NULL;
    }

    li = LIST_TYPE(sb->data);
    this = (AddrSpace_t *) li->data;
    list_push(&addrspace_list, li);

    this->tid = tid;
    this->memory = NULL;
    this->pager = pager;
    this->creation = L4_SystemClock();
    memset(this->threads, 0, MAX_THREADS_PER_TASK / 8);

    return this;
}

void address_space_destroy(AddrSpace_t *this)
{
    slab_t *sb;
    list_t *page;

    assert (this != NULL);

    while (NULL != (page = list_pop(&(this->memory)))) {
        mempage_t *mp = MEMPAGE_TYPE(page->data);
        kfree(L4_Address(mp->p_page), L4_Size(mp->p_page));
    }

    list_remove(&addrspace_list, LIST_FROM_DATA(this));
    sb = SLAB_FROM_DATA(LIST_FROM_DATA(this));
    slab_free(&aspool, sb);
}

L4_Fpage_t address_space_request_page(AddrSpace_t *this, L4_Fpage_t vpage)
{
    L4_Word_t start = 0;
    L4_Word_t size = 0;
    list_t *page = NULL;
    slab_t *sb;

    assert (this != NULL);

    size = L4_Size(vpage);

    debug("address_space_request_page: requesting page [%lx:%lx] for space %p\n", L4_Address(vpage), L4_Size(vpage), this);

    /* check if it's already there - vpage is first so we can search by it */
    if (this->memory) {
        page = list_find(this->memory, &vpage, sizeof(L4_Fpage_t));
        if (page) {
            return (MEMPAGE_TYPE(page->data))->p_page;
        }
    }

    sb = slab_alloc(&mempool);
    if (!sb) return L4_Nilpage;

    start = (L4_Word_t) kmalloc(size);
    if (!start) {
        slab_free(&mempool, sb);
        debug("address_space_request_page: couldn't allocate memory, returning nilpage\n");
        return L4_Nilpage;
    }

    page = LIST_TYPE(sb->data);
    MEMPAGE_TYPE(page->data)->v_page = vpage;
    MEMPAGE_TYPE(page->data)->p_page = L4_Fpage(start, size);
    list_push(&(this->memory), page);

    debug("address_space_request_page: returning ppage: [%lx:%lx]\n", start, size);

    return L4_Fpage(start, size);
}

boolean address_space_release_page(AddrSpace_t *this, L4_Fpage_t vpage)
{
    mempage_t *mp;
    list_t *page;
    slab_t *sb;

    assert (this != NULL);

    page = list_find(this->memory, &vpage, sizeof(L4_Fpage_t));
    if (!page) return FALSE;

    mp = MEMPAGE_TYPE(page->data);
    kfree(L4_Address(mp->p_page), L4_Size(mp->p_page));

    list_remove(&(this->memory), page);
    sb = SLAB_FROM_DATA(page);
    slab_free(&mempool, sb);

    return TRUE;
}

mempage_t *address_space_contains(AddrSpace_t *this, L4_Word_t addr)
{
    list_t *page;

    assert (this != NULL);
    for (page = this->memory; page; page = page->next) {
        L4_Word_t start = L4_Address(MEMPAGE_TYPE(page->data)->v_page);
        L4_Word_t end = start + L4_Size(MEMPAGE_TYPE(page->data)->v_page);
        if ((addr >= start) && (addr < end)) return MEMPAGE_TYPE(page->data);
    }
    return NULL;
}

void address_space_get_capability(HpfCapability *cap, AddrSpace_t *space, L4_Word_t flags)
{
    hpf_capability_new(cap, L4_nilthread, (L4_Word_t) space, flags, (L4_Word_t) space->creation.raw);
}

AddrSpace_t *address_space_from_capability(HpfCapability *cap)
{
    list_t *li;
    AddrSpace_t *as;

    assert(cap != NULL);

    for (li = addrspace_list; li; li = li->next) {
        if ((L4_Word_t) ADDRSPACE_TYPE(li->data) == cap->object) break;
    }
    if (!li) return NULL;

    as = ADDRSPACE_TYPE(li->data);

    if (hpf_capability_check(cap, as->creation.raw)) return as;
    return NULL;
}

boolean address_space_lesser_capability(HpfCapability *cap, L4_Word_t flags)
{
    AddrSpace_t *as;

    assert(cap != NULL);

    /* only lesser privileges */
    if ((flags & cap->flags) != flags) return FALSE;

    as = address_space_from_capability(cap);
    if (!as) return FALSE;
    
    address_space_get_capability(cap, as, flags);
    return TRUE;
}

