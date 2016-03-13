#ifndef REF_SESSIONS_MGR_H_
#define REF_SESSIONS_MGR_H_

#include <map>
#include <vector>
#include "common.h"
#define REF_SESSIONS_CACHE_MAX	128

class message_t;
class context_lua_t;
class ref_sessions_t;

class ref_sessions_mgr_t
{
public:
public:
	ref_sessions_mgr_t() {}
	~ref_sessions_mgr_t();
	void free_nonblocking_callback(uint32_t id, lua_State* L);
	bool make_nonblocking_callback(uint32_t id, lua_State *L, uint32_t argc, lua_CFunction adjust_resume_args = NULL, int32_t* out_ref = NULL);
	bool is_empty(uint32_t id);
	int32_t push_blocking(uint32_t id, lua_State *root_coro);
	int32_t wakeup_all_blocking(uint32_t id, context_lua_t* lctx, message_t& message);
	int32_t wakeup_one_blocking(uint32_t id, context_lua_t* lctx, message_t& message);
	int32_t wakeup_noneblocking(uint32_t id, context_lua_t* lctx, message_t& message, bool free_callback = false);
	int32_t wakeup_one_fixed(uint32_t id, context_lua_t* lctx, message_t& message, bool free_callback = false);
	int32_t wakeup_once(uint32_t id, context_lua_t* lctx, message_t& message, bool free_callback = false);
	void wakeup_all(uint32_t id, context_lua_t* lctx, message_t& message, bool free_callback = false);

private:
	typedef std::map<uint32_t, ref_sessions_t*> ref_sessions_map_t;
	typedef std::vector<ref_sessions_t*> ref_sessions_vec_t;
	ref_sessions_map_t m_sessions_map;
	ref_sessions_vec_t m_cached_sessions;

	ref_sessions_t* make_sessions(uint32_t id);
	ref_sessions_t* find_sessions(uint32_t id);
	void check_sessions(uint32_t id, ref_sessions_t* ref_sessions);
	void clear_sessions();
	
	ref_sessions_t* get_cached_sessions();
	void put_cached_sessions(ref_sessions_t* ref_sessions);
	void clear_cached_sessions();
};

#endif