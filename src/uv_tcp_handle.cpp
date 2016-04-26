#include "message.h"
#include "network.h"
#include "node_lua.h"
#include "uv_tcp_handle.h"
#include "lua_tcp_handle.h"

#if defined (CC_MSVC)
#define uv_tcp_fd(handle) ((handle)->socket)
#elif defined(__APPLE__)
extern "C" int uv___stream_fd(uv_stream_t* handle);
#define uv_tcp_fd(handle) (uv___stream_fd((uv_stream_t*) (handle)))
#else
#define uv_tcp_fd(handle) ((handle)->io_watcher.fd)
#endif /* defined(__APPLE__) */


void uv_tcp_listen_handle_t::on_accept(uv_stream_t* server, int status)
{
	uv_tcp_listen_handle_t *listen_handle = (uv_tcp_listen_handle_t*)(server->data);
	if (status == 0) {
		listen_handle->m_has_accept_event = true;
		listen_handle->try_accept();
	} else if (!uv_is_closing((uv_handle_t*)server)) { /* may be we have already called uv_close. */
		/* Here we depend on the lua_service socket object destructor to close uv_tcp_listen_handle_t object. */
		singleton_ref(node_lua_t).context_send(listen_handle->m_source, 0, listen_handle->m_lua_ref, RESPONSE_TCP_ACCEPT, singleton_ref(network_t).last_error());
	}
}

void uv_tcp_listen_handle_t::listen_tcp(request_tcp_listen_t& request)
{
	uv_tcp_t* server = (uv_tcp_t*)m_handle;
#ifdef CC_MSVC
	uv_tcp_simultaneous_accepts(server, 0);
#endif
	if ((!request.m_ipv6 ? uv_tcp_bind(server, uv_ip4_addr(REQUEST_SPARE_PTR(request), request.m_port)) == 0 : uv_tcp_bind6(server, uv_ip6_addr(REQUEST_SPARE_PTR(request), request.m_port)) == 0) && uv_listen((uv_stream_t*)server, request.m_backlog, on_accept) == 0) {
		if (!singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_TCP_LISTEN, (void*)this)) {
			uv_close((uv_handle_t*)server, on_closed);
		}
	} else {
		singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_TCP_LISTEN, singleton_ref(network_t).last_error());
		uv_close((uv_handle_t*)server, on_closed);
	}
}

void uv_tcp_listen_handle_t::listen_sock(request_tcp_listens_t& request)
{
	uv_pipe_t* server = (uv_pipe_t*)m_handle;
	char* sock = REQUEST_SPARE_PTR(request);
#ifndef CC_MSVC
	{
		uv_fs_t req;
		uv_fs_unlink(server->loop, &req, sock, NULL);
		uv_fs_req_cleanup(&req);
	}
#endif
	if (uv_pipe_bind(server, sock) == 0 && uv_listen((uv_stream_t*)server, -1, on_accept) == 0) {
		if (!singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_TCP_LISTEN, (void*)this)) {
			uv_close((uv_handle_t*)server, on_closed);
		}
	} else {
		singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_TCP_LISTEN, singleton_ref(network_t).last_error());
		uv_close((uv_handle_t*)server, on_closed);
	}
}

void uv_tcp_listen_handle_t::try_accept()
{
	if ((m_has_noblocking_accept || m_blocking_accept_count > 0) && (m_lua_ref != LUA_REFNIL && m_has_accept_event)) {
		m_has_accept_event = false;
		uv_tcp_socket_handle_t *client_handle = new uv_tcp_socket_handle_t(m_handle->loop, m_source, (m_handle->type == UV_NAMED_PIPE));
		uv_handle_t *client = (uv_handle_t*)client_handle->m_handle;
		if (uv_accept((uv_stream_t*)(m_handle), (uv_stream_t*)client) == 0) {
			if (m_handle->type == UV_TCP) {
				client_handle->m_tcp_sock = uv_tcp_fd((uv_tcp_t*)client);
			}
			if (!singleton_ref(node_lua_t).context_send(m_source, 0, m_lua_ref, RESPONSE_TCP_ACCEPT, (void*)client_handle)) {
				uv_close((uv_handle_t*)client, on_closed);
			} else if (m_blocking_accept_count > 0) {
				--m_blocking_accept_count;
			}
		} else {
			uv_close((uv_handle_t*)client, on_closed);
		}
	}
}

