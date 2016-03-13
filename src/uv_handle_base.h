#ifndef UV_HANDLE_BASE_H_
#define UV_HANDLE_BASE_H_
#include "common.h"
#include "handle_base_def.h"

class lua_handle_base_t;

class uv_handle_base_t
{
public:
	uv_handle_base_t(uv_loop_t* loop, uv_handle_type type, uint32_t source);
	uv_handle_base_t(uv_handle_type type, uint32_t source);
	virtual ~uv_handle_base_t();
	virtual void set_option(uint8_t type, const char* option) {};
	void init(uv_loop_t* loop);
	handle_set get_handle_set() const;
	uv_handle_type get_handle_type() const { return m_handle_type; };
	static void on_closed(uv_handle_t* handle);
	void close();
	friend class lua_handle_base_t;
protected:
	uv_handle_type m_handle_type;
	uint32_t     m_source;
	int32_t      m_lua_ref;
	uv_handle_t* m_handle;
};

#endif
