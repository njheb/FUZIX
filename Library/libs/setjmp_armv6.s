
	.section .text.setjmp

.global	setjmp
setjmp:
	/* r0: jmp_buf address */
	stmia r0, {r1-r13, lr}

	mov r0, #0
	bx lr

