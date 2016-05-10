#include "common.h"
#include "utils.h"
#include "lbuffer.h"
#include "context.h"
#include "context_lua.h"
#include "node_lua.h"
#include "worker.h"
#include "uv_handle_base.h"
#include "lua_tcp_handle.h"
#include "lua_udp_handle.h"
#include "lua_timer_handle.h"
#include "errors.h"


char context_lua_t::m_root_coro_table_key = 0;
char context_lua_t::m_ref_session_table_key = 0;
char context_lua_t::m_ref_timer_table_key = 0;

context_lua_t::context_lua_t() 
	: m_lstate(NULL),
	  m_ref_session_count(0),
	  m_ref_session_base(INT_MAX)
{
}

context_lua_t::~context_lua_t()
{
	if (m_lstate) {
		lua_close(m_lstate);
		m_lstate = NULL;
	}
}

bool context_lua_t::init(int32_t argc, char* argv[], char* env[])
{
	m_lstate = luaL_newstateex(this);
	if (argc <= 0) {
		singleton_ref(node_lua_t).log_fmt(m_handle, "[error] context:0x%08x init failed: lua file is needed to initialize lua context", m_handle);
		return false;
	}
	int32_t envc = 2;
	if (argc > MAX_CTX_ARGC || !lua_checkstack(m_lstate, argc + envc)) {
		singleton_ref(node_lua_t).log_fmt(m_handle, "[error] context:0x%08x init failed: too many arguments to initialize lua context %s", m_handle, argv[0]);
		return false;
	}
	int32_t total_length = 0;
	int32_t lengths[MAX_CTX_ARGC];
	for (int32_t i = 0; i < argc; ++i) {
		lengths[i] = strlen(argv[i]);
		lua_pushlstring(m_lstate, argv[i], lengths[i]);
		total_length += lengths[i] + 1;
	}
	char** envp = env;
	char* lua_path = NULL;
	char* lua_cpath = NULL;
	while (*envp) {
		if (strncmp(*envp, "LUA_PATH=", 9) == 0) {
			lua_path = *envp + 9;
		} else if (strncmp(*envp, "LUA_CPATH=", 10) == 0) {
			lua_cpath = *envp + 10;
		}
		++envp;
	}
	lua_pushstring(m_lstate, lua_path);
	lua_pushstring(m_lstate, lua_cpath);
	char* args = (char*)nl_calloc(total_length);
	char* argp = args;
	for (int32_t i = 0; i < argc; ++i) {
		memcpy(argp, argv[i], lengths[i]);
		argp += lengths[i];
		*argp++ = (i != argc - 1) ? ' ' : '\0';
	}
	singleton_ref(node_lua_t).log_fmt(m_handle, "[alert] context:0x%08x init: %s", m_handle, args);
	nl_free(args);
	return singleton_ref(node_lua_t).context_send(this, m_handle, 0, LUA_CTX_INIT, (int64_t)argc);
}

bool context_lua_t::deinit(const char *arg)
{
	for (std::set<uint32_t>::iterator it = m_context_wait_handles.begin(); it != m_context_wait_handles.end(); ++it) {
		singleton_ref(node_lua_t).context_send(*it, m_handle, LUA_REFNIL, LUA_CTX_WAKEUP, UV_OK);
	}
	m_context_wait_handles.clear();
	if (m_lstate) {
		lua_close(m_lstate);
		m_lstate = NULL;
	}
	singleton_ref(node_lua_t).log_fmt(m_handle, "[alert] context:0x%08x deinit: %s", m_handle, arg);
	return true;
}

void context_lua_t::on_worker_attached()
{
	m_shared = m_worker->m_context_lua_shared;
	m_shared->m_yielding_destination = 0;
	m_shared->m_yielding_depth = 0;
	m_shared->m_yielding_finalize = NULL;
	m_shared->m_yielding_finalize_userdata = NULL;
	m_shared->m_yielding_status = UV_OK;
	m_shared->m_yielding_timeout = 0;
}

void context_lua_t::on_worker_detached()
{
	m_shared = NULL;
}

#define LUA_COLIBNAME		"coroutine"
LUAMOD_API int (luaopen_my_coroutine)(lua_State *L);
#define LUA_TCPLIBNAME		"tcp"
LUAMOD_API int (luaopen_tcp)(lua_State *L);
#define LUA_UDPLIBNAME		"udp"
LUAMOD_API int (luaopen_udp)(lua_State *L);
#define LUA_BUFFERLIBNAME	"buffer"
LUAMOD_API int (luaopen_buffer)(lua_State *L);
#define LUA_CONTEXTLIBNAME	"context"
LUAMOD_API int (luaopen_context)(lua_State *L);
#define LUA_TIMERLIBNAME	"timer"
LUAMOD_API int (luaopen_timer)(lua_State *L);
#define LUA_BSONLIBNAME	    "bson"
LUAMOD_API int (luaopen_bson)(lua_State *L);

void context_lua_t::lua_open_libs(lua_State *L)
{
	static const luaL_Reg loadedlibs[] = {
		{"_G", luaopen_base},
		{LUA_LOADLIBNAME, luaopen_package},
		{LUA_COLIBNAME, luaopen_my_coroutine},
		{LUA_TABLIBNAME, luaopen_table},
		{LUA_IOLIBNAME, luaopen_io},
		{LUA_OSLIBNAME, luaopen_os},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_BITLIBNAME, luaopen_bit32},
		{LUA_MATHLIBNAME, luaopen_math},
		{LUA_DBLIBNAME, luaopen_debug},
#if defined(LUA_CACHELIB)
		{LUA_CACHELIB, luaopen_cache},
#endif
		{LUA_TCPLIBNAME, luaopen_tcp},
		{LUA_UDPLIBNAME, luaopen_udp},
		{LUA_BUFFERLIBNAME, luaopen_buffer},
		{LUA_CONTEXTLIBNAME, luaopen_context},
		{LUA_TIMERLIBNAME, luaopen_timer},
		{LUA_BSONLIBNAME, luaopen_bson},
		{NULL, NULL}
	};
	static const luaL_Reg preloadedlibs[] = {
		{ NULL, NULL }
	};
	const luaL_Reg *lib;
	/* call open functions from 'loadedlibs' and set results to global table */
	for (lib = loadedlibs; lib->func; lib++) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}
	/* add open functions from 'preloadedlibs' into 'package.preload' table */
	luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");
	for (lib = preloadedlibs; lib->func; lib++) {
		lua_pushcfunction(L, lib->func);
		lua_setfield(L, -2, lib->name);
	}
	lua_pop(L, 1);  /* remove _PRELOAD table */
}

