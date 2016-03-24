#include "lua_tcp_handle.h"
#include "request.h"
#include "context_lua.h"
#include "network.h"
#include "lbuffer.h"
#include "node_lua.h"
#include "uv_tcp_handle.h"

#define TCP_LISTEN_SOCKET_METATABLE	"class.tcp_listen_handle_t"
#define TCP_SOCKET_METATABLE		"class.tcp_socket_handle_t"

/////////////////////////////////////////////////////////////////////////////////////////////////////////

static int32_t lua_tcp_close(lua_State* L)
{
	if (luaL_testudata(L, 1, TCP_LISTEN_SOCKET_METATABLE)) {
		return lua_tcp_listen_handle_t::close(L);
	} else {
		return lua_tcp_socket_handle_t::close(L);
	}
}

static int32_t lua_tcp_is_closed(lua_State* L)
{
	lua_handle_base_t* handle;
	handle = (lua_handle_base_t*)luaL_testudata(L, 1, TCP_LISTEN_SOCKET_METATABLE);
	if (handle != NULL) {
		lua_pushboolean(L, handle->is_closed());
		return 1;
	}
	handle = (lua_handle_base_t*)luaL_testudata(L, 1, TCP_SOCKET_METATABLE);
	if (handle != NULL) {
		lua_pushboolean(L, handle->is_closed());
		return 1;
	}
	lua_pushboolean(L, 0);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

lua_tcp_listen_handle_t* lua_tcp_listen_handle_t::create_tcp_listen_socket(uv_tcp_listen_handle_t* handle, lua_State* L)
{
	lua_tcp_listen_handle_t* socket = new(lua_newuserdata(L, sizeof(lua_tcp_listen_handle_t)))lua_tcp_listen_handle_t(handle, L);
	if (luaL_newmetatable(L, TCP_LISTEN_SOCKET_METATABLE)) { /* create new metatable */
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, lua_tcp_listen_handle_t::release);
		lua_setfield(L, -2, "__gc");
		lua_pushcfunction(L, lua_tcp_listen_handle_t::accept);
		lua_setfield(L, -2, "accept");
		lua_pushcfunction(L, lua_tcp_listen_handle_t::close);
		lua_setfield(L, -2, "close");
		lua_pushcfunction(L, lua_tcp_is_closed);
		lua_setfield(L, -2, "is_closed");
	}
	lua_setmetatable(L, -2);
	return socket;
}

int32_t lua_tcp_listen_handle_t::_listen(lua_State* L, bool ipv6)
{
	size_t host_len;
	const char* host = luaL_checklstring(L, 1, &host_len);
	if (host_len + 1 > REQUEST_SPARE_SIZE(request_tcp_listen_t)) {
		luaL_argerror(L, 1, "listen host invalid(too long)");
	}
	uint16_t port = luaL_checkunsigned(L, 2);
	uint16_t backlog = (uint16_t)-1;
	int32_t top = lua_gettop(L);
	int32_t callback = 0;
	if (top >= 3) {
		if (lua_isfunction(L, 3)) {
			callback = 3;
		} else {
			backlog = luaL_checkunsigned(L, 3);
			if (top >= 4) {
				luaL_checktype(L, 4, LUA_TFUNCTION);
				callback = 4;
			}
		}
	}
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	request_t& request = lctx->get_yielding_request();
	request.m_type = REQUEST_TCP_LISTEN;
	request.m_length = REQUEST_SIZE(request_tcp_listen_t, host_len + 1);
	request.m_tcp_listen.m_source = lctx->get_handle();
	request.m_tcp_listen.m_backlog = backlog;
	request.m_tcp_listen.m_port = port;
	request.m_tcp_listen.m_ipv6 = ipv6;
	memcpy(REQUEST_SPARE_PTR(request.m_tcp_listen), host, host_len + 1);
	if (callback > 0) { /* nonblocking callback */
		lua_settop(L, callback);
		request.m_tcp_listen.m_session = context_lua_t::lua_ref_callback(L, callback - 1, LUA_REFNIL, listen_callback_adjust);
		singleton_ref(network_t).send_request(request);
		return 0;
	} else { /* blocking */
		return lctx->lua_yield_send(L, 0, listen_yield_finalize, NULL, listen_yield_continue);
	}
}

