#include "common.h"
#include "utils.h"

char* filter_arg(char **param, int32_t *len) {
	char *begptr = *param;
	if (begptr == NULL) return NULL;

	while (*begptr != '\0' && (*begptr == ' ' || *begptr == '\t')) {
		++begptr;
	}
	char *endptr = begptr;
	while (*endptr != '\0' && (*endptr != ' ' && *endptr != '\t')) {
		++endptr;
	}
	if (begptr != endptr) {
		if (*endptr != '\0') {  /* don't reach the end */
			*param = endptr + 1;
			*endptr = '\0';
		} else {				/* reach the end */
			*param = NULL;
		}
		if (len) {
			*len = endptr - begptr;
		}
		return begptr;
	} else {					/* reach the end */
		*param = NULL;
		return NULL;
	}
}

extern void* nl_memdup(const void* src, uint32_t len)
{
	if (src) {
		if (len != 0) {
			void* temp = nl_malloc(len);
			if (temp) {
				return memcpy(temp, src, len);
			}
		} else {
			return nl_malloc(1);
		}
	}
	return NULL;
}
