#ifndef MESSAGE_QUEUE_H_
#define MESSAGE_QUEUE_H_
#include "common.h"

class message_t;

class message_queue_t {
public:
	message_queue_t();
	~message_queue_t();
	bool push(message_t& message);
	bool pop(message_t& message);
	FORCE_INLINE uint32_t get_length() const { return m_size; }

private:
	message_queue_t(const message_queue_t& mq);
	message_queue_t& operator= (const message_queue_t& mq);
	bool expand_queue();

	uint32_t m_cap;
	uint32_t m_head;
	uint32_t m_tail;
	uint32_t m_size;
	message_t *m_queue;
};

#endif
