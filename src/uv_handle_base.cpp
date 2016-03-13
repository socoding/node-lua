#include "uv_handle_base.h"
#include "node_lua.h"

uv_handle_base_t::uv_handle_base_t(uv_loop_t* loop, uv_handle_type type, uint32_t source)
	: m_handle_type(type),
	  m_source(source),
	  m_lua_ref(LUA_REFNIL),
	  m_handle(NULL)
{
	init(loop);
}

uv_handle_base_t::uv_handle_base_t(uv_handle_type type, uint32_t source)
	: m_handle_type(type),
	  m_source(source),
	  m_lua_ref(LUA_REFNIL),
	  m_handle(NULL) {}

void uv_handle_base_t::init(uv_loop_t* loop)
{
	ASSERT(m_handle == NULL);
	switch (m_handle_type) {
	case UV_TCP:
		m_handle = (uv_handle_t*)nl_malloc(sizeof(uv_tcp_t));
		uv_tcp_init(loop, (uv_tcp_t*)m_handle);
		break;
	case UV_UDP:
		m_handle = (uv_handle_t*)nl_malloc(sizeof(uv_udp_t));
		uv_udp_init(loop, (uv_udp_t*)m_handle);
		break;
	case UV_NAMED_PIPE:
		m_handle = (uv_handle_t*)nl_malloc(sizeof(uv_pipe_t));
		uv_pipe_init(loop, (uv_pipe_t*)m_handle, 0);
		break;
	case UV_TIMER:
		m_handle = (uv_handle_t*)nl_malloc(sizeof(uv_timer_t));
		uv_timer_init(loop, (uv_timer_t*)m_handle);
		break;
	default:
		assert(false);
	}
	m_handle->data = this;
}

uv_handle_base_t::~uv_handle_base_t()
{
	if (m_handle) {
		nl_free(m_handle);
		m_handle = NULL;
	}
	m_lua_ref = LUA_REFNIL;
}

handle_set uv_handle_base_t::get_handle_set() const
{
	switch (m_handle_type) {
	case UV_TCP:
	case UV_UDP:
	case UV_NAMED_PIPE:
		return SOCKET_SET;
	case UV_TIMER:
		return TIMER_SET;
	default:
		return NONE_SET;
	}
}

void uv_handle_base_t::close()
{
	if (m_handle) {
		uv_close(m_handle, on_closed);
	} else if (m_lua_ref != LUA_REFNIL) {
		singleton_ref(node_lua_t).context_send(m_source, 0, m_lua_ref, RESPONSE_HANDLE_CLOSE, (double)get_handle_set());
		delete this;
	}
}

void uv_handle_base_t::on_closed(uv_handle_t* handle)
{
	uv_handle_base_t* tcp_handle = (uv_handle_base_t*)handle->data;
	if (tcp_handle->m_lua_ref != LUA_REFNIL) {
		singleton_ref(node_lua_t).context_send(tcp_handle->m_source, 0, tcp_handle->m_lua_ref, RESPONSE_HANDLE_CLOSE, (double)(tcp_handle->get_handle_set()));
	}
	delete tcp_handle;
}

