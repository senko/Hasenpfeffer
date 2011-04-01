#include <stdio.h>
#include <l4/thread.h>
#include <l4/ipc.h>

int main(int argc, char *argv[])
{
    L4_Sleep(L4_TimePeriod(4 * 1000000UL));

    printf("Hello world\n");

    FILE *fp = fopen("/disk/scheme/init.scm", "r");
    if (fp) {
        printf("opened!\n");
    } else {
        printf("can't open\n");
    }

    char buf[1024];

    fgets(buf, 1023, fp);
    printf("fgets returns: [%s]\n", buf);

    fclose(fp);

    printf("\nsay something: ");
    fgets(buf, 1023, stdin);

    printf("you said: [%s]\n", buf);
    return 0;
}

