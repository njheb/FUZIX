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

/* A lot of this is based on studying this driver:
 *
 * https://github.com/raspberrypi/linux/blob/rpi-4.1.y/drivers/mmc/host/bcm2835-sdhost.c
 */

#undef DEBUG_MMC
#undef DEBUG

static uint32_t card_address;
static bool block_addressed;

static void busy_sleep_ms(int ms)
{
	timer_t timer = set_timer_ms(ms);
	while (!timer_expired(timer))
		;
	//kprintf("wake\n");
}

static void wait_for_mmc(void)
{
	while (SDHOST.CMD & SDHOST_CMD_NEW)
		;
}

static void wait_for_data(void)
{
	while (!(SDHOST.HSTS & SDHOST_HSTS_DATA))
		;
	SDHOST.HSTS = SDHOST_HSTS_DATA;
}

static uint32_t mmc_rpc(uint32_t cmd, uint32_t arg)
{
	wait_for_mmc();

	SDHOST.HSTS = SDHOST.HSTS;

	#if defined(DEBUG_MMC)
		kprintf("cmd: %x arg: %x\n", cmd, arg);
	#endif
	SDHOST.ARG = arg;
	SDHOST.CMD = SDHOST_CMD_NEW | cmd;

	wait_for_mmc();
	#if defined(DEBUG_MMC)
		kprintf("-> %x %x %x\n", SDHOST.CMD, SDHOST.HSTS, SDHOST.RSP0);
	#endif
	return SDHOST.HSTS & SDHOST_HSTS_ERROR;
}

static uint32_t mmc_arpc(uint32_t cmd, uint32_t arg)
{
	int s = mmc_rpc(55, 0); /* APP_CMD */
	if (s)
		return s;

	return mmc_rpc(cmd, arg);
}

uint8_t devsd_transfer_sector(void)
{
	int i;
	int attempts = 6;

	while (attempts--)
	{
		if (blk_op.is_read)
		{
			#if defined(DEBUG)
				kprintf("sd: read %x\n", blk_op.lba);
			#endif
			uint32_t addr = block_addressed ? blk_op.lba : (blk_op.lba << 9);
			if (mmc_rpc(17 | SDHOST_CMD_NEW | SDHOST_CMD_BUSYWAIT | SDHOST_CMD_READ, addr)) /* READ_SINGLE_BLOCK */
				goto retryable_error;

			uint32_t* ptr = (uint32_t*) blk_op.addr;
			for (i=0; i<128; i++)
			{
				wait_for_data();

				if (SDHOST.HSTS & SDHOST_HSTS_ERROR)
					goto retryable_error;

				*ptr++ = SDHOST.DATA;
			}

			return 1;
		}
		else
		{
			#if defined(DEBUG)
				kprintf("sd: write %x\n", blk_op.lba);
			#endif
			uint32_t addr = block_addressed ? blk_op.lba : (blk_op.lba << 9);
			if (mmc_rpc(24 | SDHOST_CMD_NEW | SDHOST_CMD_BUSYWAIT | SDHOST_CMD_WRITE, addr)) /* WRITE_BLOCK */
				goto retryable_error;

			uint32_t* ptr = (uint32_t*) blk_op.addr;
			for (i=0; i<128; i++)
			{
				wait_for_data();

				if (SDHOST.HSTS & SDHOST_HSTS_ERROR)
					goto retryable_error;

				SDHOST.DATA = *ptr++;
			}

			return 1;
		}

retryable_error:
		busy_sleep_ms(10);
		mmc_rpc(12 | SDHOST_CMD_NEW, 0); /* STOP_TRANSMISSION */
	}

	kprintf("sd: fatal error %x\n", SDHOST.HSTS);
	udata.u_error = EIO;
	return 0;
}

