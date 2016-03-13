#ifndef LUA_TCP_HANDLE_H_
#define LUA_TCP_HANDLE_H_
#include "common.h"
#include "message.h"
#include "lua_handle_base.h"
#include "context_lua.h"
#include <queue>

class context_lua_t;
class uv_handle_base_t;
class uv_tcp_listen_handle_t;
class uv_tcp_socket_handle_t;

class lua_tcp_listen_handle_t : public lua_handle_base_t
{
public:
	lua_tcp_listen_handle_t(uv_tcp_listen_handle_t* handle, lua_State* L)
		: lua_handle_base_t((uv_handle_base_t*)handle, L) {}
	~lua_tcp_listen_handle_t() {}

public:
	static int32_t listen(lua_State* L);
	static int32_t listen6(lua_State* L);
	static int32_t listens(lua_State* L);
	static int32_t accept(lua_State* L);
	static int32_t close(lua_State* L);
private:
	static int32_t _listen(lua_State* L, bool ipv6);
	static int32_t listen_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination);
	static int32_t accept_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination);
	static int32_t listen_callback_adjust(lua_State* L);
	static int32_t accept_callback_adjust(lua_State* L);
	static int32_t listen_yield_continue(lua_State* L);
	static int32_t accept_yield_continue(lua_State* L);
	static int32_t accept_timeout(lua_State *L, int32_t session, void* userdata, bool is_repeat);

private:
	ref_sessions_t m_accept_ref_sessions;

public:
	static lua_tcp_listen_handle_t* create_tcp_listen_socket(uv_tcp_listen_handle_t* handle, lua_State* L);
	static int32_t wakeup_listen(lua_State* L, message_t& message);
	static int32_t wakeup_accept(lua_State* L, message_t& message);
};

class lua_tcp_socket_handle_t : public lua_handle_base_t
{
public:
	lua_tcp_socket_handle_t(uv_tcp_socket_handle_t* handle, lua_State* L)
		: lua_handle_base_t((uv_handle_base_t*)handle, L) {}
	~lua_tcp_socket_handle_t();

public:
	static int32_t connect(lua_State* L);
	static int32_t connect6(lua_State* L);
	static int32_t connects(lua_State* L);
	static int32_t write(lua_State* L);
	static int32_t read(lua_State* L);
	static int32_t set_rwopt(lua_State* L);
	static int32_t get_rwopt(lua_State* L);
	static int32_t set_nodelay(lua_State* L);
	static int32_t close(lua_State* L);
private:
	static int32_t _connect(lua_State* L, bool ipv6);
	static int32_t connect_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination);
	static int32_t write_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination);
	static int32_t read_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination);
	static int32_t connect_callback_adjust(lua_State* L);
	static int32_t connect_yield_continue(lua_State* L);
	static int32_t read_timeout(lua_State *L, int32_t session, void* userdata, bool is_repeat);

public:
	static lua_tcp_socket_handle_t* create_tcp_socket(uv_tcp_socket_handle_t* handle, lua_State* L);
	static int32_t wakeup_connect(lua_State* L, message_t& message);
	static int32_t wakeup_read(lua_State* L, message_t& message);

private:
	void release_read_overflow_buffers();
private:
	ref_sessions_t m_read_ref_sessions;
	std::queue<buffer_t> m_read_overflow_buffers;
};

#endif
