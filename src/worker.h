#ifndef WORKER_H_
#define WORKER_H_
#include "common.h"
#include "context_lua.h"
#include "context_queue.h"

class worker_t
{
public:
	worker_t(int32_t index) : m_index(index)
	{
		m_context_lua_shared = new context_lua_shared_t();
	}
	~worker_t()
	{
		delete m_context_lua_shared;
		m_context_lua_shared = NULL;
	}
	const int32_t m_index;

private:
	context_queue_t m_context_queue;

public:
	FORCE_INLINE void push_one_context(context_t* context) { m_context_queue.push_one(context); }
	FORCE_INLINE void push_many_context(context_t *head, context_t *tail, int32_t length) { m_context_queue.push_many(head, tail, length); }
	FORCE_INLINE context_t* pop_all_context() { return m_context_queue.pop_all(); }
	FORCE_INLINE context_t* try_pop_one_context() { return m_context_queue.try_pop_one(); }
	FORCE_INLINE int32_t try_pop_half_context(context_t* &head, context_t* &tail) { return m_context_queue.try_pop_half(head, tail); }
	FORCE_INLINE void wait() { m_context_queue.wait(); }
	FORCE_INLINE void wakeup() { m_context_queue.wakeup(); }
	FORCE_INLINE int32_t get_context_length() const { return m_context_queue.get_length(); }

private:
	friend class context_lua_t;
	context_lua_shared_t* m_context_lua_shared;
};

#endif
