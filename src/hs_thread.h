#ifndef _HS_THREAD_H_
#define _HS_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#if __linux && !defined(_GNU_SOURCE)
# define _GNU_SOURCE
#endif

/* just in case */
#define _REENTRANT 1

#if __solaris
# define _POSIX_PTHREAD_SEMANTICS 1
/* try to bribe solaris headers into providing a current pthread API
 * despite environment being configured for an older version.
 */
# define __EXTENSIONS__ 1
#endif

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>

typedef pthread_mutex_t hs_mutex_t;

#if __linux && defined (PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP)
# define HS_MUTEX_INIT		PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
# define HS_MUTEX_CREATE(mutex)						\
  do {									\
    pthread_mutexattr_t attr;						\
    pthread_mutexattr_init (&attr);					\
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ADAPTIVE_NP);	\
    pthread_mutex_init (&(mutex), &attr);				\
  } while (0)
#else
# define HS_MUTEX_INIT		PTHREAD_MUTEX_INITIALIZER
# define HS_MUTEX_CREATE(mutex)	pthread_mutex_init (&(mutex), 0)
#endif
#define HS_LOCK(mutex)		pthread_mutex_lock   (&(mutex))
#define HS_UNLOCK(mutex)		pthread_mutex_unlock (&(mutex))

typedef pthread_cond_t hs_cond_t;

#define HS_COND_INIT			PTHREAD_COND_INITIALIZER
#define HS_COND_CREATE(cond)		pthread_cond_init (&(cond), 0)
#define HS_COND_SIGNAL(cond)		pthread_cond_signal (&(cond))
#define HS_COND_WAIT(cond,mutex)		pthread_cond_wait (&(cond), &(mutex))
#define HS_COND_TIMEDWAIT(cond,mutex,to)	pthread_cond_timedwait (&(cond), &(mutex), &(to))

typedef pthread_t hs_thread_t;

#define HS_THREAD_PROC(name) static void *name (void *thr_arg)
#define HS_THREAD_ATFORK(prepare,parent,child) pthread_atfork (prepare, parent, child)

// the broken bsd's once more
#ifndef PTHREAD_STACK_MIN
# define PTHREAD_STACK_MIN 0
#endif

#ifndef HS_STACKSIZE
# define HS_STACKSIZE sizeof (void *) * 4096
#endif

int hs_thread_join(hs_thread_t tid, void **retval = NULL);
int hs_thread_create (hs_thread_t *tid, void *(*proc)(void *), void *arg);

#ifdef __cplusplus
}
#endif

#endif
