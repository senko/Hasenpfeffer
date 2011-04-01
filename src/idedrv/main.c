#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <capability.h>
#include <dir.h>
#include <string.h>
#include <stdlib.h>
#include "mindrvr.h"

#define CORBA_alloc(x) malloc(x)
#define CORBA_free(x) free(x)

#include <client/DirectoryService.h>

#include <l4/kdebug.h>

HpfCapability my_cap;

extern void service_init(HpfCapability *cap, unsigned char dev, unsigned long size);
extern void service_main();

int main(int argc, char *argv[])
{
    int ndev = reg_config();
    printf("Found %d IDE/ATAPI devices:\n", ndev);
    int i;
    char *desc[] = { "no", "unknown", "ATA", "ATAPI" };
    unsigned long dev_size = 0;

    for (i = 0; i <  ndev; i++) {
        if (!reg_config_info[i]) continue;

        char tmp[41];

        int size = reg_info(i, tmp);
        if (size) {
            char *a;
            for (a = tmp + 40; a >= tmp; a--) {
                if (*a == ' ') *a = 0; else break;
            }
        } else {
            tmp[0] = 0;
        }

        printf("  hd%d - %s drive: %s [%d MB]\n", i, desc[reg_config_info[i]], tmp, size / (1024 * 2));
        if (i == 0) dev_size = 512UL * size; // hack
    }

    if (reg_config_info[0] == 0) {
        printf("dunno what to do, panicking out\n");
        return 0;
    }

    service_init(&my_cap, 0, dev_size);

    HpfCapability cap;
    if (!hpf_resolve("", &cap)) {
        printf("can't resolve directory service");
        return -1;
    }

    L4_ThreadId_t tid = cap.server;

    CORBA_Environment env = idl4_default_environment;
    HpfCapability newcap;

    bool retval = 
    DirectoryService_RegisterPath(tid, (Capability_t *) &cap, "/drivers/hd0", (Capability_t *) &my_cap, (Capability_t *) &newcap, &env);

    switch (env._major) {
        case CORBA_SYSTEM_EXCEPTION:
            printf("IPC failed, code %d\n", CORBA_exception_id(&env));
            CORBA_exception_free(&env);
            return 0;
    }

    if (!retval) {
        printf("idedrv: can't register with directory service\n");
        return 0;
    }

    service_main();


    return 0;
}

