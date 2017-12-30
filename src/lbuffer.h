#ifndef LBUFFER_H_
#define LBUFFER_H_
#include "buffer.h"

#define BUFFER_METATABLE		"class buffer_t"

extern buffer_t* create_buffer(lua_State* L);
extern buffer_t* create_buffer(lua_State* L, size_t cap, const char *init, size_t init_len);
extern buffer_t* create_buffer(lua_State* L, buffer_t& buffer);

#endif