void context_lua_t::unload()
{
#if defined(LUA_CACHELIB)
	luaL_closesharedclosure();
#endif
}

void context_lua_t::lua_load_env(lua_State *L, char* env[])
{
	lua_checkstack(L, 3);
	luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
	lua_getfield(L, -1, LUA_LOADLIBNAME);  /* _LOADED[name] */
	if (lua_istable(L, -1)) {
		char** envp = env;
		lua_getfield(L, -1, "path");
		size_t path_len;
		const char* lua_path = lua_tolstring(L, -1, &path_len);
		if (lua_path != NULL && path_len > 0) {
			char* lua_path_env = (char*)nl_malloc(path_len + 10);
			strcpy(lua_path_env, "LUA_PATH=");
			strcpy(lua_path_env + 9, lua_path);
			*envp++ = lua_path_env;
		}
		lua_pop(L, 1);
		lua_getfield(L, -1, "cpath");
		size_t cpath_len;
		const char* lua_cpath = lua_tolstring(L, -1, &cpath_len);
		if (lua_cpath != NULL && cpath_len > 0) {
			char* lua_cpath_env = (char*)nl_malloc(cpath_len + 11);
			strcpy(lua_cpath_env, "LUA_CPATH=");
			strcpy(lua_cpath_env + 10, lua_cpath);
			*envp++ = lua_cpath_env;
		}
		lua_pop(L, 3);
		return;
	}
	lua_pop(L, 2);
	return;
}

void context_lua_t::lua_init_env(lua_State *L, int32_t envc)
{
	if (envc <= 0) return;
	const char* path = lua_tostring(L, -envc);
	const char* cpath = lua_tostring(L, -envc + 1);
	if (path == NULL && cpath == NULL) {
		lua_pop(L, envc);
		return;
	}
	lua_checkstack(L, 3);
	luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
	lua_getfield(L, -1, LUA_LOADLIBNAME);  /* _LOADED[name] */
	if (lua_istable(L, -1)) {
		if (path) {
			lua_pushvalue(L, -envc - 2);
			lua_setfield(L, -2, "path");
		}
		if (cpath) {
			lua_pushvalue(L, -envc - 1);
			lua_setfield(L, -2, "cpath");
		}
	}
	lua_pop(L, envc + 2);
	return;
}

void context_lua_t::lua_free_env(lua_State *L, char* env[])
{
	char** envp = env;
	while (*envp) {
		nl_free((void*)*envp);
		*envp++ = NULL;
	}
}

void context_lua_t::lua_init(lua_State *L)
{
	lua_open_libs(L);
	lua_pushlightuserdata(L, &m_root_coro_table_key);  /* create root coro table */
	lua_createtable(L, 0, 4);
	lua_rawset(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, &m_ref_session_table_key);
	lua_createtable(L, 4, 0);
	lua_rawset(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, &m_ref_timer_table_key);
	lua_createtable(L, 4, 0);
	lua_rawset(L, LUA_REGISTRYINDEX);

}

lua_State* context_lua_t::lua_new_root_coro(lua_State *L)
{
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	if (!lctx->m_cached_root_coros.empty()) {
		lua_State *co = lctx->m_cached_root_coros.back();
		lctx->m_cached_root_coros.pop_back();
		return co;
	} 
	lua_checkstack(L, 3);
	lua_pushlightuserdata(L, &m_root_coro_table_key);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_State *co = lua_newthread(L);
	lua_pushboolean(L, 1);
	lua_rawset(L, -3);
	lua_pop(L, 1);	/* pop root_coro_table */
	return co;
}

void context_lua_t::lua_free_root_coro(lua_State *co)
{
	context_lua_t* lctx = context_lua_t::lua_get_context(co);
	if (lua_status(co) == LUA_OK && lctx->m_cached_root_coros.size() < CONTEXT_ROOT_CORO_CACHE_MAX) {
		lctx->m_cached_root_coros.push_back(co);
		return;
	}
	lua_checkstack(co, 3);
	lua_pushlightuserdata(co, &m_root_coro_table_key);
	lua_rawget(co, LUA_REGISTRYINDEX);
	lua_pushthread(co);
	lua_pushnil(co);
	lua_rawset(co, -3);
	lua_pop(co, 1);	/* pop root_coro_table */
}

int32_t context_lua_t::lua_ref_yield_coroutine(lua_State *co)
{
	lua_checkstack(co, 2);
	lua_pushlightuserdata(co, &m_ref_session_table_key);
	lua_rawget(co, LUA_REGISTRYINDEX);
	lua_pushthread(co);
	int32_t session = lua_ref_session(co, -2);
	lua_pop(co, 1); /* pop ref_session_table */
	++lua_get_context(co)->m_ref_session_count;
	return session;
}

int32_t context_lua_t::lua_ref_timer(lua_State *L, int32_t session, uint64_t timeout, uint64_t repeat, bool is_cancel, void *userdata, timeout_callback_t callback)
{
	lua_checkstack(L, 2);
	lua_pushlightuserdata(L, &m_ref_timer_table_key);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_timer_handle_t* timer = lua_timer_handle_t::create_timer(L, session, timeout, repeat, is_cancel, userdata, callback);
	lua_rawseti(L, -2, session);
	lua_pop(L, 1); /* pop ref_timer_table */
	return timer->get_handle_ref();
}

void context_lua_t::lua_unref_timer(lua_State *L, int32_t session)
{
	lua_checkstack(L, 2);
	lua_pushlightuserdata(L, &m_ref_timer_table_key);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_rawgeti(L, -1, session);
	lua_timer_handle_t* timer = (lua_timer_handle_t*)lua_touserdata(L, -1);
	if (timer) {
		timer->close();
		lua_pop(L, 1);
		lua_pushnil(L);
		lua_rawseti(L, -2, session);
		lua_pop(L, 1);
	} else {
		lua_pop(L, 2);
	}
}

int32_t context_lua_t::yielding_finalize(lua_State *root_coro, int32_t status)
{
	if (!root_coro) {
		ASSERT(status != UV_OK);	/* error occurs directly, status mustn't be UV_OK! */
		m_shared->m_yielding_status = status;
	}
	if (m_shared->m_yielding_finalize != NULL) {
		int32_t fnz_status = m_shared->m_yielding_finalize(root_coro, m_lstate, m_shared->m_yielding_finalize_userdata, m_shared->m_yielding_destination);
		if (root_coro) m_shared->m_yielding_status = fnz_status;
		m_shared->m_yielding_finalize = NULL;
	}
	m_shared->m_yielding_depth = 0;
	m_shared->m_yielding_destination = 0;
	m_shared->m_yielding_finalize_userdata = NULL;
	m_shared->m_yielding_timeout = 0;
	return m_shared->m_yielding_status;
}

