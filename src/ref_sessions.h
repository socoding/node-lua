#ifndef REF_SESSIONS_H_
#define REF_SESSIONS_H_
#include <queue>
#include "common.h"
#include "message.h"

class context_lua_t;

class ref_sessions_t {
private:
	int32_t m_nonblocking_ref;					/* nonblocking_ref callback ref */
	std::vector<int32_t> m_blocking_sessions; 	/* blocking ref coroutine queue */
public:
	ref_sessions_t() : m_nonblocking_ref(LUA_REFNIL) {}
	void free_nonblocking_callback(lua_State* L);
	bool make_nonblocking_callback(lua_State *L, uint32_t argc, lua_CFunction adjust_resume_args = NULL, int32_t* out_ref = NULL);
	FORCE_INLINE
	bool is_empty() const { return (m_nonblocking_ref == LUA_REFNIL) && m_blocking_sessions.empty(); }
	int32_t push_blocking(lua_State *root_coro);
	int32_t wakeup_all_blocking(context_lua_t* lctx, message_t& message);
	int32_t wakeup_one_blocking(context_lua_t* lctx, message_t& message);
	int32_t wakeup_noneblocking(context_lua_t* lctx, message_t& message, bool free_callback = false);
	int32_t wakeup_one_fixed(context_lua_t* lctx, message_t& message, bool free_callback = false);
	FORCE_INLINE
	int32_t wakeup_once(context_lua_t* lctx, message_t& message, bool free_callback = false)
	{
		return wakeup_one_blocking(lctx, message) || wakeup_noneblocking(lctx, message, free_callback);
	}
	FORCE_INLINE
	void wakeup_all(context_lua_t* lctx, message_t& message, bool free_callback = false)
	{
		wakeup_all_blocking(lctx, message);
		wakeup_noneblocking(lctx, message, free_callback);
	}
};

#endif
