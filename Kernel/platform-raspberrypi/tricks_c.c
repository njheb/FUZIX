#include <kernel.h>
#include <printf.h>
#include <kdata.h>
#include "raspberrypi.h"
#include "externs.h"

void page_in_new_process(ptptr newproc)
{
	if (newproc->p_page != udata.u_page)
	{
		led_halt_and_blink(2);
	}

	newproc->p_status = P_RUNNING;
}

