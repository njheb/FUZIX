#include <kernel.h>
#include <printf.h>
#include <kdata.h>
#include "raspberrypi.h"
#include "externs.h"

extern uint8_t __bssstart;
extern uint8_t __bssend;
extern uint8_t __kerneltop;
extern uint8_t __vectors;
extern uint8_t __vectorsstart;
extern uint8_t __vectorsend;

uaddr_t ramtop;

void jtag_init(void)
{
	gpio_set_pin_func(4, GPIO_FUNC_ALT5, GPIO_PULL_OFF); /* TDI */
	gpio_set_pin_func(22, GPIO_FUNC_ALT4, GPIO_PULL_OFF); /* TRST */
	gpio_set_pin_func(24, GPIO_FUNC_ALT4, GPIO_PULL_OFF); /* TDO */
	gpio_set_pin_func(25, GPIO_FUNC_ALT4, GPIO_PULL_OFF); /* TCK */
	gpio_set_pin_func(27, GPIO_FUNC_ALT4, GPIO_PULL_OFF); /* TMS */
}

void pagemap_init(void)
{
	/* We use megabyte pages --- gross overkill for Fuzix, but it's easy.
	 * The kernel occupies page 0 and is mapped at 0. User processes occupy
	 * all the others, and each one is mapped at 0x80000000. */

	int i;
	for (i=2; i<PTABSIZE; i++)
		pagemap_add(i);

	/* Page 1 must be mapped *last*, so that it gets allocated first. */

	pagemap_add(1);
}

void map_init(void)
{
}

void program_vectors(uint16_t* pageptr)
{
}

void platform_discard(void)
{
}

void platform_init(uint8_t* atags)
{
	/* Copy the exception vectors to their final home. */

	memcpy(&__vectors, &__vectorsstart, &__vectorsend - &__vectorsstart);

	/* Wipe BSS. */

	memset(&__bssstart, 0, &__bssend - &__bssstart);

	/* Create a 1:1 TLB table and turn it on, so that our peripherals end up
	 * being in a known good location. */

	/* Kernel page 1:1 mapping */
	set_tlb_entry(0x00000000, 0x00000000, CACHED|BUFFERED|PRIV);

	/* The 16 peripheral pages (we map them to the Pi2 virtual address, for
	 * simplicity) */
	for (int page=0; page<0x10; page++)
	{
		uint32_t offset = page * MEGABYTE;
		set_tlb_entry(0x3f000000|offset, 0x20000000|offset, PRIV);
	}

	/* ...and our user address space. */
	set_tlb_entry((uint32_t)&__progbase, 1*MEGABYTE, CACHED|BUFFERED|USER);
	enable_mmu();

	/* Initialise system peripherals. */

	kmemaddblk(&__bssend, &__kerneltop - &__bssend);
	led_init();
	//jtag_init();
	tty_rawinit();

	/* Detect how much memory we have. */

	ramsize = mbox_get_arm_memory() / 1024;
	procmem = ramsize - 1024; /* reserve 1MB for the kernel */

	/* Make sure the udata block is sane. Note that We've ensured that init
	 * will go in page 1, by means of the nasty logic in pagemap_init().
	 */

	memset(&udata, 0, sizeof(udata));
	udata.u_page = 1;

	/* And go! */

	fuzix_main();
}

/* Uget/Uput 32bit */

uint32_t ugetl(void *uaddr)
{
	if (!valaddr(uaddr, 4)) {
		return -1;
	}
	return *(uint32_t *)uaddr;

}

int uputl(uint32_t val, void *uaddr)
{
	if (!valaddr(uaddr, 4))
		return -1;
	*(uint32_t *)uaddr = val;
	return 0;
}

void trap_monitor(void)
{
	led_halt_and_blink(3);
}

void unexpected_handler_c(const char* reason, uint32_t address)
{
	kprintf("panic: unexpected %s at pc 0x%x\n", reason, address);
	for (;;);
}

void dabt_handler_c(void)
{
	uint32_t insn = (uint32_t)__builtin_return_address(0) - 8;
	uint32_t reason = mrc(15, 0, 5, 0, 0) & 0xf;
	uint32_t fault_addr = mrc(15, 0, 6, 0, 0);
	kprintf("abort for address %x at %x because %x\n", fault_addr, insn, reason);
	for (;;);
}

struct registers
{
	uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr;
	uint32_t pc, spsr;
};

void dump_user_registers_c(struct registers* r)
{
	kprintf("User mode registers:\n"
	        "spsr: %x %c%c%c%c%c %c%c mode: %x\n"
	        " r0: %x  r4: %x  r8: %x r12: %x\n"
			" r1: %x  r5: %x  r9: %x  sp: %x\n"
			" r2: %x  r6: %x r10: %x  lr: %x\n"
			" r3: %x  r7: %x r11: %x  pc: %x\n",
			r->spsr,
			(r->spsr & (1<<31)) ? 'N' : 'n',
			(r->spsr & (1<<30)) ? 'Z' : 'z',
			(r->spsr & (1<<29)) ? 'C' : 'c',
			(r->spsr & (1<<28)) ? 'V' : 'v',
			(r->spsr & (1<<27)) ? 'Q' : 'q',
			(r->spsr & (1<<7)) ? 'i' : 'I',
			(r->spsr & (1<<6)) ? 'f' : 'F',
			r->spsr & 0x1f,
			r->r0, r->r4, r->r8, r->r12,
			r->r1, r->r5, r->r9, r->sp,
			r->r2, r->r6, r->r10, r->lr,
			r->r3, r->r7, r->r11, r->pc);
}

void print_r0(uint32_t r0)
{
	kprintf("<%x>\n", r0);
}

