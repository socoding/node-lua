#ifndef LTPACK_H_
#define LTPACK_H_
#include "common.h"
#include "sds.h"

typedef struct tpack_t {
	sds m_data;
} tpack_t;

extern bool ltpack(tpack_t* pack, lua_State *L, int idx);
extern bool ltunpack(tpack_t* pack, lua_State *L);

#endif
