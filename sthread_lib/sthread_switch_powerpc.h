/* This is the PowerPC assembly for the actual register saver/stack switcher.
 *
 *
 */

#include <config.h>

#ifndef __ASM__ /* in C mode */

void Xsthread_switch(char **old_sp, char *new_sp);
void Xsthread_switch_end(void);

/* Tell the stack-setup code how much space we expect
 * to be pushed on the stack. Since we are cheating (see
 * comment below about PowerPC's red zone), it's actually
 * -1; the stackpointer is above the values.
 */
#define STHREAD_CPU_SWITCH_REGISTERS (-1)

#else /* in assembly mode */


	/* PowerPC tools adds a _ to C functions names, so,
	 * it is _Xsthread_switch here, but from C, just
	 * Xsthread_switch. */
	.globl _Xsthread_switch
	
_Xsthread_switch:	
	/* On PowerPC, the return address is given it's own register
	 * (the Link Register, or LR). It needs to be saved specially.
	 * Other registers of note include r1, which is used as the
	 * stack pointer, r3, the first argument, and r4, the second arg.
	 *
	 * Registers r13-r31 and fp14-fp31 are non-volatile (callee-saved). 
	 *
	 * Since this is a leaf procedure, we can use PowerPC's 
	 * stack "red zone" to store state without going through
	 * the full stack frame setup. Thus, unlike on i386, we
	 * don't alter the stack pointers, we just use space that
	 * would normally be considered unallocated. */

	/* Get the old return value from LR. Save to stack. */
	mflr r5
	stw  r5,-4(r1)

	/* Save integer registers to stack */
	stw  r13,-8(r1)  
	stw  r14,-12(r1) 
	stw  r15,-16(r1) 
	stw  r16,-20(r1) 
	stw  r18,-24(r1) 
	stw  r19,-28(r1) 
	stw  r20,-32(r1) 
	stw  r21,-36(r1) 
	stw  r22,-40(r1) 
	stw  r23,-44(r1) 
	stw  r24,-48(r1) 
	stw  r25,-52(r1) 
	stw  r26,-56(r1) 
	stw  r27,-60(r1) 
	stw  r28,-64(r1) 
	stw  r29,-68(r1) 
	stw  r30,-72(r1) 
	stw  r31,-76(r1)

	/* Save FP registers to stack */
        stfd f13,-80(r1)
        stfd f14,-88(r1)
        stfd f15,-96(r1)
        stfd f16,-104(r1)
        stfd f17,-112(r1)
        stfd f18,-120(r1)
        stfd f19,-128(r1)
        stfd f20,-136(r1)
        stfd f21,-144(r1)
        stfd f22,-152(r1)
        stfd f23,-160(r1)
        stfd f24,-168(r1)
        stfd f25,-176(r1)
        stfd f26,-184(r1)
        stfd f27,-192(r1)
        stfd f28,-200(r1)
        stfd f29,-208(r1)
        stfd f30,-216(r1)
        stfd f31,-224(r1)
		
	/* Swap the stacks */	
	stw  r1,0(r3) /* Save sp to old_sp */
	mr   r1,r4    /* Copy new_sp into sp */

	/* Load integer registers */
	lwz  r13,-8(r1)  
	lwz  r14,-12(r1) 
	lwz  r15,-16(r1) 
	lwz  r16,-20(r1) 
	lwz  r18,-24(r1) 
	lwz  r19,-28(r1) 
	lwz  r20,-32(r1) 
	lwz  r21,-36(r1) 
	lwz  r22,-40(r1) 
	lwz  r23,-44(r1) 
	lwz  r24,-48(r1) 
	lwz  r25,-52(r1) 
	lwz  r26,-56(r1) 
	lwz  r27,-60(r1) 
	lwz  r28,-64(r1) 
	lwz  r29,-68(r1) 
	lwz  r30,-72(r1) 
	lwz  r31,-76(r1)

	/* Load FP registers */
	lfd f13,-80(r1)
        lfd f14,-88(r1)
        lfd f15,-96(r1)
        lfd f16,-104(r1)
        lfd f17,-112(r1)
        lfd f18,-120(r1)
        lfd f19,-128(r1)
        lfd f20,-136(r1)
        lfd f21,-144(r1)
        lfd f22,-152(r1)
        lfd f23,-160(r1)
        lfd f24,-168(r1)
        lfd f25,-176(r1)
        lfd f26,-184(r1)
        lfd f27,-192(r1)
        lfd f28,-200(r1)
        lfd f29,-208(r1)
        lfd f30,-216(r1)
        lfd f31,-224(r1)

	/* Restore the LR */
	lwz  r5,-4(r1)
	mtlr r5

	/* Return to LR */
	blr
_Xsthread_switch_end:	
	
#endif /* __ASM__ */
