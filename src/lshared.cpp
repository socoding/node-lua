#include "lshared.h"
#include <typeinfo>

int shared_t::meta_index(lua_State *L)
{
	shared_t** shared = (shared_t**)luaL_checkudata(L, 1, SHARED_METATABLE);
	if (*shared && (*shared)->m_meta_reg && luaL_getmetatable(L, typeid(**shared).name()) != LUA_TNIL) {
		lua_pushvalue(L, 2);
		lua_rawget(L, -2);
		return 1;
	}
	return 0;
}

int shared_t::meta_gc(lua_State *L)
{
	shared_t** shared = (shared_t**)luaL_checkudata(L, 1, SHARED_METATABLE);
	if (*shared) {
		(*shared)->release();
		*shared = NULL;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

shared_t** shared_t::create(lua_State* L)
{
	shared_t** ret = (shared_t**)lua_newuserdata(L, sizeof(shared_t*));
	*ret = NULL;
	if (luaL_newmetatable(L, SHARED_METATABLE)) { /* create new metatable */
		lua_pushcclosure(L, meta_index, 0);
		lua_setfield(L, -2, "__index");
		lua_pushcclosure(L, meta_gc, 0);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);
	return ret;
}

shared_t** shared_t::create(lua_State* L, shared_t* shared)
{
	shared_t** ret = create(L);
	if (shared) {
		if (shared->m_meta_reg) {
			if (luaL_newmetatable(L, typeid(*shared).name())) { /* create new metatable */
				luaL_setfuncs(L, shared->m_meta_reg, 0);
			}
			lua_pop(L, 1);
		}
		shared->grab();
		*ret = shared;
	}
	return ret;
}

