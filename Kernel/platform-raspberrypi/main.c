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

#define MEGABYTE (1024*1024)
extern volatile uint32_t tlbtable[4096];

#define CACHED (1<<3)
#define BUFFERED (1<<2)

#define CR_M (1<<0) /* MMU on */
#define CR_A (1<<1) /* Strict alignment checking */
#define CR_C (1<<2) /* L1 data cache on */
#define CR_Z (1<<11) /* Branch flow prediction on */
#define CR_I (1<<12) /* L1 instruction cache on */

uaddr_t ramtop;

void set_tlb_entry(uint32_t virtual, uint32_t physical, uint32_t flags)
{
	int page = virtual / MEGABYTE;
	tlbtable[page] = physical | flags
		| (3<<10) /* AP = 3 (global access) */
		| (2<<0)  /* 1MB page */
		;
}

void jtag_init(void)
{
	gpio_set_pin_func(4, GPIO_FUNC_ALT5, GPIO_PULL_OFF); /* TDI */
	gpio_set_pin_func(22, GPIO_FUNC_ALT4, GPIO_PULL_OFF); /* TRST */
	gpio_set_pin_func(24, GPIO_FUNC_ALT4, GPIO_PULL_OFF); /* TDO */
	gpio_set_pin_func(25, GPIO_FUNC_ALT4, GPIO_PULL_OFF); /* TCK */
	gpio_set_pin_func(27, GPIO_FUNC_ALT4, GPIO_PULL_OFF); /* TMS */
}

static void change_control_register(uint32_t set, uint32_t reset)
{
	uint32_t value = mrc(15, 0, 1, 0, 0);
	value |= set;
	value &= ~reset;
	mcr(15, 0, 1, 0, 0, value);
}

static void enable_mmu(void)
{
	mcr(15, 0, 3, 0,  0, 0x3); /* domain */
	mcr(15, 0, 2, 0,  0, tlbtable); /* tlb base register 0 */
	mcr(15, 0, 2, 0,  1, tlbtable); /* tlb base register 1 */

	change_control_register(CR_A, CR_M|CR_C|CR_I);

	mcr(15, 0, 7, 7,  0, 0); /* invalidate caches */
	mcr(15, 0, 8, 7,  0, 0); /* invalidate entire TLB */

	change_control_register(CR_M|CR_C|CR_I, 0);
}

static inline void tlb_flush(void* address)
{
	mcr(15, 0, 8, 7, 1, address);
}

void pagemap_init(void)
{
	/* We use megabyte pages --- gross overkill for Fuzix, but it's easy.
	 * The kernel occupies page 0 and is mapped at 0. User processes occupy
	 * all the others, and each one is mapped at 0x80000000. */

	int i;
	for (i=1; i<PTABSIZE; i++)
		pagemap_add(i);
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

	set_tlb_entry(0x00000000, 0x00000000, CACHED|BUFFERED); /* Kernel 1:1 mapping */
	set_tlb_entry(0x3f200000, 0x20200000, 0);               /* I/O ports (use Pi2 virtual address) */
	set_tlb_entry(0x80000000, 0x00100000, CACHED|BUFFERED); /* Startup process */
	enable_mmu();

	/* Detect how much memory we have. */

	ramsize = 1024 * 1024;
	procmem = 1023 * 1024;
	kmemaddblk(&__bssend, &__kerneltop - &__bssend);

	/* Initialise system peripherals. */

	jtag_init();
	tty_rawinit();

	/* And go! */

	fuzix_main();
}

/* Uget/Uput 32bit */

uint32_t ugetl(void *uaddr, int *err)
{
	if (!valaddr(uaddr, 4)) {
		if (err)
			*err = -1;
		return -1;
	}
	if (err)
		*err = 0;
	return *(uint32_t *)uaddr;

}

int uputl(uint32_t val, void *uaddr)
{
	if (!valaddr(uaddr, 4))
		return -1;
	return *(uint32_t *)uaddr;
}


