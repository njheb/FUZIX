#include <kernel.h>
#include <kdata.h>
#include <printf.h>
#include <stdbool.h>
#include <devtty.h>
#include <vt.h>
#include <tty.h>
#include "raspberrypi.h"
#include "externs.h"

static uint8_t ttybuf[TTYSIZ];

struct s_queue ttyinq[NUM_DEV_TTY+1] = { /* ttyinq[0] is never used */
	{ 0,      0,      0,      0,      0, 0        },
	{ ttybuf, ttybuf, ttybuf, TTYSIZ, 0, TTYSIZ/2 },
};

/* Output for the system console (kprintf etc) */
void kputchar(char c)
{
	if (c == '\n')
		tty_putc(0, '\r');
	tty_putc(0, c);
}

void tty_putc(uint8_t minor, unsigned char c)
{
	while (UART0.FR & (1<<5))
		;

	UART0.DR = (uint8_t)c;
}

void tty_sleeping(uint8_t minor)
{
}

ttyready_t tty_writeready(uint8_t minor)
{
	return (UART0.FR & (1<<5)) ? TTY_READY_NOW : TTY_READY_SOON;
}

void tty_setup(uint8_t minor)
{
	/* Already done on system boot (because we only support one TTY so far). */
}

/* For the moment */
int tty_carrier(uint8_t minor)
{
    return 1;
}

#if 0
void tty_interrupt(void)
{
	tty_inproc(minor(BOOT_TTY), UCA0RXBUF);
}
#endif

void tty_rawinit(void)
{
	/* UART0 is the full UART; UART1 is the mini UART */

	gpio_set_pin_func(14, GPIO_FUNC_ALT0, GPIO_PULL_OFF); /* UART0_TXD */
	gpio_set_pin_func(15, GPIO_FUNC_ALT0, GPIO_PULL_OFF); /* UART0_RXD */

	/* Clear pending interruppts. */

	UART0.ICR = 0;

	/* Set integer & fractional part of baud rate.
	 * Divider = UART_CLOCK/(16 * Baud)
	 * Fraction part register = (Fractional part * 64) + 0.5
	 * UART_CLOCK = 3000000; Baud = 115200.
     * 
	 * Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
	 * Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
	 */
	const int baud = 115200;
	const int uart_clock = 3000000;
	const int divider = (64*uart_clock) / (16 * baud);
	UART0.IBRD = divider / 64;
	UART0.FBRD = divider % 64;
 
	/* Enable FIFO & 8 bit data transmissio (1 stop bit, no parity). */

	UART0.LCRH = (1 << 4) | (1 << 5) | (1 << 6);
 
	/* Mask all interrupts. */

	UART0.IMSC = (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
	             (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10);
 
	/* Enable UART0, receive & transfer part of UART. */

	UART0.CR = (1 << 0) | (1 << 8) | (1 << 9);
}

