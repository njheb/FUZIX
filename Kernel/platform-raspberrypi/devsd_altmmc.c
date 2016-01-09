#include <kernel.h>
#include <kdata.h>
#include <printf.h>
#include <stdbool.h>
#include <devtty.h>
#include <vt.h>
#include <tty.h>
#include <timer.h>
#include <blkdev.h>
#include "raspberrypi.h"
#include "externs.h"

#define _SD_PRIVATE
#include "devsd.h"

/* The logic here is mostly stolen from:
 *
 * https://github.com/jncronin/rpi-boot/blob/master/emmc.c
 *
 * ...which is the only emmc code I can find which seems to work. The EMMC
 * unit is documented here:
 *
 * https://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 */

static int emmc_clock_rate;
static int sdversion;
static timer_t setup_timer;
static timer_t command_timer;

/* Rough and ready microseconds. */
static void shortwait(int us)
{
	us = (us * 3) / 2;

	volatile int i = 0;
	while (us--)
		i;
}

static uint32_t calculate_clock_divider(int frequency)
{
	/* Work out the closest divider which will result in a frequency equal
	 * or less than that requested. */

	int targetted_divisor;
	if (frequency > emmc_clock_rate)
		targetted_divisor = 1;
	else
	{
		targetted_divisor = emmc_clock_rate / frequency;
		if (emmc_clock_rate % frequency)
			targetted_divisor--;
	}

	int divisor = -1;
	for(int first_bit = 31; first_bit >= 0; first_bit--)
	{
		uint32_t bit_test = (1 << first_bit);
		if(targetted_divisor & bit_test)
		{
			divisor = first_bit;
			targetted_divisor &= ~bit_test;
			if(targetted_divisor)
			{
				// The divisor is not a power-of-two, increase it
				divisor++;
			}
			break;
		}
	}

	if(divisor == -1)
		divisor = 31;
	if(divisor >= 32)
		divisor = 31;

	if(divisor != 0)
		divisor = (1 << (divisor - 1));

	if(divisor >= 0x400)
		divisor = 0x3ff;

	kprintf("divisor: %x\n", divisor);
	uint32_t freq_select = divisor & 0xff;
	uint32_t upper_bits = (divisor >> 8) & 0x3;
	return (freq_select*EMMC_C1_CLK_FREQ8) | (upper_bits*EMMC_C1_CLK_FREQ_MS2);
}

static int init(void)
{
	/* Reset host controller. */

	EMMC.CONTROL1 = EMMC.CONTROL1
		| EMMC_C1_SRST_HC
		& ~EMMC_C1_CLK_INTLEN
		& ~EMMC_C1_CLK_EN;

	while (EMMC.CONTROL1
			& (EMMC_C1_SRST_HC | EMMC_C1_SRST_DATA | EMMC_C1_SRST_CMD))
	{
		shortwait(10);
		if (timer_expired(setup_timer))
			return -1;
	}

	/* Look for a card. */

	kprintf("status=%x\n", EMMC.STATUS);

	/* Initialise the clock. */

	EMMC.CONTROL1 |= EMMC_C1_CLK_INTLEN;
	EMMC.CONTROL1 |= calculate_clock_divider(EMMC_FREQ_SETUP);
	EMMC.CONTROL1 = EMMC.CONTROL1
		& ~EMMC_C1_TOUNIT_DIS
		| (7 * EMMC_C1_DATA_TOUNIT);
	
	while (!(EMMC.CONTROL1 & EMMC_C1_CLK_STABLE))
	{
		shortwait(10);
		if (timer_expired(setup_timer))
			return -1;
	}

	shortwait(2000);
	EMMC.CONTROL1 |= EMMC_C1_CLK_EN;

	/* Record interrupts internally, but don't send them to the ARM. */

	EMMC.IRPT_EN = 0;
	EMMC.INTERRUPT = 0xffffffff;
	EMMC.IRPT_MASK = 0xffffffff;

	return 0;
}

