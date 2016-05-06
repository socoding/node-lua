#include "common.h"
#include "context.h"
#include "context_mgr.h"

#define DEFAULT_CTX_SLOT_SIZE 4

context_mgr_t::context_mgr_t() : m_handle_index(1), m_ctx_size(0)
{
	uv_rwlock_init(&m_lock);
	m_ctx_slot = new context_t*[DEFAULT_CTX_SLOT_SIZE];
	memset(m_ctx_slot, 0, sizeof(context_t*)*DEFAULT_CTX_SLOT_SIZE);
	m_slot_cap = DEFAULT_CTX_SLOT_SIZE;
}

context_mgr_t::~context_mgr_t()
{
	if (m_ctx_slot) {
		delete []m_ctx_slot;
		m_ctx_slot = NULL;
	}
}

void context_mgr_t::set_handle_index(uint32_t index)
{
	uv_rwlock_wrlock(&m_lock);
	m_handle_index = index;
	uv_rwlock_wrunlock(&m_lock);
}

void context_mgr_t::expand_slot()
{
	uint32_t new_cap = m_slot_cap << 2;
	context_t ** new_slot = new context_t *[new_cap];
	memset(new_slot, 0, sizeof(context_t*)*new_cap);
	for (uint32_t i = 0; i < m_slot_cap; ++i) {
		int hash = m_ctx_slot[i]->get_handle()&(new_cap - 1);
		ASSERT(new_slot[hash] == NULL);
		new_slot[hash] = m_ctx_slot[i];
	}
	delete []m_ctx_slot;
	m_ctx_slot = new_slot;
	m_slot_cap = new_cap;
}

bool context_mgr_t::register_context(context_t* ctx, uint32_t parent)
{
	uv_rwlock_wrlock(&m_lock);
	if (m_ctx_size == MAX_CTX_SIZE || ctx->get_handle() != 0) {
		uv_rwlock_wrunlock(&m_lock);
		return false;
	}
	for (;;) {
		for (uint32_t i = 0; i < m_slot_cap; ++i) {
			uint32_t handle = i + m_handle_index;
			if (handle == 0) {		//handle 0 is invalid (reserved for network thread)!!! 
				m_handle_index += m_slot_cap;
				handle = i + m_handle_index;
			}
			uint32_t hash = handle & (m_slot_cap - 1);
			if (m_ctx_slot[hash] == NULL) {
				m_ctx_slot[hash] = ctx;
				m_handle_index = handle + 1;
				++m_ctx_size;
				ctx->on_registered(parent, handle);
				uv_rwlock_wrunlock(&m_lock);
				return true;
			}
		}
		expand_slot();
	}
	uv_rwlock_wrunlock(&m_lock);
}

void context_mgr_t::retire_context( context_t* ctx )
{
	uv_rwlock_wrlock(&m_lock);
	uint32_t handle = ctx->get_handle();
	if (handle == 0) {
		uv_rwlock_wrunlock(&m_lock);
		return;
	}
	int hash = handle&(m_slot_cap - 1);
	ASSERT(ctx == m_ctx_slot[hash]);
	m_ctx_slot[hash] = NULL;
	--m_ctx_size;
	ctx->on_retired();
	uv_rwlock_wrunlock(&m_lock);
}

context_t* context_mgr_t::grab_context( uint32_t handle )
{
	uv_rwlock_rdlock(&m_lock);
	context_t* ctx = m_ctx_slot[handle&(m_slot_cap - 1)];
	if (ctx && ctx->get_handle() == handle) {
		ctx->grab();
		uv_rwlock_rdunlock(&m_lock);
		return ctx; 
	}
	uv_rwlock_rdunlock(&m_lock);
	return NULL;
}
