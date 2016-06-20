#include "lua_udp_handle.h"
#include "request.h"
#include "context_lua.h"
#include "network.h"
#include "lbuffer.h"
#include "node_lua.h"
#include "uv_udp_handle.h"

#define UDP_SOCKET_METATABLE		"class.udp_socket_handle_t"

lua_udp_handle_t* lua_udp_handle_t::create_udp_socket(uv_udp_handle_t* handle, lua_State* L)
{
	lua_udp_handle_t* socket = new(lua_newuserdata(L, sizeof(lua_udp_handle_t)))lua_udp_handle_t(handle, L);
	if (luaL_newmetatable(L, UDP_SOCKET_METATABLE)) { /* create new metatable */
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, lua_udp_handle_t::release);
		lua_setfield(L, -2, "__gc");
		lua_pushcfunction(L, lua_udp_handle_t::write);
		lua_setfield(L, -2, "write");
		lua_pushcfunction(L, lua_udp_handle_t::write6);
		lua_setfield(L, -2, "write6");
		lua_pushcfunction(L, lua_udp_handle_t::read);
		lua_setfield(L, -2, "read");
		lua_pushcfunction(L, lua_udp_handle_t::set_wshared);
		lua_setfield(L, -2, "set_wshared");
		lua_pushcfunction(L, lua_udp_handle_t::get_local_addr);
		lua_setfield(L, -2, "local_addr");
		lua_pushcfunction(L, lua_udp_handle_t::get_remote_addr);
		lua_setfield(L, -2, "remote_addr");
		lua_pushcfunction(L, lua_udp_handle_t::get_local_port);
		lua_setfield(L, -2, "local_port");
		lua_pushcfunction(L, lua_udp_handle_t::get_remote_port);
		lua_setfield(L, -2, "remote_port");
		lua_pushcfunction(L, lua_udp_handle_t::close);
		lua_setfield(L, -2, "close");
		lua_pushcfunction(L, lua_udp_handle_t::udp_is_closed);
		lua_setfield(L, -2, "is_closed");
		lua_pushcfunction(L, lua_udp_handle_t::get_fd);
		lua_setfield(L, -2, "fd");
	}
	lua_setmetatable(L, -2);
	return socket;
}

int32_t lua_udp_handle_t::open(lua_State* L)
{
	return _open(L, false);
}

int32_t lua_udp_handle_t::open6(lua_State* L)
{
	return _open(L, true);
}

int32_t lua_udp_handle_t::open_callback_adjust(lua_State* L)
{
	uv_udp_handle_t* handle = (uv_udp_handle_t*)lua_touserdata(L, -3);
	if (handle) {
		lua_pop(L, 3);
		create_udp_socket(handle, L);
	} else {
		lua_pop(L, 2);
	}
	return 2;
}

int32_t lua_udp_handle_t::open_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination)
{
	if (root_coro != NULL) {
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		request_t& request = lctx->get_yielding_request();
		request.m_udp_open.m_session = context_lua_t::lua_ref_yield_coroutine(root_coro);
		singleton_ref(network_t).send_request(request);
	}
	return UV_OK;
}

int32_t lua_udp_handle_t::open_yield_continue(lua_State* L, int status, lua_KContext ctx)
{
	uv_udp_handle_t* handle = (uv_udp_handle_t*)lua_touserdata(L, -3);
	if (handle) {
		lua_pop(L, 3);
		create_udp_socket(handle, L);
	} else {
		lua_pop(L, 2);
		if (context_lua_t::lua_get_context(L)->get_yielding_status() != UV_OK) {
			context_lua_t::lua_throw(L, -1);
		}
	}
	return 2;
}

int32_t lua_udp_handle_t::_open(lua_State* L, bool ipv6)
{
	const char* host = NULL;
	size_t host_len;
	uint16_t port;
	int32_t callback = false;
	int32_t top = lua_gettop(L);
	if (!lua_isfunction(L, 1) && top > 0) {
		host = luaL_checklstring(L, 1, &host_len);
		port = luaL_checkunsigned(L, 2);
		if (host_len + 1 > REQUEST_SPARE_SIZE(request_udp_open_t)) {
			luaL_argerror(L, 1, "udp open host invalid(too long)");
		}
		if (lua_gettop(L) >= 3) {
			luaL_checktype(L, 3, LUA_TFUNCTION);
			callback = 3;
		}
	} else {
		host = !ipv6 ? "0.0.0.0" : "::";
		host_len = !ipv6 ? 7 : 2;
		port = 0;
		callback = top > 0 ? 1 : 0;
	}
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	request_t& request = lctx->get_yielding_request();
	request.m_type = REQUEST_UDP_OPEN;
	request.m_length = REQUEST_SIZE(request_udp_open_t, host_len + 1);
	request.m_udp_open.m_source = lctx->get_handle();
	request.m_udp_open.m_port = port;
	request.m_udp_open.m_ipv6 = ipv6;
	memcpy(REQUEST_SPARE_PTR(request.m_udp_open), host, host_len + 1);
	if (callback > 0) { /* nonblocking callback */
		lua_settop(L, callback);
		request.m_udp_open.m_session = context_lua_t::lua_ref_callback(L, callback - 1, LUA_REFNIL, open_callback_adjust);
		singleton_ref(network_t).send_request(request);
		return 0;
	} else { /* blocking */
		return lctx->lua_yield_send(L, 0, open_yield_finalize, NULL, open_yield_continue);
	}
}

