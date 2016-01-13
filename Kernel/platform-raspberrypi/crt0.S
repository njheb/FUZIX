#include "kernel.def"

.macro stack n s
	.section .stack.\n, "ax"
	.globl \n
	\n = .
	. = . + \s
	.globl \n\()_top
	\n\()_top = .
.endm

#define INTERRUPT_FLAGS ((1<<6) | (1<<7))
#define MODE_MASK (0x1f<<0)

#define MODE_USER 0x10
#define MODE_FIQ  0x11
#define MODE_IRQ  0x12
#define MODE_SVC  0x13
#define MODE_ABT  0x17
#define MODE_UND  0x1b
#define MODE_SYS  0x1f

stack fiqstack, 256
stack svcstack, 256
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

proc svc_handler
	b svc_handler
	movs pc, r14

proc pabt_handler
	mov r0, #4
	b led_halt_and_blink
	subs pc, r14, #4

//proc dabt_handler
//	subs pc, r14, #8

proc irq_handler
	ldr sp, =irqstack_top
	push {r0-r12, lr}
	bl platform_interrupt
	pop {r0-r12, lr}
	subs pc, lr, #4

proc fiq_handler
	b fiq_handler
	subs pc, r14, #4

proc und_handler
	b und_handler
	movs pc, r14

proc reset
	/* On entry:
	 *   r0 = 0
	 *   r1 = arm machine (useless on the Pi)
	 *   r2 = physical address of atags list
	 *
	 * r2 gets passed through to platform_main so it can look at it.
	 */

	/* Set up stacks. The ARM remembers the stack pointer for each processor
	 * mode. */

	mrs r1, CPSR
	bic r1, r1, #MODE_MASK
	orr r1, r1, #INTERRUPT_FLAGS

	orr r3, r1, #MODE_FIQ
	msr CPSR, r3
	ldr sp, =fiqstack_top
	
	orr r3, r1, #MODE_IRQ
	msr CPSR, r3
	ldr sp, =irqstack_top
	
	orr r3, r1, #MODE_SVC
	msr CPSR, r3
	ldr sp, =svcstack_top
	
	orr r3, r1, #MODE_ABT
	msr CPSR, r3
	ldr sp, =abtstack_top
	
	orr r3, r1, #MODE_UND
	msr CPSR, r3
	ldr sp, =undstack_top
	
	orr r3, r1, #MODE_SYS
	msr CPSR, r3
	ldr sp, =sysstack_top
	
	/* Now call into C. */

	mov r0, r2
	b platform_init

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
	
