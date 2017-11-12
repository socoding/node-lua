#include <stdlib.h>
#include "utils.h"
#include "buffer.h"
#include "message.h"
#include "uv_tcp_handle.h"
#include "uv_udp_handle.h"
#include "uv_timer_handle.h"
#include "network.h"
#include "node_lua.h"
#include "request.h"
#ifndef CC_MSVC
#include <errno.h>
#include <unistd.h>
#endif

initialise_singleton(network_t);

network_t::network_t() : m_exiting(0)
{
	int rc;
	rc = make_socketpair(&m_request_r_fd, &m_request_w_fd);
	assert(rc == 0);
	rc = set_noneblocking(m_request_r_fd);
	assert(rc == 0);
	m_uv_loop = uv_loop_new();
	rc = uv_poll_init_socket(m_uv_loop, &m_request_handle, m_request_r_fd);
	assert(rc == 0);
	rc = uv_poll_start(&m_request_handle, UV_READABLE, on_request_polled_in);
	assert(rc == 0);
	//rc = uv_pipe_init(m_uv_loop, &m_remoter_handle, 1);
	//assert(rc == 0);
	memset(&m_shared_read_buffer, 0, sizeof(m_shared_read_buffer));
}

network_t::~network_t()
{
	uv_loop_delete(m_uv_loop);
	free_shared_read_buffer();
}

void network_t::start()
{
	uv_thread_create(&m_thread, thread_entry, (void*)this);
}

void network_t::thread_entry(void* arg)
{
	uv_run(((network_t*)arg)->m_uv_loop, UV_RUN_DEFAULT);
}

void network_t::stop()
{
	if (atomic_cas(m_exiting, 0, 1)) {
		request_t request;
		request.m_type = REQUEST_EXIT;
		request.m_length = REQUEST_SIZE(request_exit_t, 0);
		send_request(request);
	}
}

void network_t::wait()
{
	uv_thread_join(&m_thread);
}

void network_t::send_request(request_t& request)
{
	int32_t size = (sizeof(request.m_length) + sizeof(request.m_type) + request.m_length);
#ifdef CC_MSVC
	int32_t nbytes = ::send(m_request_w_fd, (char*)(&request.m_length), size, 0);
	ASSERT(nbytes == size);
#else
	while (true) {
		int32_t nbytes = ::send(m_request_w_fd, (char*)(&request.m_length), size, 0);
		if (unlikely(nbytes == -1 && errno == EINTR))
			continue;
		ASSERT(nbytes == size);
		return;
	}
#endif
}

void network_t::on_request_polled_in(uv_poll_t* handle, int status, int events)
{
	if (status != -1 && (events & UV_READABLE) && singleton_ref(network_t).recv_request()) {
		return;
	}
	if (!uv_is_closing((uv_handle_t*)handle)) {
		uv_close((uv_handle_t*)handle, on_request_handle_closed);
	}
}

void network_t::on_request_handle_closed(uv_handle_t* handle)
{
	close_socketpair(&singleton_ref(network_t).m_request_r_fd, &singleton_ref(network_t).m_request_w_fd);
	uv_stop(singleton_ref(network_t).m_uv_loop);
}

void network_t::request_exit(request_exit_t& request)
{
	if (!uv_is_closing((uv_handle_t*)&m_request_handle)) {
		uv_close((uv_handle_t*)&m_request_handle, on_request_handle_closed);
	}
}

void network_t::request_tcp_listen(request_tcp_listen_t& request)
{
	(new uv_tcp_listen_handle_t(m_uv_loop, request.m_source, false))->listen_tcp(request);
}

void network_t::request_tcp_listens(request_tcp_listens_t& request)
{
	(new uv_tcp_listen_handle_t(m_uv_loop, request.m_source, true))->listen_sock(request);
}

void network_t::request_tcp_accept(request_tcp_accept_t& request)
{
	request.m_listen_handle->accept(request);
}

void network_t::request_tcp_connect(request_tcp_connect_t& request)
{
	(new uv_tcp_socket_handle_t(m_uv_loop, request.m_source, false))->connect_tcp(request);
}

void network_t::request_tcp_connects(request_tcp_connects_t& request)
{
	(new uv_tcp_socket_handle_t(m_uv_loop, request.m_source, true))->connect_sock(request);
}

/* When RESPONSE_TCP_READ error occurs, all read request must also stop in context_lua thread. */
void network_t::request_tcp_read(request_tcp_read_t& request)
{
	request.m_socket_handle->read(request);
}

void network_t::request_tcp_write(request_tcp_write_t& request)
{
	uv_tcp_socket_handle_t::write(request);
}

