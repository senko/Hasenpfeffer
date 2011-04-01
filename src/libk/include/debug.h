#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifndef VERBOSE_DEBUG
#define debug(x...)
#else
#include <l4io.h>
#define debug(x...) printf("[DEBUG] " x)
#endif

#endif /* _DEBUG_H_ */

