#ifndef WORKER_MGR_H_
#define WORKER_MGR_H_
#include "common.h"
#include "singleton.h"

class worker_t;

class worker_mgr_t : public singleton_t <worker_mgr_t> {
public:
	worker_mgr_t();
	~worker_mgr_t();
	void start();
	void stop();
	void wait();
	FORCE_INLINE int32_t get_worker_num() const { return m_worker_num; }
	int32_t steal_context(int32_t running, context_t* &head, context_t* &tail);
	void push_context(context_t* context);
	void repush_context(int32_t running, context_t* context);
	void check_load(uint32_t ctx_num);

private:
	void start_worker(uint32_t ctx_num);
	void worker_process(int32_t worker);
	static void worker_entry(void* arg);
private:
	atomic_t m_worker_num;
	worker_t*   m_worker[MAX_WORKER_RESERVE];
	uv_thread_t m_thread[MAX_WORKER_RESERVE];
	atomic_t m_exiting;
	atomic_t m_worker_lock;
	atomic_t m_worker_push_index;
private:
	static int32_t m_max_worker_num;
	static void init_max_worker_num();
};
#endif // !WORKER_MGR_H_
