#include <kernel.h>
#include <kdata.h>
#include <printf.h>
#include <stdbool.h>
#include <vt.h>
#include <tty.h>
#include "picosdk.h"
#include <hardware/uart.h>
//#include <hardware/irq.h>
#include <pico/multicore.h>
#include "core1.h"
#include "queue_shim.h"

static uint8_t ttybuf[TTYSIZ];

struct s_queue ttyinq[NUM_DEV_TTY+1] = { /* ttyinq[0] is never used */
	{ 0,         0,         0,         0,      0, 0        },
	{ ttybuf,    ttybuf,    ttybuf,    TTYSIZ, 0, TTYSIZ/2 },
};

tcflag_t termios_mask[NUM_DEV_TTY+1] = { 0, _CSYS };

/* Output for the system console (kprintf etc) */

void kputchar(uint_fast8_t c)
{
    if (c == '\n')
//        usbconsole_putc_blocking('\r');
//    usbconsole_putc_blocking(c);
	shim_push_vga_queue_blocking('\r');
    shim_push_vga_queue_blocking(c);

    if (c == '\n')
    	shim_push_tx_queue_blocking('\r');

    shim_push_tx_queue_blocking(c);
}

void tty_putc(uint_fast8_t minor, uint_fast8_t c)
{
	kputchar(c);
}

ttyready_t tty_writeready(uint_fast8_t minor)
{
//    return usbconsole_is_writable() ? TTY_READY_NOW : TTY_READY_SOON;
    return !shim_vga_queue_is_full() ? TTY_READY_NOW : TTY_READY_SOON;
}

/* For the moment */
int tty_carrier(uint_fast8_t minor)
{
    return 1;
}

void tty_sleeping(uint_fast8_t minor) {}
void tty_data_consumed(uint_fast8_t minor) {}
void tty_setup(uint_fast8_t minor, uint_fast8_t flags) {}

/*
static void tty_isr(void)
{
    while (uart_is_readable(uart_default))
    {
        uint8_t b = uart_get_hw(uart_default)->dr;
        tty_inproc(minor(BOOT_TTY), b);
    }
}
*/

void tty_rawinit(void)
{
    uart_init(uart_default, PICO_DEFAULT_UART_BAUD_RATE);
    gpio_set_function(PICO_DEFAULT_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(PICO_DEFAULT_UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_translate_crlf(uart_default, false);
    uart_set_fifo_enabled(uart_default, true);
/*
    uart_set_translate_crlf(uart_default, true);
    uart_set_fifo_enabled(uart_default, false);

    int irq = (uart_default == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(irq, tty_isr);
    irq_set_enabled(irq, true);
    uart_set_irq_enables(uart_default, true, false);
*/
}


/* vim: sw=4 ts=4 et: */

