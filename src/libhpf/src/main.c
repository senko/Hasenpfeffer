/* main.c - Hasenpfeffer OS routines used by every program
 *
 * MIT X11 license, Copyright (c) 2006 by Senko Rasic <senko@senko.net>
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

#include <ctype.h>
#include <dir.h>
#include <string.h>
#include <capability.h>

extern int main(int argc, char **argv);

extern HpfDirectory hpf_directory;

int hpf_main(char *cmdline, int n_paths, char *paths, HpfCapability *caps)
{
    int argc = 0;
    char *argv[256];
    int space = 1;
    int i;

    while (*cmdline) {
        if (space == 1) {
            if (!isspace(*cmdline)) {
                space = 0;
                argv[argc] = cmdline;
                argc++;
            }
        } else {
            if (isspace(*cmdline)) {
                space = 1;
                *cmdline = 0;
            }
        }
        cmdline++;
    }

    hpf_directory_init(&hpf_directory);
    for (i = 0; i < n_paths; i++) {
        hpf_directory_add(&hpf_directory, paths, caps[i]);
        paths += strlen(paths) + 1;
    }
    
    return main(argc, argv);
}


