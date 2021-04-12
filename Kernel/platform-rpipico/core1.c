/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <tusb.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <pico/binary_info.h>
//#include <pico/multicore.h>
#include <hardware/uart.h>
#include <hardware/irq.h>
#include "core1.h"
#include "pico/util/queue.h"


extern queue_t rx_queue;
extern queue_t tx_queue;
extern queue_t vga_tx_queue;

int ypos = 0; //available to textmode.c for scrolling
static const int xmax=79;

void vga_drain(void)
{
static int xpos = 0;

		while  (!queue_is_empty(&vga_tx_queue) ) 
		{
			uint8_t c;

			queue_remove_blocking(&vga_tx_queue, &c);


			if (!(c=='\r' || c=='\n' || c==8))
			{
			    	message_text[ypos%25][xpos]=c;

			   	if (!((ypos>=0) && (ypos<=24)))
				{
			   	  //this should not need checking, but looking for cause of erratic behaviour
				// not managed to trigger problem so far with ypos%25 used as index
				const uint LED_PIN = 25;
    				gpio_init(LED_PIN);
    				gpio_set_dir(LED_PIN, GPIO_OUT);
				gpio_put(LED_PIN, 1);
				}
			}
			if (c == '\r') 
			{
			    xpos=0;
			}
			else if (c == '\n')
			{ 
			    ypos++;
			    if (ypos >= 25) ypos=0;

			    for (int i=0;i<xmax;i++)
				message_text[ypos][i]='\0';

			}
			else if (c==8)
			{
			    xpos--;
			    if (xpos==1) xpos=2;

			    message_text[ypos][xpos]='\0';
			} 
			else
				xpos++;

			if (xpos > xmax)
			{
			  xpos = 0;
			  ypos++;
			  if (ypos>=25) ypos = 0;
			}

		}

}

void cdc_task(void)
{


		if (
			!queue_is_full(&rx_queue) && 
			(uart_is_readable(uart_default) || 
			 (tud_cdc_connected() && tud_cdc_available())
			)  
		   )
		{
/*
 * Need to look at moving usb com into a separate device
 *
 */
			if (tud_cdc_connected() && tud_cdc_available())
			{
				uint8_t b;
				int count = tud_cdc_read(&b, 1);
				if (count==1)
				{
					queue_add_blocking(&rx_queue,&b);
				}
			}


			if (uart_is_readable(uart_default) && !queue_is_full(&rx_queue))
			{
				uint8_t b = uart_get_hw(uart_default)->dr;
				queue_add_blocking(&rx_queue,&b);
			}

		}

			int tx_count=0;
			uint8_t buffer[32];
			int tx_limit = tud_cdc_connected() ? MIN(sizeof(buffer), tud_cdc_write_available()) : sizeof(buffer);

			while (uart_is_writable(uart_default) && tx_count<tx_limit && !queue_is_empty(&tx_queue))
			{
				uint8_t b;
				queue_remove_blocking(&tx_queue, &b);

				buffer[tx_count++]=b;
				uart_putc(uart_default, b);
			}


			if (tud_cdc_connected() && tud_cdc_write_available() && tx_count)
			{
				tud_cdc_write(buffer, tx_count);
				tud_cdc_write_flush();

			}

}


