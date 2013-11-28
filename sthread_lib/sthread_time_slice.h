#include <sthread_ctx.h>


#define HIGH 1
#define LOW  0

typedef int lock_t;


/* start time slices - func will be called every period microseconds */
void sthread_time_slices_init(sthread_ctx_start_func_t func, int period);

/* Turns inturrupts ON and off 
 * Returns the last state of the inturrupts
 * LOW = inturrupts ON
 * HIGH = inturrupts OFF
 */
int splx(int splval);

/*
 * atomic_test_and_set - using the native compare and exchange on the 
 * Intel x86.
 *
 * Example usage:
 *
 *   lock_t lock;
 *   while(atomic_test_and_set(&lock)) { } // spin
 *   _critical section_
 *   atomic_clear(&lock); 
 */

int atomic_test_and_set(lock_t *l);
void atomic_clear(lock_t *l);


/*
 * sthread_print_stats - prints out the number of drupped interrupts
 *   and "successful" interrupts
 */
void sthread_print_stats();
