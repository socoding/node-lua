#ifndef CONTEXT_MGR_H_
#define CONTEXT_MGR_H_

class context_t;

class context_mgr_t {
public:
	context_mgr_t();
	~context_mgr_t();
	bool register_context(context_t* ctx);
	void retire_context(context_t* ctx);
	context_t* grab_context(uint32_t handle);
	uint32_t get_context_count() const { return m_ctx_size; }
private:
	void expand_slot();

	uint32_t m_handle_index;
	uint32_t m_ctx_size;
	uint32_t m_slot_cap;
	context_t ** m_ctx_slot;
	uv_rwlock_t m_lock;
};

#endif
