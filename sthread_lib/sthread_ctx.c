/* sthread_ctx.c - Support for creating and switching thread contexts.
 *
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <sthread_ctx.h>

#ifdef STHREAD_CPU_I386	
#include "sthread_switch_i386.h"
#endif

#ifdef STHREAD_CPU_POWERPC
#include "sthread_switch_powerpc.h"
#endif		


const size_t sthread_stack_size = 64 * 1024;

static void sthread_init_stack(sthread_ctx_t *ctx,
			       sthread_ctx_start_func_t func);


sthread_ctx_t *sthread_new_ctx(sthread_ctx_start_func_t func) {
    sthread_ctx_t *ctx;
    
    ctx = (sthread_ctx_t*)malloc(sizeof(sthread_ctx_t));
    if (ctx == NULL) {
	fprintf(stderr, "Out of memory (sthread_new_ctx)\n");
	return NULL;
    }

    ctx->stackbase = (char*)malloc(sthread_stack_size);
    if (ctx->stackbase == NULL) {
	free(ctx);
	fprintf(stderr, "Out of memory (sthread_new_ctx)\n");
	return NULL;
    }

    /* The stack grows down, so the first SP is at the top. */
    ctx->sp = ctx->stackbase + sthread_stack_size - 16;
    
    sthread_init_stack(ctx, func);
    
    return ctx;
}

/* Initialize a stack as if it had been saved by sthread_switch. */
static void sthread_init_stack(sthread_ctx_t *ctx, sthread_ctx_start_func_t func) {
    memset(ctx->stackbase, 0, sthread_stack_size);

    ctx->sp -= sizeof(sthread_ctx_start_func_t);
    *((sthread_ctx_start_func_t*)ctx->sp) = func;

    /* Leave room for the values pushed on the stack by the
     * "save" half of _sthread_switch. The amount of room
     * varies between CPUs, so we get it from sthread_cpu.h
     * (which is automatically configured by ./configure). */
    ctx->sp -= STHREAD_CPU_SWITCH_REGISTERS*sizeof(void *);
}

/* Create a new sthread_ctx_t, but don't initialize it.
 * This new sthread_ctx_t is suitable for use as 'old' in
 * a call to sthread_switch, since sthread_switch is defined to overwrite
 * 'old'. It should not be used as 'new' until it has been initialized.
 */
sthread_ctx_t *sthread_new_blank_ctx() {
  sthread_ctx_t *ctx;
  ctx = (sthread_ctx_t*)malloc(sizeof(sthread_ctx_t));
  /* Put some bogus values in */
  ctx->sp = (char*)0xbeefcafe;
  ctx->stackbase = NULL;
  return ctx;
}

/* Free resources used by given (not currently running) context. */
void sthread_free_ctx(sthread_ctx_t *ctx) {
    if(ctx->stackbase)
	free(ctx->stackbase);
    ctx->stackbase = (char*)0xdeaddead;
    ctx->sp = (char*)0xdeaddead;
    free(ctx);
}

/* Avoid allowing the compiler to optimize the call to
 * Xsthread_switch as a tail-call on architectures that support
 * that (powerpc). */
void sthread_anti_optimize(void) __attribute__ ((noinline));
void sthread_anti_optimize() {
}

/* Save the currently running thread context into old, and
 * start running the context new. Old may be uninitialized,
 * but new must contain a valid saved context. */
void sthread_switch(sthread_ctx_t *old, sthread_ctx_t *new) {
    /* Call the assembly */
    if (old != new)
	Xsthread_switch(&(old->sp), new->sp);
    /* Do not put anything useful here. In some cases (namely, the
     * first time a new thread is run), _sthread_switch does
     * not return to this line. (It instead returns to the
     * start function set when the stack was initialized.)
     *
     * sthread_anti_optimize just forces gcc not to optimize
     * the call to Xsthread_switch.
     */
    sthread_anti_optimize();
}
