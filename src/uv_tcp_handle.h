#ifndef UV_TCP_HANDLE_H_
#define UV_TCP_HANDLE_H_
#include "buffer.h"
#include "request.h"
#include "endian_conv.h"
#include "uv_handle_base.h"
#include <vector>
#include <queue>

#define TCP_TRANSPORT_DEFAULT_ENDIAN	LITTLE_ENDIAN_VAL
#define TCP_WRITE_UV_REQUEST_CACHE_MAX	32

typedef struct {
	bool frozen;		/* head info can't change after first socket read-start or write-start request had been sent in context thread. */
	char endian;		/* head endian: 'L' or 'B', default 'L'. */
	uint8_t bytes;		/* head byte width: 0, 1, 2, or 4. */
	uint32_t max;		/* head max value restrict, in case of hostile attack from unknown clients(it is valid and greater than 0 only if bytes > 0). */
	/* to be implemented : separator support */
} head_option_t;

#define init_head_option(option)			(option).frozen = false;						\
											(option).endian = TCP_TRANSPORT_DEFAULT_ENDIAN;	\
											(option).bytes = 0;								\
											(option).max = 0
#define check_head_option_max(option, len)	((option).bytes == 0 || (uint32_t)(len) <= (option).max)
#define is_head_option_frozen(option)		((option).frozen)
#define froze_head_option(option)			if(!((option).frozen)) (option).frozen = true

enum tcp_option_type {
	OPT_TCP_NODELAY,
	OPT_TCP_WSHARED,
};

class uv_tcp_listen_handle_t : public uv_handle_base_t
{
public:
	uv_tcp_listen_handle_t(uv_loop_t* loop, uint32_t source, bool sock)
		: uv_handle_base_t(loop, (sock ? UV_NAMED_PIPE : UV_TCP), source),
		  m_has_accept_event(false),
		  m_has_noblocking_accept(false),
		  m_blocking_accept_count(0) {}

	friend class lua_tcp_listen_handle_t;
private:
	static void on_accept(uv_stream_t* server, int status);
	void try_accept();
public:
	void listen_tcp(request_tcp_listen_t& request);
	void listen_sock(request_tcp_listens_t& request);
	void accept(request_tcp_accept_t& request);
private:
	bool m_has_accept_event;
	bool m_has_noblocking_accept;		/* can't change to false if set to be true */
	uint32_t m_blocking_accept_count;	/* maybe not so precise only if m_has_noblocking_accept has been set true,
										   since nonblocking may be treated as blocking in context thread. */
};

class uv_tcp_socket_handle_t : public uv_handle_base_t
{
public:
	uv_tcp_socket_handle_t(uv_loop_t* loop, uint32_t source, bool sock)
		: uv_handle_base_t(loop, (sock ? UV_NAMED_PIPE : UV_TCP), source),
		  m_tcp_sock(-1),
		  m_has_noblocking_read(false),
		  m_blocking_read_count(0),
		  m_read_error(UV_OK),
		  m_read_head_bytes(0),
		  m_read_bytes(0),
		  m_write_error(UV_OK)
	{
		init_head_option(m_read_head_option);
		init_head_option(m_write_head_option);
		buffer_make_invalid(m_read_buffer);
	}
	~uv_tcp_socket_handle_t();

	friend class uv_tcp_listen_handle_t;
	friend class lua_tcp_socket_handle_t;
private:
	static void on_connect(uv_connect_t* req, int status);
	static void on_resolve(uv_getaddrinfo_t* req, int status, struct addrinfo* res);
	static void on_write(uv_write_t* req, int status);
	static void on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf);
	static uv_buf_t on_read_alloc(uv_handle_t* handle, size_t suggested_size); /* return shared read buffer */
	
	int32_t write_handle(request_tcp_write_t& request);
	void write_read_buffer(ssize_t nread, uv_buf_t buf);
	void write_read_buffer_finish(uv_err_code err_code);
	void trigger_read_error(uv_err_code read_error);
	void clear_read_cached_buffers();

public:
	void connect_tcp(request_tcp_connect_t& request);
	void connect_sock(request_tcp_connects_t& request);
	void read(request_tcp_read_t& request);
	static void write(request_tcp_write_t& request);
public:
	bool read_start_state() const
	{
		return m_has_noblocking_read || m_blocking_read_count > 0;
	}
	void set_option(uint8_t type, const char *option);
	void set_tcp_nodelay(bool enable);
	void set_tcp_wshared(bool enable);
	uv_err_code get_read_error()  const { return m_read_error; }
	uv_err_code get_write_error() const { return m_write_error; }
private:
	uv_os_sock_t m_tcp_sock;
	/* read parameters */
	bool m_has_noblocking_read;		/* can't change to false if set to be true */
	uint32_t m_blocking_read_count;	/* maybe not so precise only if m_has_noblocking_accept has been set true,
									   since nonblocking may be treated as blocking in context thread. */

	uv_err_code m_read_error;		/* can be accessed in context_lua thread and should check it before read in context_lua thread. */
	int8_t m_read_head_bytes;
	int8_t m_read_head_buffer[4];
	uint32_t m_read_bytes;
	buffer_t m_read_buffer;
	std::queue<buffer_t> m_read_cached_buffers;

	/* the following members are read-acceleration options and set in context thread before recv starts. */
	head_option_t m_read_head_option;
	
private:
	typedef struct {
		uv_write_t m_write_req;
		union {
			const char* m_string;
			buffer_t m_buffer;
		};
		uint32_t m_length;   /* judge it's string or buffer */
		uint32_t m_source;
		uint32_t m_session;
		int8_t m_head_buffer[4];
	} write_uv_request_t;

	/* write parameters */
	uv_err_code m_write_error;	/* can be accessed in context_lua thread and should check it before write in context_lua thread. */

	/* the following members are write-acceleration options and set in context thread before write starts. */
	head_option_t m_write_head_option;

private:
	std::vector<write_uv_request_t*> m_write_cached_reqs;

	write_uv_request_t* get_write_cached_request() {
		write_uv_request_t* cache;
		if (!m_write_cached_reqs.empty()) {
			cache = m_write_cached_reqs.back();
			m_write_cached_reqs.pop_back();
		} else {
			cache = (write_uv_request_t*)nl_malloc(sizeof(write_uv_request_t));
			cache->m_write_req.data = cache;
		}
		return cache;
	}
	void put_write_cached_request(write_uv_request_t* request) {
		if (m_write_cached_reqs.size() >= TCP_WRITE_UV_REQUEST_CACHE_MAX) {
			nl_free(m_write_cached_reqs.back());
			m_write_cached_reqs.pop_back();
		}
		m_write_cached_reqs.push_back(request);
	}
	void clear_write_cached_requests() {
		for (size_t i = 0; i < m_write_cached_reqs.size(); ++i) {
			nl_free(m_write_cached_reqs[i]);
		}
		m_write_cached_reqs.clear();
	}
};

#endif
