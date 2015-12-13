.section .text.longjmp
.global longjmp
longjmp:
	/* r0: address of jmp_buf
	 * r1: value which setjmp should raturn to
	 */
	mov r2, r0
	mov r0, r1
	ldmia r2, {r1-r13, pc}
