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

void set_tlb_entry(uint32_t virtual, uint32_t physical, uint32_t flags)
{
	int page = virtual / MEGABYTE;
	tlbtable[page] = physical | 0xc00 | flags | 2;
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

void platform_init(uint8_t* atags)
{
	/* Create a 1:1 TLB table and turn it on, so that our peripherals end up
	 * being in a known good location. */

	memset((void*) tlbtable, 0, sizeof(tlbtable));
	set_tlb_entry(0x00000000, 0x00000000, CACHED|BUFFERED); /* Kernel 1:1 mapping */
	set_tlb_entry(0x3f000000, 0x3f000000, 0); /* I/O ports 1:1 mapping */
	enable_mmu();
	tlb_flush(&platform_init);

	/* Wipe BSS. */

	memset(&__bssstart, 0, &__bssend - &__bssstart);

	/* Initialise system peripherals. */

	jtag_init();
	tty_rawinit();

	kprintf("Hello, world!\n");
	for (;;);
}

void panic(char* message)
{
	kprintf("panic: %s\n", message);
	for (;;);
}

