#include "queue_shim.h"

#include <kernel.h>
#include <version.h>
#include <kdata.h>
#include <devsys.h>
#include <blkdev.h>
#include <tty.h>
#include <devtty.h>
#include <dev/devsd.h>
#include <printf.h>
//#include "printf.h"
#include "globals.h"
#include "picosdk.h"
#include <hardware/irq.h>
#include <hardware/structs/timer.h>
#ifndef USE_SERIAL_ONLY
#include <pico/multicore.h>
#include "core1.h"
#endif
extern void tusb_init(void);
extern void tud_task(void);

#include "textmode/textmode.h"

struct devsw dev_tab[] =  /* The device driver switch table */
{
// minor    open         close        read      write           ioctl
// ---------------------------------------------------------------------
  /* 0: /dev/hd - block device interface */
  {  blkdev_open,   no_close,   blkdev_read,    blkdev_write,	blkdev_ioctl},
  /* 1: /dev/fd - Floppy disk block devices */
  {  no_open,	    no_close,	no_rdwr,	no_rdwr,	no_ioctl},
  /* 2: /dev/tty	TTY devices */
  {  tty_open,     tty_close,   tty_read,  tty_write,  tty_ioctl },
  /* 3: /dev/lpr	Printer devices */
  {  no_open,     no_close,   no_rdwr,   no_rdwr,  no_ioctl  },
  /* 4: /dev/mem etc	System devices (one offs) */
  {  no_open,      no_close,    sys_read, sys_write, sys_ioctl  },
  /* Pack to 7 with nxio if adding private devices and start at 8 */
};

//static absolute_time_t now;

bool validdev(uint16_t dev)
{
    /* This is a bit uglier than needed but the right hand side is
       a constant this way */
    if(dev > ((sizeof(dev_tab)/sizeof(struct devsw)) << 8) - 1)
	return false;
    else
        return true;
}

struct repeating_timer timer;

bool timer_tick_cb(struct repeating_timer *t)
{
    int rx_character;

    tud_task();
    irqflags_t f = di();
    cdc_task();
    irqrestore(f);

    rx_character = shim_pop_rx_queue();
    if (rx_character!=-1)
    {
	uint8_t c=rx_character;
        tty_inproc(minor(BOOT_TTY), c);
    }

    timer_interrupt();

    return true;
}

void device_init(void)
{
    /* The flash device is too small to be useful, and a corrupt flash will
     * cause a crash on startup... oddly. */
//	queue_init(&rx_queue, sizeof(uint8_t), 32);
//	queue_init(&tx_queue, sizeof(uint8_t), 32);
//	shim_init_queues(); //moved to main
	tusb_init();
	flash_dev_init();
	sd_rawinit();

	devsd_init();
//    add_repeating_timer_ms(-(1000/TICKSPERSEC), timer_tick_cb, NULL, &timer);
    add_repeating_timer_ms((1000/TICKSPERSEC), timer_tick_cb, NULL, &timer);

#if 1
//eat up a stray character on uart, should find cause
//current debug is to show '!' between [] if stray usb rx char comes in
//see the now very badly named core1.c
    usbconsole_putc_blocking('[');

    while (uart_is_readable(uart_default))
    {
       uint8_t b= uart_get_hw(uart_default)->dr;
    }
#endif

    int holdoff_left=shim_extra(10000); //wait for usb to connect in ms

    usbconsole_putc_blocking(']');

    sleep_ms(10); //5ms not enough, 10ms ok, need grace time for tinyusb 
                  //or there will be a loss of characters
    if (holdoff_left!=-1)
    	kprintf("\n<<%dms>>\n",10000-holdoff_left);

    holdoff_left=shim_extra(1); //another go at draining the usb rx buffer

}

/* vim: sw=4 ts=4 et: */

