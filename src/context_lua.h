#ifndef CONTEXT_LUA_H_
#define CONTEXT_LUA_H_

#include <set>
#include <vector>
#include "common.h"
#include "request.h"
#include "context.h"
#include "ref_sessions.h"
#include "ref_sessions_mgr.h"

#define CONTEXT_ROOT_CORO_CACHE_MAX	32

typedef int32_t(*yiled_finalize_t)(lua_State *root_coro, lua_State *main_L, void* userdata, uint32_t destination);
typedef int32_t(*timeout_callback_t)(lua_State *root_coro, int32_t session, void* userdata, bool is_repeat);

struct context_lua_shared_t {
	uint32_t m_yielding_destination;	/* 0 for network thread */
	uint32_t m_yielding_depth;			/* 0 if not yielding up */
	yiled_finalize_t m_yielding_finalize; /* mustn't call any function in it */
	void* m_yielding_finalize_userdata;
	int32_t m_yielding_status;
	uint64_t m_yielding_timeout;
	request_t m_yielding_request;
	message_t m_yielding_message;
};

class context_lua_t : public context_t {
public:
	context_lua_t();
	bool init(int32_t argc, char* argv[], char* env[]);
	bool deinit(const char *arg);
	void on_received(message_t& message);
	void on_dropped(message_t& message);
	void on_worker_attached();
	void on_worker_detached();

	bool is_active() const { return m_ref_session_count > 0; }
	uint32_t yielding_up() { return (m_shared->m_yielding_depth) ? ++m_shared->m_yielding_depth : 0; }
	request_t& get_yielding_request() const { return m_shared->m_yielding_request; }
	message_t& get_yielding_message() const { return m_shared->m_yielding_message; }
	int32_t get_yielding_status() const { return m_shared->m_yielding_status; }
	uint64_t get_yielding_timeout() const { return m_shared->m_yielding_timeout; }
	int32_t yielding_finalize(lua_State *root_coro, int32_t status); /* status MUSTN'T be UV_OK only IF root_coro is null. */

private:
	~context_lua_t();
	context_lua_t(const context_lua_t& lctx);
	context_lua_t& operator=(const context_lua_t& lctx);

	/* @m_lstate 
	** The main lvm where the lua context is running.
	*/
	lua_State *m_lstate;

	/* @m_shared
	** The shared data owned by the running worker.
	*/
	context_lua_shared_t *m_shared;

	/* @m_session_count 
	** The yielded coroutine count, corresponding to the table
	** binded to 'session_table_key'.
	*/
	int32_t m_ref_session_count;

	/* @m_session_id
	** session_id generator base.
	*/
	int32_t m_ref_session_base;

	/* @m_cached_root_coros
	** cached root coroutines.
	*/
	std::vector<lua_State*> m_cached_root_coros;

private:
	ref_sessions_mgr_t m_context_recv_sessions;
	ref_sessions_mgr_t m_context_wait_sessions;
	std::set<uint32_t> m_context_wait_handles;

private:
	static lua_State* lua_new_root_coro(lua_State *L);
	static void lua_free_root_coro(lua_State *co);
	static void lua_open_libs(lua_State *L);
	static void lua_load_env(lua_State *L, char* env[]);
	static void lua_free_env(lua_State *L, char* env[]);
	static void lua_init_env(lua_State *L, int32_t envc);
	static void lua_init(lua_State *L);

private:
	void lua_ctx_init(message_t& message);
	void response_context_query(message_t& response);
	void response_context_reply(message_t& response);
	void response_context_wait(message_t& response);
	void response_context_wakeup(message_t& response);
	void response_tcp_listen(message_t& response);
	void response_tcp_accept(message_t& response);
	void response_tcp_connect(message_t& response);
	void response_tcp_write(message_t& response);
	void response_tcp_read(message_t& response);
	void response_tcp_closing(message_t& response);
	void response_udp_open(message_t& response);
	void response_udp_write(message_t& response);
	void response_udp_read(message_t& response);
	void response_udp_closing(message_t& response);
	void response_handle_close(message_t& response);
	void response_timeout(message_t& response);

public:
	static void unload();
	static int32_t lua_ref_callback_entry(lua_State *L);				/* for lua_ref_callback */
	static int32_t lua_ref_callback_entry_continue(lua_State *L, int status, lua_KContext ctx);		/* for lua_ref_callback */
	static int32_t lua_ref_callback_entry_finish(lua_State *L, int32_t status);
	static int32_t lua_ref_callback_error_handler(lua_State *L);		/* for lua_ref_callback */

public:
	static uint32_t lua_get_context_handle(lua_State *L);
	static context_lua_t* lua_get_context(lua_State *L);
	static int32_t lua_strerror(lua_State *L, int32_t idx);
	static void lua_throw(lua_State *L, int32_t idx);

