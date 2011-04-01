#ifndef _HPF_CAPABILITY_H_
#define _HPF_CAPABILITY_H_

#define HPF_CAP_PERM_READ 4
#define HPF_CAP_PERM_WRITE 2
#define HPF_CAP_PERM_EXECUTE 1
#define HPF_CAP_PERM_FULL 7
#define HPF_CAP_PERM_NONE 0

#define HPF_CAP_CAN_READ(x) ((x).flags & HPF_CAP_PERM_READ)
#define HPF_CAP_CAN_WRITE(x) ((x).flags & HPF_CAP_PERM_WRITE)
#define HPF_CAP_CAN_EXECUTE(x) ((x).flags & HPF_CAP_PERM_EXECUTE)

#include <l4/types.h>

/* FIXME - this *has* to be in sync with Capability_t from interfaces */
struct _capability {
    L4_ThreadId_t server;
    L4_Word_t object;
    L4_Word_t flags;
    L4_Word_t signature;
};
typedef struct _capability HpfCapability;

void hpf_capability_new(HpfCapability *cap, L4_ThreadId_t server, L4_Word_t object, L4_Word_t flags, L4_Word_t secret);
int hpf_capability_check(HpfCapability *cap, L4_Word_t secret);

#endif

