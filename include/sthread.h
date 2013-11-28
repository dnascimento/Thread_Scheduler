/* 
 * sthread.h - The public interface to simplethreads, used by
 *             applications.
 *
 */

#ifndef STHREAD_H
#define STHREAD_H 1

/* Define the sthread_t type (a pointer to an _sthread structure)
 * without knowing how it is actually implemented (that detail is
 * hidden from the public API).
 */
typedef struct _sthread *sthread_t;

/* New threads always begin life in a routine of this type */
typedef void *(*sthread_start_func_t)(void *);

/* Sthreads supports multiple implementations, so that one can test
 * programs with different kinds of threads (e.g. compare kernel to user
 * threads). This enum represents an implementation choice. (Which
 * is actually a compile time choice.)
 */
typedef enum { STHREAD_PTHREAD_IMPL, STHREAD_USER_IMPL } sthread_impl_t;

/* Return the implementation that was selected at compile time. */
sthread_impl_t sthread_get_impl(void);

/* 
 * Perform any initialization needed. Should be called exactly
 * once, before any other sthread functions (sthread_get_impl 
 * excepted).
 */
void sthread_init();

/* Create a new thread starting at the routine given, which will
 * be passed arg. The new thread does not necessarily execute immediatly
 * (as in, sthread_create shouldn't force a switch to the new thread).
 *
 *ARGUMENTO ADICONADO: int priority*/
sthread_t sthread_create(sthread_start_func_t start_routine, void *arg,int priority);

/* Exit the calling thread with return value ret.
 * Note: In this version of simplethreads, there is no way
 * to retrieve the return value.
 */
void sthread_exit(void *ret);

/* Voluntarily yield the CPU to another waiting
 * thread (or, possibly, another process).
 */
void  sthread_yield(void);


/* Suspends current thread for the time specified. The time is defined
 * in milliseconds (e.g. 1s -> time=1000). Returns 0 if successful.
 */
int sthread_sleep(int time);


int sthread_join(sthread_t thread, void **value_ptr);

 /*Invocacao do Dump pela tarefa*/
void sthread_dump();

/**********************************************************************/
/* Synchronization Primitives: Mutexs and Condition Variables         */
/**********************************************************************/

typedef struct _sthread_mutex *sthread_mutex_t;

/* Return a new, unlocked mutex */
sthread_mutex_t sthread_mutex_init();

/* Free a no-longer needed mutex.
 * Assume mutex has no waiters. */
void sthread_mutex_free(sthread_mutex_t lock);

/* Acquire the lock, blocking if neccessary. */
void sthread_mutex_lock(sthread_mutex_t lock);

/* Release the lock. Assumed that the calling thread owns the lock */
void sthread_mutex_unlock(sthread_mutex_t lock);

typedef struct _sthread_mon *sthread_mon_t;

/* Return a new, unlocked monitor */
sthread_mon_t sthread_monitor_init();

/* Free a no-longer needed monitor.
 * Assume monitor has no waiters. */
void sthread_monitor_free(sthread_mon_t mon);

/* Enter the monitor's critical section, blocking if neccessary. */
void sthread_monitor_enter(sthread_mon_t mon);

/* Exit the monitor's critical section. */
void sthread_monitor_exit(sthread_mon_t mon);

/* Wait on monitor, blocking if neccessary. */
void sthread_monitor_wait(sthread_mon_t mon);

/* Signal the monitor's waiters */
void sthread_monitor_signal(sthread_mon_t mon);

/* Signal the monitor's waiters */
void sthread_monitor_signalall(sthread_mon_t mon);



#endif /* STHREAD_H */


