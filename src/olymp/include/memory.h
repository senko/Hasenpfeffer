#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <l4/types.h>
#include <list.h>
#include <capability.h>

// FIXME - these are ok only for ia32 with max 2MB of RAM
#define MEMORY_CAPACITY (2 * 1024 * 1024 * 1024UL)
#define PAGE_SIZE (4 * 1024UL) // <- should be a constant initialized somewhere
#define KIP_AREA_LOCATION (0x0e0000)
#define KIP_AREA_SIZE (1<<12) // <- should be a constant initialized somewhere
#define UTCB_AREA_LOCATION (0x0f0000)
#define UTCB_AREA_SIZE (0x010000)
#define UTCB_SIZE 512
#define MAX_THREADS_PER_TASK 128

// #define STACK_BASE  (0x40000000UL - 32) // <- why -32?? found it in pingpong example
#define STACK_BASE  (0x40000000UL)
#define STACK_SIZE  (0x400000UL) // <- 4MB

// FIXME - this doesn't belong here
#define boolean uint8_t
#define FALSE 0
#define TRUE (~FALSE)


L4_Word_t memory_init(void);
L4_Word_t memory_get_free(void);
void *kmalloc(L4_Word_t size);
void kfree(L4_Word_t start, L4_Word_t size);

struct mempage {
    L4_Fpage_t v_page; /* virtual page */
    L4_Fpage_t p_page; /* physical page */
};
typedef struct mempage mempage_t;
#define MEMPAGE_TYPE(ptr) ((mempage_t *) ptr)

struct _AddrSpace {
    L4_ThreadId_t tid;
    L4_ThreadId_t pager;
    L4_Clock_t creation;
    list_t *memory;
    uint8_t threads[MAX_THREADS_PER_TASK / 8];
};
typedef struct _AddrSpace AddrSpace_t;
#define ADDRSPACE_TYPE(ptr) ((AddrSpace_t *) ptr)

AddrSpace_t *address_space_new(L4_ThreadId_t tid, L4_ThreadId_t pager);
void address_space_destroy(AddrSpace_t *this);
L4_Fpage_t address_space_request_page(AddrSpace_t *this, L4_Fpage_t vpage);
boolean address_space_release_page(AddrSpace_t *this, L4_Fpage_t vpage);
mempage_t *address_space_contains(AddrSpace_t *this, L4_Word_t addr);

void address_space_get_capability(HpfCapability *cap, AddrSpace_t *space, L4_Word_t flags);
AddrSpace_t *address_space_from_capability(HpfCapability *cap);
boolean address_space_lesser_capability(HpfCapability *cap, L4_Word_t flags);

#endif

