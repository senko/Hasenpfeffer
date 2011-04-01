#ifndef _HPF_DIR_H_
#define _HPF_DIR_H_

#include <stdint.h>
#include <stdbool.h>
#include <capability.h>

struct _dir_entry {
    uint8_t *name;
    HpfCapability cap;
    struct _dir_entry *next;
};
typedef struct _dir_entry HpfDirEntry;

struct _directory {
    struct _dir_entry *first;
};
typedef struct _directory HpfDirectory;

void hpf_directory_init(HpfDirectory *dir);
HpfDirEntry *hpf_directory_add(HpfDirectory *dir, uint8_t *name, HpfCapability cap);
bool hpf_directory_remove(HpfDirectory *dir, uint8_t *name);
HpfDirEntry *hpf_directory_resolve(HpfDirectory *dir, uint8_t *name, uint32_t *resolved_chars);
bool hpf_resolve(char *path, HpfCapability *cap);

#endif 