int32_t lua_tcp_listen_handle_t::listens(lua_State* L)
{
	size_t sock_len;
	const char* sock = luaL_checklstring(L, 1, &sock_len);
	if (sock_len + 1 > REQUEST_SPARE_SIZE(request_tcp_listens_t)) {
		luaL_argerror(L, 1, "listen sock name invalid(too long)");
	}
	int32_t top = lua_gettop(L);
	int32_t callback = 0;
	if (top >= 2) {
		luaL_checktype(L, 2, LUA_TFUNCTION);
		callback = 2;
	}
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	request_t& request = lctx->get_yielding_request();
	request.m_type = REQUEST_TCP_LISTENS;
	request.m_length = REQUEST_SIZE(request_tcp_listens_t, sock_len + 1);
	request.m_tcp_listens.m_source = lctx->get_handle();
	memcpy(REQUEST_SPARE_PTR(request.m_tcp_listens), sock, sock_len + 1);
	if (callback > 0) { /* nonblocking callback */
		lua_settop(L, callback);
		request.m_tcp_listens.m_session = context_lua_t::lua_ref_callback(L, callback - 1, LUA_REFNIL, listen_callback_adjust);
		singleton_ref(network_t).send_request(request);
		return 0;
	} else { /* blocking */
		return lctx->lua_yield_send(L, 0, listen_yield_finalize, NULL, listen_yield_continue);
	}
}

int32_t lua_tcp_listen_handle_t::listen_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination)
{
	if (root_coro != NULL) {
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		request_t& request = lctx->get_yielding_request();
		if (request.m_type == REQUEST_TCP_LISTEN) {
			request.m_tcp_listen.m_session = context_lua_t::lua_ref_yield_coroutine(root_coro);
		} else {
			request.m_tcp_listens.m_session = context_lua_t::lua_ref_yield_coroutine(root_coro);
		}
		singleton_ref(network_t).send_request(request);
	}
	return UV_OK;
}

int32_t lua_tcp_listen_handle_t::listen_yield_continue(lua_State* L, int status, lua_KContext ctx)
{
	uv_tcp_listen_handle_t* listen_handle = (uv_tcp_listen_handle_t*)lua_touserdata(L, -3);
	if (listen_handle) {
		lua_pop(L, 3);
		create_tcp_listen_socket(listen_handle, L);
	} else {
		lua_pop(L, 2);
		if (context_lua_t::lua_get_context(L)->get_yielding_status() != UV_OK) {
			context_lua_t::lua_throw(L, -1);
		}
	}
	return 2;
}

int32_t lua_tcp_listen_handle_t::listen_callback_adjust(lua_State* L)
{
	uv_tcp_listen_handle_t* listen_handle = (uv_tcp_listen_handle_t*)lua_touserdata(L, -3);
	if (listen_handle) {
		lua_pop(L, 3);
		create_tcp_listen_socket(listen_handle, L);
	} else {
		lua_pop(L, 2);
	}
	return 2;
}

int32_t lua_tcp_listen_handle_t::listen(lua_State* L)
{
	return _listen(L, false);
}

int32_t lua_tcp_listen_handle_t::listen6(lua_State* L)
{
	return _listen(L, true);
}

int32_t lua_tcp_listen_handle_t::accept(lua_State* L)
{
	lua_tcp_listen_handle_t* listen_socket = (lua_tcp_listen_handle_t*)luaL_checkudata(L, 1, TCP_LISTEN_SOCKET_METATABLE);
	if (listen_socket->is_closed()) {
		return luaL_error(L, "attempt to perform accept on a invalid or closed listen socket");
	}
	uint64_t timeout = 0;
	bool nonblocking = false;
	if (lua_gettop(L) >= 2) {
		if (lua_isfunction(L, 2)) {
			nonblocking = true;
			lua_settop(L, 2);
		} else {
			timeout = 1000 * luaL_checknumber(L, 2);
		}
	}
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	request_t& request = lctx->get_yielding_request();
	request.m_type = REQUEST_TCP_ACCEPT;
	request.m_length = REQUEST_SIZE(request_tcp_accept_t, 0);
	request.m_tcp_accept.m_listen_handle = (uv_tcp_listen_handle_t*)listen_socket->m_uv_handle;
	request.m_tcp_accept.m_blocking = !nonblocking;
	if (nonblocking) { /* nonblocking callback */
		listen_socket->m_accept_ref_sessions.make_nonblocking_callback(L, 1, accept_callback_adjust);
		singleton_ref(network_t).send_request(request);
		return 0;
	} else { /* blocking */
		return lctx->lua_yield_send(L, 0, accept_yield_finalize, listen_socket, accept_yield_continue, timeout);
	}
}

int32_t lua_tcp_listen_handle_t::accept_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination)
{
	if (root_coro != NULL) {
		lua_tcp_listen_handle_t* listen_socket = (lua_tcp_listen_handle_t*)userdata;
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		int32_t session = listen_socket->m_accept_ref_sessions.push_blocking(root_coro);
		singleton_ref(network_t).send_request(lctx->get_yielding_request());
		int64_t timeout = lctx->get_yielding_timeout();
		if (timeout > 0) {
			context_lua_t::lua_ref_timer(main_coro, session, timeout, 0, false, listen_socket, accept_timeout);
		}
	}
	return UV_OK;
}

