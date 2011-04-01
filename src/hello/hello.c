#include <stdio.h>
#include <l4/ipc.h>

#define SEKUNDA 1000000UL

int main(int argc, char *argv[])
{
    char buf[1024];

    L4_Sleep(L4_TimePeriod(4 * SEKUNDA));
    printf("Pozdrav, ja sam %s!\nTko ste vi: ", argv[0]);

    fgets(buf, 1023, stdin);
    printf("Pozdrav, %s\n", buf);

    return 0;
}
