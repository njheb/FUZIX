#include <kernel.h>
#include <kdata.h>
#include <printf.h>
#include <stdbool.h>
#include <vt.h>
#include <tty.h>
#include "picosdk.h"
#ifdef USE_SERIAL_ONLY
#include <hardware/uart.h>
#include <hardware/irq.h>
#else
#include <pico/multicore.h>
#include "core1.h"
#endif

static uint8_t ttybuf[TTYSIZ];

struct s_queue ttyinq[NUM_DEV_TTY+1] = { /* ttyinq[0] is never used */
	{ 0,         0,         0,         0,      0, 0        },
	{ ttybuf,    ttybuf,    ttybuf,    TTYSIZ, 0, TTYSIZ/2 },
};

tcflag_t termios_mask[NUM_DEV_TTY+1] = { 0, _CSYS };

/* Output for the system console (kprintf etc) */
void kputchar(uint_fast8_t c)
{
#ifdef USE_SERIAL_ONLY
	uart_putc(uart_default, c);
#else
    if (c == '\n')
        usbconsole_putc_blocking('\r');
    usbconsole_putc_blocking(c);
#endif
}

void tty_putc(uint_fast8_t minor, uint_fast8_t c)
{
	kputchar(c);
}

ttyready_t tty_writeready(uint_fast8_t minor)
{
#ifdef USE_SERIAL_ONLY
    return uart_is_writable(uart_default) ? TTY_READY_NOW : TTY_READY_SOON;
#else
    return usbconsole_is_writable() ? TTY_READY_NOW : TTY_READY_SOON;
#endif
}

/* For the moment */
int tty_carrier(uint_fast8_t minor)
{
    return 1;
}

void tty_sleeping(uint_fast8_t minor) {}
void tty_data_consumed(uint_fast8_t minor) {}
void tty_setup(uint_fast8_t minor, uint_fast8_t flags) {}

#ifdef USE_SERIAL_ONLY
static void tty_isr(void)
{
    while (uart_is_readable(uart_default))
    {
        uint8_t b = uart_get_hw(uart_default)->dr;
        tty_inproc(minor(BOOT_TTY), b);
    }
}


void tty_rawinit(void)
{
    uart_init(uart_default, PICO_DEFAULT_UART_BAUD_RATE);
    gpio_set_function(PICO_DEFAULT_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(PICO_DEFAULT_UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_translate_crlf(uart_default, true);
    uart_set_fifo_enabled(uart_default, false);

    int irq = (uart_default == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(irq, tty_isr);
    irq_set_enabled(irq, true);
    uart_set_irq_enables(uart_default, true, false);
}

#endif

/* vim: sw=4 ts=4 et: */

