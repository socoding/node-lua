#ifndef LSHARED_H_
#define LSHARED_H_
#include "common.h"

#define SHARED_METATABLE		"class shared_t"

class shared_t {
public:
	shared_t() : m_ref(1) {}

	FORCE_INLINE shared_t* grab() { atomic_inc(m_ref); return this; }
	FORCE_INLINE void release() { if (atomic_dec(m_ref) == 1) delete this; }

protected:
	shared_t(const shared_t& shared);
	shared_t& operator=(const shared_t& shared);

	virtual ~shared_t() = 0;

private:
	static int meta_index(lua_State *L);
	static int meta_gc(lua_State *L);
	virtual lua_CFunction index_function(const char* name) { return NULL; }

public:
	static shared_t** create(lua_State* L);
	static shared_t** create(lua_State* L, shared_t* shared);

private:
	atomic_t m_ref;
};

#endif