int32_t context_lua_t::common_yield_continue(lua_State* L, int status, lua_KContext ctx)
{
	int32_t old_top = (int32_t)ctx;
	if (!lua_toboolean(L, old_top + 1) && context_lua_t::lua_get_context(L)->get_yielding_status() != UV_OK) {
		context_lua_t::lua_throw(L, old_top + 2);
	}
	lua_pop(L, 2); //pop out two useless arguments
	return lua_gettop(L) - old_top;
}

int32_t context_lua_t::lua_yield_send(lua_State *L, uint32_t destination, yiled_finalize_t fnz, void* fnz_ud, lua_KFunction yieldk, int64_t timeout)
{
	int stacks;
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	lctx->m_shared->m_yielding_destination = destination;
	lctx->m_shared->m_yielding_depth = 1;
	lctx->m_shared->m_yielding_finalize = fnz; /* fnz should not be null better */
	lctx->m_shared->m_yielding_finalize_userdata = fnz_ud;
	lctx->m_shared->m_yielding_status = UV_OK;
	lctx->m_shared->m_yielding_timeout = timeout;
	if ((stacks = lua_checkstack(L, LUA_MINSTACK)) && lua_yieldable(L)) { /* reserve LUA_MINSTACK stack for resume result */
		return lua_yieldk(L, 0, (lua_KContext)lua_gettop(L), yieldk);
	} else {
		if (stacks) { /* stack enough, but not yield-able */
			lctx->yielding_finalize(NULL, NL_EYIELD);
			int32_t top = lua_gettop(L);
			lua_pushboolean(L, 0);
			lua_pushinteger(L, NL_EYIELD);
			lua_pushnil(L);
			lua_pushnil(L);
			return (yieldk == NULL) ? 4 : yieldk(L, LUA_YIELD, (lua_KContext)top);
		} else { /* error jump directly */
			lctx->yielding_finalize(NULL, NL_ESTACKLESS);
			return luaL_error(L, "attempt to yield across a stack-less coroutine");
		}
	}
}

int32_t context_lua_t::common_callback_adjust(lua_State* L)
{
	lua_pop(L, 2);
	return 2;
}

int32_t context_lua_t::lua_ref_callback_entry_finish(lua_State *L, int32_t status)
{
	if (!status) { /* error message is at top */
		uint32_t handle = lua_get_context_handle(L);
		const char* error = lua_tostring(L, -1);
		singleton_ref(node_lua_t).log_fmt(handle, "[error] context:0x%08x error: %s", handle, error ? error : "unknown error");
	}
	return 0; //return none result out!!!
}

int32_t context_lua_t::lua_ref_callback_entry_continue(lua_State *L, int status, lua_KContext ctx)
{
	return lua_ref_callback_entry_finish(L, (status == LUA_YIELD));
}

int32_t context_lua_t::lua_ref_callback_error_handler(lua_State *L)
{
	const char *msg = lua_tostring(L, 1);
	if (msg == NULL) {  /* is error object not a string? */
		if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
			lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
			return 1;  /* that is the message */
		else
			msg = lua_pushfstring(L, "(error object is a %s value)",
			luaL_typename(L, 1));
	}
	luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
	return 1;  /* return the traceback */
}

int32_t context_lua_t::lua_ref_callback_entry(lua_State *L)
{
	int upvalue_count = 0;
	while (!lua_isnone(L, lua_upvalueindex(upvalue_count + 1))) {
		++upvalue_count;
	}
	ASSERT(upvalue_count > 1);
	lua_CFunction adjust_resume_args = (lua_CFunction)lua_touserdata(L, lua_upvalueindex(upvalue_count));
	if (adjust_resume_args) {
		adjust_resume_args(L);
	}
	--upvalue_count;
	int resume_count = lua_gettop(L); /* record resume arg count */
	lua_checkstack(L, upvalue_count + 1); /* reserve `upvalue_count + 1` stack size */
	lua_pushcclosure(L, lua_ref_callback_error_handler, 0);
	lua_insert(L, 1); /* put error handler function at bottom (the `1st` arg) */
	lua_pushvalue(L, lua_upvalueindex(upvalue_count));
	lua_insert(L, 2); /* put callback closure as the first arg behind error handler */
	for (int i = 1; i < upvalue_count; ++i) { /* put `upvalue_count - 1` args */
		lua_pushvalue(L, lua_upvalueindex(i));
	}
	/* lua_callback_entry_continue makes the coroutine can be recovered. */
	int status = lua_pcallk(L, upvalue_count + resume_count - 1, LUA_MULTRET, 1, 0, lua_ref_callback_entry_continue);
	return lua_ref_callback_entry_finish(L, (status == LUA_OK));
}

int32_t context_lua_t::lua_ref_session(lua_State *L, int32_t idx)
{
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);  /* remove from stack */
		return LUA_REFNIL;  /* `nil' has a unique fixed reference */
	}
	context_lua_t* lctx = lua_get_context(L);
	idx = lua_absindex(L, idx);
	for (int i = 0; i < INT_MAX; ++i) {
		if (lctx->m_ref_session_base > 0) {
			lua_rawgeti(L, idx, lctx->m_ref_session_base);
			if (lua_isnil(L, -1)) {
				lua_pop(L, 1);
				lua_rawseti(L, idx, lctx->m_ref_session_base);
				return lctx->m_ref_session_base--;
			}
			lua_pop(L, 1);
			--lctx->m_ref_session_base;
		} else {
			lctx->m_ref_session_base = INT_MAX;
		}
	}
	lua_pop(L, 1);  /* remove from stack */
	return LUA_REFNIL;  /* `nil' has a unique fixed reference */
}

void context_lua_t::lua_unref_session(lua_State *L, int32_t idx, int32_t session)
{
	if (session != LUA_REFNIL) {
		idx = lua_absindex(L, idx);
		lua_pushnil(L);
		lua_rawseti(L, idx, session);
	}
}

