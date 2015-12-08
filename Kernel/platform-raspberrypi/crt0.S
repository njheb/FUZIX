#include "kernel.def"

.macro stack n s
	.section .bss.\n
	\n = .
	. = . + \s
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
stack irqstack, 256
stack fiqstack, 256
stack undstack, 256
stack sysstack, 256
stack kernelstack, 1024

.section .text._start
.globl _start
_start:
	/* Exception table. */

	b reset /* 0x00: Reset */
	b reset /* 0x04: Undefined instruction */
	b reset /* 0x08: SWI */
	b reset /* 0x0c: Prefetch abort */
	b reset /* 0x10: Data abort */
	b reset /* 0x14: Hypervisor trap (?) */
	b reset /* 0x18: IRQ interrupt */
	b reset /* 0x1c: FIQ interrupt */

.globl trap_monitor
trap_monitor = reset

proc reset
	/* On entry:
	 *   r0 = 0
	 *   r1 = arm machine (useless on the Pi)
	 *   r2 = physical address of atags list
	 *
	 * r2 gets passed through to platform_main so it can look at it.
	 */

	/* Tell the processor where the exception table is. */

	ldr r0, =_start
	mcr p15, 0, r0, c12, c0, 0

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
	orr r1, r0, #INTERRUPT_FLAGS
	msr CPSR, r1
	bx lr

proc irqrestore
	/* Return interrupts to the state they were at when di() was called. */
	mrs r1, CPSR
	bic r1, r1, #INTERRUPT_FLAGS
	orr r1, r1, r0
	msr CPSR, r1
	bx lr

