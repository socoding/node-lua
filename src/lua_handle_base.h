#ifndef LUA_HANDLE_BASE_H_
#define LUA_HANDLE_BASE_H_
#include "common.h"
#include "handle_base_def.h"

class uv_handle_base_t;

class lua_handle_base_t
{
protected:
	lua_handle_base_t(uv_handle_base_t* handle, lua_State* L);
	virtual ~lua_handle_base_t() {};

public:
	FORCE_INLINE int32_t get_handle_ref() const { return m_lua_ref; };
	FORCE_INLINE bool is_closed() const { return m_uv_handle == NULL; };
	FORCE_INLINE handle_set get_handle_set() const { return m_handle_set; };
	uv_handle_type get_uv_handle_type() const;
	virtual bool set_option(uint8_t type, const char* option, uint8_t option_len);
	static int32_t close(lua_State* L);
	static int32_t release(lua_State* L);
	static int32_t is_closed(lua_State* L);

public:
	static void push_ref_table(lua_State* L, handle_set type);
	static int32_t grab_ref(lua_State* L, lua_handle_base_t* handle, handle_set type);
	static void release_ref(lua_State* L, int32_t ref, handle_set type);
	static lua_handle_base_t* get_lua_handle(lua_State* L, int32_t ref, handle_set type);
	static void close_uv_handle(uv_handle_base_t* handle);

protected:
	void _close();

	const handle_set m_handle_set;
	int32_t m_lua_ref;
	uv_handle_base_t* m_uv_handle;

public:
	static char m_lua_ref_table_key;
};

#endif
