#include "kernel.def"

.macro stack n s
	.section .stack.\n, "ax"
	.globl \n
	\n = .
	. = . + \s
	.globl \n\()_top
	\n\()_top = .
.endm

stack fiqstack, 256
stack abtstack, 256
stack irqstack, 4096
stack fiqstack, 256
stack undstack, 256
stack sysstack, 4096

.section .vectors, "ax"
.globl __vectors
__vectors:
	/* Exception table. */

	b reset        /* 0x00: Reset */
	b und_handler  /* 0x04: Undefined instruction */
	b svc_handler  /* 0x08: SWI */
	b pabt_handler /* 0x0c: Prefetch abort */
	b dabt_handler /* 0x10: Data abort */
	b reset        /* 0x14: Hypervisor trap (?) */
	b irq_handler  /* 0x18: IRQ interrupt */
	b fiq_handler  /* 0x1c: FIQ interrupt */

proc pabt_handler
	cpsid if
	ldr sp, =irqstack_top
	ldr r0, =1f
	sub r1, lr, #4
	b unexpected_handler_c
	//subs pc, r14, #4
1:	.string "physical abort"

proc dabt_handler
	cpsid if
	ldr sp, =irqstack_top
	b dabt_handler_c

proc irq_handler
	ldr sp, =irqstack_top
	push {r0-r12, lr}
	bl platform_interrupt
	pop {r0-r12, lr}
	subs pc, lr, #4

proc fiq_handler
	cpsid if
	ldr sp, =irqstack_top
	ldr r0, =1f
	sub r1, lr, #4
	b unexpected_handler_c
	//subs pc, r14, #4
1:	.string "FIQ"

proc und_handler
	cpsid if
	ldr sp, =irqstack_top
	ldr r0, =1f
	mov r1, lr
	b unexpected_handler_c
	//movs pc, r14
1:	.string "undefined instruction"

proc reset
	/* On entry:
	 *   r0 = 0
	 *   r1 = arm machine (useless on the Pi)
	 *   r2 = physical address of atags list
	 *
	 * r2 gets passed through to platform_main so it can look at it.
	 */

	cpsid if, #MODE_SVC
	ldr sp, =sysstack_top
	
	mov r0, r2
	b platform_init

proc trap_reboot
	cpsid if
	ldr sp, =irqstack_top
	ldr r0, =1f
	mov r1, lr
	b unexpected_handler_c
1:	.string "reset"

proc busy_wait
	subs r0, r0, #1
	bne busy_wait
	bx lr
	
proc di
	/* Disable interrupts, returning the old interrupt state in r0. */
	mrs r0, CPSR
	cpsid if
	bx lr

proc irqrestore
	/* Return interrupts to the state they were at when di() was called. */
	mrs r1, CPSR
	bic r1, r1, #INTERRUPT_FLAGS
	orr r1, r1, r0
	msr CPSR_c, r1
	bx lr

proc ei
	/* Enable interrupts unconditionally. */
	cpsie if
	bx lr

