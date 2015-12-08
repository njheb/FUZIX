#include <kernel.h>
#include <printf.h>
#include "raspberrypi.h"
#include "externs.h"

extern uint8_t __bssstart;
extern uint8_t __bssend;

#define MEGABYTE (1024*1024)
extern volatile uint32_t tlbtable[4096];

#define CACHED (1<<3)
#define BUFFERED (1<<2)

#define CR_M (1<<0) /* MMU on */
#define CR_A (1<<1) /* Strict alignment checking */
#define CR_C (1<<2) /* L1 data cache on */
#define CR_Z (1<<11) /* Branch flow prediction on */
#define CR_I (1<<12) /* L1 instruction cache on */

extern volatile uint32_t AUX_ENABLES;
extern volatile uint32_t AUX_MU_IO_REG;
extern volatile uint32_t AUX_MU_IER_REG;
extern volatile uint32_t AUX_MU_IIR_REG;
extern volatile uint32_t AUX_MU_LCR_REG;
extern volatile uint32_t AUX_MU_MCR_REG;
extern volatile uint32_t AUX_MU_LSR_REG;
extern volatile uint32_t AUX_MU_MSR_REG;
extern volatile uint32_t AUX_MU_SCRATCH;
extern volatile uint32_t AUX_MU_CNTL_REG;
extern volatile uint32_t AUX_MU_STAT_REG;
extern volatile uint32_t AUX_MU_BAUD_REG;

extern volatile uint32_t GPFSEL1;
extern volatile uint32_t GPSET0;
extern volatile uint32_t GPCLR0;
extern volatile uint32_t GPPUD;
extern volatile uint32_t GPPUDCLK0;

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
	/* Disable pullup/pulldown for the JTAG pins. */

	GPIO.PUD = 0;
	busy_wait(150);
	GPIO.PUDCLK0 = (1<<4) | (1<<22) | (1<<24) | (1<<25) | (1<<27);
	busy_wait(150);
	GPIO.PUDCLK0 = 0;

	GPIO.FSEL1 = GPIO.FSEL1
		& ~(7<<12) /* GPIO4 */
		| (2<<12) /* = alt5 (ARM_TDI) */
		;

	GPIO.FSEL2 = GPIO.FSEL2
		& ~(7<<6) /* GPIO22 */
		| (3<<6) /* = alt4 (ARM_TRST) */
		& ~(7<<12) /* GPIO24 */
		| (3<<12) /* = alt4 (ARM_TDO) */
		& ~(7<<15) /* GPIO25 */
		| (3<<15) /* = alt4 (ARM_TCK) */
		& ~(7<<21) /* GPIO27 */
		| (3<<21) /* = alt4 (ARM_TMS) */
		;
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

void platform_init(uint8_t* atags)
{
	/* Create a 1:1 TLB table and turn it on, so that our peripherals end up
	 * being in a known good location. */

	memset((void*) tlbtable, 0, sizeof(tlbtable));
	set_tlb_entry(0x00000000, 0x00000000, CACHED|BUFFERED); /* Kernel 1:1 mapping */
	set_tlb_entry(0x3f200000, 0x3f200000, 0);               /* I/O ports 1:1 mapping */
	set_tlb_entry(0x80000000, 0x00200000, CACHED|BUFFERED); /* Startup process */
	enable_mmu();

	/* Wipe BSS. */

	memset(&__bssstart, 0, &__bssend - &__bssstart);

	/* Initialise system peripherals. */

	jtag_init();
	tty_rawinit();

	/* And go! */

	fuzix_main();
}

