#ifndef LSHARED_H_
#define LSHARED_H_
#include "common.h"

#define SHARED_METATABLE		"class shared_t"

class shared_t {
public:
	shared_t() : m_ref(1), m_meta_reg(NULL) {}
	shared_t(const luaL_Reg reg[]) : m_ref(1), m_meta_reg(NULL) {
		if (sizeof(reg) > 0) {
			m_meta_reg = new luaL_Reg[ARRAY_SIZE(reg)];
			memcpy((void*)m_meta_reg, reg, sizeof(reg));
		}
	}

	FORCE_INLINE shared_t* grab() { atomic_inc(m_ref); return this; }
	FORCE_INLINE void release() { if (atomic_dec(m_ref) == 1) delete this; }

protected:
	shared_t(const shared_t& shared);
	shared_t& operator=(const shared_t& shared);

	virtual ~shared_t(){
		if (m_meta_reg != NULL) {
			delete[] m_meta_reg;
			m_meta_reg = NULL;
		}
	}

private:
	static int meta_index(lua_State *L);
	static int meta_gc(lua_State *L);

public:
	static shared_t** create(lua_State* L);
	static shared_t** create(lua_State* L, shared_t* shared);

private:
	atomic_t m_ref;
	const luaL_Reg* m_meta_reg;
};

#endif
