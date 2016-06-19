#ifndef LTPACK_H_
#define LTPACK_H_
#include "sds.h"

typedef struct tpack_t {
	sds m_data;
} tpack_t;

extern tpack_t* bson_encode(lua_State *L, int idx);
extern bool bson_decode(tpack_t* pack, lua_State *L);

#endif