int32_t lua_udp_handle_t::write(lua_State* L)
{
	return _write(L, false);
}

int32_t lua_udp_handle_t::write6(lua_State* L)
{
	return _write(L, true);
}

int32_t lua_udp_handle_t::_write(lua_State* L, bool ipv6)
{
	lua_udp_handle_t* socket = NULL;
	int64_t fd = 0;
	const char *s = NULL;
	uint32_t length;
	buffer_t* buffer = NULL;
	const char* host;
	size_t host_len;
	uint16_t port;
	if (!lua_isinteger(L, 1)) {
		socket = (lua_udp_handle_t*)luaL_checkudata(L, 1, UDP_SOCKET_METATABLE);
		if (socket->is_closed()) {
			return luaL_error(L, "attempt to send data on a invalid or closed socket");
		}
	} else {
		fd = lua_tointeger(L, 1);
	}
	if (((s = lua_tolstring(L, 2, (size_t*)&length)) == NULL) && ((buffer = (buffer_t*)luaL_testudata(L, 2, BUFFER_METATABLE)) == NULL)) {
		luaL_argerror(L, 2, "string expected");
	}
	if (buffer) {
		length = (uint32_t)buffer_data_length(*buffer);
	}
	host = luaL_checklstring(L, 3, &host_len);
	port = luaL_checkunsigned(L, 4);
	if (host_len + 1 > REQUEST_SPARE_SIZE(request_udp_write_t)) {
		luaL_argerror(L, 3, "udp send host invalid(too long)");
	}
	bool safety = false;
	bool nonblocking = false;
	if (lua_gettop(L) >= 5) {
		if (lua_isfunction(L, 5)) {
			nonblocking = true;
			lua_settop(L, 5);
		} else {
			safety = lua_toboolean(L, 5);
		}
	}
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	if (length == 0) { /* empty string or buffer */
		if (nonblocking) {
			int32_t session = context_lua_t::lua_ref_callback(L, 4, LUA_REFNIL, context_lua_t::common_callback_adjust);
			singleton_ref(node_lua_t).context_send(lctx, lctx->get_handle(), session, RESPONSE_UDP_WRITE, UV_OK);
		} else if (safety) { /* real blocking mode */
			lua_pushboolean(L, true);
			lua_pushinteger(L, UV_OK);
			return 2;
		}
		return 0;
	}
	request_t& request = lctx->get_yielding_request();
	request.m_type = REQUEST_UDP_WRITE;
	request.m_length = REQUEST_SIZE(request_udp_write_t, host_len + 1);
	if (socket != NULL) {
		request.m_udp_write.m_socket_handle = (uv_udp_handle_t*)socket->m_uv_handle;
		request.m_udp_write.m_shared_write = false;
	} else {
		request.m_udp_write.m_socket_fd = fd;
		request.m_udp_write.m_shared_write = true;
	}
	request.m_udp_write.m_source = lctx->get_handle();
	request.m_udp_write.m_port = port;
	request.m_udp_write.m_ipv6 = ipv6;
	memcpy(REQUEST_SPARE_PTR(request.m_udp_write), host, host_len + 1);
	if (s) {
		request.m_udp_write.m_length = length; /* length > 0 */
		request.m_udp_write.m_string = (const char*)nl_memdup(s, length);
		if (!request.m_udp_write.m_string) {
			return luaL_error(L, "attempt to send data(length %d) failed: memory not enough", length);
		}
	} else {
		request.m_udp_write.m_length = 0;
		request.m_udp_write.m_buffer = buffer_grab(*buffer);
	}
	if (nonblocking) { /*nonblocking callback mode*/
		request.m_udp_write.m_session = context_lua_t::lua_ref_callback(L, 4, LUA_REFNIL, context_lua_t::common_callback_adjust);
		singleton_ref(network_t).send_request(request);
		return 0;
	} else { /* blocking mode */
		if (!safety) { /* wait for nothing */
			request.m_udp_write.m_session = LUA_REFNIL;
			singleton_ref(network_t).send_request(request);
			return 0;
		} else { /* real blocking mode */
			return lctx->lua_yield_send(L, 0, write_yield_finalize, NULL, context_lua_t::common_yield_continue);
		}
	}
}

