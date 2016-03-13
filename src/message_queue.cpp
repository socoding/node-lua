#include "common.h"
#include "message.h"
#include "message_queue.h"

#define DEFAULT_MQ_SIZE  64

message_queue_t::message_queue_t()
	: m_head(0),
	  m_tail(0),
	  m_size(0)
{
	m_cap = DEFAULT_MQ_SIZE;
	m_queue = new message_t[DEFAULT_MQ_SIZE];
}
message_queue_t::~message_queue_t()
{
	if (m_queue) {
		delete []m_queue;
		m_queue = NULL;
	}
}

bool message_queue_t::push(message_t& message)
{
	if (m_size < m_cap || expand_queue()) {
		m_size = m_size + 1;
		m_queue[m_tail] = message;
		if (m_tail + 1 != m_cap) {
			m_tail = m_tail + 1;
			return true;
		}
		m_tail = 0;
		return true;
	}
	return false;
}

bool message_queue_t::pop(message_t& message)
{
	if (m_size > 0) {
		m_size = m_size - 1;
		message = m_queue[m_head];
		if (m_head + 1 != m_cap) {
			m_head = m_head + 1;
			return true;
		}
		m_head = 0;
		return true;
	}
	return false;
}

bool message_queue_t::expand_queue()
{
	uint32_t new_cap = m_cap << 1;
	if (new_cap > m_cap) {
		message_t *new_queue = new message_t[new_cap];
		if (new_queue) {
			for (uint32_t i = 0; i < m_size; i++) {
				new_queue[i] = m_queue[(m_head + i)&(m_cap - 1)];
			}
			m_head = 0;
			m_tail = m_size;
			m_cap = new_cap;
			delete []m_queue;
			m_queue = new_queue;
			return true;
		}
	}
	return false;
}
