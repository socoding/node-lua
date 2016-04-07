#ifndef LBSON_H_
#define LBSON_H_
#include "common.h"

#define DEFAULT_CAP		64
#define BSON_METATABLE	"bson"

typedef struct bson {
	bool extract;
	int size;
	int cap;
	uint8_t *ptr;
	uint8_t buffer[DEFAULT_CAP];
} bson_t;

extern void* create_bson(bson_t* bson_ptr, lua_State* L);
extern bson_t* bson_new(bool extract);
extern void bson_release(bson_t* bson_ptr);
extern bool bson_encode(bson_t* bson_ptr, lua_State *L, int idx);
extern bool bson_decode(bson_t* bson_ptr, lua_State *L);

#endif