int32_t lua_udp_handle_t::write_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination)
{
	if (root_coro != NULL) { //yield succeeded
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		request_t& request = lctx->get_yielding_request();
		request.m_udp_write.m_session = context_lua_t::lua_ref_yield_coroutine(root_coro);
		singleton_ref(network_t).send_request(request);
	} else {  //yield failed, clear your data in case of memory leak.
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		request_t& request = lctx->get_yielding_request();
		if (request.m_udp_write.m_length > 0) {
			nl_free((void*)request.m_udp_write.m_string);
			request.m_udp_write.m_string = NULL;
			request.m_udp_write.m_length = 0;
		} else {
			buffer_release(request.m_udp_write.m_buffer);
		}
	}
	return UV_OK;
}

int32_t lua_udp_handle_t::read(lua_State* L)
{
	lua_udp_handle_t* socket = (lua_udp_handle_t*)luaL_checkudata(L, 1, UDP_SOCKET_METATABLE);
	if (socket->is_closed()) {
		return luaL_error(L, "attempt to read data on a invalid or closed socket");
	}
	luaL_checktype(L, 2, LUA_TFUNCTION);
	lua_settop(L, 2);
	socket->m_nonblocking_ref = context_lua_t::lua_ref_callback(L, 1, socket->m_nonblocking_ref, read_callback_adjust);
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	request_t& request = lctx->get_yielding_request();
	request.m_type = REQUEST_UDP_READ;
	request.m_length = REQUEST_SIZE(request_udp_read_t, 0);
	request.m_udp_read.m_socket_handle = (uv_udp_handle_t*)socket->m_uv_handle;
	singleton_ref(network_t).send_request(request);
	return 0;
}

int32_t lua_udp_handle_t::read_callback_adjust(lua_State* L)
{
	lua_pop(L, 2);
	for (int32_t i = lua_gettop(L); i < 5; ++i) {
		lua_pushnil(L);
	}
	return 5;
}

