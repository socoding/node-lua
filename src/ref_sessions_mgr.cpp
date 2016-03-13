#include "message.h"
#include "context_lua.h"
#include "ref_sessions.h"
#include "ref_sessions_mgr.h"

ref_sessions_mgr_t::~ref_sessions_mgr_t()
{
	clear_sessions();
	clear_cached_sessions();
}

void ref_sessions_mgr_t::free_nonblocking_callback(uint32_t id, lua_State* L)
{
	ref_sessions_t* ref_sessions = find_sessions(id);
	if (ref_sessions != NULL) {
		ref_sessions->free_nonblocking_callback(L);
		check_sessions(id, ref_sessions);
	}
}

bool ref_sessions_mgr_t::make_nonblocking_callback(uint32_t id, lua_State *L, uint32_t argc, lua_CFunction adjust_resume_args, int32_t* out_ref)
{
	return make_sessions(id)->make_nonblocking_callback(L, argc, adjust_resume_args, out_ref);
}

bool ref_sessions_mgr_t::is_empty(uint32_t id)
{
	ref_sessions_t* ref_sessions = find_sessions(id);
	return !ref_sessions || ref_sessions->is_empty();
}

int32_t ref_sessions_mgr_t::push_blocking(uint32_t id, lua_State *root_coro)
{
	return make_sessions(id)->push_blocking(root_coro);
}

int32_t ref_sessions_mgr_t::wakeup_all_blocking(uint32_t id, context_lua_t* lctx, message_t& message)
{
	ref_sessions_t* ref_sessions = find_sessions(id);
	if (ref_sessions != NULL) {
		int32_t resume_size = ref_sessions->wakeup_all_blocking(lctx, message);
		check_sessions(id, ref_sessions);
		return resume_size;
	}
	return 0;
}

int32_t ref_sessions_mgr_t::wakeup_one_blocking(uint32_t id, context_lua_t* lctx, message_t& message)
{
	ref_sessions_t* ref_sessions = find_sessions(id);
	if (ref_sessions != NULL) {
		int32_t resume_size = ref_sessions->wakeup_one_blocking(lctx, message);
		check_sessions(id, ref_sessions);
		return resume_size;
	}
	return 0;
}

int32_t ref_sessions_mgr_t::wakeup_noneblocking(uint32_t id, context_lua_t* lctx, message_t& message, bool free_callback)
{
	ref_sessions_t* ref_sessions = find_sessions(id);
	if (ref_sessions != NULL) {
		int32_t result = ref_sessions->wakeup_noneblocking(lctx, message, free_callback);
		check_sessions(id, ref_sessions);
		return result;
	}
	return 0;
}

int32_t ref_sessions_mgr_t::wakeup_one_fixed(uint32_t id, context_lua_t* lctx, message_t& message, bool free_callback)
{
	ref_sessions_t* ref_sessions = find_sessions(id);
	if (ref_sessions != NULL) {
		int32_t result = ref_sessions->wakeup_one_fixed(lctx, message, free_callback);
		check_sessions(id, ref_sessions);
		return result;
	}
	return 0;
}

int32_t ref_sessions_mgr_t::wakeup_once(uint32_t id, context_lua_t* lctx, message_t& message, bool free_callback)
{
	ref_sessions_t* ref_sessions = find_sessions(id);
	if (ref_sessions != NULL) {
		if (ref_sessions->wakeup_one_blocking(lctx, message) || ref_sessions->wakeup_noneblocking(lctx, message, free_callback)) {
			check_sessions(id, ref_sessions);
			return 1;
		}
	}
	return 0;
}

void ref_sessions_mgr_t::wakeup_all(uint32_t id, context_lua_t* lctx, message_t& message, bool free_callback)
{
	ref_sessions_t* ref_sessions = find_sessions(id);
	if (ref_sessions != NULL) {
		ref_sessions->wakeup_all_blocking(lctx, message);
		ref_sessions->wakeup_noneblocking(lctx, message, free_callback);
		check_sessions(id, ref_sessions);
	}
}

ref_sessions_t* ref_sessions_mgr_t::make_sessions(uint32_t id)
{
	ref_sessions_map_t::iterator it = m_sessions_map.find(id);
	if (it != m_sessions_map.end()) {
		return it->second;
	}
	ref_sessions_t* ref_session = get_cached_sessions();
	m_sessions_map.insert(ref_sessions_map_t::value_type(id, ref_session));
	return ref_session;
}

ref_sessions_t* ref_sessions_mgr_t::find_sessions(uint32_t id)
{
	ref_sessions_map_t::iterator it = m_sessions_map.find(id);
	return it != m_sessions_map.end() ? it->second : NULL;
}

void ref_sessions_mgr_t::check_sessions(uint32_t id, ref_sessions_t* ref_sessions)
{
	if (ref_sessions->is_empty() && m_sessions_map.erase(id)) {
		put_cached_sessions(ref_sessions);
	}
}

void ref_sessions_mgr_t::clear_sessions()
{
	for (ref_sessions_map_t::iterator it = m_sessions_map.begin(); it != m_sessions_map.end(); ++it) {
		delete it->second;
	}
	m_sessions_map.clear();
}

ref_sessions_t* ref_sessions_mgr_t::get_cached_sessions()
{
	if (!m_cached_sessions.empty()) {
		ref_sessions_t* cache = m_cached_sessions.back();
		m_cached_sessions.pop_back();
		return cache;
	}
	return new ref_sessions_t();
}

void ref_sessions_mgr_t::put_cached_sessions(ref_sessions_t* ref_sessions)
{
	if (m_cached_sessions.size() >= REF_SESSIONS_CACHE_MAX) {
		delete (m_cached_sessions.back());
		m_cached_sessions.pop_back();
	}
	m_cached_sessions.push_back(ref_sessions);
}

void ref_sessions_mgr_t::clear_cached_sessions()
{
	for (int32_t i = 0; i < m_cached_sessions.size(); ++i) {
		delete (m_cached_sessions[i]);
	}
	m_cached_sessions.clear();
}