int32_t lua_tcp_listen_handle_t::accept_yield_continue(lua_State* L, int status, lua_KContext ctx)
{
	uv_tcp_socket_handle_t* accept_handle = (uv_tcp_socket_handle_t*)lua_touserdata(L, -3);
	if (accept_handle) {
		lua_pop(L, 3);
		lua_tcp_socket_handle_t::create_tcp_socket(accept_handle, L);
	} else {
		lua_pop(L, 2);
		if (context_lua_t::lua_get_context(L)->get_yielding_status() != UV_OK) {
			context_lua_t::lua_throw(L, -1);
		}
	}
	return 2;
}

int32_t lua_tcp_listen_handle_t::accept_timeout(lua_State *L, int32_t session, void* userdata, bool is_repeat)
{
	lua_tcp_listen_handle_t* listen_socket = (lua_tcp_listen_handle_t*)userdata;
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	message_t message(0, session, RESPONSE_TCP_ACCEPT, NL_ETIMEOUT);
	return listen_socket->m_accept_ref_sessions.wakeup_one_fixed(lctx, message, false);
}

int32_t lua_tcp_listen_handle_t::accept_callback_adjust(lua_State* L)
{
	uv_tcp_socket_handle_t* accept_handle = (uv_tcp_socket_handle_t*)lua_touserdata(L, -3);
	if (accept_handle) {
		lua_pop(L, 3);
		lua_tcp_socket_handle_t::create_tcp_socket(accept_handle, L);
	} else {
		lua_pop(L, 2);
	}
	return 2;
}

int32_t lua_tcp_listen_handle_t::wakeup_listen(lua_State* L, message_t& message)
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

int32_t lua_tcp_listen_handle_t::wakeup_accept(lua_State* L, message_t& message)
{
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	lua_tcp_listen_handle_t* socket = (lua_tcp_listen_handle_t*)get_lua_handle(L, message.m_session, SOCKET_SET);
	if (socket) {
		if (!socket->is_closed()) {
			if (message_is_userdata(message)) {
				if (socket->m_accept_ref_sessions.wakeup_once(lctx, message)) {
					return 1;
				}
				close_uv_handle((uv_handle_base_t*)message_userdata(message));
				return 0;
			}
			if (message_is_error(message)) {
				socket->m_accept_ref_sessions.wakeup_all(lctx, message);
				return 1;
			}
			return 0;
		} else { /* listen socket is already closed */
			if (message_is_userdata(message)) {
				close_uv_handle((uv_handle_base_t*)message_userdata(message));
			}
			message_t error = message_t(message.m_source, message.m_session, RESPONSE_TCP_CLOSING, NL_ETCPLCLOSED);
			socket->m_accept_ref_sessions.free_nonblocking_callback(L);
			socket->m_accept_ref_sessions.wakeup_all_blocking(lctx, error);
			return 1;
		}
	} else {
		if (message_is_userdata(message)) {
			close_uv_handle((uv_handle_base_t*)message_userdata(message));
			return 0;
		}
	}
	return 0;
}

