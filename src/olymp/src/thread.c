/* thread.c - core thread management implementation
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
#include <l4/thread.h>
#include <l4/types.h>
#include <l4/message.h>
#include <l4/ipc.h>
#include <l4/schedule.h>
#include <mutex/mutex.h>
#include <string.h>
#include <memory.h>
#include <list.h>
#include <slab.h>
#include <thread.h>
#include <capability.h>
#include <assert.h>
#include <debug.h>

static list_t *thread_list;

static uint8_t bitmap[MAX_TASKS / 8];

#define THREAD_SLAB_BUFFER_COUNT 1024
static uint8_t thread_slab_buffer[THREAD_SLAB_BUFFER_COUNT * SLAB_SIZE(LIST_SIZE(sizeof(thread_t)))];
static slab_pool_t thrpool;

mutex_t thrlock;

static L4_Word_t thread_prepare_stack(AddrSpace_t *as, L4_Word_t sp, char *cmdline, int n, char **paths, HpfCapability *caps);

static void threadno_alloc(uint8_t *bmp, L4_Word_t tno)
{
    assert (bmp != NULL);

    L4_Word_t byte = tno / 8;
    uint8_t bit = 1 << (tno % 8);
    bmp[byte] |= bit;
}

static void threadno_free(uint8_t *bmp, L4_Word_t tno)
{
    assert (bmp != NULL);

    L4_Word_t byte = tno / 8;
    uint8_t bit = 1 << (tno % 8);
    bmp[byte] &= ~bit;
}

static L4_Word_t threadno_find_free(uint8_t *bmp, L4_Word_t max_tasks)
{
    assert (bmp != NULL);

    L4_Word_t tno, byte;
    uint8_t bit;
    for (tno = 0; tno < max_tasks; tno++) {
        byte = tno / 8;
        bit = 1 << (tno % 8);
        if (!(bmp[byte] & bit)) return tno;
    }
    return 0;
}

void thread_init(L4_ThreadId_t tid)
{
    L4_Word_t my_threadno, i;

    mutex_init(&thrlock);
    mutex_lock(&thrlock);

    slab_init(LIST_SIZE(sizeof(thread_t)), THREAD_SLAB_BUFFER_COUNT, &thrpool, thread_slab_buffer, kmalloc, THREAD_SLAB_BUFFER_COUNT);
    thread_list = NULL;

    memset(bitmap, 0, MAX_TASKS / 8);
    my_threadno = L4_ThreadNo(tid);
    for (i = 0; i <= my_threadno; i++) threadno_alloc(bitmap, i);

    mutex_unlock(&thrlock);
}

void thread_start(L4_ThreadId_t tid, L4_Word_t ip, char *cmdline, int npaths, char **paths, HpfCapability *caps)
{
    list_t *li;
    L4_Word_t sp;

    mutex_lock(&thrlock);

    li = list_find(thread_list, &tid, sizeof(L4_ThreadId_t));
    if (!li) {
        mutex_unlock(&thrlock);
        return;
    }

    sp = STACK_BASE - (THREAD_TYPE(li->data)->index * STACK_SIZE);

    sp -= thread_prepare_stack(THREAD_TYPE(li->data)->space, sp, cmdline, npaths, paths, caps);

    /* send "start" message */
    L4_Msg_t msg;
    L4_MsgClear(&msg);
    L4_MsgAppendWord(&msg, ip);
    L4_MsgAppendWord(&msg, sp);
    L4_Set_Propagation(&msg.tag);
    L4_Set_VirtualSender(THREAD_TYPE(li->data)->space->pager);
    L4_MsgLoad(&msg);
    L4_Send(tid);

    mutex_unlock(&thrlock);
}

AddrSpace_t *thread_get_address_space(L4_ThreadId_t tid)
{
    list_t *li;

    mutex_lock(&thrlock);
    li = list_find(thread_list, &tid, sizeof(L4_ThreadId_t));
    mutex_unlock(&thrlock);

    if (!li) return NULL;
    return THREAD_TYPE(li->data)->space;
}

