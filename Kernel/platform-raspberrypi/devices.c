#include <kernel.h>
#include <version.h>
#include <kdata.h>
#include <tty.h>
#include <devsd.h>
#include <devsys.h>
#include <blkdev.h>
#include <printf.h>
#include <timer.h>
#include <devsd.h>
#include "raspberrypi.h"
#include "externs.h"

#define TIMER_INTERVAL 1000000
//#define TIMER_INTERVAL (1000000 / TICKSPERSEC)

uint8_t need_resched;

struct devsw dev_tab[] =  /* The device driver switch table */
{
// minor    open         close        read      write       ioctl
// -----------------------------------------------------------------
  /* 0: /dev/sd	   SD disk  */
  {  blkdev_open,  no_close,    blkdev_read, blkdev_write, blkdev_ioctl },
  /* 1: /dev/hd    Hard disc block devices (sd card) */
  {  no_open,      no_close,    no_rdwr,     no_rdwr,      no_ioctl },
  /* 2: /dev/tty   TTY devices */
  {  tty_open,     tty_close,   tty_read,    tty_write,    tty_ioctl },
  /* 3: /dev/lpr   Printer devices */
  {  no_open,      no_close,    no_rdwr,     no_rdwr,      no_ioctl  },
  /* 4: /dev/mem etc	System devices (one offs) */
  {  no_open,      no_close,    sys_read,    sys_write,    sys_ioctl  },
  /* Pack to 7 with nxio if adding private devices and start at 8 */
};

bool validdev(uint16_t dev)
{
    /* This is a bit uglier than needed but the right hand side is
       a constant this way */
    if(dev > ((sizeof(dev_tab)/sizeof(struct devsw)) << 8) + 255)
	return false;
    else
        return true;
}

/* This is called with interrupts off. */
void platform_interrupt(void)
{
	udata.u_ininterrupt = 1;

	if (ARMIC.PENDING1 & (1<<ARMIC_IRQ1_TIMER3))
	{
		SYSTIMER.C3 += TIMER_INTERVAL;
		SYSTIMER.CS = SYSTIMER_CS_M3;
		timer_interrupt();
	}

	if (ARMIC.PENDING2 & (1<<ARMIC_IRQ2_UART))
		tty_interrupt();

	udata.u_ininterrupt = 0;
}

void device_init(void)
{
	/* The ARM timer runs at a variable rate due to power control. GPU timer 3
	 * is apparently reserved for OS use and runs at a useful 1MHz. */

	SYSTIMER.C3 = SYSTIMER.CLO + TIMER_INTERVAL;
	SYSTIMER.CS = SYSTIMER_CS_M3;
	while (!SYSTIMER.CS & SYSTIMER_CS_M3)
		;
	SYSTIMER.CS = SYSTIMER_CS_M3;
	SYSTIMER.C3 += TIMER_INTERVAL;
	ARMIC.ENABLE1 = 1<<ARMIC_IRQ1_TIMER3;

	devsd_init();
}


