#include "kernel.def"
#include "kernelarm.def"

proc switchin
	b switchin

proc switchout
	b switchout

proc doexec
	b doexec

proc platform_idle
	b platform_idle

proc dofork
	b dofork

proc svc_handler
	/* We assume that the svc stack has already been correctly set up when
	 * this process was switched in.
	 *
	 * On entry, r0-r3 are the syscall parameters, and r4 is the number.
	 * On exit, r0 is the result and r1 is an errno.
	 */
	
	push {r5, lr}
	ldr r5, =udata

	strb r4, [r5, #U_DATA__U_CALLNO_OFFSET]
	str r0, [r5, #U_DATA__U_ARGN_OFFSET]
	str r1, [r5, #U_DATA__U_ARGN1_OFFSET]
	str r2, [r5, #U_DATA__U_ARGN2_OFFSET]
	str r3, [r5, #U_DATA__U_ARGN3_OFFSET]

	mov r0, #1
	strb r0, [r5, #U_DATA__U_INSYS_OFFSET]

	bl unix_syscall

	cpsid if

	mov r0, #0
	strb r0, [r5, #U_DATA__U_INSYS_OFFSET]

	// TODO: deliver signals here.

	ldr r0, [r5, #U_DATA__U_RETVAL_OFFSET]
	ldr r1, [r5, #U_DATA__U_ERROR_OFFSET]
	pop {r5, pc}^

