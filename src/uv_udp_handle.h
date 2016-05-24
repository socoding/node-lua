#ifndef UV_UDP_HANDLE_H_
#define UV_UDP_HANDLE_H_
#include "buffer.h"
#include "request.h"
#include "uv_handle_base.h"
#include <queue>

#define UDP_WRITE_UV_REQUEST_CACHE_MAX	32

enum udp_option_type {
	OPT_UDP_WSHARED,
};

class uv_udp_handle_t : public uv_handle_base_t
{
public:
	uv_udp_handle_t(uv_loop_t* loop, uint32_t source)
		: uv_handle_base_t(loop, UV_UDP, source),
		  m_read_started(false),
		  m_udp_sock(-1) {}
	~uv_udp_handle_t();

	friend class lua_udp_handle_t;
private:
	static void on_read(uv_udp_t* handle, ssize_t nread, uv_buf_t buf, struct sockaddr* addr, unsigned flags);
	static uv_buf_t on_read_alloc(uv_handle_t* handle, size_t suggested_size);
	static void on_write(uv_udp_send_t* req, int status);

	int32_t write_handle(request_udp_write_t& request);
public:
	void set_option(uint8_t type, const char *option);
	void set_udp_wshared(bool enable);

public:
	void open(request_udp_open_t& request);
	void read(request_udp_read_t& request);
	static void write(request_udp_write_t& request);
private:
	bool m_read_started;
	uv_os_sock_t m_udp_sock;

	typedef struct {
		uv_udp_send_t m_write_req;
		union {
			const char* m_string;
			buffer_t m_buffer;
		};
		uint32_t m_length;   /* judge it's string or buffer */
		uint32_t m_source;
		uint32_t m_session;
	} write_uv_request_t;

	std::vector<write_uv_request_t*> m_write_cached_reqs;

	write_uv_request_t* get_write_cached_request() {
		write_uv_request_t* cache;
		if (!m_write_cached_reqs.empty()) {
			cache = m_write_cached_reqs.back();
			m_write_cached_reqs.pop_back();
		}
		else {
			cache = (write_uv_request_t*)nl_malloc(sizeof(write_uv_request_t));
			cache->m_write_req.data = cache;
		}
		return cache;
	}
	void put_write_cached_request(write_uv_request_t* request) {
		if (m_write_cached_reqs.size() >= UDP_WRITE_UV_REQUEST_CACHE_MAX) {
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