/* ref a new callback (remove argc args and the top closure) */
int32_t context_lua_t::lua_ref_callback(lua_State *L, uint32_t argc, int32_t ref, lua_CFunction adjust_resume_args)
{
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_checkstack(L, 2);
	lua_pushlightuserdata(L, (void*)adjust_resume_args);
	lua_pushcclosure(L, lua_ref_callback_entry, argc + 2); /* make the c closure callback */
	lua_pushlightuserdata(L, &m_ref_session_table_key);
	lua_rawget(L, LUA_REGISTRYINDEX);	/* fetch callback_func_table */
	lua_pushvalue(L, -2);				/* put the c closure callback at top */
	if (ref == LUA_REFNIL) {
		ref = lua_ref_session(L, -2);
		++lua_get_context(L)->m_ref_session_count;
		lua_pop(L, 2);					/* pop callback_func_table and the c closure callback */
	} else {
		lua_rawseti(L, -2, ref);
		lua_pop(L, 2);					/* pop callback_func_table and the c closure callback */
		lua_unref_timer(L, ref);
	}
	return ref;
}

context_lua_t* context_lua_t::lua_get_context(lua_State *L)
{
	context_lua_t* ctx;
	lua_getallocf(L, (void**)&ctx);
	return ctx;
}

uint32_t context_lua_t::lua_get_context_handle(lua_State *L)
{
	return lua_get_context(L)->m_handle;
}

int32_t context_lua_t::lua_strerror(lua_State *L, int32_t idx)
{
	lua_checkstack(L, 1);
	lua_pushstring(L, common_strerror(lua_tointeger(L, idx)));
	return 1;
}

void context_lua_t::lua_throw(lua_State *L, int32_t idx)
{
	lua_strerror(L, idx);
	lua_error(L);
}

///////////////////////////////////////////////////////////////////////////////////////////
void context_lua_t::lua_push_ref_session(lua_State *L, int32_t ref)
{
	if (ref != LUA_REFNIL) {
		lua_checkstack(L, 2);
		lua_pushlightuserdata(L, &m_ref_session_table_key);
		lua_rawget(L, LUA_REGISTRYINDEX);
		lua_rawgeti(L, -1, ref);
		lua_remove(L, -2);
	} else {
		lua_pushnil(L);
	}
}

void context_lua_t::lua_free_ref_session(lua_State *L, int32_t ref)
{
	if (ref != LUA_REFNIL) {
		lua_checkstack(L, 1);
		lua_pushlightuserdata(L, &m_ref_session_table_key);
		lua_rawget(L, LUA_REGISTRYINDEX);
		lua_unref_session(L, -1, ref);
		lua_pop(L, 1); /* pop ref_session_table and top coroutine */
		--lua_get_context(L)->m_ref_session_count;
		lua_unref_timer(L, ref);
	}
}

void context_lua_t::lua_pushmessage(lua_State *L, message_t& message)
{
	message_array_t *array = NULL;
	switch (message_data_type(message)) {
	case NIL:
		lua_pushnil(L);
		return;
	case TBOOLEAN:
		lua_pushboolean(L, message_bool(message));
		return;
	case USERDATA:
		lua_pushlightuserdata(L, message_userdata(message));
		return;
	case NUMBER:
		lua_pushnumber(L, message_number(message));
		return;
	case INTEGER:
		lua_pushinteger(L, message_integer(message));
		return;
	case BUFFER:
		create_buffer(L, message_buffer(message));
		return;
	case STRING:
		if (message_string(message)) {
			lua_pushstring(L, message_string(message));
		} else {
			lua_pushstring(L, "");
		}
		return;
	case SDS:
		if (message_sds(message)) {
			lua_pushlstring(L, message_sds(message), sdslen(message_sds(message)));
		} else {
			lua_pushstring(L, "");
		}
		return;
	case BSON:
		if (message_bson(message)->extract) {
			bson_decode(message_bson(message), L);
		} else {
			create_bson(message_bson(message), L);
		}
		return;
	case TERROR:
		lua_pushinteger(L, message_error(message));
		return;
	case ARRAY:
		array = message_array(message);
		if (array != NULL && array->m_count > 0) {
			lua_checkstack(L, array->m_count);
			for (int32_t i = 0; i < array->m_count; ++i) { //extract them
				lua_pushmessage(L, array->m_array[i]);
			}
		} else {
			lua_pushnil(L);
		}
		return;
	default:
		lua_pushnil(L);
		return;
	}
}

int32_t context_lua_t::resume_coroutine(lua_State *co, int32_t narg)
{
	int32_t status, yieldup_status;
	while ((status = lua_resume(co, m_lstate, narg)) == LUA_YIELD) {
		ASSERT(yielding_up()); /* must be yielding up(root coroutine mustn't yield directly) */
		if ((yieldup_status = yielding_finalize(co, UV_OK)) == UV_OK) {
			return 1;
		} else {
			lua_pushboolean(co, 0);
			lua_pushinteger(co, yieldup_status);
			lua_pushnil(co);
			lua_pushnil(co);
			narg = 4;
		}
	}
	ASSERT(status == LUA_OK);
	lua_settop(co, 0);
	lua_free_root_coro(co);
	return status == LUA_OK;
}

int32_t context_lua_t::wakeup_ref_session(int32_t ref, message_t& message, bool free_callback)
{
	if (ref != LUA_REFNIL && m_lstate != NULL) {
		lua_push_ref_session(m_lstate, ref);
		int32_t type = lua_type(m_lstate, -1);
		if (type == LUA_TNIL) {
			lua_pop(m_lstate, 1);
			return 0;
		}
		lua_State *co;
		bool blocking = (type == LUA_TTHREAD);
		if (blocking) {
			co = lua_tothread(m_lstate, -1);
			lua_pop(m_lstate, 1);
		} else {
			co = lua_new_root_coro(m_lstate);
			lua_xmove(m_lstate, co, 1);
		}
		if (blocking || free_callback) {
			lua_free_ref_session(m_lstate, ref);
		}
		int32_t oldTop = lua_gettop(co);
		lua_checkstack(co, 2);
		lua_pushboolean(co, !message_is_pure_error(message));
		lua_pushmessage(co, message);
		lua_checkstack(co, 2);
		lua_pushinteger(co, message.m_source);
		lua_pushinteger(co, message.m_session);
		int32_t newTop = lua_gettop(co);
		resume_coroutine(co, newTop - oldTop);
		return 1;
	}
	return 0;

}

