#include "common.h"
#include "context.h"
#include "network.h"
#include "context_lua.h"
#include "context_mgr.h"
#include "worker_mgr.h"
#include "node_lua.h"

initialise_singleton(node_lua_t);

bool    node_lua_t::m_inited = false;
int32_t node_lua_t::m_cpu_count = 1;

void node_lua_t::init_once()
{
	if (m_inited) return;
	uv_cpu_info_t* cpus;
	uv_err_t err = uv_cpu_info(&cpus, &m_cpu_count);
	assert(UV_OK == err.code);
	uv_free_cpu_info(cpus, m_cpu_count);
#ifndef RELEASE
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
#endif
#ifndef CC_MSVC
	signal(SIGPIPE, SIG_IGN); /* close SIGPIPE signal */
#endif
	m_inited = true;
}

node_lua_t::node_lua_t(int argc, char* argv[], char* env[])
{
	node_lua_t::init_once();
	m_network = new network_t();
	m_ctx_mgr = new context_mgr_t();
	m_worker_mgr = new worker_mgr_t();
	m_network->start();
	m_worker_mgr->start();
	context_create<context_lua_t>(0, argc, argv, env);
	m_worker_mgr->wait();
	m_network->stop();
	m_network->wait();
}

node_lua_t::~node_lua_t()
{
	delete m_ctx_mgr;
	delete m_network;
	delete m_worker_mgr;
}

bool node_lua_t::context_send(context_t* ctx, message_t& msg)
{
	if (ctx->get_handle() != 0 && (ctx->is_inited() || ctx->get_handle() == msg.m_source)) {
		bool processing;
		if (ctx->push_message(msg, processing)) {
			if (!processing) {
				m_worker_mgr->push_context(ctx);
			}
			return true;
		}
	}
	return false;
}

bool node_lua_t::context_send(uint32_t handle, message_t& msg)
{
	context_t *ctx = m_ctx_mgr->grab_context(handle);
	if (ctx) {
		bool ret = context_send(ctx, msg);
		ctx->release();
		return ret;
	}
	return false;
}
