#ifndef COMMON_H_
#define COMMON_H_
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "config.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(WINCE) || defined(_WIN32_WCE) || defined(_WIN64)
#ifndef CC_MSVC
#define CC_MSVC
#endif
#endif

#ifdef CC_MSVC
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))

#ifdef RELEASE
#define WPAssert( assertion ) { if( !(assertion) ) { fprintf( stderr, "\n%s:%i ASSERTION FAILED:\n  %s\n", __FILE__, __LINE__, #assertion ); } }
#else
#define WPAssert( assertion ) { if( !(assertion) ) { fprintf( stderr, "\n%s:%i ASSERTION FAILED:\n  %s\n", __FILE__, __LINE__, #assertion ); assert(assertion); } }
#endif

#define ASSERT(x) WPAssert(x)

#if defined(_MSC_VER) && _MSC_VER < 1600
# include "uv-private/stdint-msvc2008.h"
#else
# include <stdint.h>
#endif

#include "uv.hpp"
#include "lua.hpp"
#include "latomic.h"
#include "utils.h"
#include "errors.h"
#include "nlmalloc.h"

#define nl			(singleton_ref(node_lua_t))

#if defined __GNUC__
#define likely(x) __builtin_expect ((x), 1)
#define unlikely(x) __builtin_expect ((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define nl_max(a,b)            (((a) > (b)) ? (a) : (b))
#define nl_min(a,b)            (((a) < (b)) ? (a) : (b))

#endif
