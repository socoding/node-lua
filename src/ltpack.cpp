#include "ltpack.h"
#include "common.h"
#define __STDC_LIMIT_MACROS
#ifndef _WIN32
# include <unistd.h>
#endif

#define MAX_DEPTH 128

#define TYPE_INT8   0
#define TYPE_INT16  1
#define TYPE_INT32  2
#define TYPE_INT64  3
#define TYPE_DOUBLE 4
#define TYPE_STRING 5
#define TYPE_BOOL   6
#define TYPE_TABLE  7
//
//static inline void
//pack_int(tpack_t *pack, int64_t v) {
//	if (v >= INT8_MIN && v <= INT8_MAX) {
//		pack->m_data = sdsMakeRoomFor(pack->m_data, 2);
//		return TYPE_INT8;
//	}
//	if (v >= INT16_MIN && v <= INT16_MAX) {
//		return TYPE_INT16;
//	}
//	if (v >= INT32_MIN && v <= INT32_MAX) {
//		return TYPE_INT32;
//	}
//	return TYPE_INT64;
//}
//
//static void
//pack_dict(lua_State *L, tpack_t *b, int depth) {
//	if (depth > MAX_DEPTH) {
//		luaL_error(L, "Too depth while encoding bson");
//	}
//	luaL_checkstack(L, 16, NULL);	// reserve enough stack space to pack table
//	lua_pushnil(L);
//	while (lua_next(L, -2) != 0) {
//		int kt = lua_type(L, -2);
//		const char * key = NULL;
//		size_t sz;
//		switch (kt) {
//		case LUA_TNUMBER:
//			if (lua_isinteger(L, -2)) {
//				lua_tointeger(L, -2);
//			} else {
//				lua_tonumber(L, -2);
//			}
//			// copy key, don't change key type
//			lua_pushvalue(L, -2);
//			lua_insert(L, -2);
//			key = lua_tolstring(L, -2, &sz);
//			append_one(b, L, key, sz, depth);
//			lua_pop(L, 2);
//			break;
//		case LUA_TSTRING:
//			key = lua_tolstring(L, -2, &sz);
//			append_one(b, L, key, sz, depth);
//			lua_pop(L, 1);
//			break;
//		case LUA_TBOOLEAN:
//			break;
//		default:
//			luaL_error(L, "Invalid key type : %s", lua_typename(L, kt));
//			return;
//		}
//
//	}
//}
