#include "kernel.def"

.section .bss
.globl kernelstack
kernelstack:
	.fill 1024
kernelstack_top:

.text
.globl _start
_start:
	// Exception table.

	b reset // 0x00: Reset
	b reset // 0x04: Undefined instruction
	b reset // 0x08: SWI
	b reset // 0x0c: Prefetch abort
	b reset // 0x10: Data abort
	b reset // 0x14: Hypervisor trap (?)
	b reset // 0x18: IRQ interrupt
	b reset // 0x1c: FIQ interrupt

proc reset
	// On entry:
	//   r0 = 0
	//   r1 = arm machine (useless on the Pi)
	//   r2 = physical address of atags list

	// Tell the processor where the exception table is.
	ldr r0, =_start
	mcr p15, 0, r0, c12, c0, 0

	mov r0, r2
	ldr sp, =kernelstack_top
	b platform_init

proc busy_wait
	subs r0, r0, #1
	bne busy_wait
	bx lr
	
