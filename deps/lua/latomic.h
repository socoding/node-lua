#ifndef latomic_h
#define latomic_h

//////////////////////////////////////////////////////////////////////////
/*
    The compiler, must be one of: (CC_X)

    MSVC    - Microsoft Visual C++
    GNUC    - GNU C++
*/
//////////////////////////////////////////////////////////////////////////

#define CHECK_GNUC(ver, minor, patch) \
    defined(__GNUC__) /* >= ver.minor.patch */ && \
    ((__GNUC__ > (ver)) || ((__GNUC__ == (ver)) && \
    ((__GNUC_MINOR__ > (minor)) || ((__GNUC_MINOR__ == (minor)) && \
    (__GNUC_PATCHLEVEL__ >= (patch)) ) ) ) )

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(WINCE) || defined(_WIN32_WCE) || defined(_WIN64)
#ifndef CC_MSVC
#define CC_MSVC
#endif
#elif CHECK_GNUC(4, 1, 2)						/* >= 4.1.2 */
#define CC_GNUC
#else
#   error "This Compiler is unsupported!"
#endif

#if defined(CC_MSVC)
#include <windows.h>
typedef volatile LONG   atomic_t;
typedef volatile ULONG  uatomic_t;
#else
#include <stdlib.h>
typedef volatile long   atomic_t;
typedef volatile unsigned long  uatomic_t;
#endif

#if defined(CC_GNUC)

#   define atomic_inc(var)           __sync_fetch_and_add        (&(var), 1)
#   define atomic_dec(var)           __sync_fetch_and_sub        (&(var), 1)
#   define atomic_add(var, val)      __sync_fetch_and_add        (&(var), (val))
#   define atomic_sub(var, val)      __sync_fetch_and_sub        (&(var), (val))
#   define atomic_set(var, val)      __sync_lock_test_and_set    (&(var), (val))
#   define atomic_cas(var, cmp, val) __sync_bool_compare_and_swap(&(var), (cmp), (val))

#if CHECK_GNUC(4, 2, 0)
#   define atomic_barrier()          __sync_synchronize()
#else
#   define atomic_barrier()          __asm__ __volatile__("":::"memory")
#endif

#define spin_lock_init (0)
#define spin_lock(lock)					 while (__sync_lock_test_and_set(&(lock), 1)) {}
#define spin_trylock(lock)				 (!__sync_lock_test_and_set(&(lock), 1))
#define spin_unlock(lock)				 __sync_lock_release(&(lock))
#define spin_lock_ptr(lockptr)			 while (__sync_lock_test_and_set((lockptr), 1)) {}
#define spin_trylock_ptr(lockptr)		 (!__sync_lock_test_and_set((lockptr), 1))
#define spin_unlock_ptr(lockptr)		 __sync_lock_release((lockptr))

#elif defined(CC_MSVC)

#   define atomic_inc(var)           InterlockedExchangeAdd      (&(var), 1)
#   define atomic_dec(var)           InterlockedExchangeAdd      (&(var),-1)
#   define atomic_add(var, val)      InterlockedExchangeAdd      (&(var), (val))
#   define atomic_sub(var, val)      InterlockedExchangeAdd      (&(var),-(val))
#   define atomic_set(var, val)      InterlockedExchange         (&(var), (val))
#   define atomic_cas(var, cmp, val) ((cmp) == InterlockedCompareExchange(&(var), (val), (cmp)))

#   define atomic_barrier()			 MemoryBarrier()

#define spin_lock_init (0)
#define spin_lock(lock)					 while (InterlockedExchange(&(lock), 1)) {}
#define spin_trylock(lock)				 (!InterlockedExchange(&(lock), 1))
#define spin_unlock(lock)				 InterlockedExchange(&(lock), 0)
#define spin_lock_ptr(lockptr)			 while (InterlockedExchange((lockptr), 1)) {}
#define spin_trylock_ptr(lockptr)		 (!InterlockedExchange((lockptr), 1))
#define spin_unlock_ptr(lockptr)		 InterlockedExchange((lockptr), 0)

#else
#   error "This platform is unsupported"
#endif

#endif
