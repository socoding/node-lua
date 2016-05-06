#include "context_log.h"

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

}

void context_log_t::on_dropped(message_t& message)
{

}

