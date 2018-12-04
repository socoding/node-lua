#include "common.h"
#include "context_lua.h"

static int32_t coresume_continue(lua_State *L, int status, lua_KContext ctx);

static int auxresume (lua_State *L, lua_State *co, int narg) {
	int status, stacks;
	int ismain = lua_pushthread(L);
	lua_pop(L, 1);
	if (ismain) {
		lua_pushliteral(L, "cannot call resume in main thread");
		return -1;  /* error flag */
	}
	if (!lua_checkstack(co, narg)) {
		lua_pushliteral(L, "too many arguments to resume");
		return -1;  /* error flag */
	}
	if (lua_status(co) == LUA_OK && lua_gettop(co) == 0) {
		lua_pushliteral(L, "cannot resume dead coroutine");
		return -1;  /* error flag */
	}
	lua_xmove(L, co, narg);
	context_lua_t *lctx = context_lua_t::lua_get_context(L);
	while (((status = lua_resume(co, L, narg)) == LUA_YIELD) && lctx->yielding_up()) { /* can happen only when status == LUA_YIELD */
		if ((stacks = lua_checkstack(L, LUA_MINSTACK)) && lua_isyieldable(L)) { /* reserve LUA_MINSTACK stack for resume result */
			return lua_yieldk(L, 0, 0, coresume_continue); /* yield again! n(0) result to yield and ctx(0) is not allowed */
		} else { /* lua_gettop(co) must be 0 since context_lua_t::lua_yield_send yield 0 result */
			int yieldup_status = stacks ? NL_EYIELD : NL_ESTACKLESS;
			lctx->yielding_finalize(NULL, yieldup_status);
			lua_pushboolean(co, 0); /* stack on co must be enough since we have checked it at last yield */
			lua_pushinteger(co, yieldup_status);
			lua_pushnil(co);
			lua_pushnil(co);
			narg = 4;
		}
	}
	if (status == LUA_OK || status == LUA_YIELD) {
		int nres = lua_gettop(co);
		if (!lua_checkstack(L, nres + 1)) {
			lua_pop(co, nres);  /* remove results anyway */
			lua_pushliteral(L, "too many results to resume");
			return -1;  /* error flag */
		}
		lua_xmove(co, L, nres);  /* move yielded values */
		return nres;
	} else {
		lua_xmove(co, L, 1);  /* move error message */
		return -1;  /* error flag */
	}
}

static int lua_coresume (lua_State *L) {
	lua_State *co = lua_tothread(L, 1);
	int r;
	luaL_argcheck(L, co, 1, "coroutine expected");
	r = auxresume(L, co, lua_gettop(L) - 1);
	if (r < 0) {
		lua_pushboolean(L, 0);
		lua_insert(L, -2);
		return 2;  /* return false + error message */
	}
	else {
		lua_pushboolean(L, 1);
		lua_insert(L, -(r + 1));
		return r + 1;  /* return true + `resume' returns */
	}
}

static int32_t coresume_continue(lua_State *L, int status, lua_KContext ctx)
{
	return lua_coresume(L);
}

static int lua_auxwrap (lua_State *L) {
	lua_State *co = lua_tothread(L, lua_upvalueindex(1));
	int r = auxresume(L, co, lua_gettop(L));
	if (r < 0) {
		if (lua_isstring(L, -1)) {  /* error object is a string? */
			luaL_where(L, 1);  /* add extra info */
			lua_insert(L, -2);
			lua_concat(L, 2);
		}
		return lua_error(L);  /* propagate error */
	}
	return r;
}


static int lua_cocreate (lua_State *L) {
	lua_State *NL;
	luaL_checktype(L, 1, LUA_TFUNCTION);
	NL = lua_newthread(L);
	lua_pushvalue(L, 1);  /* move function to top */
	lua_xmove(L, NL, 1);  /* move function from L to NL */
	return 1;
}


static int lua_cowrap (lua_State *L) {
	lua_cocreate(L);
	lua_pushcclosure(L, lua_auxwrap, 1);
	return 1;
}


static int lua_coyield (lua_State *L) {
	lua_pushlightuserdata(L, &context_lua_t::m_root_coro_table_key);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_pushthread(L);
	lua_rawget(L, -2);
	int ret = lua_toboolean(L, -1);
	lua_pop(L, 2);
	if (!ret) {
		return lua_yield(L, lua_gettop(L));
	} else {  /* is root coroutine */
		return luaL_error(L, "attempt to yield from root coroutine");
	}
}

static int lua_costatus (lua_State *L) {
	lua_State *co = lua_tothread(L, 1);
	luaL_argcheck(L, co, 1, "coroutine expected");
	if (L == co) lua_pushliteral(L, "running");
	else {
		switch (lua_status(co)) {
	  case LUA_YIELD:
		  lua_pushliteral(L, "suspended");
		  break;
	  case LUA_OK: {
		  lua_Debug ar;
		  if (lua_getstack(co, 0, &ar) > 0)  /* does it have frames? */
			  lua_pushliteral(L, "normal");  /* it is running */
		  else if (lua_gettop(co) == 0)
			  lua_pushliteral(L, "dead");
		  else
			  lua_pushliteral(L, "suspended");  /* initial state */
		  break;
				   }
	  default:  /* some error occurred */
		  lua_pushliteral(L, "dead");
		  break;
		}
	}
	return 1;
}


static int lua_corunning (lua_State *L) {
	int ismain = lua_pushthread(L);
	lua_pushboolean(L, ismain);
	return 2;
}


static const luaL_Reg co_funcs[] = {
	{"create", lua_cocreate},
	{"resume", lua_coresume},
	{"running", lua_corunning},
	{"status", lua_costatus},
	{"wrap", lua_cowrap},
	{"yield", lua_coyield},
	{NULL, NULL}
};

LUAMOD_API int luaopen_my_coroutine (lua_State *L) {
	luaL_newlib(L, co_funcs);
	return 1;
}