void context_lua_t::on_received(message_t& message)
{
	switch (message_raw_type(message)) {
	case LUA_CTX_INIT:
		lua_ctx_init(message);
		return;
	case CONTEXT_QUERY:
		response_context_query(message);
		break;
	case CONTEXT_REPLY:
		response_context_reply(message);
		break;
	case LUA_CTX_WAIT:
		response_context_wait(message);
		break;
	case LUA_CTX_WAKEUP:
		response_context_wakeup(message);
		break;
	case RESPONSE_TCP_LISTEN:
		response_tcp_listen(message);
		break;
	case RESPONSE_TCP_ACCEPT:
		response_tcp_accept(message);
		break;
	case RESPONSE_TCP_CONNECT:
		response_tcp_connect(message);
		break;
	case RESPONSE_TCP_READ:
		response_tcp_read(message);
		break;
	case RESPONSE_TCP_WRITE:
		response_tcp_write(message);
		break;
	case RESPONSE_TCP_CLOSING:
		response_tcp_closing(message);
		break;
	case RESPONSE_UDP_OPEN:
		response_udp_open(message);
		break;
	case RESPONSE_UDP_WRITE:
		response_udp_write(message);
		break;
	case RESPONSE_UDP_READ:
		response_udp_read(message);
		break;
	case RESPONSE_UDP_CLOSING:
		response_udp_closing(message);
		break;
	case RESPONSE_HANDLE_CLOSE:
		response_handle_close(message);
		break;
	case RESPONSE_TIMEOUT:
		response_timeout(message);
		break;
	case SYSTEM_CTX_DESTROY:
		singleton_ref(node_lua_t).context_destroy(this, message.m_source, message_string(message));
		return;
	default:
		break;
	}
	if (!is_active()) {
		singleton_ref(node_lua_t).context_destroy(this, m_handle, "lua context exit normally");
	}
}

void context_lua_t::on_dropped(message_t& message)
{
	switch (message_raw_type(message)) {
	case RESPONSE_TCP_LISTEN:
	case RESPONSE_TCP_ACCEPT:
	case RESPONSE_TCP_CONNECT:
	case RESPONSE_UDP_OPEN:
		if (message_is_userdata(message)) {
			uv_handle_base_t* handle = (uv_handle_base_t*)message_userdata(message);
			lua_handle_base_t::close_uv_handle(handle);
		}
		break;
	case CONTEXT_QUERY:
		if (message.m_source != 0 && message.m_session != LUA_REFNIL) {
			singleton_ref(node_lua_t).context_send(message.m_source, m_handle, message.m_session, CONTEXT_REPLY, NL_ENOREPLY);
		}
		return;
	case LUA_CTX_WAIT:
		singleton_ref(node_lua_t).context_send(message.m_source, (uint32_t)message_integer(message), LUA_REFNIL, LUA_CTX_WAKEUP, UV_OK);
		return;
	default:
		break;
	}
}

void context_lua_t::lua_ctx_init(message_t& message)
{
	int32_t argc = (int32_t)message_integer(message);
	int32_t envc = lua_gettop(m_lstate) - argc;
	const char *file = lua_tostring(m_lstate, 1);
	if (luaL_loadfile(m_lstate, file) == LUA_OK) {
		lua_State *co;
		lua_init(m_lstate);
		lua_insert(m_lstate, -envc - 1);
		lua_init_env(m_lstate, envc);
		co = lua_new_root_coro(m_lstate);
		lua_pushlightuserdata(m_lstate, NULL);
		lua_pushcclosure(m_lstate, lua_ref_callback_entry, argc + 1); /* make the c closure callback */
		lua_xmove(m_lstate, co, 1);  /* move top c closure to co */
		lua_pop(m_lstate, 1);		 /* pop the file name on top */
		resume_coroutine(co, 0);
		if (!is_active()) {
			singleton_ref(node_lua_t).context_destroy(this, m_handle, "lua context exit normally");
		}
		return;
	}
	const char *error = lua_tostring(m_lstate, -1);
	singleton_ref(node_lua_t).context_destroy(this, m_handle, "fail to initialize lua context %s, %s", file, error);
}

void context_lua_t::response_context_query(message_t& response)
{
	if (response.m_source == 0) return;
	if (!m_context_recv_sessions.wakeup_once(response.m_source, this, response) && !m_context_recv_sessions.wakeup_once(0, this, response)) {
		if (response.m_session != LUA_REFNIL) {
			singleton_ref(node_lua_t).context_send(response.m_source, m_handle, response.m_session, CONTEXT_REPLY, NL_ENOREPLY);
		}
	}
}

void context_lua_t::response_context_reply(message_t& response)
{
	wakeup_ref_session(response.m_session, response, true);
}

void context_lua_t::response_context_wait(message_t& response)
{
	m_context_wait_handles.insert(response.m_source);
}

void context_lua_t::response_context_wakeup(message_t& response)
{
	m_context_wait_sessions.wakeup_all(response.m_source, this, response, true);
}

void context_lua_t::response_tcp_listen(message_t& response)
{
	lua_tcp_listen_handle_t::wakeup_listen(m_lstate, response);
}

void context_lua_t::response_tcp_accept(message_t& response)
{
	lua_tcp_listen_handle_t::wakeup_accept(m_lstate, response);
}

void context_lua_t::response_tcp_connect(message_t& response)
{
	lua_tcp_socket_handle_t::wakeup_connect(m_lstate, response);
}

void context_lua_t::response_tcp_write(message_t& response)
{
	wakeup_ref_session(response.m_session, response, true);
}

void context_lua_t::response_tcp_read(message_t& response)
{
	lua_tcp_socket_handle_t::wakeup_read(m_lstate, response);
}

void context_lua_t::response_tcp_closing(message_t& response)
{
	if (message_error(response) == NL_ETCPLCLOSED) { /* listen socket */
		lua_tcp_listen_handle_t::wakeup_accept(m_lstate, response);
	} else {
		lua_tcp_socket_handle_t::wakeup_read(m_lstate, response);
	}
}

void context_lua_t::response_udp_open(message_t& response)
{
	lua_udp_handle_t::wakeup_open(m_lstate, response);
}

void context_lua_t::response_udp_write(message_t& response)
{
	wakeup_ref_session(response.m_session, response, true);
}

void context_lua_t::response_udp_read(message_t& response)
{
	lua_udp_handle_t::wakeup_read(m_lstate, response);
}

void context_lua_t::response_udp_closing(message_t& response)
{
	lua_udp_handle_t::wakeup_read(m_lstate, response);
}

void context_lua_t::response_handle_close(message_t& response)
{
	lua_handle_base_t::release_ref(m_lstate, response.m_session, (handle_set)(int32_t)message_integer(response));
}

void context_lua_t::response_timeout(message_t& response)
{
	lua_timer_handle_t::wakeup(m_lstate, response);
}

////////////////////////////////////////////////context lua api/////////////////////////////////////////////////

int32_t context_lua_t::context_strerror(lua_State *L)
{
	return lua_strerror(L, -1);
}