void network_t::request_udp_open(request_udp_open_t& request)
{
	(new uv_udp_handle_t(m_uv_loop, request.m_source))->open(request);
}

void network_t::request_udp_write(request_udp_write_t& request)
{
	uv_udp_handle_t::write(request);
}

void network_t::request_udp_read(request_udp_read_t& request)
{
	request.m_socket_handle->read(request);
}

void network_t::request_handle_option(request_handle_option_t& request)
{
	request.m_handle->set_option(request.m_option_type, REQUEST_SPARE_PTR(request));
}

void network_t::request_handle_close(request_handle_close_t& request)
{
	request.m_handle_base->close();
}

void network_t::request_timer_start(request_timer_start_t& request)
{
	request.m_timer_handle->start(m_uv_loop, request);
}

#ifdef CC_MSVC
#define SOCKET_WOULD_BLOCK() (WSAGetLastError() == WSAEWOULDBLOCK)
#else
#define SOCKET_WOULD_BLOCK() (errno == EAGAIN)
#endif

bool network_t::recv_request()
{
	int32_t need_read = 0, readed = 0;
	int32_t processed = 0, nbytes = 0;
	request_t request;
	for (;;) {
		if (need_read == 0) {
			nbytes = ::recv(m_request_r_fd, (char*)(&request.m_length), sizeof(request.m_length), 0);
			if (nbytes > 0) {
				need_read = request.m_length + sizeof(request.m_type);
				readed = 0;
				continue;
			}
			return !(nbytes == 0 || !SOCKET_WOULD_BLOCK());
		} else {
			nbytes = ::recv(m_request_r_fd, (char*)(&request.m_type) + readed, need_read - readed, 0);
			if (nbytes > 0) {
				readed += nbytes;
				if (readed == need_read) {
					need_read = 0;
					process_request(request);
					if (++processed >= node_lua_t::m_cpu_count) {
						return true;
					}
				}
			} else if (nbytes == 0 || !SOCKET_WOULD_BLOCK()) {
				return false;
			}
		}
	}
}

void network_t::process_request(request_t& request)
{
	switch (request.m_type) {
	case REQUEST_EXIT:
		request_exit(request.m_exit);
		break;
	case REQUEST_TCP_LISTEN:
		request_tcp_listen(request.m_tcp_listen);
		break;
	case REQUEST_TCP_LISTENS:
		request_tcp_listens(request.m_tcp_listens);
		break;
	case REQUEST_TCP_ACCEPT:
		request_tcp_accept(request.m_tcp_accept);
		break;
	case REQUEST_TCP_CONNECT:
		request_tcp_connect(request.m_tcp_connect);
		break;
	case REQUEST_TCP_CONNECTS:
		request_tcp_connects(request.m_tcp_connects);
		break;
	case REQUEST_TCP_WRITE:
		request_tcp_write(request.m_tcp_write);
		break;
	case REQUEST_TCP_READ:
		request_tcp_read(request.m_tcp_read);
		break;
	case REQUEST_UDP_OPEN:
		request_udp_open(request.m_udp_open);
		break;
	case REQUEST_UDP_WRITE:
		request_udp_write(request.m_udp_write);
		break;
	case REQUEST_UDP_READ:
		request_udp_read(request.m_udp_read);
		break;
	case REQUEST_HANDLE_OPTION:
		request_handle_option(request.m_handle_option);
		break;
	case REQUEST_HANDLE_CLOSE:
		request_handle_close(request.m_handle_close);
		break;
	case REQUEST_TIMER_START:
		request_timer_start(request.m_timer_start);
		break;
	default:
		break;
	}
}