void devsd_init(void)
{
	gpio_set_pin_func(47, GPIO_FUNC_INPUT, GPIO_PULL_UP); /* card detect */
	gpio_set_pin_func(48, GPIO_FUNC_ALT0, GPIO_PULL_OFF); /* CLK */
	gpio_set_pin_func(49, GPIO_FUNC_ALT0, GPIO_PULL_OFF); /* CMD */
	gpio_set_pin_func(50, GPIO_FUNC_ALT0, GPIO_PULL_OFF); /* DAT0 */
	gpio_set_pin_func(51, GPIO_FUNC_ALT0, GPIO_PULL_OFF); /* DAT1 */
	gpio_set_pin_func(52, GPIO_FUNC_ALT0, GPIO_PULL_OFF); /* DAT2 */
	gpio_set_pin_func(53, GPIO_FUNC_ALT0, GPIO_PULL_OFF); /* DAT3 */

	/* CDIV holds (clock divisor - 2). Ident mode has to be a 40kHz.
	 * So, for a core clock of 250MHz, we need a divisor of 625.
	 * (The maximum is 2047.)
	 *
	 * However... the hardware will attempt to automatically switch to a fast
	 * clock in data mode, where it only uses the bottom three bits of the
	 * divisor. Our core clock is 250MHz, which means the slowest this will
	 * go is 50MHz, which is way too fast. So we set SLOW_CARD to disable
	 * this behaviour and force the hardware to use the divisor we specify.
	 * (We'll speed it up later.)
	 * */

	SDHOST.CDIV = 625 - 2;
	SDHOST.HCFG = SDHOST_HCFG_WIDE_INT_BUS | SDHOST_HCFG_SLOW_CARD;
	SDHOST.VDD = 1; /* power on */

	/* Apparently there's a silicon bug; this is a workaround. */

	SDHOST.EDM = SDHOST.EDM
		& ~(0x1f * SDHOST_EDM_READ_THRESHOLD)
		& ~(0x1f * SDHOST_EDM_WRITE_THRESHOLD)
		| (4 * SDHOST_EDM_READ_THRESHOLD)
		| (4 * SDHOST_EDM_WRITE_THRESHOLD);

	kprintf("sdhost: ");

	mmc_rpc(0, 0);

	/* CARD IS NOW IN IDLE MODE */

	int sdhc = 0;

	/* Test for SDHC card. */

	if (!mmc_rpc(8, 0x155) && (SDHOST.RSP0 == 0x155))
	{
		kprintf("SDHC ");
		sdhc = 2;
	}
	else
	{
		kprintf("SDv1 ");
		sdhc = 1;
	}

	/* Enable high capacity mode (if available). */

	for (;;)
	{
		int i = mmc_arpc(41, (sdhc == 2) ? 0x40100000 : 0x00100000); /* APP_SEND_OP_CMD */

		if (!i && (SDHOST.RSP0 & SD_OCR_BUSY))
			break;
	}

	block_addressed = !!(SDHOST.RSP0 & SD_OCR_HIGHCAP);
	if (block_addressed)
		kprintf("(high capacity) ");

	/* CARD IS IN READY MODE */

	/* Fetch the CID. (We don't care, but the card wants this or else it won't
	 * initialise correctly. */

	if (mmc_rpc(2 | SDHOST_CMD_LONG_RSP, 0)) /* ALL_SEND_CID */
		goto failed;

	/* CARD IS IN IDENT MODE */

	/* Fetch the card address and select the card. */

	if (mmc_rpc(3, 0)) /* SEND_RELATIVE_ADDR */
		goto failed;
	card_address = SDHOST.RSP0;

	/* CARD IS IN STANDBY MODE */

	/* Fetch the CSD. This has the card size in it. */

	uint32_t number_of_sectors = 0;
	{
		if (mmc_rpc(9 | SDHOST_CMD_LONG_RSP, card_address)) /* SEND_CSD */
			goto failed;

		uint32_t csd0 = SDHOST.RSP0;
		uint32_t csd1 = SDHOST.RSP1;
		uint32_t csd2 = SDHOST.RSP2;
		uint32_t csd3 = SDHOST.RSP3;

		switch (csd3 >> 30)
		{
			case 0: /* SDC 1.XX or MMC */
			{
				uint32_t c_size = (csd1 >> 30) | ((csd2 & 0x3ff) << 2);
				uint32_t c_size_mult = 1 << (((csd1 >> 15) & 0x7) + 2);
				uint32_t read_bl_len = 1 << ((csd2 >> 16) & 0xf);
				/* Scale to sectors here to avoid overflow on cards bigger than
				 * 4GB. */
				read_bl_len /= 512;
				number_of_sectors = (c_size + 1) * c_size_mult * read_bl_len;
				break;
			}
				
			case 1: /* SDC 2.00 */
			{
				uint32_t c_size = (csd1 >> 16) | ((csd2 & 0x3f) << 16);
				number_of_sectors = (c_size + 1) * 1024;
				break;
			}

			case 2:
			case 3:
				kprintf("\nsdhost: unsupported card type! Please quote:\n%x %x %x %x\n",
					csd0, csd1, csd2, csd3);
				return;
		}

		kprintf("%d MB: ", number_of_sectors / (2*1024));
	}

	if (mmc_rpc(7, card_address)) /* SELECT_CARD */
		goto failed;

	/* CARD IS IN TRANSFER MODE */

	/* Make the clock faster. We're going to aim at 10MHz, which is
	 * conservative and should be supported everywhere. */

	SDHOST.CDIV = 10 - 2;

	/* Set the block size. */

	if (mmc_rpc(16, 512)) /* SET_BLOCKLEN */
		goto failed;

	/* We think the card's ready now, so create the Fuzix block device for it.
	 * */

    blkdev_t* blk = blkdev_alloc();
    if(!blk)
        return;

    blk->transfer = devsd_transfer_sector;
    blk->driver_data = 0;
	blk->drive_lba_count = number_of_sectors;

    blkdev_scan(blk, 0);
	return;

failed:
	kprintf("card init failure\n");
}

