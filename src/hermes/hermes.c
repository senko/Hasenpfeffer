#include <l4io.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <capability.h>
#include <dir.h>

#include <mutex/mutex.h>

extern void service_main(void);
extern void service_init(L4_ThreadId_t root);

int main(int argc, char *argv[])
{
    int x;

    HpfCapability cap;
    if (!hpf_resolve("/process/TaskManager", &cap)) {
        printf("hermes: can't resolve task manager\n");
        return -1;
    }

    service_init(cap.server);
    service_main();
    return 0;
}

