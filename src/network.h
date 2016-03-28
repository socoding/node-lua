#ifndef NETWORK_H_
#define NETWORK_H_
#include "singleton.h"
#include "request.h"

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
private:
	void request_exit(request_exit_t& request);
	void request_tcp_listen(request_tcp_listen_t& request);
	void request_tcp_listens(request_tcp_listens_t& request);
	void request_tcp_accept(request_tcp_accept_t& request);
	void request_tcp_connect(request_tcp_connect_t& request);
	void request_tcp_connects(request_tcp_connects_t& request);
	void request_tcp_write(request_tcp_write_t& request);
	void request_tcp_write2(request_tcp_write2_t& request);
	void request_tcp_read(request_tcp_read_t& request);
	void request_handle_option(request_handle_option_t& request);
	void request_handle_close(request_handle_close_t& request);
	void request_timer_start(request_timer_start_t& request);
};

#endif
