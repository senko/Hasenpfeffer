#include <stdio.h>
#include <stdlib.h>
#include <dir.h>
#include <capability.h>

#define CORBA_string_alloc(x) malloc(x)
#define CORBA_alloc(x) malloc(x)
#define CORBA_free(x) free(x)

#include <client/File.h>

extern size_t hpf_stdio_write(void *data, long int position, size_t count, void *handle);
extern size_t hpf_stdio_read(void *data, long int position, size_t count, void *handle);

static long int eof_fn(void *handle)
{
    HpfCapability *cap = (HpfCapability *) handle;

    CORBA_Environment env = idl4_default_environment;
    unsigned long size = File_Size(cap->server, (Capability_t *) cap, &env);

    switch (env._major) {
        case CORBA_SYSTEM_EXCEPTION:
            CORBA_exception_free(&env);
            return 0;
    }

    return size;
}

static int close(void *handle)
{
    free(handle);
    return 0;
}

/** fixme - we ignore protection for now */
FILE *fopen(const char *fname, const char *prot)
{
    HpfCapability cap;

    if (!hpf_resolve((char *) fname, &cap)) return NULL;

    HpfCapability *ptr = malloc(sizeof(HpfCapability));
    if (!ptr) return NULL;

    *ptr = cap;

    FILE *fp = malloc(sizeof(FILE));
    if (!fp) {
        free(ptr);
        return NULL;
    }

    fp->handle = ptr;
//    fp->write_fn = hpf_stdio_write;
    fp->write_fn = NULL;
    fp->read_fn = hpf_stdio_read;
    fp->close_fn = close;
    fp->eof_fn = eof_fn;
    fp->current_pos = 0;
    fp->buffering_mode = _IONBF;
    fp->buffer = NULL;
    fp->unget_pos = 0;
    fp->eof = 0;

#ifdef THREAD_SAFE
    fp->mutex.holder = 0;
    fp->mutex.needed = 0;
    fp->mutex.count = 0;
#endif

    return fp;
}

