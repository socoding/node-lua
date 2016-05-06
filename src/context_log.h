#ifndef CONTEXT_LOG_H_
#define CONTEXT_LOG_H_

#include "common.h"
#include "request.h"
#include "context.h"

class context_log_t : public context_t {
public:
	bool init(int32_t argc, char* argv[], char* env[]);
	bool deinit(const char *arg);
	void on_received(message_t& message);
	void on_dropped(message_t& message);
};

#endif