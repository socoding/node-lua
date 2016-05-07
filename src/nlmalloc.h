#ifndef NLMALLOC_H_
#define NLMALLOC_H_
#include <stdlib.h>
#include <string.h>

#define nl_malloc		malloc
#define nl_free			free
#define nl_calloc(size)	calloc(1, size)
#define nl_realloc		realloc
#define nl_strdup		strdup

#endif
