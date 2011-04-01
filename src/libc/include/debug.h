#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef NDEBUG
#define debug(x...)
#else
#include <stdio.h>
#define debug(x...) printf("[DEBUG] " x)
#endif

#endif /* _DEBUG_H_ */