int32_t context_lua_t::context_create(lua_State *L)
{
	int32_t argc = lua_gettop(L);
	if (argc <= 0) {
		luaL_error(L, "lua file is needed to initialize lua context");
	}
	if (argc > MAX_CTX_ARGC) {
		luaL_error(L, "too many arguments to initialize lua context");
	}
	char* argv[MAX_CTX_ARGC];
	char* env[16] = { NULL };
	lua_load_env(L, env);
	for (int32_t i = 0; i < argc; ++i) {
		argv[i] = (char*)luaL_checkstring(L, i + 1);
	}
	uint32_t parent = lua_get_context_handle(L);
	uint32_t handle = singleton_ref(node_lua_t).context_create<context_lua_t>(parent, argc, argv, env);
	lua_free_env(L, env);
	if (handle > 0) {
		lua_pushinteger(L, handle);
		return 1;
	}
	return 0;
}

int32_t context_lua_t::context_destroy(lua_State *L)
{
	int32_t type = lua_type(L, 1);
	uint32_t src_handle = lua_get_context_handle(L);	
	if (type == LUA_TNIL || type == LUA_TSTRING) { //kill self
		singleton_ref(node_lua_t).context_destroy(src_handle, src_handle, lua_tostring(L, 1));
		return 0;
	}
	uint32_t handle = luaL_checkunsigned(L, 1);
	if (handle == 0 || singleton_ref(node_lua_t).is_handle_illegal(handle)) {
		luaL_error(L, "attempt to destroy an illegal context:0x%08x", handle);
	}
	singleton_ref(node_lua_t).context_destroy(handle, src_handle, lua_tostring(L, 2));
	return 0;
}

int32_t context_lua_t::context_check_message(lua_State *L, int32_t idx, uint32_t msg_type, message_t& message)
{
	buffer_t* buffer;
	bson_t* bson_ptr;
	int32_t* bson_data;
	const char* data;
	uint32_t length;
	switch (lua_type(L, idx)) {
	case LUA_TNUMBER:
		if (lua_isinteger(L, idx)) {
			message.m_type = MAKE_MESSAGE_TYPE(msg_type, INTEGER);
			message.m_data.m_integer = (int64_t)lua_tointeger(L, idx);
		} else {
			message.m_type = MAKE_MESSAGE_TYPE(msg_type, NUMBER);
			message.m_data.m_number = (double)lua_tonumber(L, idx);
		}
		message.m_source = lua_get_context_handle(L);
		return 1;
	case LUA_TSTRING:
		data = lua_tolstring(L, idx, &length);
		message.m_type = MAKE_MESSAGE_TYPE(msg_type, SDS);	
		message.m_data.m_sds = sdsnewlen(data, length);
		message.m_source = lua_get_context_handle(L);
		return 1;
	case LUA_TBOOLEAN:
		message.m_type = MAKE_MESSAGE_TYPE(msg_type, TBOOLEAN);
		message.m_data.m_bool = (bool)lua_toboolean(L, idx);
		message.m_source = lua_get_context_handle(L);
		return 1;
	case LUA_TNIL:
		message.m_type = MAKE_MESSAGE_TYPE(msg_type, NIL);
		message.m_source = lua_get_context_handle(L);
		return 1;
	case LUA_TUSERDATA:
		buffer = (buffer_t*)luaL_testudata(L, idx, BUFFER_METATABLE);
		if (buffer) {
			message.m_type = MAKE_MESSAGE_TYPE(msg_type, BUFFER);
			message.m_data.m_buffer = buffer_grab(*buffer);
			message.m_source = lua_get_context_handle(L);
			return 1;
		}
		bson_data = (int32_t*)luaL_testudata(L, idx, BSON_METATABLE);
		if (bson_data) {
			message.m_type = MAKE_MESSAGE_TYPE(msg_type, BSON);
			message.m_data.m_bson = bson_new(bson_data, false);
			message.m_source = lua_get_context_handle(L);
			return 1;
		}
		lua_pushstring(L, "transfer data type not supported");
		return 0;
	case LUA_TLIGHTUSERDATA:
		message.m_type = MAKE_MESSAGE_TYPE(msg_type, USERDATA);
		message.m_data.m_userdata = (void*)lua_touserdata(L, idx);
		message.m_source = lua_get_context_handle(L);
		return 1;
	case LUA_TTABLE:
		message.m_type = MAKE_MESSAGE_TYPE(msg_type, BSON);
		message.m_data.m_bson = bson_new(NULL, true);
		message.m_source = lua_get_context_handle(L);
		return bson_encode(message.m_data.m_bson, L, idx) ? 1 : 0;
	default:
		lua_pushstring(L, "transfer data type not supported");
		return 0;
	}
}

int32_t context_lua_t::context_check_message(lua_State *L, int32_t idx, uint32_t count, uint32_t msg_type, message_t& message)
{
	if (count <= 1) {
		return context_check_message(L, idx, msg_type, message);
	}
	message_array_t* array = message_array_create(count);
	message.m_type = MAKE_MESSAGE_TYPE(msg_type, ARRAY);
	message.m_data.m_array = array;
	message.m_source = lua_get_context_handle(L);
	for (int32_t i = 0; i < count; ++i) {
		if (context_check_message(L, idx + i, msg_type, array->m_array[i]) == 0) {
			return 0;
		}
	}
	return 1;
}

int32_t context_lua_t::context_send(lua_State *L, int32_t idx, uint32_t count, uint32_t handle, int32_t session, uint32_t msg_type)
{
	message_t message;
	int32_t status = context_check_message(L, idx, count, msg_type, message);
	if (status == 0) {
		message_clean(message);
		return NL_ETRANSTYPE;
	}
	message.m_session = session;
	if (singleton_ref(node_lua_t).context_send(handle, message)) {
		return UV_OK;
	}
	message_clean(message);
	return NL_ENOCONTEXT;
}

int32_t context_lua_t::context_send(lua_State *L)
{
	uint32_t handle = luaL_checkunsigned(L, 1);
	if (handle == 0 || singleton_ref(node_lua_t).is_handle_illegal(handle)) {
		luaL_error(L, "attempt to send to an illegal context:0x%08x", handle);
	}
	int32_t top = lua_gettop(L);
	luaL_checkany(L, 2);
	int32_t ret = context_send(L, 2, top - 1, handle, LUA_REFNIL, CONTEXT_QUERY);
	if (ret == UV_OK) {
		lua_pushboolean(L, 1);
		return 1;
	}
	if (ret == NL_ETRANSTYPE) {
		lua_error(L);
		return 0;
	}
	lua_pushboolean(L, 0);
	lua_pushinteger(L, NL_ENOCONTEXT);
	return 2;
}

