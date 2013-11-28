/* Simplethreads Instructional Thread Package
 * 
 * sthread_pthread.c - Implements the sthread API using the system-provided
 *                     POSIX threads API. This is provided so you can
 *                     compare your user-level threads with a kernel-level
 *                     thread implementation.
 * Change Log:
 * 2002-04-15        rick
 *   - Initial version.
 * 2009-11-06       so-ist-utl-pt
 *   - Support for monitors
 */

#include <config.h>

#include <unistd.h>
#include <sys/types.h>


#if defined(HAVE_SCHED_H)
#include <sched.h>
#elif defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#include <string.h>

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <sthread.h>
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif


struct _sthread {
  pthread_t pth;
};

#if !defined(HAVE_SCHED_YIELD) && defined(HAVE_SELECT)
const int sthread_select_sec_timeout = 0;
const int sthread_select_usec_timeout = 1;
#endif

void sthread_pthread_init(void) {
  /* pthreads don't need to be initialized explicitly */
}

sthread_t sthread_pthread_create(sthread_start_func_t start_routine, void *arg) {
  sthread_t sth;
  int err;
  
  sth = (sthread_t)malloc(sizeof(struct _sthread));

  err = pthread_create(&(sth->pth), NULL, start_routine, arg);

  return sth;
}

void sthread_pthread_exit(void *ret) {
  pthread_exit(ret);
  assert(0); /* pthread_exit should never return */
}

void sthread_pthread_yield(void) {
  /* Pthreads doesn't provide an explict yield, but we can try */
#if defined(HAVE_SCHED_YIELD)
  sched_yield();
#elif defined(HAVE_SELECT)
  fd_set fdset;
  struct timeval select_timeout;
  FD_ZERO(&fdset);
  select_timeout.tv_sec = sthread_select_sec_timeout;
  select_timeout.tv_usec = sthread_select_usec_timeout;
  select(0, &fdset, &fdset, &fdset, &select_timeout);
#endif
}

int sthread_pthread_sleep(int timevalue) {
  pthread_mutex_t mutex;
  pthread_cond_t conditionvar;

  struct timespec timetoexpire;

  if(pthread_mutex_init(&mutex,NULL))
      return -1;
  if(pthread_cond_init(&conditionvar,NULL))
      return -1;

  //When to expire is an absolute time, so get the current time and add it
  //to our delay time

  timetoexpire.tv_sec = (unsigned int)time(NULL) + timevalue/100000;
  timetoexpire.tv_nsec = 0;

  return pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
}

int sthread_pthread_join(sthread_t thread, void **value_ptr) {

   pthread_join(thread->pth, value_ptr);
   return 0;
}



/**********************************************************************/
/* Synchronization Primitives: Mutexs and Condition Variables         */
/**********************************************************************/

/* In the pthreads implementation, sthread_mutex_t is just
 * a wrapper for pthread_mutext_t. (And similarly for sthread_cond_t.)
 */
struct _sthread_mutex {
  pthread_mutex_t plock;
};

sthread_mutex_t sthread_pthread_mutex_init() {
  sthread_mutex_t lock;
  lock = (sthread_mutex_t)malloc(sizeof(struct _sthread_mutex));
  assert(lock != NULL);
  pthread_mutex_init(&(lock->plock), NULL);
  return lock;
}

void sthread_pthread_mutex_free(sthread_mutex_t lock) {
  if (pthread_mutex_destroy(&(lock->plock)) != 0) {
    fprintf(stderr, "pthread_mutex_destroy failed: mutex not unlocked\n");
    abort();
  }
  free(lock);
}

void sthread_pthread_mutex_lock(sthread_mutex_t lock) {
  int err;
  if ((err = pthread_mutex_lock(&(lock->plock))) != 0) {
    fprintf(stderr, "pthred_mutex_lock error: %s", strerror(err));
    abort();
  }
}

void sthread_pthread_mutex_unlock(sthread_mutex_t lock) {
  int err;
  if ((err = pthread_mutex_unlock(&(lock->plock))) != 0) {
    fprintf(stderr, "pthred_mutex_unlock error: %s\n", strerror(err));
    abort();
  }
}



/* Monitors (implemented on pthread mutexes and conditions */

struct _sthread_mon {
  pthread_mutex_t plock;
  pthread_cond_t pcondition;
};

sthread_mon_t sthread_pthread_monitor_init() {
  sthread_mon_t monitor;
  monitor = (sthread_mon_t)malloc(sizeof(struct _sthread_mon));
  assert(monitor != NULL);
  pthread_mutex_init(&(monitor->plock), NULL);
  pthread_cond_init(&(monitor->pcondition), NULL);
  return monitor;
}

void sthread_pthread_monitor_free(sthread_mon_t mon) {
  if (pthread_mutex_destroy(&(mon->plock)) != 0) {
    fprintf(stderr, "pthread_monitor_destroy failed: mutex not unlocked\n");
    abort();
  }
  if (pthread_cond_destroy(&(mon->pcondition)) != 0) {
    fprintf(stderr, "pthread_monitor_destroy failed: problem destroying condition\n");
    abort();
  }
  free(mon);
}

void sthread_pthread_monitor_enter(sthread_mon_t mon) {
  int err;
  if ((err = pthread_mutex_lock(&(mon->plock))) != 0) {
    fprintf(stderr, "pthred_monitor_enter error: %s", strerror(err));
    abort();
  }
}

void sthread_pthread_monitor_exit(sthread_mon_t mon) {
  int err;
  if ((err = pthread_mutex_unlock(&(mon->plock))) != 0) {
    fprintf(stderr, "pthred_monitor_exit error: %s", strerror(err));
    abort();
  }
}

void sthread_pthread_monitor_wait(sthread_mon_t mon) {
  int err;
  if ((err = pthread_cond_wait(&(mon->pcondition), &(mon->plock))) != 0) {
    fprintf(stderr, "pthred_monitor_wait error: %s", strerror(err));
    abort();
  }
}

void sthread_pthread_monitor_signal(sthread_mon_t mon) {
  int err;
  if ((err = pthread_cond_signal(&(mon->pcondition))) != 0) {
    fprintf(stderr, "pthred_monitor_signal error: %s", strerror(err));
    abort();
  }
}

void sthread_pthread_monitor_signalall(sthread_mon_t mon) {
  int err;
  if ((err = pthread_cond_broadcast(&(mon->pcondition))) != 0) {
    fprintf(stderr, "pthred_monitor_signalall error: %s", strerror(err));
    abort();
  }
}
