#ifndef LUA_UDP_HANDLE_H_
#define LUA_UDP_HANDLE_H_
#include "common.h"
#include "message.h"
#include "lua_handle_base.h"
#include "context_lua.h"

class lua_udp_handle_t : public lua_handle_base_t
{
public:
	lua_udp_handle_t(uv_udp_handle_t* handle, lua_State* L)
		: lua_handle_base_t((uv_handle_base_t*)handle, L),
		  m_nonblocking_ref(LUA_REFNIL) {}
	~lua_udp_handle_t() {}
private:
	int32_t m_nonblocking_ref;					/* nonblocking_ref callback ref */
public:
	static int32_t open(lua_State* L);
	static int32_t open6(lua_State* L);
	static int32_t write(lua_State* L);
	static int32_t write6(lua_State* L);
	static int32_t read(lua_State* L);
	static int32_t close(lua_State* L);
	static int32_t udp_is_closed(lua_State* L);
	static int32_t set_wshared(lua_State* L);
	static int32_t get_fd(lua_State* L);
private:
	static int32_t _open(lua_State* L, bool ipv6);
	static int32_t open_callback_adjust(lua_State* L);
	static int32_t open_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination);
	static int32_t open_yield_continue(lua_State* L, int status, lua_KContext ctx);
	static int32_t _write(lua_State* L, bool ipv6);
	static int32_t write_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination);
	static int32_t read_callback_adjust(lua_State* L);
public:
	static lua_udp_handle_t* create_udp_socket(uv_udp_handle_t* handle, lua_State* L);
	static int32_t wakeup_open(lua_State* L, message_t& message);
	static int32_t wakeup_read(lua_State* L, message_t& message);
};

#endif