int32_t context_lua_t::context_query_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination)
{
	if (root_coro != NULL) {
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		message_t& message = lctx->get_yielding_message();
		message.m_session = context_lua_t::lua_ref_yield_coroutine(root_coro);
		bool ret = singleton_ref(node_lua_t).context_send(destination, message);
		if (ret) {
			int64_t timeout = lctx->get_yielding_timeout();
			if (timeout > 0) {
				context_lua_t::lua_ref_timer(main_coro, message.m_session, timeout, 0, false);
			}
			return UV_OK;
		} else {
			lua_free_ref_session(main_coro, message.m_session);
			message_clean(message);
			return NL_ENOCONTEXT;
		}
	} else {
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		message_t& message = lctx->get_yielding_message();
		message_clean(message);
	}
	return UV_OK;
}

int32_t context_lua_t::context_query_yield_continue(lua_State* L, int status, lua_KContext ctx)
{
	int32_t old_top = (int32_t)ctx;
	if (!lua_toboolean(L, old_top + 1)) {
		int32_t ret = context_lua_t::lua_get_context(L)->get_yielding_status();
		if (ret != UV_OK && ret != NL_ENOCONTEXT) {
			context_lua_t::lua_throw(L, old_top + 2);
		}
	}
	lua_pop(L, 2); //pop two useless arguments
	return lua_gettop(L) - old_top;
}

int32_t context_lua_t::context_query(lua_State *L, bool timed_query)
{
	uint32_t handle = luaL_checkunsigned(L, 1);
	if (handle == 0 || singleton_ref(node_lua_t).is_handle_illegal(handle)) {
		luaL_error(L, "attempt to query to an illegal context:0x%08x", handle);
	}
	int32_t top = lua_gettop(L);
	uint64_t timeout = 0;
	if (timed_query) {
		timeout = 1000 * luaL_checknumber(L, 2);
		luaL_checkany(L, 3);
	} else {
		luaL_checkany(L, 2);
	}
	bool callback = lua_isfunction(L, -1);
	int32_t idx = timed_query ? 3 : 2;
	int32_t count = top - 1 - (timed_query ? 1 : 0) - (callback ? 1 : 0);
	if (count <= 0) {
		luaL_error(L, "no data to be sent");
	}
	if (callback) {
		int32_t session = context_lua_t::lua_ref_callback(L, 0, LUA_REFNIL, common_callback_adjust);
		int32_t ret = context_send(L, idx, count, handle, session, CONTEXT_QUERY);
		if (ret == UV_OK) {
			if (timeout > 0) {
				context_lua_t::lua_ref_timer(L, session, timeout, 0, false);
			}
			lua_pushboolean(L, 1);
			lua_pushinteger(L, session);
			return 2;
		}
		context_lua_t::lua_free_ref_session(L, session);
		if (ret == NL_ETRANSTYPE) {
			lua_error(L);
			return 0;
		}
		lua_pushboolean(L, 0);
		lua_pushinteger(L, NL_ENOCONTEXT);
		return 2;
	}
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	message_t& message = lctx->get_yielding_message();
	int ret = context_check_message(L, idx, count, CONTEXT_QUERY, message);
	if (ret == 0) {
		message_clean(message);
		lua_error(L);
		return 0;
	}
	return lctx->lua_yield_send(L, handle, context_query_yield_finalize, NULL, context_lua_t::context_query_yield_continue, timeout);
}

int32_t context_lua_t::context_query(lua_State *L)
{
	return context_query(L, false);
}

int32_t context_lua_t::context_timed_query(lua_State *L)
{
	return context_query(L, true);
}

int32_t context_lua_t::context_reply(lua_State *L)
{
	uint32_t handle = luaL_checkunsigned(L, 1);
	if (handle == 0 || singleton_ref(node_lua_t).is_handle_illegal(handle)) {
		luaL_error(L, "attempt to reply to an illegal context:0x%08x", handle);
	}
	int32_t session = luaL_checkinteger(L, 2);
	luaL_checkany(L, 3);
	int32_t ret = context_send(L, 3, lua_gettop(L) - 2, handle, session, CONTEXT_REPLY);
	if (ret == UV_OK) {
		lua_pushboolean(L, 1);
		return 1;
	}
	if (ret == NL_ETRANSTYPE) {
		singleton_ref(node_lua_t).context_send(handle, lua_get_context_handle(L), session, CONTEXT_REPLY, NL_ETRANSTYPE);
		lua_error(L);
		return 0;
	}
	lua_pushboolean(L, 0);
	lua_pushinteger(L, NL_ENOCONTEXT);
	return 2;
}

int32_t context_lua_t::context_recv_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination)
{
	if (root_coro) {
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		int32_t session = lctx->m_context_recv_sessions.push_blocking(destination, root_coro);
		int64_t timeout = lctx->get_yielding_timeout();
		if (timeout > 0) {
			context_lua_t::lua_ref_timer(main_coro, session, timeout, 0, false, (void*)destination, context_recv_timeout);
		}
	}
	return UV_OK;
}

int32_t context_lua_t::context_recv_yield_continue(lua_State* L, int status, lua_KContext ctx)
{
	int32_t old_top = (int32_t)ctx;
	bool result = lua_toboolean(L, old_top + 1);
	if (!result && context_lua_t::lua_get_context(L)->get_yielding_status() != UV_OK) {
		context_lua_t::lua_throw(L, old_top + 2);
	}
	if (result) {
		lua_rotate(L, old_top + 2, 2);
	}
	return lua_gettop(L) - old_top;
}

int32_t context_lua_t::context_recv_callback_adjust(lua_State* L)
{
	bool result = lua_toboolean(L, 1); //old_top is 0
	if (result) {
		lua_rotate(L, 2, 2); //push source handle and session before recved data.
	}
	return lua_gettop(L);
}

int32_t context_lua_t::context_recv_timeout(lua_State *L, int32_t session, void* userdata, bool is_repeat)
{
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	message_t message(0, session, CONTEXT_REPLY, NL_ETIMEOUT);
	return lctx->m_context_recv_sessions.wakeup_one_fixed((uint64_t)userdata, lctx, message, false);
}

