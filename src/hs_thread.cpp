#include "hs_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

int hs_thread_create (hs_thread_t *tid, void *(*proc)(void *), void *arg)
{
  int retval;
  sigset_t fullsigset, oldsigset;
  pthread_attr_t attr;

  pthread_attr_init (&attr);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setstacksize (&attr, PTHREAD_STACK_MIN < HS_STACKSIZE ? HS_STACKSIZE : PTHREAD_STACK_MIN);
#ifdef PTHREAD_SCOPE_PROCESS
  pthread_attr_setscope (&attr, PTHREAD_SCOPE_PROCESS);
#endif

  sigfillset (&fullsigset);

  pthread_sigmask (SIG_SETMASK, &fullsigset, &oldsigset);
  retval = pthread_create (tid, &attr, proc, arg) == 0;
  pthread_sigmask (SIG_SETMASK, &oldsigset, 0); 

  pthread_attr_destroy (&attr);

  return retval;
}

int hs_thread_join(hs_thread_t tid, void **retval)
{
	return pthread_join(tid, retval);
}

#ifdef __cplusplus
}
#endif
