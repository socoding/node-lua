#include "lbuffer.h"

static int32_t lbuffer_new(lua_State* L)
{
	size_t cap = DEFAULT_BUFFER_CAPACITY;
	const char *init = NULL;
	size_t init_len = 0;
	int32_t type = lua_type(L, 1);
	if (type == LUA_TNUMBER) {
		cap = lua_tounsigned(L, 1);
		if (lua_gettop(L) > 1) {
			init = luaL_checklstring(L, 2, &init_len);
		}
	} else if (type == LUA_TSTRING) {
		init = lua_tolstring(L, 1, &init_len);
		cap = init_len + DEFAULT_BUFFER_CAPACITY;
	} else if (type != LUA_TNIL) {
		buffer_t* buffer = (buffer_t*)luaL_testudata(L, 1, BUFFER_METATABLE);
		if (!buffer) {
			luaL_argerror(L, 1, "unexpected argument type");
		}
		create_buffer(L, *buffer);
		return 1;
	}
	create_buffer(L, cap, init, init_len);
	return 1;
}

static int32_t lbuffer_append(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	size_t len = 0;
	const char* str = lua_tolstring(L, 2, &len);
	if (str) {
		lua_pushboolean(L, buffer_append(*buffer, str, len));
	} else {
		buffer_t* append = (buffer_t*)luaL_testudata(L, 2, BUFFER_METATABLE);
		if (!append) {
			luaL_argerror(L, 2, "unexpected argument type");
		}
		lua_pushboolean(L, buffer_concat(*buffer, *append));
	}
	return 1;
}

static int32_t lbuffer_split(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	int32_t top = lua_gettop(L);
	uint32_t length = top >= 2 ? luaL_checkunsigned(L, 2) : buffer_data_length(*buffer);
	buffer_t* split_buffer = create_buffer(L);
	*split_buffer = buffer_split(*buffer, length);
	return 1;
}

static int32_t lbuffer_clear(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	lua_pushboolean(L, buffer_clear(*buffer));
	return 1;
}

static int32_t lbuffer_length(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	lua_pushinteger(L, buffer_data_length(*buffer));
	return 1;
}

static int32_t lbuffer_unfill(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	lua_pushinteger(L, buffer_write_size(*buffer));
	return 1;
}

static int32_t lbuffer_tostring(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	if (buffer_is_valid(*buffer)) {
		lua_pushlstring(L, buffer_data_ptr(*buffer), buffer_data_length(*buffer));
	} else {
		lua_pushstring(L, "");
	}
	return 1;
}

static int32_t lbuffer_find(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	if (buffer_is_valid(*buffer)) {
		int32_t top = lua_gettop(L);
		if (top >= 2) {
			const char* ptr = buffer_data_ptr(*buffer);
			const char* find = strstr(ptr, luaL_checklstring(L, 2, NULL));
			if (find) {
				lua_pushinteger(L, (int32_t)(find - ptr) + 1);
				return 1;
			}
			return 0;
		} else {
			lua_pushinteger(L, 1);
			return 1;
		}
	}
	return 0;
}

static int32_t lbuffer_valid(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	lua_pushboolean(L, buffer_is_valid(*buffer));
	return 1;
}

static int32_t lbuffer_release(lua_State* L)
{
	buffer_t* buffer = (buffer_t*)luaL_checkudata(L, 1, BUFFER_METATABLE);
	if (buffer_is_valid(*buffer)) {
		buffer_release(*buffer);
		buffer_make_invalid(*buffer);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

static luaL_Reg lbuffer[] = {
	{ "new", lbuffer_new },
	{ "append", lbuffer_append },
	{ "split", lbuffer_split },
	{ "clear", lbuffer_clear },
	{ "length", lbuffer_length },
	{ "unfill", lbuffer_unfill },
	{ "find", lbuffer_find },
	{ "tostring", lbuffer_tostring },
	{ "valid", lbuffer_valid },
	{ "release", lbuffer_release },
	{ "__gc", lbuffer_release },
	{ "__tostring", lbuffer_tostring },
	{ NULL, NULL },
};

int luaopen_buffer(lua_State *L)
{
	luaL_newlib(L, lbuffer);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

buffer_t* create_buffer(lua_State* L)
{
	buffer_t* ret = (buffer_t*)lua_newuserdata(L, sizeof(buffer_t));
	buffer_make_invalid(*ret);
	if (luaL_newmetatable(L, BUFFER_METATABLE)) { /* create new metatable */
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		luaL_setfuncs(L, lbuffer, 0);
	}
	lua_setmetatable(L, -2);
	return ret;
}

buffer_t* create_buffer(lua_State* L, size_t cap, const char *init, size_t init_len)
{
	buffer_t* ret = create_buffer(L);
	*ret = buffer_new(cap, init, init_len);
	return ret;
}

buffer_t* create_buffer(lua_State* L, buffer_t& buffer)
{
	buffer_t* ret = create_buffer(L);
	buffer_grab(buffer);
	*ret = buffer;
	return ret;
}
