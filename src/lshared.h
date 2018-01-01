#ifndef LSHARED_H_
#define LSHARED_H_
#include "common.h"
#include <typeinfo>

#define SHARED_METATABLE		"class shared_t"

class shared_t {
public:
	shared_t() : m_ref(1) {}

	FORCE_INLINE shared_t* grab() { atomic_inc(m_ref); return this; }
	FORCE_INLINE void release() { if (atomic_dec(m_ref) == 1) delete this; }
	FORCE_INLINE bool writeable() { return m_ref == 1; }

protected:
	shared_t(const shared_t& shared);
	shared_t& operator=(const shared_t& shared);

	virtual ~shared_t() {};

private:
	static int meta_index(lua_State *L);
	static int meta_gc(lua_State *L);
	
	/*
	* index_function must return a multi-thread safe lua_CFunction on shared_t object.
	* It's better all operations on the shared_t object is read-only or thread safe.
	* index_function is not essential for the derived class unless you want a meta-method for your userdata,
	* so you can choose whether overwriting this function in the derived class.
	**/
	virtual lua_CFunction index_function(const char* name) { return NULL; }

public:
	static shared_t** create(lua_State* L);
	static shared_t** create(lua_State* L, shared_t* shared);
	
	template < class type >
	static type* check_shared(lua_State *L, int idx, bool writable)
	{
		shared_t* shared = *(shared_t**)luaL_checkudata(L, idx, SHARED_METATABLE);
		if (shared == NULL) {
			luaL_argerror(L, 1, "shared_t invalid");
		}
		if (writable && !shared->writeable()) {
			luaL_argerror(L, 1, "shared_t not writable");
		}
		type* p = dynamic_cast<type*>(shared);
		if (!p) {
			luaL_error(L, "bad argument #%d (%s expected, %s found)", idx, typeid(type).name(), typeid(*shared).name());
		}
		return p;
	}

private:
	atomic_t m_ref;
};

#endif
