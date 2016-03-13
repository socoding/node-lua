#include "lua_timer_handle.h"
#include "uv_timer_handle.h"
#include "request.h"
#include "context_lua.h"
#include "network.h"

#define TIMER_METATABLE		"class.timer_handle_t"

int32_t lua_timer_handle_t::sleep(lua_State* L)
{
	uint64_t timeout = 1000 * luaL_checknumber(L, 1);
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	request_t& request = lctx->get_yielding_request();
	request.m_timer_start.m_timeout = timeout > 0 ? timeout : 1;
	return lctx->lua_yield_send(L, 0, sleep_yield_finalize, NULL, sleep_yield_continue);
}

int32_t lua_timer_handle_t::sleep_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination)
{
	if (root_coro != NULL) {
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		request_t& request = lctx->get_yielding_request();
		int32_t session = context_lua_t::lua_ref_yield_coroutine(root_coro);
		context_lua_t::lua_ref_timer(root_coro, session, request.m_timer_start.m_timeout, 0, false);
	}
	return UV_OK;
}

int32_t lua_timer_handle_t::sleep_yield_continue(lua_State* L)
{
	return 0;
}

int32_t lua_timer_handle_t::timeout(lua_State* L)
{
	int32_t top = lua_gettop(L);
	uint64_t timeout = 1000 * luaL_checknumber(L, 1);
	luaL_checktype(L, -1, LUA_TFUNCTION);
	int32_t session = context_lua_t::lua_ref_callback(L, top - 2, LUA_REFNIL, timer_callback_adjust);
	int32_t timer_ref = context_lua_t::lua_ref_timer(L, session, timeout > 0 ? timeout : 1, 0, true);
	lua_pushinteger(L, timer_ref);
	return 1;
}

int32_t lua_timer_handle_t::repeat(lua_State* L)
{
	int32_t top = lua_gettop(L);
	uint64_t interval = 1000 * luaL_checknumber(L, 1);
	uint64_t repeat_time = 1000 * luaL_checknumber(L, 2);
	luaL_checktype(L, -1, LUA_TFUNCTION);
	if (interval != 0 || repeat_time != 0) {
		int32_t session = context_lua_t::lua_ref_callback(L, top - 3, LUA_REFNIL, timer_callback_adjust);
		int32_t timer_ref = context_lua_t::lua_ref_timer(L, session, interval, repeat_time, true);
		lua_pushinteger(L, timer_ref);
		return 1;
	}
	return 0;
}

int32_t lua_timer_handle_t::timer_callback_adjust(lua_State* L)
{
	lua_pop(L, 4);
	return 0;
}

lua_timer_handle_t* lua_timer_handle_t::create_timer(lua_State* L, int32_t session, uint64_t timeout, uint64_t repeat, bool is_cancel, void *userdata, timeout_callback_t callback)
{
	lua_checkstack(L, 3);
	uv_timer_handle_t* uv_timer = new uv_timer_handle_t(context_lua_t::lua_get_context_handle(L));
	lua_timer_handle_t* lua_timer = new(lua_newuserdata(L, sizeof(lua_timer_handle_t)))lua_timer_handle_t(uv_timer, L, session, repeat > 0, is_cancel, userdata, callback);
	if (luaL_newmetatable(L, TIMER_METATABLE)) { /* create new metatable */
		lua_pushcfunction(L, lua_timer_handle_t::release);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);
	request_t request;
	request.m_type = REQUEST_TIMER_START;
	request.m_length = REQUEST_SIZE(request_timer_start_t, 0);
	request.m_timer_start.m_timer_handle = uv_timer;
	request.m_timer_start.m_timeout = timeout;
	request.m_timer_start.m_repeat = repeat;
	singleton_ref(network_t).send_request(request);
	return lua_timer;
}

void lua_timer_handle_t::close()
{
	m_session = LUA_REFNIL;
	m_userdata = NULL;
	m_callback = NULL;
	_close();
}

int32_t lua_timer_handle_t::close(lua_State* L)
{
	int32_t timer_ref = luaL_checkunsigned(L, 1);
	lua_timer_handle_t* handle = (lua_timer_handle_t*)get_lua_handle(L, timer_ref, TIMER_SET);
	if (handle && !handle->is_closed() && handle->is_cancel()) {
		context_lua_t::lua_free_ref_session(L, handle->m_session);
	}
	return 0;
}

int32_t lua_timer_handle_t::wakeup(lua_State* L, message_t& message)
{
	lua_timer_handle_t* handle = (lua_timer_handle_t*)get_lua_handle(L, message.m_session, TIMER_SET);
	if (handle && !handle->is_closed() && handle->m_session != LUA_REFNIL) {
		if (handle->m_callback) {
			handle->m_callback(L, handle->m_session, handle->m_userdata, handle->is_repeat());
		} else {
			context_lua_t* lctx = context_lua_t::lua_get_context(L);
			return lctx->wakeup_ref_session(handle->m_session, message, !handle->is_repeat());
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

int luaopen_timer(lua_State *L)
{
	luaL_Reg l[] = {
		{ "sleep", lua_timer_handle_t::sleep },
		{ "timeout", lua_timer_handle_t::timeout },
		{ "close", lua_timer_handle_t::close },
		{ "loop", lua_timer_handle_t::repeat },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}