int32_t lua_tcp_listen_handle_t::close(lua_State* L)
{
	lua_tcp_listen_handle_t* socket = (lua_tcp_listen_handle_t*)luaL_checkudata(L, 1, TCP_LISTEN_SOCKET_METATABLE);
	if (!socket->is_closed()) {
		context_lua_t* lctx = context_lua_t::lua_get_context(L);
		/* The message will wake up all blocking coroutine only*/
		singleton_ref(node_lua_t).context_send(lctx, lctx->get_handle(), socket->m_lua_ref, RESPONSE_TCP_CLOSING, NL_ETCPLCLOSED);
		socket->_close();
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
lua_tcp_socket_handle_t::~lua_tcp_socket_handle_t()
{
	release_read_overflow_buffers();
}

lua_tcp_socket_handle_t* lua_tcp_socket_handle_t::create_tcp_socket(uv_tcp_socket_handle_t* handle, lua_State* L)
{
	lua_tcp_socket_handle_t* socket = new(lua_newuserdata(L, sizeof(lua_tcp_socket_handle_t)))lua_tcp_socket_handle_t(handle, L);
	if (luaL_newmetatable(L, TCP_SOCKET_METATABLE)) { /* create new metatable */
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, lua_tcp_socket_handle_t::release);
		lua_setfield(L, -2, "__gc");
		lua_pushcfunction(L, lua_tcp_socket_handle_t::close);
		lua_setfield(L, -2, "close");
		lua_pushcfunction(L, lua_tcp_socket_handle_t::write);
		lua_setfield(L, -2, "write");
		lua_pushcfunction(L, lua_tcp_socket_handle_t::read);
		lua_setfield(L, -2, "read");
		lua_pushcfunction(L, lua_tcp_socket_handle_t::set_rwopt);
		lua_setfield(L, -2, "set_rwopt");
		lua_pushcfunction(L, lua_tcp_socket_handle_t::get_rwopt);
		lua_setfield(L, -2, "get_rwopt");
		lua_pushcfunction(L, lua_tcp_socket_handle_t::set_nodelay);
		lua_setfield(L, -2, "set_nodelay");
		lua_pushcfunction(L, lua_tcp_is_closed);
		lua_setfield(L, -2, "is_closed");
	}
	lua_setmetatable(L, -2);
	return socket;
}

int32_t lua_tcp_socket_handle_t::connect(lua_State* L)
{
	return _connect(L, false);
}

int32_t lua_tcp_socket_handle_t::connect6(lua_State* L)
{
	return _connect(L, true);
}

int32_t lua_tcp_socket_handle_t::_connect(lua_State* L, bool ipv6)
{
	size_t host_len;
	const char* host = luaL_checklstring(L, 1, &host_len);
	if (host_len + 2 > REQUEST_SPARE_SIZE(request_tcp_connect_t)) {
		luaL_argerror(L, 1, "connect host invalid(too long)");
	}
	uint16_t port = luaL_checkunsigned(L, 2);
	int32_t top = lua_gettop(L);
	uint64_t timeout = 0;
	int32_t callback = 0;
	if (top >= 3) {
		if (lua_isfunction(L, 3)) {
			callback = 3;
		} else {
			timeout = 1000 * luaL_checknumber(L, 3);
			if (top >= 4) {
				luaL_checktype(L, 4, LUA_TFUNCTION);
				callback = 4;
			}
		}
	}
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	request_t& request = lctx->get_yielding_request();
	request.m_type = REQUEST_TCP_CONNECT;
	request.m_length = REQUEST_SIZE(request_tcp_connect_t, host_len + 2);
	request.m_tcp_connect.m_source = lctx->get_handle();
	request.m_tcp_connect.m_remote_port = port;
	request.m_tcp_connect.m_local_port = 0;     /* we don't support it at this version */
	request.m_tcp_connect.m_remote_ipv6 = ipv6;
	request.m_tcp_connect.m_local_ipv6 = false; /* we don't support it at this version */
	char* remote_host = REQUEST_SPARE_PTR(request.m_tcp_connect);
	char* local_host = remote_host + host_len + 1;
	memcpy(remote_host, host, host_len + 1);
	*local_host = '\0'; /* we don't support it at this version */
	if (callback > 0) { /* nonblocking callback */
		lua_settop(L, callback);
		request.m_tcp_connect.m_session = context_lua_t::lua_ref_callback(L, callback - 1, LUA_REFNIL, context_lua_t::common_callback_adjust);
		singleton_ref(network_t).send_request(request);
		if (timeout > 0) {
			context_lua_t::lua_ref_timer(L, request.m_tcp_connect.m_session, timeout, 0, false);
		}
		return 0;
	} else { /* blocking */
		return lctx->lua_yield_send(L, 0, connect_yield_finalize, NULL, connect_yield_continue, timeout);
	}
}

int32_t lua_tcp_socket_handle_t::connects(lua_State* L)
{
	size_t sock_len;
	const char* sock = luaL_checklstring(L, 1, &sock_len);
	if (sock_len + 1 > REQUEST_SPARE_SIZE(request_tcp_connects_t)) {
		luaL_argerror(L, 1, "connect sock name invalid(too long)");
	}
	int32_t top = lua_gettop(L);
	uint64_t timeout = 0;
	int32_t callback = 0;
	if (top >= 2) {
		if (lua_isfunction(L, 2)) {
			callback = 2;
		} else {
			timeout = 1000 * luaL_checknumber(L, 2);
			if (top >= 3) {
				luaL_checktype(L, 3, LUA_TFUNCTION);
				callback = 3;
			}
		}
	}
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	request_t& request = lctx->get_yielding_request();
	request.m_type = REQUEST_TCP_CONNECTS;
	request.m_length = REQUEST_SIZE(request_tcp_connects_t, sock_len + 1);
	request.m_tcp_connects.m_source = lctx->get_handle();
	memcpy(REQUEST_SPARE_PTR(request.m_tcp_connects), sock, sock_len + 1);
	if (callback > 0) { /* nonblocking callback */
		lua_settop(L, callback);
		request.m_tcp_connects.m_session = context_lua_t::lua_ref_callback(L, callback - 1, LUA_REFNIL, context_lua_t::common_callback_adjust);
		singleton_ref(network_t).send_request(request);
		if (timeout > 0) {
			context_lua_t::lua_ref_timer(L, request.m_tcp_connects.m_session, timeout, 0, false);
		}
		return 0;
	} else { /* blocking */
		return lctx->lua_yield_send(L, 0, connect_yield_finalize, NULL, connect_yield_continue, timeout);
	}
}

int32_t lua_tcp_socket_handle_t::connect_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination)
{
	if (root_coro != NULL) {
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		request_t& request = lctx->get_yielding_request();
		uint32_t session = context_lua_t::lua_ref_yield_coroutine(root_coro);
		if (request.m_type == REQUEST_TCP_CONNECT) {
			request.m_tcp_connect.m_session = session;
		} else {
			request.m_tcp_connects.m_session = session;
		}
		singleton_ref(network_t).send_request(request);
		int64_t timeout = lctx->get_yielding_timeout();
		if (timeout > 0) {
			context_lua_t::lua_ref_timer(main_coro, session, timeout, 0, false);
		}
	}
	return UV_OK;
}

