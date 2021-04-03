/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <tusb.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <hardware/uart.h>
#include <hardware/irq.h>
#include "core1.h"

bool usbconsole_is_readable(void)
{
	return multicore_fifo_rvalid();
}

bool usbconsole_is_writable(void)
{
	return multicore_fifo_wready();
}

uint8_t usbconsole_getc_blocking(void)
{
	return multicore_fifo_pop_blocking();
}

void usbconsole_putc_blocking(uint8_t b)
{
	multicore_fifo_push_blocking(b);
}

//overloading the use of USE_SERIAL_ONLY for the moment
//only interested in the effect of #undef USE_SERIAL_ONLY
#ifdef USE_SERIAL_ONLY
static void core1_main(void)
{
    uart_init(uart_default, PICO_DEFAULT_UART_BAUD_RATE);
    gpio_set_function(PICO_DEFAULT_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(PICO_DEFAULT_UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_translate_crlf(uart_default, false);
    uart_set_fifo_enabled(uart_default, true);

    tusb_init();

	for (;;)
	{
		tud_task();

#else
/*
 don't forget 

    tusb_init();
		tud_task();

NB board_init(); not needed
check if crlf translate already setup or needed?
*/

int ypos = 0; //available to textmode.c for scrolling
static const int xmax=79;
//14 ''''''''t (c) 2014-2020 Alan Cox...etc
void cdc_task(void)
{
#endif //USE_SERIAL_ONLY
//static bool first = false;
static int xpos = 0;
extern char message_text[25][81]; //partial line 26 is a shadow of line 0 on display


//		if (multicore_fifo_rvalid()
//			&& (!tud_cdc_connected() || tud_cdc_write_available())
//			&& uart_is_writable(uart_default))

//		if ( (multicore_fifo_rvalid() && (tud_cdc_connected() && tud_cdc_write_available()) )
//		     ||
//		     (multicore_fifo_rvalid() && uart_is_writable(uart_default)) )

		if ( (multicore_fifo_rvalid())
		     ||
		     (multicore_fifo_rvalid() && uart_is_writable(uart_default)) )
		{
			int b = multicore_fifo_pop_blocking();
			uint8_t c;
			c=(uint8_t)b;

			if (tud_cdc_connected() && tud_cdc_write_available())
			{
//				tud_cdc_write(&b, 1);
				tud_cdc_write_char(c);
				tud_cdc_write_flush();
			}

//			uart_putc(uart_default, b);
			if (uart_is_writable(uart_default))
				uart_putc(uart_default, c);

			if (!(c=='\r' || c=='\n' || c==8))
			{
			    message_text[ypos][xpos]=c;
			}
			if (c == '\r') 
			{
			    xpos=0;
			}
			else if (c == '\n')
			{ 
			    ypos++;
			    if (ypos > 25) ypos=0;

			    for (int i=0;i<xmax;i++)
				message_text[ypos][i]='\0';

			}
			else if (c==8)
			{
			    xpos--;
			    if (xpos==2) xpos=3;

			    message_text[ypos][xpos]='\0';
			} 
			else
				xpos++;

			if (xpos > xmax)
			{
			  xpos = 0;
			  ypos++;
			  if (ypos>25) ypos = 0;
			}

		}
			

		if (multicore_fifo_wready()
			&& ((tud_cdc_connected() && tud_cdc_available())
				|| uart_is_readable(uart_default)))
		{
			/* Only service a byte from CDC *or* the UART, in case two show
			 * up at the same time and we end up blocking. No big loss, the
			 * next one will be read the next time around the loop. */
			static int c;
			if (tud_cdc_available())
			{
				uint8_t b; c=-1;
				int count = tud_cdc_read(&b, 1);
				if (count==1)
				{
					multicore_fifo_push_blocking(b);
					c=b;
				}
			}
//			else if (uart_is_readable(uart_default))
			if (uart_is_readable(uart_default) && multicore_fifo_wready())
			{
				uint8_t b = uart_get_hw(uart_default)->dr; c=b;
				multicore_fifo_push_blocking(b);
			}

		}
#ifdef USE_SERIAL_ONLY
	}
}
#else
}
#endif // USE_SERIAL_ONLY

#ifdef USE_SERIAL_ONLY
void core1_init(void)
{
	multicore_launch_core1(core1_main);
}
#endif

