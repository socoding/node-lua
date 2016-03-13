#include "common.h"
#include "context.h"
#include "context_queue.h"

context_queue_t::context_queue_t() 
	: m_lock(spin_lock_init),
	  m_head(NULL),
	  m_tail(NULL),
	  m_queue_count(0)
{
	uv_sem_init(&m_sem, 0);
}

context_queue_t::~context_queue_t()
{
	ASSERT(m_queue_count == 0);
}

void context_queue_t::push_one(context_t *context)
{
	bool wakeup = false;
	context->unlink();
	spin_lock(m_lock);
	if (m_tail) {
		m_tail->link(context);
		m_tail = context;
		++m_queue_count;
	} else {
		m_head = m_tail = context;
		m_queue_count = 1;
		wakeup = true;
	}
	spin_unlock(m_lock);
	if (wakeup) {
		uv_sem_post(&m_sem);
	}
}

void context_queue_t::push_many(context_t *head, context_t *tail, int32_t length)
{
	if (length == 1) {
		push_one(head);
		return;
	}
	bool wakeup = false;
	tail->unlink();
	spin_lock(m_lock);
	if (m_tail) {
		m_tail->link(head);
		m_tail = tail;
		m_queue_count += length;
	} else {
		m_head = head;
		m_tail = tail;
		m_queue_count = length;
		wakeup = true;
	}
	spin_unlock(m_lock);
	if (wakeup) {
		uv_sem_post(&m_sem);
	}
}

context_t* context_queue_t::try_pop_one()
{
	context_t *context;
	spin_lock(m_lock);
	if (m_head) {
		context = m_head;
		m_head = m_head->next();
		if (m_head) {
			--m_queue_count;
		} else {
			m_queue_count = 0;
			m_tail = NULL;
		}
		spin_unlock(m_lock);
		context->unlink();
		return context;
	}
	spin_unlock(m_lock);
	return NULL;
}

int32_t context_queue_t::try_pop_half(context_t* &head, context_t* &tail)
{
	int32_t half, i = 1;
	spin_lock(m_lock);
	if (m_head && (half = m_queue_count >> 1)) {
		m_queue_count -= half;
		head = tail = m_head;
		for (; i < half; ++i) {
			tail = tail->next();
		}
		m_head = tail->next();
		spin_unlock(m_lock);
		tail->unlink();
		return half;
	}
	spin_unlock(m_lock);
	head = tail = NULL;
	return 0;
}

context_t* context_queue_t::pop_all()
{
	context_t *context;
	spin_lock(m_lock);
	context = m_head;
	m_head = m_tail = NULL;
	m_queue_count = 0;
	spin_unlock(m_lock);
	return context;
}