int32_t lua_tcp_socket_handle_t::connect_callback_adjust(lua_State* L)
{
	uv_tcp_socket_handle_t* connect_handle = (uv_tcp_socket_handle_t*)lua_touserdata(L, -3);
	if (connect_handle) {
		lua_pop(L, 3);
		lua_tcp_socket_handle_t::create_tcp_socket(connect_handle, L);
	} else {
		lua_pop(L, 2);
	}
	return 2;
}

int32_t lua_tcp_socket_handle_t::connect_yield_continue(lua_State* L, int status, lua_KContext ctx)
{
	uv_tcp_socket_handle_t* connect_handle = (uv_tcp_socket_handle_t*)lua_touserdata(L, -3);
	if (connect_handle) {
		lua_pop(L, 3);
		lua_tcp_socket_handle_t::create_tcp_socket(connect_handle, L);
	} else {
		lua_pop(L, 2);
		if (context_lua_t::lua_get_context(L)->get_yielding_status() != UV_OK) {
			context_lua_t::lua_throw(L, -1);
		}
	}
	return 2;
}

int32_t lua_tcp_socket_handle_t::write(lua_State* L)
{
	lua_tcp_socket_handle_t* socket = (lua_tcp_socket_handle_t*)luaL_checkudata(L, 1, TCP_SOCKET_METATABLE);
	if (socket->is_closed()) {
		return luaL_error(L, "attempt to send data on a invalid or closed socket");
	}
	const char *s = NULL;
	uint32_t length;
	buffer_t* buffer = NULL;
	if (((s = lua_tolstring(L, 2, (size_t*)&length)) == NULL) && ((buffer = (buffer_t*)luaL_testudata(L, 2, BUFFER_METATABLE)) == NULL)) {
		luaL_argerror(L, 2, "string expected");
	}
	if (buffer) {
		length = (uint32_t)buffer_data_length(*buffer);
	}
	uv_tcp_socket_handle_t* handle = (uv_tcp_socket_handle_t*)socket->m_uv_handle;
	if (!check_head_option_max(handle->m_write_head_option, length)) {
		return luaL_error(L, "attempt to send data(length %lu) too long(max %lu)", length, handle->m_write_head_option.max);
	}
	bool safety = false;
	bool nonblocking = false;
	if (lua_gettop(L) >= 3) {
		if (lua_isfunction(L, 3)) {
			nonblocking = true;
			lua_settop(L, 3);
		} else {
			safety = lua_toboolean(L, 3);
		}
	}
	/* error already occurred in network thread, stop write immediately. */
	uv_err_code err = UV_OK;
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	if (length == 0 || (err = handle->get_write_error()) != UV_OK) { /* empty string or buffer or error already occurred */
		if (nonblocking) {
			int32_t session = context_lua_t::lua_ref_callback(L, 2, LUA_REFNIL, context_lua_t::common_callback_adjust);
			singleton_ref(node_lua_t).context_send(lctx, lctx->get_handle(), session, RESPONSE_TCP_WRITE, (length == 0 ? UV_OK : err));
		} else if (safety) { /* real blocking mode */
			lua_pushboolean(L, length == 0);
			lua_pushinteger(L, err);
			return 2;
		}
		return 0;
	}
	request_t& request = lctx->get_yielding_request();
	request.m_type = REQUEST_TCP_WRITE;
	request.m_length = REQUEST_SIZE(request_tcp_write_t, 0);
	request.m_tcp_write.m_socket_handle = (uv_tcp_socket_handle_t*)socket->m_uv_handle;
	if (s) {
		request.m_tcp_write.m_length = length; /* length > 0 */
		request.m_tcp_write.m_string = (const char*)nl_memdup(s, length);
		if (!request.m_tcp_write.m_string) {
			return luaL_error(L, "attempt to send data(length %lu) failed: memory not enough", length);
		}
	} else {
		request.m_tcp_write.m_length = 0;
		request.m_tcp_write.m_buffer = buffer_grab(*buffer);
	}
	if (nonblocking) { /*nonblocking callback mode*/
		request.m_tcp_write.m_session = context_lua_t::lua_ref_callback(L, 2, LUA_REFNIL, context_lua_t::common_callback_adjust);
		froze_head_option(handle->m_write_head_option);
		singleton_ref(network_t).send_request(request);
		return 0;
	} else { /* blocking mode */
		if (!safety) { /* wait for nothing */
			request.m_tcp_write.m_session = LUA_REFNIL;
			froze_head_option(handle->m_write_head_option);
			singleton_ref(network_t).send_request(request);
			return 0;
		} else { /* real blocking mode */
			return lctx->lua_yield_send(L, 0, write_yield_finalize, socket, context_lua_t::common_yield_continue);
		}
	}
}

