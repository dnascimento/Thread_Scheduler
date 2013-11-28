/*
 * sthread.c - Implements the public API (the functions defined in
 *             include/sthread.h). Since sthreads supports two implementations
 *             (pthreads and student supplied user-level threads), this
 *             just consists of dispatching the calls to the implementation
 *             that the application choose.
 *
 */

#include <config.h>

#include <assert.h>
#include <sthread.h>
#include <sthread_pthread.h>
#include <sthread_user.h>
#include <stdio.h>

#ifdef USE_PTHREADS
#define IMPL_CHOOSE(pthread, user) pthread
#else
#define IMPL_CHOOSE(pthread, user) user
#endif

void sthread_init(void) {
  IMPL_CHOOSE(sthread_pthread_init(), sthread_user_init());
}
/*argumento adiconado:int priority*/
sthread_t sthread_create(sthread_start_func_t start_routine, void *arg,int priority) { 
  sthread_t newth;
  IMPL_CHOOSE(newth = sthread_pthread_create(start_routine, arg),
	      newth = sthread_user_create(start_routine, arg, priority));
  return newth;
}

void sthread_exit(void *ret) {
  IMPL_CHOOSE(sthread_pthread_exit(ret), sthread_user_exit(ret));
}

void sthread_yield(void) {
  IMPL_CHOOSE(sthread_pthread_yield(), sthread_user_yield());
}

int sthread_sleep(int time) {
   return IMPL_CHOOSE(sthread_pthread_sleep(time),sthread_user_sleep(time));
}

int sthread_join(sthread_t thread, void **value_ptr) {
  return IMPL_CHOOSE(sthread_pthread_join(thread,value_ptr),sthread_user_join(thread,value_ptr));
}

/*NOSSO*/
void sthread_dump() {
	IMPL_CHOOSE(printf("No Define Func"),sthread_user_dump());
}

/**********************************************************************/
/* Synchronization Primitives: Mutexs and Condition Variables         */
/**********************************************************************/


sthread_mutex_t sthread_mutex_init() {
  sthread_mutex_t lock;
  IMPL_CHOOSE(lock = sthread_pthread_mutex_init(),
	      lock = sthread_user_mutex_init());
  return lock;
}

void sthread_mutex_free(sthread_mutex_t lock) {
  IMPL_CHOOSE(sthread_pthread_mutex_free(lock),
	      sthread_user_mutex_free(lock));
}

void sthread_mutex_lock(sthread_mutex_t lock) {
  IMPL_CHOOSE(sthread_pthread_mutex_lock(lock),
	      sthread_user_mutex_lock(lock));
}

void sthread_mutex_unlock(sthread_mutex_t lock) {
  IMPL_CHOOSE(sthread_pthread_mutex_unlock(lock),
	      sthread_user_mutex_unlock(lock));
}

sthread_mon_t sthread_monitor_init() {
  sthread_mon_t mon;
  IMPL_CHOOSE(mon = sthread_pthread_monitor_init(),
	      mon = sthread_user_monitor_init());
  return mon;
}

void sthread_monitor_free(sthread_mon_t mon) {
  IMPL_CHOOSE(sthread_pthread_monitor_free(mon),
	      sthread_user_monitor_free(mon));
}

void sthread_monitor_enter(sthread_mon_t mon) {
  IMPL_CHOOSE(sthread_pthread_monitor_enter(mon),
	      sthread_user_monitor_enter(mon));
}

void sthread_monitor_exit(sthread_mon_t mon) {
  IMPL_CHOOSE(sthread_pthread_monitor_exit(mon),
	      sthread_user_monitor_exit(mon));
}


void sthread_monitor_wait(sthread_mon_t mon) {
  IMPL_CHOOSE(sthread_pthread_monitor_wait(mon),
	      sthread_user_monitor_wait(mon));
}

void sthread_monitor_signal(sthread_mon_t mon) {
  IMPL_CHOOSE(sthread_pthread_monitor_signal(mon),
	      sthread_user_monitor_signal(mon));
}

void sthread_monitor_signalall(sthread_mon_t mon) {
  IMPL_CHOOSE(sthread_pthread_monitor_signalall(mon),
	      sthread_user_monitor_signalall(mon));
}


