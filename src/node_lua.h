#ifndef NODE_LUA_H_
#define NODE_LUA_H_
#include "common.h"
#include "singleton.h"
#include "context_mgr.h"
#include "worker_mgr.h"
#include "context.h"
#include "sds.h"

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

	/* specific context send interface */
	FORCE_INLINE bool context_send_sds_safe(uint32_t handle, uint32_t source, int session, int msg_type, const char *data, int32_t length)
	{
		char* str = data ? sdsnewlen(data, length) : NULL;
		message_t msg(source, session, msg_type, str, length);
		if (context_send(handle, msg))
			return true;
		message_clean(msg);
		return false;
	}

	FORCE_INLINE bool context_send_sds_safe(context_t* ctx, uint32_t source, int session, int msg_type, const char *data, int32_t length)
	{
		char* str = data ? sdsnewlen(data, length) : NULL;
		message_t msg(source, session, msg_type, str, length);
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

	FORCE_INLINE bool context_send_array_release(uint32_t handle, uint32_t source, int session, int msg_type, message_array_t* array)
	{
		message_t msg(source, session, msg_type, array);
		bool ret = context_send(handle, msg);
		if (!ret) {
			message_clean(msg);
		}
		return ret;
	}

	FORCE_INLINE bool context_send_array_release(context_t* ctx, uint32_t source, int session, int msg_type, message_array_t* array)
	{
		message_t msg(source, session, msg_type, array);
		bool ret = context_send(ctx, msg);
		if (!ret) {
			message_clean(msg);
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
		context_check_alive(parent);
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

		char buffer[1024] = { '\0' };
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
		context_check_alive(src_handle);
	}

	void context_check_alive(uint32_t src_handle)
	{
		uint32_t ctx_size = m_ctx_mgr->get_context_count();
		if (ctx_size == 1) {
			context_destroy(m_logger->get_handle(), src_handle, NULL);
			return;
		}
		if (ctx_size == 0) {
			m_worker_mgr->stop();
			return;
		}
	}
	
	bool log_sds_release(uint32_t src_handle, sds buffer) {
		message_t message(src_handle, 0, LOG_MESSAGE, (char*)buffer, sdslen(buffer));
		if (m_logger && context_send(m_logger, message)) {
			return true;
		}
		sdsfree(buffer);
		return false;
	}

	bool log_fmt(uint32_t src_handle, const char* fmt, ...) {
		if (m_logger && fmt) {
			char *buf = NULL;
			size_t buflen = 64;
			va_list argp;
			va_list copy;
			va_start(argp, fmt);
			while (1) {
				buf = (char *)nl_malloc(buflen);
				if (buf == NULL) break;
				buf[buflen - 2] = '\0';
				va_copy(copy, argp);
				vsnprintf(buf, buflen, fmt, copy);
				if (buf[buflen - 2] != '\0') {
					nl_free(buf);
					buflen *= 2;
					continue;
				}
				break;
			}
			va_end(argp);
			if (buf && context_send(m_logger, src_handle, 0, LOG_MESSAGE, buf)) {
				return true;
			}
			nl_free(buf);
		}
		return false;
	}

	FORCE_INLINE uint32_t is_handle_illegal(uint32_t handle) const {
		return handle == m_logger->get_handle();
	}

private:
	network_t *m_network;
	context_mgr_t *m_ctx_mgr;
	worker_mgr_t *m_worker_mgr;
	context_t *m_logger;

public:
	static int32_t m_cpu_count;
private:
	static bool m_inited;
	static void init_once();
};

#endif