int32_t lua_tcp_socket_handle_t::write_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination)
{
	if (root_coro != NULL) { //yield succeeded
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		request_t& request = lctx->get_yielding_request();
		request.m_tcp_write.m_session = context_lua_t::lua_ref_yield_coroutine(root_coro);
		uv_tcp_socket_handle_t* handle = (uv_tcp_socket_handle_t*)((lua_tcp_socket_handle_t*)userdata)->m_uv_handle;
		froze_head_option(handle->m_write_head_option);
		singleton_ref(network_t).send_request(request);
	} else {  //yield failed, clear your data in case of memory leak.
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		request_t& request = lctx->get_yielding_request();
		if (request.m_tcp_write.m_length > 0) {
			nl_free((void*)request.m_tcp_write.m_string);
			request.m_tcp_write.m_string = NULL;
			request.m_tcp_write.m_length = 0;
		} else {
			buffer_release(request.m_tcp_write.m_buffer);
		}
	}
	return UV_OK;
}

int32_t lua_tcp_socket_handle_t::read(lua_State* L)
{
	lua_tcp_socket_handle_t* socket = (lua_tcp_socket_handle_t*)luaL_checkudata(L, 1, TCP_SOCKET_METATABLE);
	if (socket->is_closed()) {
		return luaL_error(L, "attempt to read data on a invalid or closed socket");
	}
	uv_tcp_socket_handle_t* handle = (uv_tcp_socket_handle_t*)socket->m_uv_handle;
	uint64_t timeout = 0;
	bool nonblocking = false;
	if (lua_gettop(L) >= 2) {
		if (lua_isfunction(L, 2)) {
			nonblocking = true;
			lua_settop(L, 2);
		} else {
			timeout = 1000 * luaL_checknumber(L, 2);
		}
	}
	/* error already occurred in network thread, stop read immediately. */
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	uv_err_code err = handle->get_read_error();
	if (err != UV_OK) {
		if (nonblocking) {
			socket->m_read_ref_sessions.make_nonblocking_callback(L, 1, context_lua_t::common_callback_adjust);
			singleton_ref(node_lua_t).context_send(lctx, lctx->get_handle(), socket->m_lua_ref, RESPONSE_TCP_READ, err); /* The message will wake up all blocking coroutine and nonblocking callback. */
			return 0;
		} else {
			lua_pushboolean(L, 0);
			lua_pushinteger(L, err);
			return 2;
		}
	}
	request_t& request = lctx->get_yielding_request();
	request.m_type = REQUEST_TCP_READ;
	request.m_length = REQUEST_SIZE(request_tcp_read_t, 0);
	request.m_tcp_read.m_socket_handle = handle;
	request.m_tcp_read.m_blocking = !nonblocking;
	if (nonblocking) { /* nonblocking callback */
		if (socket->m_read_ref_sessions.make_nonblocking_callback(L, 1, context_lua_t::common_callback_adjust)) {
			froze_head_option(handle->m_read_head_option);
			if (socket->m_read_overflow_buffers.empty()) {
				singleton_ref(network_t).send_request(request);
			} else {
				singleton_ref(node_lua_t).context_send_buffer_release(lctx, lctx->get_handle(), socket->m_lua_ref, RESPONSE_TCP_READ, socket->m_read_overflow_buffers.front());
				socket->m_read_overflow_buffers.pop();
			}
		}
		return 0;
	} else { /* blocking */
		return lctx->lua_yield_send(L, 0, read_yield_finalize, socket, context_lua_t::common_yield_continue, timeout);
	}
}

int32_t lua_tcp_socket_handle_t::read_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination)
{
	if (root_coro != NULL) {
		lua_tcp_socket_handle_t* socket = (lua_tcp_socket_handle_t*)userdata;
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		int32_t session = socket->m_read_ref_sessions.push_blocking(root_coro);
		uv_tcp_socket_handle_t* handle = (uv_tcp_socket_handle_t*)socket->m_uv_handle;
		froze_head_option(handle->m_read_head_option);
		if (socket->m_read_overflow_buffers.empty()) {
			singleton_ref(network_t).send_request(lctx->get_yielding_request());
			int64_t timeout = lctx->get_yielding_timeout();
			if (timeout > 0) {
				context_lua_t::lua_ref_timer(main_coro, session, timeout, 0, false, socket, read_timeout);
			}
		} else {
			singleton_ref(node_lua_t).context_send_buffer_release(lctx, lctx->get_handle(), socket->m_lua_ref, RESPONSE_TCP_READ, socket->m_read_overflow_buffers.front());
			socket->m_read_overflow_buffers.pop();
		}
	}
	return UV_OK;
}

