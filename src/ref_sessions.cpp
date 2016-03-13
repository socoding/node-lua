#include "ref_sessions.h"
#include "context_lua.h"

bool ref_sessions_t::make_nonblocking_callback(lua_State *L, uint32_t argc, lua_CFunction adjust_resume_args, int32_t* out_ref)
{
	bool is_new_ref = (m_nonblocking_ref == LUA_REFNIL);
	m_nonblocking_ref = context_lua_t::lua_ref_callback(L, argc, m_nonblocking_ref, adjust_resume_args);
	if (out_ref) {
		*out_ref = m_nonblocking_ref;
	}
	return is_new_ref;
}

void ref_sessions_t::free_nonblocking_callback(lua_State* L)
{
	if (m_nonblocking_ref != LUA_REFNIL) {
		context_lua_t::lua_free_ref_session(L, m_nonblocking_ref);
		m_nonblocking_ref = LUA_REFNIL;
	}
}

int32_t ref_sessions_t::push_blocking(lua_State *root_coro)
{
	int32_t session = context_lua_t::lua_ref_yield_coroutine(root_coro);
	m_blocking_sessions.push_back(session);
	return session;
}

int32_t ref_sessions_t::wakeup_all_blocking(context_lua_t* lctx, message_t& message)
{
	int32_t return_size = 0;
	int32_t resume_size = m_blocking_sessions.size();
	for (int32_t i = 0; i < resume_size; ++i) {
		int32_t session = m_blocking_sessions.front();
		m_blocking_sessions.erase(m_blocking_sessions.begin());
		if (lctx->wakeup_ref_session(session, message, false)) {
			++return_size;
		}
	}
	return return_size;
}

int32_t ref_sessions_t::wakeup_one_blocking(context_lua_t* lctx, message_t& message)
{
	if (!m_blocking_sessions.empty()) {
		int32_t session = m_blocking_sessions.front();
		m_blocking_sessions.erase(m_blocking_sessions.begin());
		return lctx->wakeup_ref_session(session, message, false);
	}
	return 0;
}

int32_t ref_sessions_t::wakeup_noneblocking(context_lua_t* lctx, message_t& message, bool free_callback)
{
	if (!free_callback) {
		return lctx->wakeup_ref_session(m_nonblocking_ref, message, false);
	}
	int32_t nonblocking_ref = m_nonblocking_ref;
	m_nonblocking_ref = LUA_REFNIL;
	return lctx->wakeup_ref_session(nonblocking_ref, message, true);
}

int32_t ref_sessions_t::wakeup_one_fixed(context_lua_t* lctx, message_t& message, bool free_callback)
{
	if (message.m_session != m_nonblocking_ref) {
		for (std::vector<int32_t>::iterator it = m_blocking_sessions.begin(); it != m_blocking_sessions.end(); ++it) {
			if (message.m_session == *it) {
				m_blocking_sessions.erase(it);
				return lctx->wakeup_ref_session(message.m_session, message, false);
			}
		}
	} else {
		return wakeup_noneblocking(lctx, message, free_callback);
	}
	return 0;
}
