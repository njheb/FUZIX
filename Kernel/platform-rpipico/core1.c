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
#include "pico/util/queue.h"

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


extern queue_t rx_queue;
extern queue_t tx_queue;

int ypos = 0; //available to textmode.c for scrolling
static const int xmax=79;
//14 ''''''''t (c) 2014-2020 Alan Cox...etc
void cdc_drain(void)
{
static int xpos = 0;



//		if  (multicore_fifo_rvalid() && uart_is_writable(uart_default)) 
		if  (multicore_fifo_rvalid() ) 
		{
			int b = multicore_fifo_pop_blocking();
			uint8_t c;
			c=(uint8_t)b;


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

}

void cdc_task(void)
{
#endif //USE_SERIAL_ONLY
			

//		if (multicore_fifo_wready()
//			&& ((tud_cdc_connected() && tud_cdc_available())
//				|| uart_is_readable(uart_default)))
//		if (multicore_fifo_wready() && uart_is_readable(uart_default))
		//rx_character=-1;
		static int counter=0;

		if (
			!queue_is_full(&rx_queue) && 
			(uart_is_readable(uart_default) || 
			 (tud_cdc_connected() && tud_cdc_available())
			)  
		   )
		{
			/* Only service a byte from CDC *or* the UART, in case two show
			 * up at the same time and we end up blocking. No big loss, the
			 * next one will be read the next time around the loop. */
//			if (tud_cdc_available())
			if (tud_cdc_connected() && tud_cdc_available())
			{
				uint8_t b;
				int count = tud_cdc_read(&b, 1);
				if (count==1)
				{
				//	multicore_fifo_push_blocking(b);
			//		rx_character=b;
				//	queue_add_blocking(&rx_queue,&b);
					if (counter<5)
					{
						usbconsole_putc_blocking('!');
					}
					else
					{
						queue_add_blocking(&rx_queue,&b);
					}
				}
			}


//			if (uart_is_readable(uart_default) && multicore_fifo_wready())
			if (uart_is_readable(uart_default) && !queue_is_full(&rx_queue))
			{
				uint8_t b = uart_get_hw(uart_default)->dr;
				//multicore_fifo_push_blocking(b);
				queue_add_blocking(&rx_queue,&b);

			}

		}

			int tx_character=-1;
			int tx_count=0;
			uint8_t buffer[16];
			while (tud_cdc_connected() && tud_cdc_write_available() && !queue_is_empty(&tx_queue) && tx_count<sizeof(buffer))
			{
//				tud_cdc_write(&b, 1);
				uint8_t b;
				queue_remove_blocking(&tx_queue, &b);
				tx_character=b;
				if (counter<5)
				{/*see debugging usb startup in main.c*/
				    counter++;
				    usbconsole_putc_blocking('{');
				    usbconsole_putc_blocking(b);
				    usbconsole_putc_blocking('}');

				}
				buffer[tx_count++]=b;
				//tud_cdc_write_char(tx_character);
				//tud_cdc_write_flush();

			}
			if (tx_count!=0)
			{
				tud_cdc_write(buffer, tx_count);
				tud_cdc_write_flush();
			}

//right now serial tx will be very lossy, just trying out usb write rather than character at a time
//			uart_putc(uart_default, b);
			//right now this could be lossy on uart, assume we are doing i/o on usb com port
			//just for test, could have a tx_uart_queue and a usb_tx_queue
			if (uart_is_writable(uart_default) && tx_character!=-1)
				uart_putc(uart_default, tx_character);

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

