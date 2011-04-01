#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <capability.h>
#include <dir.h>
#include <string.h>
#include <stdlib.h>

#define CORBA_alloc(x) malloc(x)
#define CORBA_free(x) free(x)

#include <client/DirectoryService.h>

HpfCapability my_cap;

extern void service_init(HpfCapability *cap);
extern void service_main();

int main(int argc, char *argv[])
{
    service_init(&my_cap);

    HpfCapability cap;
    if (!hpf_resolve("", &cap)) {
        printf("can't resolve directory service");
        return -1;
    }

    L4_ThreadId_t tid = cap.server;

    CORBA_Environment env = idl4_default_environment;
    HpfCapability newcap;

    bool retval = 
    DirectoryService_RegisterPath(tid, (Capability_t *) &cap, "/drivers/console", (Capability_t *) &my_cap, (Capability_t *) &newcap, &env);

    switch (env._major) {
        case CORBA_SYSTEM_EXCEPTION:
            printf("IPC failed, code %d\n", CORBA_exception_id(&env));
            CORBA_exception_free(&env);
            return 0;
    }

    if (!retval) {
        printf("console: can't register with directory service\n");
        return 0;
    }

    service_main();

    return 0;
}

