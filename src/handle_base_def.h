#ifndef HANDLE_BASE_DEF
#define HANDLE_BASE_DEF

typedef enum
{
	NONE_SET = 0,
	SOCKET_SET = 1,
	TIMER_SET = 2,
} handle_set;

#define SOCKET_MAKE_FD(lua_ref, context_id) ((int64_t)(lua_ref)%1000000 + 1000000*(int64_t)(context_id))

#endif