L4_ThreadId_t thread_new(AddrSpace_t *space)
{
    assert (space != NULL);

    L4_Word_t tno;
    L4_ThreadId_t tid;
    L4_ThreadId_t space_spec;
    L4_Word_t utcb_location;
    slab_t *sb;
    list_t *li;
    thread_t *this;

    mutex_lock(&thrlock);
    tno = threadno_find_free(bitmap, MAX_TASKS);
    if (!tno) {
        mutex_unlock(&thrlock);
        return L4_nilthread;
    }

    tid = L4_GlobalId(tno, 1);
    utcb_location = UTCB_AREA_LOCATION;

    space_spec = space->tid;
    tno = threadno_find_free(space->threads, MAX_THREADS_PER_TASK);
    if (!tno) {
        mutex_unlock(&thrlock);
        return L4_nilthread;
    }
    utcb_location += tno * UTCB_SIZE;

    sb = slab_alloc(&thrpool);
    if (!sb) {
        mutex_unlock(&thrlock);
        return L4_nilthread;
    }
    
    if (FALSE == (L4_ThreadControl(tid, space_spec, tid, space->pager, (void *) utcb_location))) {
        slab_free(&thrpool, sb);
        mutex_unlock(&thrlock);
        return L4_nilthread;
    }

    li = LIST_TYPE(sb->data);
    this = (thread_t *) li->data;
    list_push(&thread_list, li);

    this->tid = tid;
    this->space = space;
    this->index = tno;
    this->creation = L4_SystemClock();

    threadno_alloc(bitmap, L4_ThreadNo(tid));
    threadno_alloc(space->threads, tno);
    mutex_unlock(&thrlock);
    return tid;
}

AddrSpace_t *task_new(L4_ThreadId_t pager)
{
    L4_Word_t tno;
    L4_ThreadId_t tid;
    L4_ThreadId_t space_spec;
    L4_Word_t utcb_location;
    AddrSpace_t *space = NULL;
    slab_t *sb;
    list_t *li;
    thread_t *this;

    mutex_lock(&thrlock);
    tno = threadno_find_free(bitmap, MAX_TASKS);
    if (!tno) {
        mutex_unlock(&thrlock);
        return NULL;
    }

    tid = L4_GlobalId(tno, 1);
    utcb_location = UTCB_AREA_LOCATION;

    space_spec = tid;

    sb = slab_alloc(&thrpool);
    if (!sb) {
        mutex_unlock(&thrlock);
        return NULL;
    }
    
    if (FALSE == (L4_ThreadControl(tid, space_spec, L4_Myself(), L4_nilthread, (void *) utcb_location))) {
        slab_free(&thrpool, sb);
        mutex_unlock(&thrlock);
        return NULL;
    }

    space = address_space_new(tid, pager);
    if (!space) {
        L4_ThreadControl(tid, L4_nilthread, L4_nilthread, L4_nilthread, (void *) -1);
        slab_free(&thrpool, sb);
        mutex_unlock(&thrlock);
        return NULL;
    } else {
        /* set self space, and the specified pager
         * FIXME - using myself as the scheduler */
        L4_ThreadControl(tid, tid, L4_Myself(), pager, (void *) -1);
    }

    li = LIST_TYPE(sb->data);
    this = (thread_t *) li->data;
    list_push(&thread_list, li);

    this->tid = tid;
    this->space = space;
    this->index = 0;
    this->creation = L4_SystemClock();

    threadno_alloc(bitmap, L4_ThreadNo(tid));
    threadno_alloc(space->threads, 0);
    mutex_unlock(&thrlock);
    return space;
}

int thread_destroy(L4_ThreadId_t tid)
{
    list_t *li;
    AddrSpace_t *as;
    L4_Word_t tno;
   
    mutex_lock(&thrlock);

    li = list_find(thread_list, &tid, sizeof(L4_ThreadId_t));
    if (li == NULL) {
        mutex_unlock(&thrlock);
        return FALSE;
    }

    if (FALSE == L4_ThreadControl(tid, L4_nilthread, L4_nilthread, L4_nilthread, (void *) -1)) {
        mutex_unlock(&thrlock);
        return FALSE;
    }

    as = THREAD_TYPE(li->data)->space;
    tno = THREAD_TYPE(li->data)->index;

    list_remove(&thread_list, li);
    slab_free(&thrpool, SLAB_FROM_DATA(li));

    threadno_free(bitmap, L4_ThreadNo(tid));
    threadno_free(as->threads, tno);

    if (tid.raw == as->tid.raw) {
        for (li = thread_list; li; li = li->next) {
            if (THREAD_TYPE(li->data)->space == as) break;
        }
        if (li == NULL) {
            // task destroy notification should go here
            address_space_destroy(as);
        } else {
            as->tid = THREAD_TYPE(li->data)->tid;
        }
    }

    mutex_unlock(&thrlock);
    
    // thread destroy notification should go here
    return TRUE;
} 

