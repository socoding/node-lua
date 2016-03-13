#include "common.h"
#include "context.h"
#include "message.h"
#include "singleton.h"
#include "node_lua.h"

context_t::context_t()
	: m_handle(0),
	  m_worker(NULL),
	  m_inited(false),
	  m_ref(1),
	  m_processing(true),
	  m_next_processing(NULL),
	  m_message_queue_lock(spin_lock_init)
{
}

context_t::~context_t()
{

}

void context_t::on_registered( uint32_t handle )
{
	if (m_handle == 0) {
		m_handle = handle;
		grab();
	}
}

void context_t::on_retired()
{
	if (m_handle != 0) {
		m_handle = 0;
		release();
	}
}

bool context_t::push_message(message_t& message, bool& processing)
{
	spin_lock(m_message_queue_lock);
	if (m_message_queue.push(message)) {
		processing = m_processing;
		if (!m_processing) {
			m_processing = true;
		}
		spin_unlock(m_message_queue_lock);
		return true;
	}
	spin_unlock(m_message_queue_lock);
	return false;
}

bool context_t::pop_message(message_t& message)
{
	spin_lock(m_message_queue_lock);
	if (m_message_queue.pop(message)) {
		spin_unlock(m_message_queue_lock);
		return true;
	}
	m_processing = false;
#ifndef RELEASE
	detach_worker();
#endif
	spin_unlock(m_message_queue_lock);
	return false;
}

void context_t::on_received(message_t& message)
{
	switch (message_raw_type(message)) {
	case SYSTEM_CTX_DESTROY:
		singleton_ref(node_lua_t).context_destroy(this, message.m_source, message_string(message));
		return;
	case CONTEXT_QUERY:
		if (message.m_source != 0 && message.m_session != LUA_REFNIL) {
			singleton_ref(node_lua_t).context_send(message.m_source, m_handle, message.m_session, CONTEXT_REPLY, NL_ENOREPLY);
		}
		return;
	default:
		return;
	}
}

void context_t::on_dropped(message_t& message)
{
	switch (message_raw_type(message)) {
	case CONTEXT_QUERY:
		if (message.m_source != 0 && message.m_session != LUA_REFNIL) {
			singleton_ref(node_lua_t).context_send(message.m_source, m_handle, message.m_session, CONTEXT_REPLY, NL_ENOREPLY);
		}
		return;
	default:
		return;
	}
}

void context_t::attach_worker(worker_t* worker)
{
#ifndef RELEASE
	ASSERT(m_worker == NULL);
#endif
	m_worker = worker;
	on_worker_attached();
}

void context_t::detach_worker()
{
	m_worker = NULL;
	on_worker_detached();
}