int32_t lua_tcp_socket_handle_t::read_timeout(lua_State *L, int32_t session, void* userdata, bool is_repeat)
{
	lua_tcp_socket_handle_t* socket = (lua_tcp_socket_handle_t*)userdata;
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	message_t message(0, session, RESPONSE_TCP_READ, NL_ETIMEOUT);
	return socket->m_read_ref_sessions.wakeup_one_fixed(lctx, message, false);
}

static void opt_check_endian(lua_State* L, int32_t idx, const char* field, char *value, bool frozen)
{
	lua_getfield(L, idx, field);
	if (!lua_isnil(L, -1)) {
		size_t len = 0;
		const char* head_endian = luaL_tolstring(L, -1, &len);
		if (len != 1 || (*head_endian != 'L' && *head_endian != 'B')) {
			luaL_error(L, "option table error, field '%s' not correct", field);
		}
		if (frozen) {
			luaL_error(L, "field '%s' set failed, option table already frozen", field);
		}
		*value = *head_endian;
	}
	lua_pop(L, 1);
}

static void opt_check_bytes(lua_State* L, int32_t idx, const char* field, uint8_t* value, bool frozen)
{
	lua_getfield(L, idx, field);
	if (!lua_isnil(L, -1)) {
		int32_t isnum = 0;
		uint32_t bytes = lua_tounsignedx(L, -1, &isnum);
		if (!isnum || (bytes != 0 && bytes != 1 && bytes != 2 && bytes != 4)) { /* head byte width: 0, 1, 2, or 4. */
			luaL_error(L, "option table error, field '%s' not correct", field);
		}
		if (frozen) {
			luaL_error(L, "field '%s' set failed, option table already frozen", field);
		}
		*value = bytes;
	}
	lua_pop(L, 1);
}

static void opt_check_max(lua_State* L, int32_t idx, const char* field, uint32_t* value, bool frozen)
{
	lua_getfield(L, idx, field);
	if (!lua_isnil(L, -1)) {
		int32_t isnum = 0;
		uint32_t max = lua_tounsignedx(L, -1, &isnum);
		if (!isnum) {
			luaL_error(L, "option table error, field '%s' not correct", field);
		}
		if (frozen) {
			luaL_error(L, "field '%s' set failed, option table already frozen", field);
		}
		*value = max;
	}
	lua_pop(L, 1);
}

static void opt_check_bytes_max(lua_State* L, const char* bytes_field, uint8_t* bytes, const char* max_field, uint32_t* max)
{
	uint32_t bytes_max = ((uint64_t)1 << (*bytes << 3)) - 1;
	if (bytes_max > MAX_BUFFER_CAPACITY) {
		bytes_max = MAX_BUFFER_CAPACITY;
	}
	if (*max > bytes_max) {
		luaL_error(L, "option table error, field '%s' and '%s' not match", bytes_field, max_field);
	}
	if (*bytes > 0 && *max == 0) {
		*max = bytes_max;
	}
}

int32_t lua_tcp_socket_handle_t::set_rwopt(lua_State* L)
{
	lua_tcp_socket_handle_t* socket = (lua_tcp_socket_handle_t*)luaL_checkudata(L, 1, TCP_SOCKET_METATABLE);
	if (socket->is_closed()) {
		return luaL_error(L, "attempt to perform on a invalid or closed socket");
	}
	luaL_checktype(L, 2, LUA_TTABLE);
	uv_tcp_socket_handle_t* handle = (uv_tcp_socket_handle_t*)socket->m_uv_handle;
	head_option_t read_option = handle->m_read_head_option;
	opt_check_endian(L, 2, "read_head_endian", &read_option.endian, read_option.frozen);
	opt_check_bytes(L, 2, "read_head_bytes", &read_option.bytes, read_option.frozen);
	opt_check_max(L, 2, "read_head_max", &read_option.max, read_option.frozen);
	opt_check_bytes_max(L, "read_head_bytes", &read_option.bytes, "read_head_max", &read_option.max);
	
	head_option_t write_option = handle->m_write_head_option;
	opt_check_endian(L, 2, "write_head_endian", &write_option.endian, write_option.frozen);
	opt_check_bytes(L, 2, "write_head_bytes", &write_option.bytes, write_option.frozen);
	opt_check_max(L, 2, "write_head_max", &write_option.max, write_option.frozen);
	opt_check_bytes_max(L, "write_head_bytes", &write_option.bytes, "write_head_max", &write_option.max);
	
	if (!read_option.frozen) {
		handle->m_read_head_option.endian = read_option.endian;
		handle->m_read_head_option.bytes = read_option.bytes;
		handle->m_read_head_option.max = read_option.max;
	}

	if (!write_option.frozen) {
		handle->m_write_head_option.endian = write_option.endian;
		handle->m_write_head_option.bytes = write_option.bytes;
		handle->m_write_head_option.max = write_option.max;
	}

	return 0;
}

