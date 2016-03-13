#include "common.h"
#include "worker.h"
#include "context.h"
#include "node_lua.h"
#include "worker_mgr.h"

initialise_singleton(worker_mgr_t);

int32_t worker_mgr_t::m_max_worker_num = -1;

void worker_mgr_t::init_max_worker_num()
{
	if (m_max_worker_num < 0) {
		if (node_lua_t::m_cpu_count > NONE_WORKER_THREAD_NUM) {
			m_max_worker_num = nl_min(node_lua_t::m_cpu_count - NONE_WORKER_THREAD_NUM, MAX_WORKER_RESERVE);
		} else {
			m_max_worker_num = 1;
		}
	}
}

worker_mgr_t::worker_mgr_t() 
	: m_worker_num(0),
	  m_exiting(0),
	  m_worker_lock(spin_lock_init), 
	  m_worker_push_index(0)
{
	init_max_worker_num();
	memset(m_worker, 0, sizeof(m_worker));
	memset(m_thread, 0, sizeof(m_thread));
}

worker_mgr_t::~worker_mgr_t()
{
	/* nothing to do! */
}


void worker_mgr_t::start()
{
	start_worker(1);
}

void worker_mgr_t::check_load(uint32_t ctx_num)
{
	if (!m_exiting && m_worker_num < m_max_worker_num && (ctx_num > m_worker_num * WORKER_LOAD_THRESHOLD)) {
		start_worker(ctx_num);
	}
}

void worker_mgr_t::worker_entry(void* arg)
{
	singleton_ref(worker_mgr_t).worker_process((int64_t)arg);
}

void worker_mgr_t::start_worker(uint32_t ctx_num)
{
	spin_lock(m_worker_lock);
	if (!m_exiting && m_worker_num < m_max_worker_num && (ctx_num > m_worker_num * WORKER_LOAD_THRESHOLD)) {
		int32_t index = m_worker_num;
		m_worker[index] = new worker_t(index);
		atomic_barrier();
		++m_worker_num;
		uv_thread_create(&m_thread[index], worker_entry, (void*)index);
	}
	spin_unlock(m_worker_lock);
}

void worker_mgr_t::wait()
{
	for (int32_t i = 0; i < m_worker_num; ++i) {
		uv_thread_join(&m_thread[i]);
		worker_t *worker = m_worker[i];
		message_t message;
		context_t *running, *release;
		running = worker->pop_all_context();
		while (running) {
			running->attach_worker(worker);
			while (running->pop_message(message)) {
				running->on_dropped(message);
				message_clean(message);
			}
			release = running;
			running = release->next();
			release->release();
		}
		delete m_worker[i];
		m_worker[i] = NULL;
	}
}

void worker_mgr_t::stop()
{
	if (!m_exiting) {
		spin_lock(m_worker_lock);
		if (!m_exiting) {
			m_exiting = 1;
			for (int32_t i = 0; i < m_worker_num; ++i) {
				m_worker[i]->wakeup();
			}
		}
		spin_unlock(m_worker_lock);
	}
}

void worker_mgr_t::push_context(context_t* context)
{
	context->grab();
	m_worker[(uatomic_t)(atomic_inc(m_worker_push_index))%m_worker_num]->push_one_context(context);
}

int32_t worker_mgr_t::steal_context(int32_t running, context_t* &head, context_t* &tail)
{
	for (int32_t i = 0; i < m_worker_num; ++i) {
		if (running != i && m_worker[i]->get_context_length() > 1) {
			int32_t stolen = m_worker[i]->try_pop_half_context(head, tail);
			if (stolen) {
				return stolen;
			}
		}
	}
	return 0;
}

void worker_mgr_t::repush_context(int32_t running, context_t* context)
{
	if (m_worker[running]->get_context_length() <= 0) {
		m_worker[running]->push_one_context(context);
		return;
	}
	int32_t min_index = 0;
	uint32_t min_length = (uint32_t)-1;
	for (int32_t i = 0; i < m_worker_num; ++i) {
		uint32_t length = (uint32_t)m_worker[i]->get_context_length();
		if (length == 0) {
			m_worker[i]->push_one_context(context);
			return;
		}
		if (length < min_length) {
			min_length = length;
			min_index = i;
		}
	}
	m_worker[min_index]->push_one_context(context);
}


void worker_mgr_t::worker_process(int32_t index)
{
	worker_t *worker = m_worker[index];
	message_t message;
	context_t *running, *head, *tail;
	while (!m_exiting) {
		int32_t stolen = 0;
		running = worker->try_pop_one_context();
		if (!running) {
			stolen = steal_context(index, head, tail);
			if (!stolen) {
				worker->wait();
				continue;
			}
			running = head;
			if (stolen - 1 > 0) {
				worker->push_many_context(head->next(), tail, stolen - 1);
			}
		}
		bool processing = true;
		running->attach_worker(worker);
		for (int32_t i = 0; i < MAX_MESSAGE_PROC_NUM || worker->get_context_length() <= 0 || !running->is_valid(); ++i) {
			if (running->pop_message(message)) {
				if (running->is_valid()) {
					running->on_received(message);
				} else {
					running->on_dropped(message);
				}
				message_clean(message);
			} else {
				processing = false;
				break;
			}
		}
		if (!processing) {
			running->release();
		} else {
#ifndef RELEASE
			running->detach_worker();
#endif
			repush_context(index, running);
		}
	}
}