static int wait_for_command(void)
{
	while ((EMMC.STATUS & EMMC_SR_CMD_INHIBIT) &&
           !(EMMC.INTERRUPT & EMMC_INT_ERROR_MASK))
	{
		shortwait(1);
		if (timer_expired(command_timer))
			return -1;
	}

	if (EMMC.INTERRUPT & EMMC_INT_ERROR_MASK)
	{
		EMMC.INTERRUPT = EMMC.INTERRUPT;
		return -1;
	}

	return 0;
}

static int wait_for_interrupt(uint32_t mask)
{
	while (!timer_expired(command_timer))
	{
		if (EMMC.INTERRUPT & mask)
		{
			EMMC.INTERRUPT = mask;
			return 0;
		}

		if (EMMC.INTERRUPT &
				(EMMC_INT_CMD_TIMEOUT | EMMC_INT_DATA_TIMEOUT | EMMC_INT_ERROR_MASK))
		{
			kprintf("(interrupt error %x)\n", EMMC.INTERRUPT);
			break;
		}
	}

	/* Clear all interrupts on error. */
	EMMC.INTERRUPT = EMMC.INTERRUPT;
	return -1;
}

static int rpc(int command, uint32_t flags, int arg)
{
	kprintf("command=%d flags=0x%x\n", command, flags);
	command_timer = set_timer_sec(1);
	kprintf("%d\n", __LINE__);
	if (wait_for_command())
		return -1;

	/* Clear interrupt flags. */

	EMMC.INTERRUPT = EMMC.INTERRUPT;

	/* Program command. */

	EMMC.ARG1 = arg;
	EMMC.CMDTM = (command << 24) | flags;

	/* Wait for completion. */

	kprintf("%d\n", __LINE__);
	if (wait_for_interrupt(EMMC_INT_CMD_DONE))
		return -1;
	kprintf("%d\n", __LINE__);
	kprintf("INTERRUPT = %x\n", EMMC.INTERRUPT);

	return 0;
}

static int rpc_r1(int command, int arg)
{
	if (rpc(command, EMMC_CMD_RSPNS_48 | EMMC_CMD_CRCCHK_EN, arg) < 0)
		return -1;

	uint32_t r = EMMC.RESP0;
	kprintf("r1 response=%x\n", r);
	if (r & EMMC_R1_ERRORS_MASK)
		return -1;
	return r & 0xff;
}

static int rpc_r2(int command, int arg)
{
	if (rpc(command, EMMC_CMD_RSPNS_136 | EMMC_CMD_CRCCHK_EN, arg) < 0)
		return -1;
	return 0;
}

static int rpc_r3(int command, int arg)
{
	if (rpc(command, EMMC_CMD_RSPNS_48 | EMMC_CMD_CRCCHK_EN, arg) < 0)
		return -1;

	uint32_t r = EMMC.RESP0;
	kprintf("r3 response=%x\n", r);
	return r & 0xff;
}

static int rpc_r7(int command, int arg)
{
	if (rpc(command, EMMC_CMD_RSPNS_48 | EMMC_CMD_CRCCHK_EN, arg) < 0)
		return -1;
	
	return (EMMC.RESP0 == arg) ? 0 : -1;
}

static int arpc_r1(int command, int arg)
{
	if (!rpc(55, EMMC_CMD_RSPNS_NO, 0) < 0)
		return -1;
	return rpc_r1(command, arg);
}
	
static int arpc_r3(int command, int arg)
{
	if (!rpc(55, EMMC_CMD_RSPNS_NO, 0) < 0)
		return -1;
	return rpc_r3(command, arg);
}
	
