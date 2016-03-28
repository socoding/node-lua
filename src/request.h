#ifndef REQUEST_H_
#define REQUEST_H_

#include "uv.h"
#include "lua.hpp"
#include "buffer.h"

class uv_handle_base_t;
class uv_tcp_socket_handle_t;
class uv_tcp_listen_handle_t;
class uv_timer_handle_t;

#define REQUEST_SIZE_MAX				255
#define REQUEST_SPARE_MEMBER			m_spare
#define REQUEST_SPARE_SIZE(type)		(REQUEST_SIZE_MAX - offsetof(type, REQUEST_SPARE_MEMBER))
#define REQUEST_SIZE(type, extra)		(offsetof(type, REQUEST_SPARE_MEMBER) + (extra))
#define REQUEST_SPARE_PTR(request)		((request).REQUEST_SPARE_MEMBER)

enum request_type {
	REQUEST_EXIT,
	REQUEST_TCP_LISTEN,
	REQUEST_TCP_LISTENS,
	REQUEST_TCP_ACCEPT,
	REQUEST_TCP_CONNECT,
	REQUEST_TCP_CONNECTS,
	REQUEST_TCP_READ,
	REQUEST_TCP_WRITE,
	REQUEST_HANDLE_OPTION,
	REQUEST_HANDLE_CLOSE,
	REQUEST_TIMER_START
};

#define REQUEST_SPARE_REGION			char REQUEST_SPARE_MEMBER[1];

struct request_exit_t {
	REQUEST_SPARE_REGION
};

struct request_tcp_listen_t {
	uint32_t m_source;
	uint32_t m_session;
	uint16_t m_backlog;
	uint16_t m_port;
	bool m_ipv6;
	REQUEST_SPARE_REGION
};

struct request_tcp_listens_t {
	uint32_t m_source;
	uint32_t m_session;
	REQUEST_SPARE_REGION
};

struct request_tcp_accept_t {
	uv_tcp_listen_handle_t *m_listen_handle;
	bool m_blocking;
	REQUEST_SPARE_REGION
};

/* m_addrs is combination of remote host and local host, separated by '\0'. */
struct request_tcp_connect_t {
	uint32_t m_source;
	uint32_t m_session;
	uint16_t m_remote_port;
	uint16_t m_local_port;
	bool m_remote_ipv6;
	bool m_local_ipv6;
	REQUEST_SPARE_REGION
};

struct request_tcp_connects_t {
	uint32_t m_source;
	uint32_t m_session;
	REQUEST_SPARE_REGION
};

struct request_tcp_read_t {
	uv_tcp_socket_handle_t *m_socket_handle;
	bool m_blocking;
	REQUEST_SPARE_REGION
};

struct request_tcp_write_t {
	uv_tcp_socket_handle_t *m_socket_handle;
	uint32_t m_session;
	uint32_t m_length;
	union {
		const char* m_string;
		buffer_t m_buffer;
	};
	REQUEST_SPARE_REGION
};

/* write by shared fd */
struct request_tcp_write2_t {
	uint64_t m_fd;
	uint32_t m_length;
	union {
		const char* m_string;
		buffer_t m_buffer;
	};
	REQUEST_SPARE_REGION
};

struct request_handle_option_t {
	uv_handle_base_t *m_handle;
	uint8_t m_option_type;
	REQUEST_SPARE_REGION
};

struct request_handle_close_t {
	uv_handle_base_t *m_handle_base;
	REQUEST_SPARE_REGION
};

struct request_timer_start_t {
	uv_timer_handle_t *m_timer_handle;
	uint64_t m_timeout; //in mili-seconds
	uint64_t m_repeat;  //in mili-seconds
	REQUEST_SPARE_REGION
};

struct request_t {
	uint8_t m_head_dummy[6];
	uint8_t m_length;
	uint8_t m_type;
	union {
		char m_body_dummy[REQUEST_SIZE_MAX];
		request_exit_t m_exit;
		request_tcp_listen_t m_tcp_listen;
		request_tcp_listens_t m_tcp_listens;
		request_tcp_accept_t m_tcp_accept;
		request_tcp_connect_t m_tcp_connect;
		request_tcp_connects_t m_tcp_connects;
		request_tcp_read_t m_tcp_read;
		request_tcp_write_t m_tcp_write;
		request_tcp_write2_t m_tcp_write2;
		request_handle_option_t m_handle_option;
		request_handle_close_t m_handle_close;
		request_timer_start_t m_timer_start;
	};
};

#endif
