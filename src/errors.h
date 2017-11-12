#ifndef ERROR_H_
#define ERROR_H_
#include <assert.h>
#include "common.h"

/* Expand this list if necessary. */
#define NL_ERRNO_MAP(XX)															\
  XX( 200, EYIELD,			"attempt to yield across a C-call boundary")			\
  XX( 201, EYIELDFNZ,		"yielding up finalize error")							\
  XX( 202, ESTACKLESS,		"attempt to yield across a stack-less coroutine")		\
  XX( 203, ETCPSCLOSED,		"tcp socket has been closed")							\
  XX( 204, ETCPLCLOSED,		"tcp listen socket has been closed")					\
  XX( 205, ETCPNOWSHARED,	"tcp socket not set write shared")						\
  XX( 206, ETCPWRITELONG,	"attempt to send data too long")						\
  XX( 207, ETCPREADOVERFLOW,"read overflowed wake up")								\
  XX( 208, EUDPSCLOSED,		"udp socket has been closed")							\
  XX( 209, EUDPNOWSHARED,	"udp socket not set write shared")						\
  XX( 210, ENOCONTEXT,		"context not exist or has been killed")					\
  XX( 211, ETRANSTYPE,		"transfer data type not supported")						\
  XX( 212, ENOREPLY,		"no reply")												\
  XX( 213, ETIMEOUT,		"timeout")												\
  XX( 214, ESOCKFAIL,		"socket create failed")									\

#define NL_ERRNO_GEN(val, name, s) NL_##name = val,
typedef enum {
	NL_ERRNO_MAP(NL_ERRNO_GEN)
	NL_MAX_ERRORS
} nl_err_code;
#undef NL_ERRNO_GEN

///////////////////////////////////////////////////////////////////////////////////////
extern const char* nl_err_name(nl_err_code err_code);
extern const char* nl_strerror(nl_err_code err_code);

///////////////////////////////////////////////////////////////////////////////////////
extern const char* common_err_name(int32_t err_code);
extern const char* common_strerror(int32_t err_code);

#endif
