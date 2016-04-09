#ifndef NODE_LUA_H_
#define NODE_LUA_H_
#include "common.h"
#include "singleton.h"
#include "context_mgr.h"
#include "worker_mgr.h"
#include "context.h"

class message_t;
class network_t;
class worker_mgr_t;

class node_lua_t : public singleton_t<node_lua_t> {
public:
	node_lua_t(int argc, char* argv[], char* env[]);
	~node_lua_t();
	/* context send by ctx */
	bool context_send(context_t* ctx, message_t& msg);
	template < class type>
	FORCE_INLINE bool context_send(context_t* ctx, uint32_t source, int session, int msg_type, type data)
	{
		message_t msg(source, session, msg_type, data);
		return context_send(ctx, msg);
	}

	/* context send by handle */
	bool context_send(uint32_t handle, message_t& msg);
	template < class type >
	FORCE_INLINE bool context_send(uint32_t handle, uint32_t source, int session, int msg_type, type data)
	{
		message_t msg(source, session, msg_type, data);
		return context_send(handle, msg);
	}

	/* specific context send interface */
	FORCE_INLINE bool context_send_string_safe(uint32_t handle, uint32_t source, int session, int msg_type, const char *data)
	{
		char* str = data ? nl_strdup(data) : NULL;
		message_t msg(source, session, msg_type, str);
		if (context_send(handle, msg))
			return true;
		message_clean(msg);
		return false;
	}

	FORCE_INLINE bool context_send_string_safe(context_t* ctx, uint32_t source, int session, int msg_type, const char *data)
	{
		char* str = data ? nl_strdup(data) : NULL;
		message_t msg(source, session, msg_type, str);
		if (context_send(ctx, msg))
			return true;
		message_clean(msg);
		return false;
	}
	
	FORCE_INLINE bool context_send_buffer_safe(uint32_t handle, uint32_t source, int session, int msg_type, buffer_t& buffer)
	{
		buffer_grab(buffer);
		message_t msg(source, session, msg_type, buffer);
		bool ret = context_send(handle, msg);
		if (!ret) {
			buffer_release(buffer);
		}
		return ret;
	}

	FORCE_INLINE bool context_send_buffer_safe(context_t* ctx, uint32_t source, int session, int msg_type, buffer_t& buffer)
	{
		buffer_grab(buffer);
		message_t msg(source, session, msg_type, buffer);
		bool ret = context_send(ctx, msg);
		if (!ret) {
			buffer_release(buffer);
		}
		return ret;
	}

	FORCE_INLINE bool context_send_buffer_release(uint32_t handle, uint32_t source, int session, int msg_type, buffer_t& buffer)
	{
		message_t msg(source, session, msg_type, buffer);
		bool ret = context_send(handle, msg);
		if (!ret) {
			buffer_release(buffer);
		}
		return ret;
	}

	FORCE_INLINE bool context_send_buffer_release(context_t* ctx, uint32_t source, int session, int msg_type, buffer_t& buffer)
	{
		message_t msg(source, session, msg_type, buffer);
		bool ret = context_send(ctx, msg);
		if (!ret) {
			buffer_release(buffer);
		}
		return ret;
	}

	template < class type >
	uint32_t context_create(uint32_t parent, int32_t argc, char* argv[], char* env[]) {
		uint32_t handle = 0;
		type *ctx = new type();
		if (m_ctx_mgr->register_context(ctx, parent)) {
			if (ctx->init(argc, argv, env)) {
				ctx->set_inited(true);
				handle = ctx->get_handle();
				m_worker_mgr->push_context(ctx);
				ctx->release();
				m_worker_mgr->check_load(m_ctx_mgr->get_context_count());
				return handle;
			}
			m_ctx_mgr->retire_context(ctx);
		}
		ctx->release();
		if (m_ctx_mgr->get_context_count() == 0) {
			m_worker_mgr->stop();
		}
		return 0;
	}

	/* send destroy signal, can be called anywhere */
	void context_destroy(uint32_t handle, uint32_t src_handle, const char* reason, ...)
	{
		if (handle == 0) return;

		char buffer[512] = { '\0' };
		if (reason) {
			va_list argp;
			va_start(argp, reason);
			vsnprintf(buffer, sizeof(buffer), reason, argp);
			buffer[sizeof(buffer) - 1] = '\0';
			va_end(argp);
		}
		context_send_string_safe(handle, src_handle, 0, SYSTEM_CTX_DESTROY, buffer);
	}

	/* destroy self, can be called only on the running context, use it carefully */
	void context_destroy(context_t*ctx, uint32_t src_handle, const char* reason, ...)
	{
		uint32_t handle = ctx->get_handle();
		if (handle == 0) return;

		char buffer[512] = { '\0' };
		int32_t used = sprintf(buffer, "killed by context:0x%08x", src_handle);
		if (reason) {
			strcat(buffer + used, ", ");
			used = used + 2;
			va_list argp;
			va_start(argp, reason);
			vsnprintf(buffer + used, sizeof(buffer) - used, reason, argp);
			buffer[sizeof(buffer) - 1] = '\0';
			va_end(argp);
		}
		ctx->deinit(buffer);	/* ctx->m_handle is valid in this function. */
		ctx->set_inited(false);
		m_ctx_mgr->retire_context(ctx);
		if (m_ctx_mgr->get_context_count() == 0) {
			m_worker_mgr->stop();
		}
	}

private:
	network_t *m_network;
	context_mgr_t *m_ctx_mgr;
	worker_mgr_t *m_worker_mgr;

public:
	static int32_t m_cpu_count;
private:
	static bool m_inited;
	static void init_once();
};

#endif