int network_t::make_socketpair( uv_os_sock_t *r_, uv_os_sock_t *w_ )
{
#if defined CC_MSVC

	//  Windows has no 'socketpair' function. CreatePipe is no good as pipe
	//  handles cannot be polled on. Here we create the socketpair by hand.
	*w_ = INVALID_SOCKET;
	*r_ = INVALID_SOCKET;

	uv_os_sock_t listener = INVALID_SOCKET;
	//  Create listening socket.
	listener = socket (AF_INET, SOCK_STREAM, 0);
	if (listener == INVALID_SOCKET) return -1;

	//  Set SO_REUSEADDR and TCP_NODELAY on listening socket.
	int so_reuseaddr = 1;
	int rc = setsockopt (listener, SOL_SOCKET, SO_REUSEADDR,
		(char *)&so_reuseaddr, sizeof (so_reuseaddr));
	if (rc == SOCKET_ERROR) goto fail;

	int tcp_nodelay = 1;
	rc = setsockopt (listener, IPPROTO_TCP, TCP_NODELAY,
		(char *)&tcp_nodelay, sizeof (tcp_nodelay));
	if (rc == SOCKET_ERROR) goto fail;

	//  Bind listening socket to any free local port.
	struct sockaddr_in addr;
	memset (&addr, 0, sizeof (addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
	addr.sin_port = 0;
	rc = bind (listener, (const struct sockaddr*) &addr, sizeof (addr));
	if (rc == SOCKET_ERROR) goto fail;

	//  Retrieve local port listener is bound to (into addr).
	int addrlen = sizeof (addr);
	rc = getsockname (listener, (struct sockaddr*) &addr, &addrlen);
	if (rc == SOCKET_ERROR) goto fail;

	//  Listen for incomming connections.
	rc = listen (listener, 1);
	if (rc == SOCKET_ERROR) goto fail;

	//  Create the writer socket.
	*w_ = WSASocket (AF_INET, SOCK_STREAM, 0, NULL, 0,  0);
	if (*w_ == INVALID_SOCKET) goto fail;

	//  Set TCP_NODELAY on writer socket.
	rc = setsockopt (*w_, IPPROTO_TCP, TCP_NODELAY,
		(char *)&tcp_nodelay, sizeof (tcp_nodelay));
	if (rc == SOCKET_ERROR) goto fail;

	//  Connect writer to the listener.
	rc = connect (*w_, (sockaddr *) &addr, sizeof (addr));
	if (rc == SOCKET_ERROR) goto fail;

	//  Accept connection from writer.
	*r_ = accept (listener, NULL, NULL);
	if (*r_ == INVALID_SOCKET) goto fail;

	//  We don't need the listening socket anymore. Close it.
	rc = closesocket (listener);
	if (rc == SOCKET_ERROR) goto fail;

	return 0;
fail:
	if (listener != INVALID_SOCKET)
		closesocket (listener);
	if (*r_ != INVALID_SOCKET)
		closesocket(*r_);
	if (*w_ != INVALID_SOCKET)
		closesocket(*r_);
	return -1;

#else // All other implementations support socketpair()

	int sv [2];
	int rc = socketpair (AF_UNIX, SOCK_STREAM, 0, sv);
	if (rc != 0) return -1;
	*w_ = sv [0];
	*r_ = sv [1];
	return 0;

#endif
}

#ifdef CC_MSVC
int network_t::make_tcp_socket(uv_os_sock_t *sock, bool ipv6, bool reuseport)
{
	*sock = socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (*sock != INVALID_SOCKET) {
		if (reuseport) {
			DWORD yes = 1;
			if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof yes) == SOCKET_ERROR) {
				close_socket(*sock);
				*sock = INVALID_SOCKET;
				return -1;
			}
		}
		return 0;
	}
	return -1;
}
#else
extern int uv__socket(int domain, int type, int protocol);

int network_t::make_tcp_socket(uv_os_sock_t *sock, bool reuseport)
{
	*sock = uv__socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (*sock >= 0) {
		if (reuseport) {
			int yes = 1;
			if (setsockopt(*sock, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof yes) != 0) {
				close_socket(*sock);
				sock = -1;
				return -1;
			}
		}
		return 0;
	}
	return -1;
}
#endif

int network_t::set_noneblocking( uv_os_sock_t sock )
{
#if defined CC_MSVC
	unsigned long argp = 1;
	int rc = ioctlsocket (sock, FIONBIO, &argp);
	return (rc != SOCKET_ERROR) ? 0 : -1;
#else
	int flags = fcntl (sock, F_GETFL, 0);
	if (flags >= 0) {
		int rc = fcntl (sock, F_SETFL, flags | O_NONBLOCK);
		return (rc == 0) ? 0 : -1;
	}
	return -1;
#endif
}

int network_t::close_socket( uv_os_sock_t sock )
{
#if defined CC_MSVC
	int rc = closesocket (sock);
	return (rc != SOCKET_ERROR) ? 0 : -1;
#else
	return close (sock);
#endif
}

int network_t::close_socketpair( uv_os_sock_t *r, uv_os_sock_t *w )
{
#if defined CC_MSVC
	if (*r != INVALID_SOCKET) {
		close_socket(*r);
		*r = INVALID_SOCKET;
	}
	if (*w != INVALID_SOCKET) {
		close_socket(*w);
		*w = INVALID_SOCKET;
	}
#else
	if (*r != 0) {
		close_socket(*r);
		*r = 0;
	}
	if (*w != 0) {
		close_socket(*w);
		*w = 0;
	}
#endif
	return 0;
}