int32_t context_lua_t::context_recv(lua_State *L)
{
	context_lua_t* lctx = (context_lua_t*)lua_get_context(L);
	uint32_t handle = luaL_checkunsigned(L, 1);
	if (singleton_ref(node_lua_t).is_handle_illegal(handle)) {
		luaL_error(L, "attempt to recv from an illegal context:0x%08x", handle);
	}
	int32_t top = lua_gettop(L);
	uint64_t timeout = 0;
	if (top >= 2) { //nonblocking
		if (lua_isfunction(L, 2)) {
			lua_settop(L, 2);
			lctx->m_context_recv_sessions.make_nonblocking_callback(handle, L, 0, context_recv_callback_adjust);
			return 0;
		}
		if (lua_isnil(L, 2)) {
			lctx->m_context_recv_sessions.free_nonblocking_callback(handle, L);
			return 0;
		}
		timeout = 1000 * luaL_checknumber(L, 2);
	}
	return lctx->lua_yield_send(L, handle, context_recv_yield_finalize, NULL, context_recv_yield_continue, timeout);
}

int32_t context_lua_t::context_wait_yield_finalize(lua_State *root_coro, lua_State *main_coro, void *userdata, uint32_t destination)
{
	if (root_coro != NULL) {
		context_lua_t* lctx = context_lua_t::lua_get_context(root_coro);
		if (singleton_ref(node_lua_t).context_send(destination, lctx->get_handle(), LUA_REFNIL, LUA_CTX_WAIT, (int64_t)destination)) {
			int32_t session = lctx->m_context_wait_sessions.push_blocking(destination, root_coro);
			int64_t timeout = lctx->get_yielding_timeout();
			if (timeout > 0) {
				context_lua_t::lua_ref_timer(main_coro, session, timeout, 0, false, (void*)destination, context_wait_timeout);
			}
			return UV_OK;
		} else {
			return NL_ENOCONTEXT;
		}
	}
	return UV_OK;
}
int32_t context_lua_t::context_wait_yield_continue(lua_State* L, int status, lua_KContext ctx)
{
	if (!lua_toboolean(L, -4)) {
		int32_t ret = context_lua_t::lua_get_context(L)->get_yielding_status();
		if (ret != UV_OK && ret != NL_ENOCONTEXT) {
			context_lua_t::lua_throw(L, -3);
		}
	}
	lua_pop(L, 2); //pop two useless arguments
	return 2;
}

int32_t context_lua_t::context_wait_timeout(lua_State *L, int32_t session, void* userdata, bool is_repeat)
{
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	message_t message(0, session, LUA_CTX_WAKEUP, NL_ETIMEOUT);
	return lctx->m_context_wait_sessions.wakeup_one_fixed((uint64_t)userdata, lctx, message, true);
}

int32_t context_lua_t::context_wait(lua_State *L)
{
	context_lua_t* lctx = (context_lua_t*)lua_get_context(L);
	uint32_t handle = luaL_checkunsigned(L, 1);
	if (handle == 0 || singleton_ref(node_lua_t).is_handle_illegal(handle)) {
		luaL_error(L, "attempt to wait an illegal context:0x%08x", handle);
	}
	if (handle == lctx->get_handle()) {
		luaL_error(L, "can't wait self to die away");
		return 0;
	}
	int32_t top = lua_gettop(L);
	uint64_t timeout = 0;
	int32_t callback = 0;
	int32_t session;
	if (top >= 2) {
		if (lua_isnil(L, 2)) {
			lctx->m_context_wait_sessions.free_nonblocking_callback(handle, L);
			return 0;
		}
		if (lua_isfunction(L, 2)) {
			callback = 2;
		} else {
			timeout = 1000 * luaL_checknumber(L, 2);
			if (top >= 3) {
				luaL_checktype(L, 3, LUA_TFUNCTION);
				callback = 3;
			}
		}
	}
	if (callback > 0) { //nonblocking
		lua_settop(L, callback);
		if (singleton_ref(node_lua_t).context_send(handle, lctx->get_handle(), LUA_REFNIL, LUA_CTX_WAIT, (int64_t)handle)) {
			lctx->m_context_wait_sessions.make_nonblocking_callback(handle, L, callback - 1, common_callback_adjust, &session);
			if (timeout > 0) {
				context_lua_t::lua_ref_timer(L, session, timeout, 0, false, (void*)handle, context_wait_timeout);
			}
		} else {
			singleton_ref(node_lua_t).context_send(lctx, handle, LUA_REFNIL, LUA_CTX_WAKEUP, UV_OK);
		}
		return 0;
	}
	return lctx->lua_yield_send(L, handle, context_wait_yield_finalize, NULL, context_wait_yield_continue, timeout);
}

int32_t context_lua_t::context_self(lua_State *L)
{
	lua_pushinteger(L, lua_get_context_handle(L));
	return 1;
}

int32_t context_lua_t::context_thread(lua_State *L)
{
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	lua_pushinteger(L, lctx->m_worker->m_index);
	return 1;
}

int32_t context_lua_t::context_log(lua_State *L)
{
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	int32_t n = lua_gettop(L);  /* number of arguments */
	int32_t i;
	sds buffer = sdsnewlen(NULL, 64);
	sdsclear(buffer);
	lua_getglobal(L, "tostring");
	for (i = 1; i <= n; i++) {
		const char *s;
		size_t l;
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		s = lua_tolstring(L, -1, &l);  /* get result */
		if (s == NULL) {
			sdsfree(buffer);
			return luaL_error(L, "'tostring' must return a string to 'context.log'");
		}
		if (i > 1) buffer = sdscatlen(buffer, "\t", 1);
		buffer = sdscatlen(buffer, s, l);
		lua_pop(L, 1);  /* pop result */
	}
	singleton_ref(node_lua_t).log_sds_release(lctx->get_handle(), buffer);
	return 0;
}

int luaopen_context(lua_State *L)
{
	luaL_Reg l[] = {
			{ "strerror", context_lua_t::context_strerror },
			{ "create", context_lua_t::context_create },
			{ "destroy", context_lua_t::context_destroy },
			{ "thread", context_lua_t::context_thread },
			{ "send", context_lua_t::context_send },
			{ "query", context_lua_t::context_query },
			{ "timed_query", context_lua_t::context_timed_query },
			{ "reply", context_lua_t::context_reply },
			{ "recv", context_lua_t::context_recv },
			{ "wait", context_lua_t::context_wait },
			{ "log", context_lua_t::context_log },
			{ NULL, NULL },
	};
	luaL_newlib(L, l);
#ifdef CC_MSVC
	lua_pushboolean(L, 1);
#else
	lua_pushboolean(L, 0);
#endif
	lua_setfield(L, -2, "winos");
	context_lua_t* lctx = context_lua_t::lua_get_context(L);
	lua_pushinteger(L, lctx->get_handle());
	lua_setfield(L, -2, "self");
	lua_pushinteger(L, lctx->get_parent());
	lua_setfield(L, -2, "parent");
	return 1;
}














