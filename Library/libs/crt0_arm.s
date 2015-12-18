.section ".header"
.globl _start
_start:
	.ascii "bF68"
	.word 4                // rev
	.word 1f               // entrypoint
	.word __data_start
	.word __data_end
	.word __bss_end
	.word 4096             // stack_size
	.word 0                // reloc_start
	.word 0                // reloc_count
	.word 0                // flags
	.word 0, 0, 0, 0, 0, 0 // filler

1:
	/* Wipe BSS.*/

	ldr r0, =__bss_start
	mov r1, #0
	ldr r2, =__bss_end
	sub r2, r2, r0
	bl memset

	/* Initialise stdio. */

	bl __stdio_init_vars

	/* Pull argc and argv off the stack. */

	pop {r0, r1}

	/* What's left on the stack is the environment. */

	mov r2, sp
	ldr r3, =environ
	str r2, [r3, #0]

	/* When main returns, jump to _exit. */

	ldr lr, =exit
	b main
	
.globl environ
.comm environ, 4


