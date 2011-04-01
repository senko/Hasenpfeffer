/* elf.c - simple ELF image loader
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

#include <stdlib.h>
#include <l4/types.h>
#include <elf32.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#include <debug.h>

static inline L4_Word_t intlog2(L4_Word_t x)
{
    L4_Word_t y = 1;
    while (y < x) y = 2 * y;
    return y;
}

// FIXME - i386 specific (EM_386)
L4_Word_t elf_load32(AddrSpace_t *space, L4_Word_t image, L4_Word_t size)
{
    Elf32_Ehdr *eh = (Elf32_Ehdr *) image;
    Elf32_Phdr *ph;
    int i;

    assert(space != NULL);

    debug("elf_load32: loading image at %lx (size %lx) for as: %p\n", image, size, space);

    if ((eh->e_ident[EI_MAG0] != ELFMAG0) ||
        (eh->e_ident[EI_MAG1] != ELFMAG1) ||
        (eh->e_ident[EI_MAG2] != ELFMAG2) ||
        (eh->e_ident[EI_MAG3] != ELFMAG3) ||
        (eh->e_type != ET_EXEC) ||
        (eh->e_machine != EM_386) ||
        (eh->e_phoff == 0))
    {
        debug("elf_load32: illegal ELF image at %lx\n", (L4_Word_t) image);
        return 0;
    }

    for (i = 0; i < eh->e_phnum; i++) {
        L4_Fpage_t vp, fp;
        L4_Word_t size;
        uint8_t *src, *dest;

        ph = (Elf32_Phdr *) (image + eh->e_phoff + i * eh->e_phentsize); 
        if (ph->p_type != PT_LOAD) continue;

        assert((ph->p_offset + ph->p_filesz) < size);
        assert(ph->p_filesz <= ph->p_memsz);

        size = intlog2(ph->p_memsz);
        vp = L4_Fpage(ph->p_vaddr, size);
        fp = address_space_request_page(space, vp);
        if (L4_IsNilFpage(fp)) {
            debug("elf_load32: can't allocate memory\n");
            return 0;
        }
        dest = (uint8_t *) L4_Address(fp);
        src = (uint8_t *) (image + ph->p_offset);

        memcpy(dest, src, ph->p_filesz);
        memset(dest + ph->p_filesz, 0, size - ph->p_filesz);
    }
    
    return eh->e_entry;
}

