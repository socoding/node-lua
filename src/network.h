#ifndef NETWORK_H_
#define NETWORK_H_
#include <map> 
#include "singleton.h"
#include "request.h"

#define SHARED_READ_BUFFER_SIZE	(64 * 1024)
typedef std::map<int64_t, uv_handle_base_t*> shared_write_map_t;

class message_queue_t;

class network_t : public singleton_t <network_t> {
public:
	network_t();
	~network_t();
private:
	uv_thread_t m_thread;
	uv_loop_t *m_uv_loop;
	uv_os_sock_t m_request_r_fd;
	uv_os_sock_t m_request_w_fd;
	uv_poll_t m_request_handle;
	uv_buf_t m_shared_read_buffer;				/* tcp and udp shared read buffer in network thread */
	shared_write_map_t m_shared_write_sockets;  /* tcp and udp shared write socket in network thread */
	atomic_t m_exiting;
public:
	uv_err_code last_error() const { return uv_last_error(m_uv_loop).code; }
	void send_request(request_t& request);
	void start();
	void stop();
	void wait();
private:
	static void thread_entry(void* arg);

	bool recv_request();
	void process_request(request_t& request);
private:
	static void on_request_polled_in(uv_poll_t* handle, int status, int events);
	static void on_request_handle_closed(uv_handle_t* handle);
public:
    static int make_socketpair(uv_os_sock_t *r, uv_os_sock_t *w);
    static int close_socketpair(uv_os_sock_t *r, uv_os_sock_t *w);
	static int set_noneblocking(uv_os_sock_t sock);
	static int close_socket(uv_os_sock_t sock);
public:
	FORCE_INLINE void free_shared_read_buffer() {
		if (m_shared_read_buffer.base) {
			nl_free(m_shared_read_buffer.base);
			m_shared_read_buffer.base = NULL;
			m_shared_read_buffer.len = 0;
		}
	}
	FORCE_INLINE uv_buf_t get_shared_read_buffer() const {
		return m_shared_read_buffer;
	}
	FORCE_INLINE uv_buf_t make_shared_read_buffer() {
		if (m_shared_read_buffer.base) {
			return m_shared_read_buffer;
		}
		m_shared_read_buffer.base = (char*)nl_malloc(SHARED_READ_BUFFER_SIZE);
		assert(m_shared_read_buffer.base != NULL);
		m_shared_read_buffer.len = SHARED_READ_BUFFER_SIZE;
		return m_shared_read_buffer;
	}
	FORCE_INLINE void put_shared_write_socket(int64_t fd, uv_handle_base_t* socket) {
		m_shared_write_sockets[fd] = socket;
	}
	FORCE_INLINE uv_handle_base_t* get_shared_write_socket(int64_t fd) const {
		shared_write_map_t::const_iterator it = m_shared_write_sockets.find(fd);
		if (it != m_shared_write_sockets.end()) {
			return it->second;
		}
		return NULL;
	}
	FORCE_INLINE void pop_shared_write_socket(int64_t fd) {
		m_shared_write_sockets.erase(fd);
	}
private:
	void request_exit(request_exit_t& request);
	void request_tcp_listen(request_tcp_listen_t& request);
	void request_tcp_listens(request_tcp_listens_t& request);
	void request_tcp_accept(request_tcp_accept_t& request);
	void request_tcp_connect(request_tcp_connect_t& request);
	void request_tcp_connects(request_tcp_connects_t& request);
	void request_tcp_write(request_tcp_write_t& request);
	void request_tcp_read(request_tcp_read_t& request);
	void request_udp_open(request_udp_open_t& request);
	void request_udp_write(request_udp_write_t& request);
	void request_udp_read(request_udp_read_t& request);
	void request_handle_option(request_handle_option_t& request);
	void request_handle_close(request_handle_close_t& request);
	void request_timer_start(request_timer_start_t& request);
};

#endif
