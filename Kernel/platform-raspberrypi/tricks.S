#include "kernel.def"
#include "kernelarm.def"

.arch armv6

.globl switchin
proc switchout
	cpsid if                    // interrupts off
	mov r0, #0                  // set return code
	push {r0-r12, lr}           // save all registers (except sp)

	ldr r5, =udata_ptr          // belongs to old process
	ldr r5, [r5]
	str sp, [r5, #U_DATA__U_SP_OFFSET] // save sp

	ldr sp, =schedstack_top     // switch to scheduler stack
	bl getproc
switchin:
	cpsid if
	bl page_in_new_process

	ldr r5, =udata_ptr          // belongs to old process
	ldr r5, [r5]
	ldr sp, [r5, #U_DATA__U_SP_OFFSET] // load old sp

	cpsie if                    // interrupts on
	pop {r0-r12, pc}            // restore registers and return

proc doexec
	/* On entry, r0 contains the start address. */

	mov r4, r0               // r4 is now the entry address
	bl led_on

	mcr p15, 0, r0, cr7, cr10, 0 // clean data cache
	mcr p15, 0, r0, cr7, cr5, 0  // invalidate instruction cache
	mcr p15, 0, r0, cr7, cr5, 4  // flush prefetch buffer

	cpsid if                 // interrupts off

	ldr r5, =udata_ptr
	ldr r5, [r5]             // r5 is now the new process's udata

	mov r0, #0
	str r0, [r5, #U_DATA__U_INSYS_OFFSET]

	/* Initialise the process' kernel stack. */

	ldrh r0, [r5, #U_DATA__U_PAGE_OFFSET]
	bl get_svc_stack_for_page
	str r0, [r5, #U_DATA__U_SP_OFFSET]

	/* ...and now the process' user registers. */

	cps #MODE_SYS            // sys mode shares registers with usr mode
	ldr sp, [r5, #U_DATA__U_ISP_OFFSET]
	cpsie if, #MODE_USR      // switch to usr mode and enable interrupts

	bx r4                    // and go

proc platform_idle
	mcr p15, 0, r0, c7, c0, 4 // wait for interrupt
	bx lr

proc dofork
	mov r0, #4
	b led_halt_and_blink

proc svc_handler
	/*
	 * On entry, r0-r3 are the syscall parameters, and r4 is the number.
	 * On exit, r0 is the result and r1 is an errno.
	 *
	 * Interrupts are disabled (but not fast interrupts).
	 *
	 * The only spare register we have is sp, so we use that as a temporary.
	 */

	cpsid f                  // just to be sure

	/* Decode the SWI number. */

	ldr sp, [lr, #-4]
	bics sp, sp, #0xff000000

	bne swi1                 // swi 1 goes to our debug routine
swi0:                        // swi 0 is a syscall

	/* This is a system call. Before doing anything else, save the sycall
	 * parameters. */

	ldr sp, =udata_ptr
	ldr sp, [sp]

	strb r4, [sp, #U_DATA__U_CALLNO_OFFSET]
	str r0, [sp, #U_DATA__U_ARGN_OFFSET]
	str r1, [sp, #U_DATA__U_ARGN1_OFFSET]
	str r2, [sp, #U_DATA__U_ARGN2_OFFSET]
	str r3, [sp, #U_DATA__U_ARGN3_OFFSET]

	/* Okay, we finally have some registers spare. */

	mov r4, sp               // r4 becomes our new udata ptr

	ldr sp, =irqstack_top    // just for the next call
	push {lr}
	ldr r0, [r4, #U_DATA__U_PAGE_OFFSET]
	bl get_svc_stack_for_page
	pop {lr}
	mov sp, r0               // switch to the process' kernel stack

	ldr sp, [r4, #U_DATA__U_SP_OFFSET] // load the kernel stack
	srsfd sp!, #MODE_SVC     // save lr, spsr onto it
	stmfd sp!, {sp, lr}^     // ...and the user sp and lr

	/* We've now saved everything we need to be able to resume after the
	 * syscall is over (this is more complicated than it needs to be because
	 * we might need to deschedule during the execution of the system call,
	 * which means we can't leave anything in the supervisor or user
	 * registers). */

	mov r0, #1
	strb r0, [r4, #U_DATA__U_INSYS_OFFSET]

	cpsie if
	bl led_off
	bl unix_syscall
	bl led_on
	cpsid if

	mov r0, #0
	strb r0, [r4, #U_DATA__U_INSYS_OFFSET]

	// TODO: deliver signals here.

	/* Return back to user mode. */

	ldr r0, [r4, #U_DATA__U_RETVAL_OFFSET]
	ldr r1, [r4, #U_DATA__U_ERROR_OFFSET]
	ldmfd sp!, {sp, lr}^
	rfefd sp

swi1:
	/* Dump the contents of the registers and returns without doing anything
     * else. Used for debugging. */
	
	ldr sp, =irqstack_top
	srsfd sp!, #MODE_SVC
	stmfd sp!, {r0-r12, sp, lr}^

	mov r0, sp
	bl dump_user_registers_c

	ldmfd sp!, {r0-r12, sp, lr}^
	rfefd sp