void uv_tcp_listen_handle_t::accept(request_tcp_accept_t& request)
{
	if (request.m_blocking) { //blocking
		++m_blocking_accept_count;
	} else {				  //non-blocking has started
		m_has_noblocking_accept = true;
	}
	try_accept();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define LARGE_UNCOMPLETE_LIMIT	(1 * 1024)

typedef struct {
	uv_getaddrinfo_t m_request;
	uv_tcp_socket_handle_t *m_client;
	uint32_t m_session;
} uv_addr_resolver_t;

uv_tcp_socket_handle_t::~uv_tcp_socket_handle_t()
{
	clear_read_cached_buffers();
	clear_write_cached_requests();
	singleton_ref(network_t).pop_shared_write_socket(SOCKET_MAKE_FD(m_lua_ref, m_source));
}

void uv_tcp_socket_handle_t::on_connect(uv_connect_t* req, int status)
{
	uv_handle_t *client = (uv_handle_t*)req->handle;
	uv_tcp_socket_handle_t *client_handle = (uv_tcp_socket_handle_t*)client->data;
	uint32_t session = (uint64_t)req->data;
	if (status == 0) { //connect success
		if (client->type == UV_TCP) {
			client_handle->m_tcp_sock = uv_tcp_fd((uv_tcp_t*)client);
		}
		if (!singleton_ref(node_lua_t).context_send(client_handle->m_source, 0, session, RESPONSE_TCP_CONNECT, (void*)client_handle)) {
			uv_close((uv_handle_t*)client, on_closed);
		}
	} else if (!uv_is_closing((uv_handle_t*)client)) { /* may be we have already called uv_close */
		singleton_ref(node_lua_t).context_send(client_handle->m_source, 0, session, RESPONSE_TCP_CONNECT, singleton_ref(network_t).last_error());
		uv_close((uv_handle_t*)client, on_closed);
	}
	nl_free(req);
}

void uv_tcp_socket_handle_t::on_resolve(uv_getaddrinfo_t* req, int status, struct addrinfo* res)
{
	uv_addr_resolver_t* resolver = (uv_addr_resolver_t*)(req->data);
	uv_tcp_socket_handle_t *client_handle = (uv_tcp_socket_handle_t*)resolver->m_client;
	if (status == 0) {
		struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
		uv_tcp_t* client = (uv_tcp_t*)client_handle->m_handle;
		uv_connect_t* req = (uv_connect_t*)nl_malloc(sizeof(*req));
		req->data = (void*)resolver->m_session;
		if ((addr->sin_family == AF_INET) ? uv_tcp_connect(req, client, *(struct sockaddr_in*)addr, on_connect) != 0 : uv_tcp_connect6(req, client, *(struct sockaddr_in6*)addr, on_connect) != 0) {
			singleton_ref(node_lua_t).context_send(client_handle->m_source, 0, resolver->m_session, RESPONSE_TCP_CONNECT, singleton_ref(network_t).last_error());
			uv_close((uv_handle_t*)client, on_closed);
			nl_free(req);
		}
		nl_free(resolver);
		uv_freeaddrinfo(res);
	} else {
		singleton_ref(node_lua_t).context_send(client_handle->m_source, 0, resolver->m_session, RESPONSE_TCP_CONNECT, singleton_ref(network_t).last_error());
		uv_close((uv_handle_t*)client_handle, on_closed);
		nl_free(resolver);
	}
}

void uv_tcp_socket_handle_t::connect_tcp(request_tcp_connect_t& request)
{
	uv_tcp_t *client = (uv_tcp_t*)m_handle;
	const char *remote_host = REQUEST_SPARE_PTR(request);
	const char *local_host = remote_host + strlen(remote_host) + 1;
	if (strlen(local_host) > 0 || request.m_local_port != 0) {
		if (!request.m_local_ipv6 ? uv_tcp_bind(client, uv_ip4_addr(local_host, request.m_local_port)) != 0 : uv_tcp_bind6(client, uv_ip6_addr(local_host, request.m_local_port)) != 0) {
			singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_TCP_CONNECT, singleton_ref(network_t).last_error());
			uv_close((uv_handle_t*)client, on_closed);
			return;
		}
	}
	union {
		sockaddr_in addr4;
		sockaddr_in6 addr6;
	} sock_addr;
	uv_err_code err = host_sockaddr(request.m_remote_ipv6, remote_host, request.m_remote_port, (struct sockaddr*)&sock_addr);
	if (err == UV_OK) {
		uv_connect_t* req = (uv_connect_t*)nl_malloc(sizeof(*req));
		req->data = (void*)request.m_session;
		if (!request.m_remote_ipv6 ? uv_tcp_connect(req, client, sock_addr.addr4, on_connect) != 0 : uv_tcp_connect6(req, client, sock_addr.addr6, on_connect) != 0) {
			singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_TCP_CONNECT, singleton_ref(network_t).last_error());
			uv_close((uv_handle_t*)client, on_closed);
			nl_free(req);
		}
	} else if (err == UV_EINVAL) { //try resolve host name
		char port[8];
		struct addrinfo hints;
		hints.ai_family = !request.m_remote_ipv6 ? AF_INET : AF_INET6;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = 0;
		uv_addr_resolver_t* resolver = (uv_addr_resolver_t*)nl_malloc(sizeof(uv_addr_resolver_t));
		itoa(request.m_remote_port, port, 10);
		resolver->m_client = this;
		resolver->m_session = request.m_session;
		resolver->m_request.data = resolver;
		if (uv_getaddrinfo(client->loop, &resolver->m_request, on_resolve, remote_host, port, &hints) != 0) {
			singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_TCP_CONNECT, singleton_ref(network_t).last_error());
			uv_close((uv_handle_t*)client, on_closed);
			nl_free(resolver);
		}
	} else {
		singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_TCP_CONNECT, err);
		uv_close((uv_handle_t*)client, on_closed);
	}
}

