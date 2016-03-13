#ifndef CONTEXT_QUEUE_H_
#define CONTEXT_QUEUE_H_
#include "common.h"

class context_t;

class context_queue_t {
public:
	context_queue_t();
	~context_queue_t();
	void push_one(context_t *context);
	void push_many(context_t *head, context_t *tail, int32_t length);
	context_t* pop_all();
	context_t* try_pop_one();
	int32_t try_pop_half(context_t* &head, context_t* &tail);
	FORCE_INLINE void wait() { uv_sem_wait(&m_sem); }
	FORCE_INLINE void wakeup() { uv_sem_post(&m_sem); };
	FORCE_INLINE int32_t get_length() const { return m_queue_count; }

private:
	context_queue_t(const context_queue_t& mq);
	context_queue_t& operator= (const context_queue_t& mq);
	uv_sem_t m_sem;
	atomic_t m_lock;
	context_t *m_head;
	context_t *m_tail;
	atomic_t m_queue_count;
};

#endif
