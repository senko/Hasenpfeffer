#ifndef _TYPES_H_
#define _TYPES_H_

typedef unsigned long IntPtr_t;
typedef unsigned long Thread_t;
typedef unsigned long Word_t;

typedef struct {
    Thread_t server;
    Word_t object;
    Word_t flags;
    Word_t signature;
} Capability_t;

#endif 