	/******** the following are yielding facilities ********/
	static int32_t common_yield_continue(lua_State* L, int status, lua_KContext ctx);
	static int32_t lua_yield_send(lua_State *L, uint32_t destination, yiled_finalize_t fnz, void* fnz_ud = NULL, lua_KFunction yieldk = NULL, int64_t timeout = 0);
	static int32_t common_callback_adjust(lua_State* L);
	static int32_t lua_ref_session(lua_State *L, int32_t idx);
	static void lua_unref_session(lua_State *L, int32_t idx, int32_t session);
	static int32_t lua_ref_callback(lua_State *L, uint32_t argc, int32_t ref = LUA_REFNIL, lua_CFunction adjust_resume_args = NULL);
	static int32_t lua_ref_yield_coroutine(lua_State *co);	
	static int32_t lua_ref_timer(lua_State *L, int32_t session, uint64_t timeout, uint64_t repeat, bool is_cancel = false, void *userdata = NULL, timeout_callback_t callback = NULL);
	static void lua_unref_timer(lua_State *L, int32_t session);
	static void lua_pushmessage(lua_State *L, message_t& message);
	static void lua_push_ref_session(lua_State *L, int32_t ref);
	static void lua_free_ref_session(lua_State *L, int32_t ref);
	int32_t resume_coroutine(lua_State *co, int32_t narg);
	int32_t wakeup_ref_session(int32_t ref, message_t& message, bool free_callback);
	/********** the above are yielding facilities **********/

	/********** the following are context lua api **********/
	static int32_t context_check_message(lua_State *L, int32_t idx, uint32_t msg_type, message_t& message);
	static int32_t context_check_message(lua_State *L, int32_t idx, uint32_t count, uint32_t msg_type, message_t& message);
	static int32_t context_send(lua_State *L, int32_t idx, uint32_t count, uint32_t handle, int32_t session, uint32_t msg_type);
	static int32_t context_query(lua_State *L, bool timed_query);
	static int32_t context_query_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination);
	static int32_t context_query_yield_continue(lua_State* L, int status, lua_KContext ctx);
	static int32_t context_recv_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination);
	static int32_t context_recv_yield_continue(lua_State* L, int status, lua_KContext ctx);
	static int32_t context_recv_callback_adjust(lua_State* L);
	static int32_t context_recv_timeout(lua_State *L, int32_t session, void* userdata, bool is_repeat);
	static int32_t context_wait_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination);
	static int32_t context_wait_yield_continue(lua_State* L, int status, lua_KContext ctx);
	static int32_t context_wait_timeout(lua_State *L, int32_t session, void* userdata, bool is_repeat);
	static int32_t context_strerror(lua_State *L);
	static int32_t context_self(lua_State *L);
	static int32_t context_thread(lua_State *L);
	static int32_t context_create(lua_State *L);
	static int32_t context_destroy(lua_State *L);
	static int32_t context_send(lua_State *L);
	static int32_t context_query(lua_State *L);
	static int32_t context_timed_query(lua_State *L);
	static int32_t context_reply(lua_State *L);
	static int32_t context_recv(lua_State *L);
	static int32_t context_wait(lua_State *L);
	static int32_t context_log(lua_State *L);
	/************ the above are context lua api ************/

public:
	static char m_root_coro_table_key;
	static char m_ref_session_table_key;
	static char m_ref_timer_table_key;
};

#endif