int32_t lua_udp_handle_t::close(lua_State* L)
{
	lua_udp_handle_t* socket = (lua_udp_handle_t*)luaL_checkudata(L, 1, UDP_SOCKET_METATABLE);
	if (!socket->is_closed()) {
		context_lua_t* lctx = context_lua_t::lua_get_context(L);
		/* The message will close the nonblocking lua ref */
		singleton_ref(node_lua_t).context_send(lctx, lctx->get_handle(), socket->m_lua_ref, RESPONSE_UDP_CLOSING, NL_EUDPSCLOSED);
		socket->_close();
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

int32_t lua_udp_handle_t::set_wshared(lua_State* L)
{
	lua_udp_handle_t* socket = (lua_udp_handle_t*)luaL_checkudata(L, 1, UDP_SOCKET_METATABLE);
	if (!socket->is_closed()) {
		bool enable = lua_toboolean(L, 2);
		socket->set_option(OPT_UDP_WSHARED, enable ? "\x01\0" : "\x00\0", 2);
	}
	return 0;
}

int32_t lua_udp_handle_t::udp_is_closed(lua_State* L)
{
	lua_udp_handle_t* socket = (lua_udp_handle_t*)luaL_checkudata(L, 1, UDP_SOCKET_METATABLE);
	if (socket != NULL) {
		lua_pushboolean(L, socket->is_closed());
		return 1;
	}
	lua_pushboolean(L, 0);
	return 1;
}

int32_t lua_udp_handle_t::get_local_addr(lua_State* L)
{
	lua_udp_handle_t* socket = (lua_udp_handle_t*)luaL_checkudata(L, 1, UDP_SOCKET_METATABLE);
	if (socket->is_closed()) {
		luaL_error(L, "attempt to perform on a invalid or closed socket");
	}
	uv_os_sock_t sock = ((uv_udp_handle_t*)socket->m_uv_handle)->m_udp_sock;
	char host[512] = { '\0' };
	bool ipv6 = false;
	if (socket_host(sock, true, host, sizeof(host), &ipv6, NULL)) {
		lua_pushstring(L, host);
		lua_pushboolean(L, ipv6);
	} else {
		lua_pushstring(L, "unknown address");
		lua_pushboolean(L, ipv6);
	}
	return 2;
}

int32_t lua_udp_handle_t::get_remote_addr(lua_State* L)
{
	lua_udp_handle_t* socket = (lua_udp_handle_t*)luaL_checkudata(L, 1, UDP_SOCKET_METATABLE);
	if (socket->is_closed()) {
		luaL_error(L, "attempt to perform on a invalid or closed socket");
	}
	uv_os_sock_t sock = ((uv_udp_handle_t*)socket->m_uv_handle)->m_udp_sock;
	char host[512] = { '\0' };
	bool ipv6 = false;
	if (socket_host(sock, false, host, sizeof(host), &ipv6, NULL)) {
		lua_pushstring(L, host);
	} else {
		lua_pushstring(L, "unknown address");
	}
	return 1;
}

int32_t lua_udp_handle_t::get_local_port(lua_State* L)
{
	lua_udp_handle_t* socket = (lua_udp_handle_t*)luaL_checkudata(L, 1, UDP_SOCKET_METATABLE);
	if (socket->is_closed()) {
		luaL_error(L, "attempt to perform on a invalid or closed socket");
	}
	uv_os_sock_t sock = ((uv_udp_handle_t*)socket->m_uv_handle)->m_udp_sock;
	uint16_t port;
	if (socket_host(sock, true, NULL, 0, NULL, &port)) {
		lua_pushinteger(L, port);
	} else {
		lua_pushinteger(L, -1);
	}
	return 1;
}

int32_t lua_udp_handle_t::get_remote_port(lua_State* L)
{
	lua_udp_handle_t* socket = (lua_udp_handle_t*)luaL_checkudata(L, 1, UDP_SOCKET_METATABLE);
	if (socket->is_closed()) {
		luaL_error(L, "attempt to perform on a invalid or closed socket");
	}
	uv_os_sock_t sock = ((uv_udp_handle_t*)socket->m_uv_handle)->m_udp_sock;
	uint16_t port;
	if (socket_host(sock, false, NULL, 0, NULL, &port)) {
		lua_pushinteger(L, port);
	} else {
		lua_pushinteger(L, -1);
	}
	return 1;
}

int32_t lua_udp_handle_t::get_fd(lua_State* L)
{
	lua_udp_handle_t* handle = (lua_udp_handle_t*)luaL_checkudata(L, 1, UDP_SOCKET_METATABLE);
	if (handle != NULL) {
		int64_t fd = SOCKET_MAKE_FD(handle->get_handle_ref(), context_lua_t::lua_get_context_handle(L));
		lua_pushinteger(L, fd);
		return 1;
	}
	return 0;
}

int32_t lua_udp_handle_t::wakeup_open(lua_State* L, message_t& message)
{
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	if (!lctx->wakeup_ref_session(message.m_session, message, true)) {
		if (message_is_userdata(message)) {
			close_uv_handle((uv_handle_base_t*)message_userdata(message));
		}
		return 0;
	}
	return 1;
}

int32_t lua_udp_handle_t::wakeup_read(lua_State* L, message_t& message)
{
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	lua_udp_handle_t* socket = (lua_udp_handle_t*)get_lua_handle(L, message.m_session, SOCKET_SET);
	if (socket) {
		if (!socket->is_closed()) {
			return lctx->wakeup_ref_session(socket->m_nonblocking_ref, message, false);
		} else if (socket->m_nonblocking_ref != LUA_REFNIL) { /* udp socket is already closed */
			context_lua_t::lua_free_ref_session(L, socket->m_nonblocking_ref);
			socket->m_nonblocking_ref = LUA_REFNIL;
			return 0;
		}
	}
	return 0;
}

int luaopen_udp(lua_State *L)
{
	luaL_Reg l[] = {
			{ "open", lua_udp_handle_t::open },
			{ "open6", lua_udp_handle_t::open6 },
			{ "write", lua_udp_handle_t::write },
			{ "write6", lua_udp_handle_t::write6 },
			{ "read", lua_udp_handle_t::read },
			{ "set_wshared", lua_udp_handle_t::set_wshared },
			{ "local_addr", lua_udp_handle_t::get_local_addr },
			{ "remote_addr", lua_udp_handle_t::get_remote_addr },
			{ "local_port", lua_udp_handle_t::get_local_port },
			{ "remote_port", lua_udp_handle_t::get_remote_port },
			{ "close", lua_udp_handle_t::close },
			{ "is_closed", lua_udp_handle_t::udp_is_closed },
			{ "fd", lua_udp_handle_t::get_fd },
			{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
