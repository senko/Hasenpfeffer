/* capability.c - capability manipulation routines
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

#include <l4/types.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <capability.h>
#include <md5.h>

void hpf_capability_new(HpfCapability *cap, L4_ThreadId_t server, L4_Word_t object, L4_Word_t flags, L4_Word_t secret)
{
    uint8_t mdbuf[MD5_DIGEST_LEN];

    assert(cap != NULL);

    cap->server = server;
    cap->object = object;
    cap->flags = flags;
    cap->signature = secret;

    hpf_md5((unsigned char *) cap, sizeof(HpfCapability), mdbuf);
    memcpy(&(cap->signature), mdbuf, sizeof(L4_Word_t));
}

int hpf_capability_check(HpfCapability *cap, L4_Word_t secret)
{
    uint8_t mdbuf[MD5_DIGEST_LEN];
    HpfCapability tmp;

    assert(cap != NULL);

    tmp.server = cap->server;
    tmp.object = cap->object;
    tmp.flags = cap->flags;
    tmp.signature = secret;

    hpf_md5((unsigned char *) &tmp, sizeof(HpfCapability), mdbuf);
    return (!memcmp(&(cap->signature), mdbuf, sizeof(L4_Word_t)));
}

