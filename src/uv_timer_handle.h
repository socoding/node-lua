#ifndef UV_TIMER_HANDLE_H_
#define UV_TIMER_HANDLE_H_
#include "uv_handle_base.h"
#include "request.h"

class uv_timer_handle_t : public uv_handle_base_t
{
public:
	uv_timer_handle_t(uint32_t source) : uv_handle_base_t(UV_TIMER, source) {}

public:
	void start(uv_loop_t* loop, request_timer_start_t& request);
	static void on_timedout(uv_timer_t* handle, int status);
};

#endif