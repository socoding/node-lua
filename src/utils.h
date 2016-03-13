#ifndef UTILS_H_
#define UTILS_H_
#include "common.h"

extern char* filter_arg(char **param, int32_t *len);
extern void* nl_memdup(const void* src, uint32_t len);

#endif
