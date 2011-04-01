/* dir.c - simple directory implementation
 *
 * MIT X11 license, Copyright (c) 2005 by Senko Rasic <senko@senko.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#define _USE_XOPEN
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dir.h>

#define CORBA_string_alloc(x) malloc(x)
#define CORBA_alloc(x) malloc(x)
#define CORBA_free(x) free(x)
#include <client/DirectoryService.h>

HpfDirectory hpf_directory;

void hpf_directory_init(HpfDirectory *dir)
{
    assert(dir != NULL);
    dir->first = NULL;
}

static HpfDirEntry *hpf_direntry_new(uint8_t *name, HpfCapability cap)
{
    assert(name != NULL);

    HpfDirEntry *e = malloc(sizeof(HpfDirEntry));
    if (!e) return NULL;
    e->name = strdup(name);
    if (!e->name) {
        free(e);
        return NULL;
    }
    e->cap = cap;
    e->next = NULL;
    return e;
}

static void hpf_direntry_destroy(HpfDirEntry *e)
{
    assert(e != NULL);

    free(e->name);
    free(e);
}

HpfDirEntry *hpf_directory_add(HpfDirectory *dir, uint8_t *name, HpfCapability cap)
{
    HpfDirEntry *e, *tmp;
    int len;

    assert(dir != NULL);
    assert(name != NULL);

    e = hpf_direntry_new(name, cap);
    if (!e) return NULL;

    if (dir->first == NULL) {
        dir->first = e;
        return e;
    }

    len = strlen(name);

    if (len > strlen(dir->first->name)) {
        e->next = dir->first;
        dir->first = e;
        return e;
    }

    for (tmp = dir->first; tmp->next; tmp = tmp->next) {
        if (len > strlen(tmp->next->name)) break;
    }
    e->next = tmp->next;
    tmp->next = e;
    return e;
}

bool hpf_directory_remove(HpfDirectory *dir, uint8_t *name)
{
    HpfDirEntry *e;

    assert(dir != NULL);
    assert(name != NULL);

    if (dir->first == NULL) return false;
    if (!strcmp(dir->first->name, name)) {
        e = dir->first;
        dir->first = e->next;
        hpf_direntry_destroy(e);
        return true;
    }

    for (e = dir->first; e->next; e = e->next) {
        if (!strcmp(e->next->name, name)) {
            HpfDirEntry *tmp = e->next->next;
            hpf_direntry_destroy(e->next);
            e->next = tmp;
            return true;
        }
    }

    return false;
}

HpfDirEntry *hpf_directory_resolve(HpfDirectory *dir, uint8_t *name, uint32_t *resolved_chars)
{
    uint32_t i;
    HpfDirEntry *tmp;

    assert(dir != NULL);
    assert(name != NULL);

    *resolved_chars = 0;
    for (tmp = dir->first; tmp; tmp = tmp->next) {
//        printf("comparing [%s] to [%s]\n", name, tmp->name);
        for (i = 0; name[i] && tmp->name[i]; i++) if (name[i] != tmp->name[i]) break;
        if ((tmp->name[i] == 0) && ((name[i] == 0) || (name[i] == '/'))) {
            *resolved_chars = i;
            return tmp;
        }
    }
    return NULL;
}


bool hpf_resolve(char *path, HpfCapability *cap)
{
    HpfDirEntry *ent = NULL;
    int chars;

    ent = hpf_directory_resolve(&hpf_directory, path, &chars);
    if (!ent) return false;
    *cap = ent->cap;
    path += chars;

    while (*path) {
        CORBA_Environment env = idl4_default_environment;
        HpfCapability newcap;
//        printf("contacting server %lx, path is '%s' ...\n", cap->server.raw, path);
        bool retval = DirectoryService_ResolvePath(cap->server, (Capability_t *) cap, path, (Capability_t *) &newcap, &chars, &env);

        switch (env._major) {
            case CORBA_SYSTEM_EXCEPTION:
                CORBA_exception_free(&env);
                return false;
        }

//        printf("retval: %d\n", retval);
        if (!retval) return false;

        *cap = newcap;
        path += chars;
//        printf("server resolved %d chars, the rest is '%s'\n", chars, path);
    }

    return true;
}

