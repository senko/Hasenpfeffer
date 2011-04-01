/* list.c - basic linked list operations
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
#include <string.h>
#include <list.h>
#include <assert.h>

void list_push(list_t **li, list_t *obj)
{
    assert(li !=  NULL);

    obj->next = *li;
    *li = obj;
}

list_t *list_pop(list_t **li)
{
    list_t *tmp;

    assert(li !=  NULL);

    if (*li == NULL) return NULL;

    tmp = *li;
    *li = tmp->next;
    return tmp;
}

list_t *list_find(list_t *li, void *data, L4_Word_t size)
{
    assert(li !=  NULL);
    assert(data != NULL);

    while (li) {
        if (!memcmp(li->data, data, size)) return li;
        li = li->next;
    }
    return NULL;
}

list_t *list_remove(list_t **li, list_t *obj)
{
    list_t *tmp;

    assert(li !=  NULL);
    assert(*li != NULL);

    if (*li == obj) return list_pop(li);

    tmp = *li;
    while (tmp->next) {
        if (tmp->next == obj) return list_pop(&(tmp->next));
        tmp = tmp->next;
    }
    return NULL;
}

