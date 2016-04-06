#ifndef LBSON_H_
#define LBSON_H_
#include "common.h"

#define DEFAULT_CAP 64

typedef struct bson {
	int size;
	int cap;
	uint8_t *ptr;
	uint8_t buffer[DEFAULT_CAP];
} bson_t;

extern bson_t* bson_new();
extern void bson_free(bson_t* b);
extern bool bson_encode(bson_t* b, lua_State *L, int idx);

#endif