int32_t lua_tcp_socket_handle_t::get_rwopt(lua_State* L)
{
	lua_tcp_socket_handle_t* socket = (lua_tcp_socket_handle_t*)luaL_checkudata(L, 1, TCP_SOCKET_METATABLE);
	lua_createtable(L, 0, 8);
	if (!socket->is_closed()) {
		uv_tcp_socket_handle_t* handle = (uv_tcp_socket_handle_t*)socket->m_uv_handle;
		lua_pushlstring(L, &handle->m_read_head_option.endian, 1);
		lua_setfield(L, -2, "read_head_endian");
		lua_pushinteger(L, handle->m_read_head_option.bytes);
		lua_setfield(L, -2, "read_head_bytes");
		lua_pushinteger(L, handle->m_read_head_option.max);
		lua_setfield(L, -2, "read_head_max");

		lua_pushlstring(L, &handle->m_write_head_option.endian, 1);
		lua_setfield(L, -2, "write_head_endian");
		lua_pushinteger(L, handle->m_write_head_option.bytes);
		lua_setfield(L, -2, "write_head_bytes");
		lua_pushinteger(L, handle->m_write_head_option.max);
		lua_setfield(L, -2, "write_head_max");
	}
	return 1;
}

int32_t lua_tcp_socket_handle_t::set_nodelay(lua_State* L)
{
	lua_tcp_socket_handle_t* socket = (lua_tcp_socket_handle_t*)luaL_testudata(L, 1, TCP_SOCKET_METATABLE);
	if (!socket->is_closed() && socket->get_uv_handle_type() == UV_TCP) {
		socket->set_option(OPT_TCP_NODELAY, lua_toboolean(L, 2) ? "\x01\0" : "\0\0", 2);
	}
	return 0;
}

int32_t lua_tcp_socket_handle_t::wakeup_connect(lua_State* L, message_t& message)
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

int32_t lua_tcp_socket_handle_t::wakeup_read(lua_State* L, message_t& message)
{
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	lua_tcp_socket_handle_t* socket = (lua_tcp_socket_handle_t*)get_lua_handle(L, message.m_session, SOCKET_SET);
	if (socket) {
		if (!socket->is_closed()) {
			if (message_is_buffer(message)) {
				if (socket->m_read_ref_sessions.wakeup_once(lctx, message)) {
					return 1;
				}
				socket->m_read_overflow_buffers.push(buffer_grab(message_buffer(message)));
			}
			if (message_is_error(message)) {
				socket->m_read_ref_sessions.wakeup_all(lctx, message);
				return 1;
			}
			return 0;
		} else { /* listen socket is already closed */
			message_t error = message_t(message.m_source, message.m_session, RESPONSE_TCP_CLOSING, NL_ETCPSCLOSED);
			socket->m_read_ref_sessions.free_nonblocking_callback(L);
			socket->m_read_ref_sessions.wakeup_all_blocking(lctx, error);
			return 1;
		}
	}
	return 0;
}

int32_t lua_tcp_socket_handle_t::close(lua_State* L)
{
	lua_tcp_socket_handle_t* socket = (lua_tcp_socket_handle_t*)luaL_checkudata(L, 1, TCP_SOCKET_METATABLE);
	if (!socket->is_closed()) {
		context_lua_t* lctx = context_lua_t::lua_get_context(L);
		/* The message will wake up all blocking coroutine only*/
		singleton_ref(node_lua_t).context_send(lctx, lctx->get_handle(), socket->m_lua_ref, RESPONSE_TCP_CLOSING, NL_ETCPSCLOSED);
		socket->_close();
		socket->release_read_overflow_buffers();
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

void lua_tcp_socket_handle_t::release_read_overflow_buffers()
{
	for (int32_t i = 0; i < m_read_overflow_buffers.size(); ++i) {
		buffer_release(m_read_overflow_buffers.front());
		m_read_overflow_buffers.pop();
	}
}

int luaopen_tcp(lua_State *L)
{
	luaL_Reg l[] = {
		{ "listen", lua_tcp_listen_handle_t::listen },
		{ "listen6", lua_tcp_listen_handle_t::listen6 },
		{ "listens", lua_tcp_listen_handle_t::listens },
		{ "accept", lua_tcp_listen_handle_t::accept },
		{ "connect", lua_tcp_socket_handle_t::connect },
		{ "connect6", lua_tcp_socket_handle_t::connect6 },
		{ "connects", lua_tcp_socket_handle_t::connects },
		{ "write", lua_tcp_socket_handle_t::write },
		{ "read", lua_tcp_socket_handle_t::read },
		{ "set_rwopt", lua_tcp_socket_handle_t::set_rwopt },
		{ "get_rwopt", lua_tcp_socket_handle_t::get_rwopt },
		{ "set_nodelay", lua_tcp_socket_handle_t::set_nodelay },
		{ "close", lua_tcp_close },
		{ "is_closed", lua_tcp_is_closed },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}

