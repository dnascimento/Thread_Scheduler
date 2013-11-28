/* This is the i386 assembly for the actual register saver/stack switcher.
 *
 * void Xsthread_switch(void)
 *    Save any state that the compiler expects us to (on i386, this is
 *    very little. Switch stacks, and return on the new stack.
 *
 *    Instead of taking arguments normally (on the stack), this function
 *    looks to the global parameters.
 *
 * This file defines the function void _sthread_switch(void). Rather than
 * passing arguments normally, the caller places them in the globals
 * old_sp (a pointer to the location to store the old value of %esp), and
 * new_sp (the new value for %esp). _sthread_switch pushes the few
 * callee-saved registers, swaps the stack pointers, and then pops the
 * registers off the (now different) stack.
 *
 * We put it in a .S file, instead of using the gcc 'asm (...)' syntax, to
 * make it more robust (this way, the compiler won't change _anything_, and we
 * know exactly what the stack will look like. 
 * 
 * Also note that, by calling this sthread_switch.S rather than .s, this file
 * will be run through the C pre-processor before being fed to the assembler.
 */

#include <config.h>

#ifndef __ASM__ /* in C mode */

void __attribute__((regparm(2))) Xsthread_switch(char **old_sp, char *new_sp);
void Xsthread_switch_end();

/* Tell the stack-setup code how much space we expect
 * to be pushed on the stack. We push all 8 general
 * purpose registers on i386.
 */
#define STHREAD_CPU_SWITCH_REGISTERS 8

#else  /* in assembly mode */

	.globl _old_sp
	.globl _new_sp
	.globl Xsthread_switch
	.globl Xsthread_switch_end
	.globl print_sp
	.globl here
	
/* in C terms: void Xsthread_switch(void) */
Xsthread_switch:
	/* Push register state onto our current (old) stack
	 * (pusha = push all registers) */
	pusha

	/* Save old stack into *old_sp */
	movl %esp,(%eax)
	
	/* Load new stack from new_sp */
	movl %edx,%esp

	/* Pop saved registers off new stack
	 * (popa = pop all registers) */
        popa

	/* Return to whatever PC the current (new) stack
 	 * tells us to. */
	ret
Xsthread_switch_end:

#endif /* __ASM__ */	    