void uv_tcp_socket_handle_t::connect_sock(request_tcp_connects_t& request)
{
	uv_pipe_t *client = (uv_pipe_t*)m_handle;
	const char *sock_name = REQUEST_SPARE_PTR(request);
	uv_connect_t* req = (uv_connect_t*)nl_malloc(sizeof(*req));
	req->data = (void*)request.m_session;
	uv_pipe_connect(req, client, sock_name, on_connect);
}

void uv_tcp_socket_handle_t::on_write(uv_write_t* req, int status)
{
	write_uv_request_t *uv_request = (write_uv_request_t*)req->data;
	uv_tcp_socket_handle_t *socket_handle = (uv_tcp_socket_handle_t*)(req->handle->data);
	if (status != 0) {
		socket_handle->m_write_error = singleton_ref(network_t).last_error();
	}
	if (uv_request->m_length > 0) {
		nl_free((void*)uv_request->m_string);
	} else {
		buffer_release(uv_request->m_buffer);
	}
	if (uv_request->m_session != LUA_REFNIL) {
		singleton_ref(node_lua_t).context_send(uv_request->m_source, 0, uv_request->m_session, RESPONSE_TCP_WRITE, status == 0 ? UV_OK : socket_handle->m_write_error);
	}
	socket_handle->put_write_cached_request(uv_request);
}

void uv_tcp_socket_handle_t::write(request_tcp_write_t& request)
{
	int32_t err = UV_UNKNOWN;
	if (request.m_shared_write) {
		uv_tcp_socket_handle_t* handle = (uv_tcp_socket_handle_t*)singleton_ref(network_t).get_shared_write_socket(request.m_socket_fd);
		if (handle != NULL) {
			uint32_t length = request.m_length > 0 ? request.m_length : (uint32_t)buffer_data_length(request.m_buffer);
			if (uv_is_closing((uv_handle_t*)handle)) {
				err = NL_ETCPSCLOSED;
			} else if (!check_head_option_max(handle->m_write_head_option, length)) {
				err = NL_ETCPWRITELONG;
			} else {
				err = handle->write_handle(request);
			}
		} else {
			err = NL_ETCPNOWSHARED;
		}
	} else {
		err = request.m_socket_handle->write_handle(request);
	}
	if (err != UV_OK) { /* write error had been occurred */
		if (request.m_length > 0) {
			nl_free((void*)request.m_string);
		} else {
			buffer_release(request.m_buffer);
		}
		if (request.m_session != LUA_REFNIL) {
			singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_TCP_WRITE, (nl_err_code)err);
		}
	}
}

