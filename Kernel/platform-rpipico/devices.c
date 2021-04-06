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

static absolute_time_t now;

bool validdev(uint16_t dev)
{
    /* This is a bit uglier than needed but the right hand side is
       a constant this way */
    if(dev > ((sizeof(dev_tab)/sizeof(struct devsw)) << 8) - 1)
	return false;
    else
        return true;
}

static absolute_time_t next;

static void timer_tick_cb_body(unsigned alarm)
{
//    absolute_time_t next;
    update_us_since_boot(&next, to_us_since_boot(now) + (1000000 / TICKSPERSEC));
    if (hardware_alarm_set_target(0, next)) 
    {
        update_us_since_boot(&next, time_us_64() + (1000000 / TICKSPERSEC));
        hardware_alarm_set_target(0, next);
    }
    tud_task();
    cdc_task();
    timer_interrupt();
}

//queue_t rx_queue;
//queue_t tx_queue;
//these are now in queue_shim.c

static void timer_tick_cb(unsigned alarm)
{
    static int rx_character;
    timer_tick_cb_body(alarm);
    rx_character = shim_pop_rx_queue();
    if (rx_character!=-1)
    {
	uint8_t c=rx_character;
        tty_inproc(minor(BOOT_TTY), c);
    }
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

    hardware_alarm_claim(0);
    update_us_since_boot(&now, time_us_64());

    hardware_alarm_set_callback(0, timer_tick_cb); 

//    	while (usbconsole_is_readable())
//    	{
 //       	uint8_t c = usbconsole_getc_blocking();
//	}

    timer_tick_cb(0); //spurious char here
       // timer_tick_cb_body(0); //improvising

    int holdoff_left=shim_extra(10000); //wait for usb to connect
    kprintf("\n<<%d>>\n",holdoff_left);

//njh
//njh have to move this forward  (void)video_main();
//njh hope the problem with flash_dev_init starting after has been fixed
//njh by the di() ei() wrapping in there now
}

/* vim: sw=4 ts=4 et: */

