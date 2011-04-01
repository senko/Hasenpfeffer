#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <dir.h>
#include <capability.h>

#define CORBA_string_alloc(x) malloc(x)
#define CORBA_alloc(x) malloc(x)
#define CORBA_free(x) free(x)

#include <client/File.h>

static HpfCapability console;

size_t hpf_stdio_write(void *data, long int position, size_t count, void *handle)
{
    HpfCapability *cap = (HpfCapability *) handle;

    if (cap == NULL) {
        if (console.server.raw == 0) if (!hpf_resolve("/drivers/console", &console)) return -1;
        cap = &console;
    }

    CORBA_Environment env = idl4_default_environment;
    int chars = File_Write(cap->server, (Capability_t *) cap, position, (unsigned char *) data, count, &env);

    switch (env._major) {
        case CORBA_SYSTEM_EXCEPTION:
            CORBA_exception_free(&env);
            return -1;
    }

    return chars;
}

size_t hpf_stdio_read(void *data, long int position, size_t count, void *handle)
{
    HpfCapability *cap = (HpfCapability *) handle;

    if (cap == NULL) {
        if (console.server.raw == 0) if (!hpf_resolve("/drivers/console", &console)) return -1;
        cap = &console;
    }

    CORBA_Environment env = idl4_default_environment;
    byteseq_t buf;
    if (!File_Read(cap->server, (Capability_t *) cap, position, &buf, &count, &env)) return -1;

    switch (env._major) {
        case CORBA_SYSTEM_EXCEPTION:
            CORBA_exception_free(&env);
            return -1;
    }

    memcpy(data, buf._buffer, count);

    CORBA_free(buf._buffer);
    return count;
}

struct __file __stdin = {
        NULL,
        hpf_stdio_read,
        NULL,
        NULL,
        NULL,
        _IONBF,
        NULL,
        0,
        0
};


struct __file __stdout = {
        NULL,
        NULL,
        hpf_stdio_write,
        NULL,
        NULL,
        _IONBF,
        NULL,
        0,
        0
};


struct __file __stderr = {
        NULL,
        NULL,
        hpf_stdio_write,
        NULL,
        NULL,
        _IONBF,
        NULL,
        0,
        0
};

FILE *stdin = &__stdin;
FILE *stdout = &__stdout;
FILE *stderr = &__stderr;

