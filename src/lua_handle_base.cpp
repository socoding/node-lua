#include "request.h"
#include "network.h"
#include "uv_handle_base.h"
#include "lua_handle_base.h"
#include "context_lua.h"

char lua_handle_base_t::m_lua_ref_table_key = 0;

lua_handle_base_t::lua_handle_base_t(uv_handle_base_t* handle, lua_State* L)
	: m_handle_set(handle->get_handle_set()), m_lua_ref(handle->m_lua_ref), m_uv_handle(handle)
{
	grab_ref(L, this, m_handle_set);
}

uv_handle_type lua_handle_base_t::get_uv_handle_type() const
{
	return (m_uv_handle != NULL) ? m_uv_handle->get_handle_type() : UV_UNKNOWN_HANDLE;
}

void lua_handle_base_t::push_ref_table(lua_State* L, handle_set type)
{
	lua_checkstack(L, 3);
	lua_pushlightuserdata(L, &m_lua_ref_table_key); /* try to get lua_handle_base_t */
	lua_rawget(L, LUA_REGISTRYINDEX);
	if (!lua_isnil(L, -1)) {
		lua_rawgeti(L, -1, type);
		if (!lua_isnil(L, -1)) {
			lua_remove(L, -2);
			return;
		}
		lua_pop(L, 1);
	} else {
		lua_pop(L, 1);
		lua_createtable(L, 4, 0);
		lua_pushlightuserdata(L, &m_lua_ref_table_key);
		lua_pushvalue(L, -2);
		lua_rawset(L, LUA_REGISTRYINDEX);
	}
	lua_createtable(L, 4, 0);
	lua_pushvalue(L, -1);
	lua_rawseti(L, -3, type);
	lua_remove(L, -2);
}

int32_t lua_handle_base_t::grab_ref(lua_State* L, lua_handle_base_t* handle, handle_set type)
{
	push_ref_table(L, type);
	lua_pushlightuserdata(L, handle);
	if (handle != NULL) {
		ASSERT(type == handle->m_handle_set);
		if (handle->m_lua_ref == LUA_REFNIL) {
			handle->m_uv_handle->m_lua_ref = handle->m_lua_ref = luaL_ref(L, -2);
		} else {
			lua_rawseti(L, -2, handle->m_lua_ref);
		}
		lua_pop(L, 1);
		return handle->m_lua_ref;
	}
	int32_t ref = luaL_ref(L, -2);
	lua_pop(L, 1);
	return ref;
}

/*
two step release, in case of lua_ref reuse.
we hold the ref value until both uv_handle_base_t and lua_handle_base_t been freed.
*/
void lua_handle_base_t::release_ref(lua_State* L, int32_t ref, handle_set type)
{
	if (ref != LUA_REFNIL) {
		push_ref_table(L, type);
		lua_rawgeti(L, -1, ref);
		if (lua_touserdata(L, -1)) {
			lua_pushlightuserdata(L, NULL);
			lua_rawseti(L, -3, ref);
			lua_pop(L, 2);
		} else {
			luaL_unref(L, -2, ref);
			lua_pop(L, 2);
		}
	}
}

lua_handle_base_t* lua_handle_base_t::get_lua_handle(lua_State* L, int32_t ref, handle_set type)
{
	if (ref != LUA_REFNIL) {
		push_ref_table(L, type);
		lua_rawgeti(L, -1, ref);
		lua_handle_base_t* handle = (lua_handle_base_t*)lua_touserdata(L, -1);
		lua_pop(L, 2);
		return handle;
	}
	return NULL;
}

void lua_handle_base_t::close_uv_handle(uv_handle_base_t* handle)
{
	if (handle != NULL) {
		request_t request;
		request.m_type = REQUEST_HANDLE_CLOSE;
		request.m_length = REQUEST_SIZE(request_handle_close_t, 0);
		request.m_handle_close.m_handle_base = handle;
		singleton_ref(network_t).send_request(request);
	}
}

bool lua_handle_base_t::set_option(uint8_t type, const char *option, uint8_t option_len)
{
	if (option_len <= REQUEST_SPARE_SIZE(request_handle_option_t)) {
		request_t request;
		request.m_type = REQUEST_HANDLE_OPTION;
		request.m_length = REQUEST_SIZE(request_handle_option_t, option_len);
		request.m_handle_option.m_handle = m_uv_handle;
		request.m_handle_option.m_option_type = type;
		memcpy(REQUEST_SPARE_PTR(request.m_handle_option), option, option_len);
		singleton_ref(network_t).send_request(request);
		return true;
	}
	return false;
}

void lua_handle_base_t::_close()
{
	if (m_uv_handle != NULL) {
		close_uv_handle(m_uv_handle);
		m_uv_handle = NULL;
	}
}

int32_t lua_handle_base_t::close(lua_State* L)
{
	lua_handle_base_t* handle = (lua_handle_base_t*)lua_touserdata(L, 1);
	if (!handle->is_closed()) {
		handle->_close();
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

int32_t lua_handle_base_t::release(lua_State* L)
{
	lua_handle_base_t* handle = (lua_handle_base_t*)lua_touserdata(L, 1);
	if (handle != NULL) {
		handle->_close();
		release_ref(L, handle->m_lua_ref, handle->m_handle_set);
		handle->~lua_handle_base_t();
	}
	return 0;
}

int32_t lua_handle_base_t::is_closed(lua_State* L)
{
	lua_handle_base_t* handle = (lua_handle_base_t*)lua_touserdata(L, 1);
	lua_pushboolean(L, handle == NULL || handle->is_closed());
	return 1;
}

