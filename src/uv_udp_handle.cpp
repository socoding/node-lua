#include "message.h"
#include "network.h"
#include "node_lua.h"
#include "uv_udp_handle.h"

#if defined (CC_MSVC)
#define uv_udp_fd(handle) ((handle)->socket)
#else
#define uv_udp_fd(handle) ((handle)->io_watcher.fd)
#endif

uv_udp_handle_t::~uv_udp_handle_t()
{
	clear_write_cached_requests();
	singleton_ref(network_t).pop_shared_write_socket(SOCKET_MAKE_FD(m_lua_ref, m_source));
}

void uv_udp_handle_t::open(request_udp_open_t& request)
{
	uv_udp_t* server = (uv_udp_t*)m_handle;
	if ((!request.m_ipv6 ? uv_udp_bind(server, uv_ip4_addr(REQUEST_SPARE_PTR(request), request.m_port), 0) == 0 : uv_udp_bind6(server, uv_ip6_addr(REQUEST_SPARE_PTR(request), request.m_port), 0) == 0)) {
		m_udp_sock = uv_udp_fd((uv_udp_t*)server);
		if (!singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_UDP_OPEN, (void*)this)) {
			uv_close((uv_handle_t*)server, on_closed);
		}
	} else {
		singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_UDP_OPEN, singleton_ref(network_t).last_error());
		uv_close((uv_handle_t*)server, on_closed);
	}
}

uv_buf_t uv_udp_handle_t::on_read_alloc(uv_handle_t* handle, size_t suggested_size)
{
	return singleton_ref(network_t).make_shared_read_buffer();
}

void uv_udp_handle_t::on_read(uv_udp_t* handle, ssize_t nread, uv_buf_t buf, struct sockaddr* addr, unsigned flags)
{
	/* udp max datagram read pack size: SHARED_READ_BUFFER_SIZE(64 * 1024) */
	if (nread == 0) {
		return;
	}
	uv_udp_handle_t* udp_handle = (uv_udp_handle_t*)(handle->data);
	if (nread == -1) {
		singleton_ref(node_lua_t).context_send(udp_handle->m_source, 0, udp_handle->m_lua_ref, RESPONSE_UDP_READ, singleton_ref(network_t).last_error());
		return;
	}
	buffer_t buffer = buffer_new(nread, buf.base, nread);
	char* host = (char*)nl_malloc(64);
	uint16_t port = 0;
	bool ipv6 = false;
	*host = '\0';
	sockaddr_host(addr, host, 64, &ipv6, &port);
	message_array_t* array = message_array_create(4);
	array->m_array[0] = message_t(0, udp_handle->m_lua_ref, RESPONSE_UDP_READ, buffer);
	array->m_array[1] = message_t(0, udp_handle->m_lua_ref, RESPONSE_UDP_READ, host);
	array->m_array[2] = message_t(0, udp_handle->m_lua_ref, RESPONSE_UDP_READ, (int64_t)port);
	array->m_array[3] = message_t(0, udp_handle->m_lua_ref, RESPONSE_UDP_READ, ipv6);
	singleton_ref(node_lua_t).context_send_array_release(udp_handle->m_source, 0, udp_handle->m_lua_ref, RESPONSE_UDP_READ, array);
}

void uv_udp_handle_t::read(request_udp_read_t& request)
{
	if (!m_read_started) {
		if (uv_udp_recv_start((uv_udp_t*)m_handle, on_read_alloc, on_read) != 0) {
			singleton_ref(node_lua_t).context_send(m_source, 0, m_lua_ref, RESPONSE_UDP_READ, singleton_ref(network_t).last_error());
			return;
		}
		m_read_started = true;
	}
}

void uv_udp_handle_t::on_write(uv_udp_send_t* req, int status)
{
	write_uv_request_t *uv_request = (write_uv_request_t*)req->data;
	uv_udp_handle_t *socket_handle = (uv_udp_handle_t*)(req->handle->data);
	if (uv_request->m_length > 0) {
		nl_free((void*)uv_request->m_string);
	} else {
		buffer_release(uv_request->m_buffer);
	}
	if (uv_request->m_session != (uint32_t)LUA_REFNIL) {
		singleton_ref(node_lua_t).context_send(uv_request->m_source, 0, uv_request->m_session, RESPONSE_UDP_WRITE, status == 0 ? UV_OK : singleton_ref(network_t).last_error());
	}
	socket_handle->put_write_cached_request(uv_request);
}

void uv_udp_handle_t::write(request_udp_write_t& request)
{
	int32_t err = UV_UNKNOWN;
	if (request.m_shared_write) {
		uv_udp_handle_t* handle = (uv_udp_handle_t*)singleton_ref(network_t).get_shared_write_socket(request.m_socket_fd);
		if (handle != NULL) {
			if (uv_is_closing((uv_handle_t*)handle)) {
				err = NL_EUDPSCLOSED;
			} else {
				err = handle->write_handle(request);
			}
		} else {
			err = NL_EUDPNOWSHARED;
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
		if (request.m_session != (uint32_t)LUA_REFNIL) {
			singleton_ref(node_lua_t).context_send(request.m_source, 0, request.m_session, RESPONSE_UDP_WRITE, (nl_err_code)err);
		}
	}
}

int32_t uv_udp_handle_t::write_handle(request_udp_write_t& request)
{
	int result;
	uv_buf_t uv_buf;
	write_uv_request_t* uv_request = get_write_cached_request();
	uv_request->m_source = request.m_source;
	uv_request->m_session = request.m_session;
	uv_request->m_length = request.m_length;
	if (request.m_length > 0) {
		uv_request->m_string = request.m_string;
		uv_buf.len = request.m_length;
		uv_buf.base = (char*)request.m_string;
	} else {
		uv_request->m_buffer = request.m_buffer;
		uv_buf.len = buffer_data_length(request.m_buffer);
		uv_buf.base = buffer_data_ptr(request.m_buffer);
	}
	if (!request.m_ipv6) {
		result = uv_udp_send(&uv_request->m_write_req, (uv_udp_t*)m_handle, &uv_buf, 1, uv_ip4_addr(REQUEST_SPARE_PTR(request), request.m_port), on_write);
	} else {
		result = uv_udp_send6(&uv_request->m_write_req, (uv_udp_t*)m_handle, &uv_buf, 1, uv_ip6_addr(REQUEST_SPARE_PTR(request), request.m_port), on_write);
	}
	if (result == 0) return UV_OK;
	put_write_cached_request(uv_request);
	return singleton_ref(network_t).last_error(); /* write error occurs */
}

void uv_udp_handle_t::set_option(uint8_t type, const char *option)
{
	udp_option_type opt_type = (udp_option_type)type;
	switch (opt_type)
	{
	case OPT_UDP_WSHARED:
		set_udp_wshared(*option);
		break;
	default:
		break;
	}
}

void uv_udp_handle_t::set_udp_wshared(bool enable)
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
