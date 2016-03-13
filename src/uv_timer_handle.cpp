#include "uv_timer_handle.h"
#include "node_lua.h"

void uv_timer_handle_t::on_timedout(uv_timer_t* handle, int status)
{
	uv_timer_handle_t* timer_handle = (uv_timer_handle_t*)handle->data;
	singleton_ref(node_lua_t).context_send(timer_handle->m_source, 0, timer_handle->m_lua_ref, RESPONSE_TIMEOUT, NL_ETIMEOUT);
}

void uv_timer_handle_t::start(uv_loop_t* loop, request_timer_start_t& request)
{
	init(loop);
	uv_timer_start((uv_timer_t*)m_handle, on_timedout, request.m_timeout, request.m_repeat);
}