static int rpc_go_idle_state(void)          { return  rpc   ( 0, 0, 0); }
static int rpc_send_if_cond(uint32_t arg)   { return  rpc_r7( 8, arg); }
static int rpc_sendopcond(uint32_t arg)     { return  rpc_r1( 1, arg); }
static int rpc_app_sendopcond(uint32_t arg) { return arpc_r3(41, arg); }
static int rpc_set_blocklen(uint32_t arg)   { return  rpc_r1(16, arg); }
static int rpc_send_csd(void)               { return  rpc_r2( 9, 0); }

void devsd_init(void)
{
	setup_timer = set_timer_sec(2);
	emmc_clock_rate = mbox_get_clock_rate(MBOX_PROP_CLOCK_EMMC);

	/* Read the controller version. */

	uint32_t version = EMMC.SLOTISR_VER;
	sdversion = (version >> 16) & 0xff;

	if (init() < 0)
		goto card_init_failure;
	if (rpc_go_idle_state() < 0)
		goto card_init_failure;

	/* Check the supported voltage range. */

	kprintf("check voltage\n");
	if (rpc_send_if_cond(0x1AA) == 1)
	{
		kprintf("SDHC\n");

#if 0
        /* initialisation timeout 2 seconds */
        timer = set_timer_sec(2);
        if (sd_send_command(CMD8, (uint32_t)0x1AA) == 1) {    /* SDHC */
            /* Get trailing return value of R7 resp */
            for (n = 0; n < 4; n++) ocr[n] = sd_spi_receive_byte();
            /* The card can work at vdd range of 2.7-3.6V */
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
                /* Wait for leaving idle state (ACMD41 with HCS bit) */
                while(!timer_expired(timer) && sd_send_command(ACMD41, (uint32_t)1 << 30));
                /* Check CCS bit in the OCR */
                if (!timer_expired(timer) && sd_send_command(CMD58, 0) == 0) {
                    for (n = 0; n < 4; n++) ocr[n] = sd_spi_receive_byte();
                    card_type = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;   /* SDv2 */
                }
            }
#endif
	}
	else
	{
		if (init() < 0)
			goto card_init_failure;

		int (*cmd)(uint32_t arg);
		if (rpc_app_sendopcond(0) <= 1)
		{
			kprintf("SDv1\n");
			cmd = rpc_app_sendopcond;
		}
		else
		{
			kprintf("MMCv3\n");
			cmd = rpc_sendopcond;
		}

		/* Wait for card to leave idle mode. */

		while (cmd(0) == 1)
		{
			if (timer_expired(setup_timer))
				goto card_init_failure;
		}

		/* Set block size. */

		if (rpc_set_blocklen(512) != 0)
			goto card_init_failure;
	}

	/* Now we know there's a card, create a block device for it. */

    blkdev_t* blk = blkdev_alloc();
    if(!blk)
        return;

#if 0
    blk->transfer = devsd_transfer_sector;
    blk->driver_data = 0;
#endif
    
	/* Read and compute the card size. */

	{
		rpc_send_csd();
		kprintf("%x %x %x %x\n", EMMC.RESP3, EMMC.RESP2, EMMC.RESP1, EMMC.RESP0);
	}

	#if 0
    /* read and compute card size */
    if(sd_send_command(CMD9, 0) == 0 && sd_spi_wait(false) == 0xFE){
        for(n=0; n<16; n++)
            csd[n] = sd_spi_receive_byte();
        if ((csd[0] >> 6) == 1) {
            /* SDC ver 2.00 */
            blk->drive_lba_count = ((uint32_t)csd[9] 
                                   + (uint32_t)((unsigned int)csd[8] << 8) + 1) << 10;
        } else {
            /* SDC ver 1.XX or MMC*/
            n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
            blk->drive_lba_count = (csd[8] >> 6) + ((unsigned int)csd[7] << 2) 
                                   + ((unsigned int)(csd[6] & 3) << 10) + 1;
            blk->drive_lba_count <<= (n-9);
        }
    }
    sd_spi_release();

    blkdev_scan(blk, 0);
#endif

	return;

card_init_failure:
	kprintf("card init failure!\n");
}

