#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <l4/thread.h>
#include <l4/ipc.h>

#include <capability.h>
#include <dir.h>

#define CORBA_alloc(x) malloc(x)
#define CORBA_free(x) free(x)

#include <client/DirectoryService.h>

#include "ext2fs.h"

extern bool service_init(HpfCapability *cap);
extern void server(void);

int main(int argc, char *argv[])
{
    FILE *fp = NULL;

    while (!fp) {
        L4_Sleep(L4_TimePeriod(1 * 1000000UL));
        fp = fopen("/drivers/hd0", "r");
    }

    if (ext2_mount(fp)) {
        printf("Found an ext2 filesystem\n");
    } else {
        printf("Can't mount it as ext2fs\n");
        fclose(fp);
        return 0;
    }


    HpfCapability my_cap;
    if (!service_init(&my_cap)) {
        printf("can't locate root directory!\n");
        fclose(fp);
        return 0;
    }

    HpfCapability cap;
    if (!hpf_resolve("", &cap)) {
        printf("can't resolve directory service");
        return -1;
    }

    L4_ThreadId_t tid = cap.server;

    CORBA_Environment env = idl4_default_environment;
    HpfCapability newcap;

    bool retval =
    DirectoryService_RegisterPath(tid, (Capability_t *) &cap, "/disk", (Capability_t *) &my_cap, (Capability_t *) &newcap, &env);

    switch (env._major) {
        case CORBA_SYSTEM_EXCEPTION:
            printf("IPC failed, code %d\n", CORBA_exception_id(&env));
            CORBA_exception_free(&env);
            return 0;
    }

    if (!retval) {
        printf("ext2fs: can't register with directory service\n");
        return 0;
    }

    printf("ext2fs: server ready\n");
    server();

    fclose(fp);
    return 0;
}

