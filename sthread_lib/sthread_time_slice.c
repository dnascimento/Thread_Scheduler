#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include "sthread_time_slice.h"
#include "sthread_ctx.h"
#include "sthread_user.h"

#ifdef STHREAD_CPU_I386	
#include "sthread_switch_i386.h"
#endif

#ifdef STHREAD_CPU_POWERPC
#include "sthread_switch_powerpc.h"
#endif		

#include <sys/time.h>
#include <sys/timeb.h>
#include <signal.h>

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

int good_interrupts=0;
int dropped_interrupts=0;

int inited=0;

/* definded in the start.c and end.c files respectively */
extern void proc_start();
extern void proc_end();


static sthread_ctx_start_func_t interruptHandler;

void sthread_print_stats() {
    printf("\ngood interrupts: %d\n", good_interrupts);
    printf("dropped interrupts: %d\n", dropped_interrupts);
}

void sthread_init_stats() {
  struct sigaction sa, osa;
  sigset_t mask;
  
  sa.sa_handler = (void (*)(int))sthread_print_stats;
  sa.sa_flags = 0;
  sigemptyset(&mask);
  sa.sa_mask = mask;
  sigaction(SIGQUIT,&sa,&osa); // allow getting interrupts statistics
			       // via ctrl-backslash 
}

void sthread_clock_init(sthread_ctx_start_func_t func, int period) {
    struct itimerval it, temp;

    interruptHandler = func;
    sthread_init_stats();
    it.it_interval.tv_sec = period/1000000;
    it.it_interval.tv_usec = period%1000000;
    it.it_value.tv_sec = period/1000000;
    it.it_value.tv_usec = period%1000000;
    setitimer(ITIMER_REAL, &it, &temp);
    
    
    /*Aqui manda um signalALRM*/
}


/* signal handler */
void clock_tick(int sig, struct sigcontext scp)
{
    /* insures that the pc is with-in our system code, not system code (lib.c) */
    if ((scp.eip >= (long)proc_start) 
    	&& (scp.eip <  (long)proc_end)
       	&& !(scp.eip >= (long)Xsthread_switch && scp.eip < (long)Xsthread_switch_end))
	{
	    sigset_t mask,oldmask;
	    good_interrupts++;
	    sigemptyset(&mask);
	    sigaddset(&mask, SIGALRM);
	    sigprocmask(SIG_UNBLOCK, &mask, &oldmask);
	    interruptHandler();
	}
    else dropped_interrupts++;
} 

/* Turns inturrupts ON and off 
 * Returns the last state of the inturrupts
 * LOW = inturrupts ON
 * HIGH = inturrupts OFF
 */
int splx(int splval) {			/*Inibir interrupcoes*/
   
  struct sigaction sa, osa;
  sigset_t mask;
  
  if(!inited) return 0;

  if (splval == HIGH) {

    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
  }
  else {    /* LOW */
  
    sa.sa_handler = (void (*)(int))clock_tick;
    /* let the signal be regenerated before this handler is done */
    /* the SA_RESTART flag allows some system calls (like accept)
       to be restarted if they are interrupted by SIGALRM */
    sa.sa_flags = SA_RESTART; // was SA_SIGINFO; disabled to get sigcontext
  }

  sigemptyset(&mask);
  sa.sa_mask = mask;

  sigaction(SIGALRM,&sa,&osa);

  return (osa.sa_handler == SIG_IGN) ? HIGH : LOW;

}

/* start time_slices - func will be called every period microseconds */

void sthread_time_slices_init(sthread_ctx_start_func_t func, int period) {
#ifndef DISABLE_TIME_SLICE
    sthread_clock_init(func,period);
    inited = 1;
    splx(LOW);
#endif
}


/*
 * atomic_test_and_set - using the native compare and exchange on the 
 * Intel x86.
 *
 * Example usage:
 *
 *   lock_t mylock;
 *   while(atomic_test_and_set(&lock)) { } // spin
 *   _critical section_
 *   atomic_clear(&lock); 
 */

#ifdef STHREAD_CPU_I386	

int atomic_test_and_set(lock_t *l)
{
	int val;
	__asm__ __volatile__ ( "lock cmpxchgl %2, (%3)"
				: "=a" (val)
				: "a"(0), "r" (1), "r" (l));
	return val;
}


/*
 * atomic_clear - on the intel x86
 *
 */
void atomic_clear(lock_t *l)
{
	*l = 0;	
}

#endif

#ifdef STHREAD_CPU_POWERPC

int atomic_test_and_set(lock_t *l)
{
    // write me
    printf("not implemented!\n");
}


/*
 * atomic_clear - on the intel x86
 *
 */
void atomic_clear(lock_t *l)
{
    // write me
    printf("not implemented!\n");
}

#endif
