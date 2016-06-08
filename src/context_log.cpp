#include "context_log.h"
#include "node_lua.h"

bool context_log_t::init(int32_t argc, char* argv[], char* env[])
{
	return true;
}

bool context_log_t::deinit(const char *arg)
{
	return true;
}

void context_log_t::on_received(message_t& message)
{
	switch (message_raw_type(message)) {
	case LOG_MESSAGE:
		log_message(message);
		return;
	case SYSTEM_CTX_DESTROY:
		singleton_ref(node_lua_t).context_destroy(this, message.m_source, message_string(message));
		return;
	default:
		break;
	}
}

void context_log_t::on_dropped(message_t& message)
{
	switch (message_raw_type(message)) {
	case LOG_MESSAGE:
		log_message(message);
		return;
	default:
		break;
	}
}

void context_log_t::log_message(message_t& message)
{
	switch (message_data_type(message)) {
	case BINARY:
		if (message_binary(message).m_data) {
			lua_writestring(message_binary(message).m_data, sdslen(message_binary(message).m_data));
			lua_writeline();
		}
		break;
	case STRING:
		if (message_string(message)) {
			lua_writestring(message_string(message), strlen(message_string(message)));
			lua_writeline();
		}
		break;
	default:
		break;
	}
}