/* We must response something to lua-service even error occurred. */
int32_t uv_tcp_socket_handle_t::write_handle(request_tcp_write_t& request)
{
	int result;
	if (m_write_error == UV_OK) {
		uv_buf_t uv_buf[] = { { 0, NULL }, { 0, NULL } };
		write_uv_request_t* uv_request = get_write_cached_request();
		uv_request->m_source = request.m_source;
		uv_request->m_session = request.m_session;
		uv_request->m_length = request.m_length;
		if (request.m_length > 0) {
			uv_request->m_string = request.m_string;
			uv_buf[1].len = request.m_length;
			uv_buf[1].base = (char*)request.m_string;
		} else {
			uv_request->m_buffer = request.m_buffer;
			uv_buf[1].len = buffer_data_length(request.m_buffer);
			uv_buf[1].base = buffer_data_ptr(request.m_buffer);
		}
		if (m_write_head_option.bytes > 0) { /* uv_buf[1].len must be checked with m_write_head_option.max in tcp_socket write method. */
			endian_from_integer(uv_buf[1].len, (char*)uv_request->m_head_buffer, m_write_head_option.bytes, m_write_head_option.endian);
			uv_buf[0].len = m_write_head_option.bytes;
			uv_buf[0].base = (char*)uv_request->m_head_buffer;
			result = uv_write(&uv_request->m_write_req, (uv_stream_t*)(m_handle), uv_buf, 2, on_write);
		} else {
			result = uv_write(&uv_request->m_write_req, (uv_stream_t*)(m_handle), uv_buf + 1, 1, on_write);
		}
		if (result == 0) return UV_OK;
		m_write_error = singleton_ref(network_t).last_error(); /* write error occurs */
		put_write_cached_request(uv_request);
	}
	return m_write_error;
}

uv_buf_t uv_tcp_socket_handle_t::on_read_alloc(uv_handle_t* handle, size_t suggested_size)
{
	uv_tcp_socket_handle_t *socket_handle = (uv_tcp_socket_handle_t*)(handle->data);
	if (buffer_write_size(socket_handle->m_read_buffer) < LARGE_UNCOMPLETE_LIMIT) {
		return singleton_ref(network_t).make_shared_read_buffer();
	}
	/* read in m_shared_read_buffer but not complete large pack(optimization for memcpy) */
	uv_buf_t buf;
	buf.base = buffer_write_ptr(socket_handle->m_read_buffer);
	buf.len = buffer_write_size(socket_handle->m_read_buffer);
	return buf;
}

void uv_tcp_socket_handle_t::write_read_buffer(ssize_t nread, uv_buf_t buf)
{
	if (nread == 0) {
		return;
	}
	if (nread == -1) {
		write_read_buffer_finish(singleton_ref(network_t).last_error());
		return;
	}
	char* buffer = buf.base;
	if (buffer != singleton_ref(network_t).get_shared_read_buffer().base) {
		buffer_adjust_len(m_read_buffer, nread);
		if (buffer_write_size(m_read_buffer) == 0) {
			write_read_buffer_finish(UV_OK);
		}
		return;
	}
	while (nread > 0) {
		int8_t unread_head_bytes = m_read_head_option.bytes - m_read_head_bytes;
		if (unread_head_bytes > 0) {
			if (nread >= unread_head_bytes) {
				memcpy((char*)(m_read_head_buffer + m_read_head_bytes), buffer, unread_head_bytes);
				m_read_head_bytes = m_read_head_option.bytes;
				m_read_bytes = endian_to_integer((char*)m_read_head_buffer, m_read_head_option.bytes, m_read_head_option.endian);
				if (m_read_bytes <= m_read_head_option.max) {
					m_read_buffer = buffer_new(m_read_bytes, NULL, 0);
					if (!buffer_is_valid(m_read_buffer)) {
						write_read_buffer_finish(UV_ENOMEM);
						return;
					}
				} else {
					write_read_buffer_finish(UV_EMSGSIZE);
					return;
				}
				buffer += unread_head_bytes;
				nread  -= unread_head_bytes;
			} else {
				memcpy((char*)(m_read_head_buffer + m_read_head_bytes), buffer, nread);
				m_read_head_bytes += nread;
				return;
			}
		} else if (m_read_head_option.bytes == 0) {
			m_read_buffer = buffer_new(nread, buffer, nread);
			if (!buffer_is_valid(m_read_buffer)) {
				write_read_buffer_finish(UV_ENOMEM);
				return;
			}
			write_read_buffer_finish(UV_OK);
			return;
		}
		uint32_t unread_bytes = buffer_write_size(m_read_buffer);
		if (unread_bytes > 0 && nread > 0) {
			if (nread >= unread_bytes) {
				buffer_append(m_read_buffer, buffer, unread_bytes);
				buffer += unread_bytes;
				nread  -= unread_bytes;
			} else {
				buffer_append(m_read_buffer, buffer, nread);
				return;
			}
		}
		if (buffer_write_size(m_read_buffer) == 0) {
			write_read_buffer_finish(UV_OK);
		}
	}
}

void uv_tcp_socket_handle_t::clear_read_cached_buffers()
{
	buffer_release(m_read_buffer);
	int size = m_read_cached_buffers.size();
	for (int i = 0; i < size; ++i) {
		buffer_release(m_read_cached_buffers.front());
		m_read_cached_buffers.pop();
	}
	buffer_make_invalid(m_read_buffer);
}

