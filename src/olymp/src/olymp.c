/* memory.c - core memory management implementation
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

#include <memory.h>
#include <thread.h>

#include <l4/kip.h>
#include <l4/thread.h>
#include <l4/types.h>
#include <l4/schedule.h>
#include <l4/kdebug.h>
#include <l4/sigma0.h>
#include <l4/bootinfo.h>
#include <l4io.h>
#include <elf.h>
#include <debug.h>

#include <stdlib.h>

#define CORBA_alloc(x) (NULL)
#define CORBA_free(x) 
#include <client/DirectoryService.h>
#include <stdbool.h>

typedef void (*threadfunc_t)(void);

extern void Pager_server(void);

static L4_ThreadId_t mman_tid;
static void memman(void)
{
    printf("Memory manager ready and waiting for clients.\n");
    Pager_server();
}

extern void task_manager_init(HpfCapability *, HpfCapability *, HpfCapability *);
extern void task_manager_main(void);

static L4_ThreadId_t create_local_thread(char *desc, L4_KernelInterfacePage_t *kip, int idx, threadfunc_t func, L4_Word_t stack_size)
{
    L4_Word_t utcb_size = L4_UtcbSize(kip);
    L4_Word_t my_utcb = L4_MyLocalId().raw;
    my_utcb = (my_utcb & ~(utcb_size - 1));

    L4_ThreadId_t tid = L4_GlobalId(L4_ThreadNo(L4_Myself()) + idx, 1);
    L4_Word_t utcb_location = my_utcb + idx * utcb_size;

    if (FALSE == L4_ThreadControl(tid, L4_Myself(), L4_Myself(), L4_Pager(), (void *) utcb_location)) {
        printf("panic: can't execute %s: error code %d\n", desc, (int) L4_ErrorCode());
        return L4_nilthread;
    }

    void *stack = kmalloc(stack_size);

    L4_Start_SpIp(tid, (L4_Word_t) stack + stack_size - 32, (L4_Word_t) func);
    return tid;
}

#define MAX_PATHS 16
static int n_paths = 0;
static char *dir_paths[MAX_PATHS];
static HpfCapability dir_caps[MAX_PATHS];

static L4_ThreadId_t launch(L4_BootRec_t *br)
{
    AddrSpace_t *as = task_new(mman_tid);
    if (!as) {
        debug("launch: can't create address space for %s\n", L4_Module_Cmdline(br));
        return L4_nilthread;
    }

    /* we have to explicitly grab these from sigma0 */
    L4_Word_t mi;
    for (mi = 0; mi < L4_Module_Size(br); mi += 4096) {
        L4_Sigma0_GetPage(L4_nilthread, L4_Fpage(L4_Module_Start(br) + mi, 4096));
    }

    L4_Word_t start = elf_load32(as, L4_Module_Start(br), L4_Module_Size(br));
    thread_start(as->tid, start, L4_Module_Cmdline(br), n_paths, dir_paths, dir_caps);

    printf("    %s\n", L4_Module_Cmdline(br));
    return as->tid;
}

static bool hermes_capability(L4_ThreadId_t tid, HpfCapability *cap)
{
    CORBA_Environment env = idl4_default_environment;

//    printf("getting root capability from hermes\n");
    bool retval = DirectoryService_GetRootCapability(tid, (Capability_t *) cap, &env);

    switch (env._major) {
    case CORBA_SYSTEM_EXCEPTION:
        printf("IPC failed, code %d\n", CORBA_exception_id(&env));
        CORBA_exception_free(&env);
        return false;
    }

//    if (retval) printf("got it!\n"); else printf("hermes refused us\n");

    return retval;
}


int main (void)
{
    L4_Word_t total;

    printf("\n\nHasenpfeffer operating system\n");
    printf("Copyright (C) 2005,2006. Senko Rasic <senko@senko.net>\n\n");


    printf("Initializing root task...\n");
    L4_KernelInterfacePage_t *kip = (L4_KernelInterfacePage_t *) L4_GetKernelInterface();
    void *bi = (void *) L4_BootInfo(kip);
    
    // we need to preload these I/O pages
    // ATA
    L4_Sigma0_GetPage(L4_nilthread, L4_Fpage(0x0000, 4096));

    // KIP
    L4_Sigma0_GetPage(L4_nilthread, L4_Fpage(L4_BootInfo(kip), 4096));
    L4_BootRec_t *br = NULL;

    if (L4_BootInfo_Valid((void *) bi)) {
        int n = L4_BootInfo_Entries(bi);
        br = L4_BootInfo_FirstEntry(bi);
        while (--n) {
            /* touch roottask data from here, because sigma0 only gives pages to initial *thread* */
            if (L4_BootRec_Type(br) == L4_BootInfo_SimpleExec) {
                L4_Word_t mi;
                for (mi = 0; mi < L4_Module_Size(br); mi += 4096) {
                    L4_Sigma0_GetPage(L4_nilthread, L4_Fpage(L4_Module_Start(br) + mi, 4096));
                }
            }
            br = L4_BootRec_Next(br);
        }
    } else {
        printf("panic: invalid bootinfo data\n");
        return 0;
    }

    memory_init();
    total = memory_get_free();

    printf("Creating root memory manager...\n");
    mman_tid = create_local_thread("root memory manager", kip, 1, memman, 4096);
    thread_init(mman_tid);

    printf("Initializing root task manager...\n");
    HpfCapability full, newthread, newtask;
    task_manager_init(&full, &newtask, &newthread);

    dir_paths[0] = "/process/TaskManager";
    dir_caps[0] = full;
    n_paths = 1;

    printf("Loading initial programs...\n");
    int n = L4_BootInfo_Entries(bi);
    br = L4_BootInfo_FirstEntry(bi);
    int first_program = 1;
    while (--n) {
        if (L4_BootRec_Type(br) == L4_BootInfo_Module) {
            L4_ThreadId_t tid = launch(br);
            if (first_program) {
                first_program = 0;

                // this is the root directory server
                if (hermes_capability(tid, &(dir_caps[1]))) {
                    // dir_paths[1] = "/process/DirectoryService";
                    dir_paths[1] = ""; // <- catch-all
                    n_paths = 2;
                }
            }
        }
        br = L4_BootRec_Next(br);
    }

    printf("Init done, running...\n\n");
    task_manager_main();

    return 0;
}

