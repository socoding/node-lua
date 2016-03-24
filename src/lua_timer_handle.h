#ifndef LUA_TIMER_HANDLE_H_
#define LUA_TIMER_HANDLE_H_
#include "lua_handle_base.h"
#include "message.h"
#include "context_lua.h"

class uv_timer_handle_t;

class lua_timer_handle_t : public lua_handle_base_t 
{
public:
	lua_timer_handle_t(uv_timer_handle_t* handle, lua_State* L, int32_t session, bool is_repeat, bool is_cancel, void* userdata, timeout_callback_t callback)
		: lua_handle_base_t((uv_handle_base_t*)handle, L),
		  m_session(session),
		  m_is_repeat(is_repeat),
		  m_is_cancel(is_cancel),
		  m_userdata(userdata),
		  m_callback(callback) {}
	~lua_timer_handle_t() {}

private:
	int32_t m_session;
	bool m_is_repeat;
	bool m_is_cancel;
	void* m_userdata;
	timeout_callback_t m_callback;

public:
	FORCE_INLINE bool is_repeat() const { return m_is_repeat; };
	FORCE_INLINE bool is_cancel() const { return m_is_cancel; };
	void close();

public:
	static int32_t sleep(lua_State* L);
	static int32_t timeout(lua_State* L);
	static int32_t repeat(lua_State* L);
	static int32_t close(lua_State* L);

private:
	static int32_t sleep_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination);
	static int32_t sleep_yield_continue(lua_State* L, int status, lua_KContext ctx);
	static int32_t timer_callback_adjust(lua_State* L);

public:
	static lua_timer_handle_t* create_timer(lua_State* L, int32_t session, uint64_t timeout, uint64_t repeat, bool is_cancel, void *userdata, timeout_callback_t callback);
	static int32_t wakeup(lua_State* L, message_t& message);
};

#endif