void uv_tcp_socket_handle_t::write_read_buffer_finish(uv_err_code read_error)
{
	if (read_error == UV_OK) {
		if (read_start_state()) {
			singleton_ref(node_lua_t).context_send_buffer_release(m_source, 0, m_lua_ref, RESPONSE_TCP_READ, m_read_buffer);
			if (m_blocking_read_count > 0) {
				--m_blocking_read_count;
			}
			if (!read_start_state()) {
				uv_read_stop((uv_stream_t*)m_handle);
			}
		} else {
			m_read_cached_buffers.push(m_read_buffer);
		}
		buffer_make_invalid(m_read_buffer);
		if (m_read_head_option.bytes > 0) {
			m_read_head_bytes = 0;
			m_read_bytes = 0;
		}
	} else {
		if (m_read_error == UV_OK) {
			trigger_read_error(read_error);
			uv_read_stop((uv_stream_t*)m_handle);
		} else {
			clear_read_cached_buffers();
		}
	}
}

//when read error occurred, we need to send the cached buffer to lua_context in case of half closed peer socket.
//peer socket may only shut down write(half close) before totally close the socket. 
void uv_tcp_socket_handle_t::trigger_read_error(uv_err_code read_error)
{ 
	int size = m_read_cached_buffers.size();
	for (int i = 0; i < size; ++i) {
		singleton_ref(node_lua_t).context_send_buffer_release(m_source, 0, m_lua_ref, RESPONSE_TCP_READ, m_read_cached_buffers.front());
		m_read_cached_buffers.pop();
	}
	buffer_release(m_read_buffer);
	buffer_make_invalid(m_read_buffer);
	singleton_ref(node_lua_t).context_send(m_source, 0, m_lua_ref, RESPONSE_TCP_READ, read_error);
	atomic_barrier();
	m_read_error = read_error;
}

/* When RESPONSE_TCP_READ error occurs, read request must also stop in context_lua thread. */
void uv_tcp_socket_handle_t::on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf)
{
	((uv_tcp_socket_handle_t*)(stream->data))->write_read_buffer(nread, buf);
}

/* When RESPONSE_TCP_READ error occurs, all read request must also stop in context_lua thread. */
void uv_tcp_socket_handle_t::read(request_tcp_read_t& request)
{
	if (m_read_error != UV_OK) {
		return;
	}
	bool read_started = read_start_state();
	if (request.m_blocking) { //blocking
		++m_blocking_read_count;
	} else {				  //non-blocking has started
		m_has_noblocking_read = true;
	}
	int size = m_read_cached_buffers.size();
	for (int i = 0; i < size; ++i) {
		singleton_ref(node_lua_t).context_send_buffer_release(m_source, 0, m_lua_ref, RESPONSE_TCP_READ, m_read_cached_buffers.front());
		m_read_cached_buffers.pop();
		if (m_blocking_read_count > 0) {
			--m_blocking_read_count;
		}
		if (!read_start_state()) {
			return;
		}
	}
	if (!read_started && uv_read_start((uv_stream_t*)(m_handle), on_read_alloc, on_read) != 0) {
		if (!uv_is_closing((uv_handle_t*)(m_handle))) {
			trigger_read_error(singleton_ref(network_t).last_error());
		}
	}
}

void uv_tcp_socket_handle_t::set_option(uint8_t type, const char *option)
{
	tcp_option_type opt_type = (tcp_option_type)type;
	switch (opt_type)
	{
	case OPT_TCP_NODELAY:
		set_tcp_nodelay(*option);
		break;
	case OPT_TCP_WSHARED:
		set_tcp_wshared(*option);
		break;
	default:
		break;
	}
}

void uv_tcp_socket_handle_t::set_tcp_nodelay(bool enable)
{
	if (m_handle->type == UV_TCP && !uv_is_closing((uv_handle_t*)(m_handle))) {
		uv_tcp_nodelay((uv_tcp_t*)m_handle, enable);
	}
}

void uv_tcp_socket_handle_t::set_tcp_wshared(bool enable)
{
	if (!uv_is_closing((uv_handle_t*)(m_handle))) {
		int64_t fd = SOCKET_MAKE_FD(m_lua_ref, m_source);
		if (enable) {
			singleton_ref(network_t).put_shared_write_socket(fd, this);
		} else {
			singleton_ref(network_t).pop_shared_write_socket(fd);
		}
	}
}
