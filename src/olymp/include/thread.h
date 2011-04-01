#ifndef _THREAD_H_
#define _THREAD_C_

#include <l4/types.h>
#include <memory.h>
#include <capability.h>

struct thread {
    L4_ThreadId_t tid;
    L4_Word_t index;
    L4_Clock_t creation;
    AddrSpace_t *space;
};
typedef struct thread thread_t;
#define THREAD_TYPE(ptr) ((thread_t *) ptr)

#define MAX_TASKS (1<<18)

void thread_init(L4_ThreadId_t occupied);
L4_ThreadId_t thread_new(AddrSpace_t *space);
AddrSpace_t *task_new(L4_ThreadId_t pager);

void thread_start(L4_ThreadId_t tid, L4_Word_t ip, char *cmdline, int npaths, char **paths, HpfCapability *caps);
AddrSpace_t *thread_get_address_space(L4_ThreadId_t tid);

int thread_destroy(L4_ThreadId_t tid);
void task_destroy(AddrSpace_t *space);

void thread_get_capability(HpfCapability *cap, L4_ThreadId_t tid, L4_Word_t flags);
thread_t *thread_from_capability(HpfCapability *cap);
boolean thread_lesser_capability(HpfCapability *cap, L4_Word_t flags);

#endif