void task_destroy(AddrSpace_t *space)
{
    list_t *li, *next;
    AddrSpace_t *as;
    L4_Word_t tno;
    L4_ThreadId_t tid;
   
    assert(space != NULL);

    mutex_lock(&thrlock);

    next = NULL;
    for (li = thread_list; li; li = next) {
        as = THREAD_TYPE(li->data)->space;
        next = li->next;
        if (as != space) continue;

        tno = THREAD_TYPE(li->data)->index;
        tid = THREAD_TYPE(li->data)->tid;

        /* hope this succeeds */
        L4_ThreadControl(tid, L4_nilthread, L4_nilthread, L4_nilthread, (void *) -1);

        list_remove(&thread_list, li);
        slab_free(&thrpool, SLAB_FROM_DATA(li));
        threadno_free(bitmap, L4_ThreadNo(tid));

        // thread destroy notification should go here
    }

    address_space_destroy(space);

    mutex_unlock(&thrlock);
    
    // task destroy notification should go here
    return;
}

void thread_get_capability(HpfCapability *cap, L4_ThreadId_t tid, L4_Word_t flags)
{
    list_t *li;

    assert(cap != NULL);

    memset(cap, 0, sizeof(HpfCapability));
    li = list_find(thread_list, &tid, sizeof(L4_ThreadId_t));
    if (!li) return;

    hpf_capability_new(cap, L4_Myself(), (L4_Word_t) tid.raw, flags, (L4_Word_t) THREAD_TYPE(li->data)->creation.raw);
}

thread_t *thread_from_capability(HpfCapability *cap)
{
    list_t *li;
    L4_ThreadId_t tid;
    thread_t *tt;

    assert(cap != NULL);

    tid.raw = cap->object;

    li = list_find(thread_list, &tid, sizeof(L4_ThreadId_t));
    if (!li) return NULL;

    tt = THREAD_TYPE(li->data);

    if (hpf_capability_check(cap, tt->creation.raw)) return tt;
    return NULL;
}

boolean thread_lesser_capability(HpfCapability *cap, L4_Word_t flags)
{
    thread_t *tt;

    assert(cap != NULL);

    /* only lesser privileges */
    if ((flags & cap->flags) != flags) return FALSE;

    tt = thread_from_capability(cap);
    if (!tt) return FALSE;

    /* FIXME this does one linear search too many - we already know tt */
    thread_get_capability(cap, tt->tid, flags);
    return TRUE;
}

static inline L4_Word_t intlog2(L4_Word_t x)
{
    L4_Word_t y = 1;
    while (y < x) y = 2 * y;
    return y;
}

static L4_Word_t thread_prepare_stack(AddrSpace_t *as, L4_Word_t sp, char *cmdline, int n, char **paths, HpfCapability *caps)
{
    L4_Word_t size;
    int i;

    assert(as != NULL);

    /* nothing to do in this case */
    if (cmdline == NULL) return 0;

    /* we need room for cmdline and for path/capability pairs */
    size = strlen(cmdline) + 1 + 4 * sizeof(uint8_t *);

    size += n * sizeof(HpfCapability);
    for (i = 0; i < n; i++) {
        size += strlen(paths[i]) + 1;
    }

    if (size < PAGE_SIZE) size = PAGE_SIZE;
    size = intlog2(size);

    L4_Fpage_t vp = L4_Fpage(sp - size, size);
    L4_Fpage_t fp = address_space_request_page(as, vp);
    if (L4_IsNilFpage(fp)) {
        debug("thread_prepare_stack: can't setup stack!\n");
        return 0;
    }

    uint8_t *start = (uint8_t *) L4_Address(fp);
    uint32_t *tmp = (uint32_t *) start;

    tmp[0] = L4_Address(vp) + 4 * sizeof(uint8_t *); /* pointer to cmdline */
    tmp[1] = n; /* number of path/capability pairs */
    tmp[2] = L4_Address(vp) + 4 * sizeof(uint8_t *) + strlen(cmdline) + 1; /* pointer to array of paths */
    tmp[3] = L4_Address(vp) + size - n * sizeof(HpfCapability); /* pointer to array of caps */

    strcpy(start + 4 * sizeof(uint8_t *), cmdline);

    memcpy(start + size - n * sizeof(HpfCapability), caps, n * sizeof(HpfCapability));
    start += 4 * sizeof(uint8_t *) + strlen(cmdline) + 1;
    for (i = 0; i < n; i++) {
        char *x = paths[i];
        while (*x) *start++ = *x++;
        *start++ = 0;
    }

    return size;
}

