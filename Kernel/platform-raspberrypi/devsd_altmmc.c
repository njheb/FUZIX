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

#undef DEBUG_MMC
#undef DEBUG

static uint32_t card_address;
static bool block_addressed;

static void wait_for_mmc(void)
{
	while (ALTMMC.CMD & ALTMMC_ENABLE)
		;
}

static uint32_t mmc_rpc(uint32_t cmd, uint32_t arg)
{
	uint8_t e;

	wait_for_mmc();

	e = ALTMMC.STATUS;
	if (e)
		ALTMMC.STATUS = 0;

	#if defined(DEBUG_MMC)
		kprintf("cmd: %d arg: %x\n", cmd, arg);
	#endif
	ALTMMC.ARG = arg;
	ALTMMC.CMD = ALTMMC_ENABLE | cmd;

	wait_for_mmc();
	#if defined(DEBUG_MMC)
		kprintf("-> %x %x %x\n", ALTMMC.CMD, ALTMMC.STATUS, ALTMMC.RSP0);
	#endif
	return ALTMMC.STATUS & 0xff;
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
			if (mmc_rpc(17 | ALTMMC_BUSY | ALTMMC_READ, addr)) /* READ_SINGLE_BLOCK */
			{
				kprintf("sd: error during transfer setup\n");
				goto retryable_error;
			}

			uint32_t* ptr = (uint32_t*) blk_op.addr;
			for (i=0; i<128; i++)
			{
				while (!(ALTMMC.STATUS & ALTMMC_FIFO_STATUS))
					;

				if ((ALTMMC.STATUS & 0xff) != ALTMMC_FIFO_STATUS)
				{
					kprintf("sd: error during transfer: %x\n", ALTMMC.STATUS);
					goto retryable_error;
				}

				*ptr++ = ALTMMC.DATA;
			}

			return 1;
		}
		else
		{
			goto abort;
		}

retryable_error:
		mmc_rpc(12, 0); /* STOP_TRANSMISSION */
		kprintf("sd: retrying\n");
	}

abort:
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

	ALTMMC.CLKDIV = 0x96;
	ALTMMC.HOST_CFG = 0xA;
	ALTMMC.VDD = 0x1;

	kprintf("altmmc: ");

	mmc_rpc(0, 0);

	/* CARD IS NOW IN IDLE MODE */

	int sdhc = 0;

	/* Test for SDHC card. */

	if (!mmc_rpc(8, 0x155) && (ALTMMC.RSP0 == 0x155))
	{
		kprintf("SDHC: ");
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

		if (!i && (ALTMMC.RSP0 & SD_OCR_BUSY))
			break;
	}

	block_addressed = !!(ALTMMC.RSP0 & SD_OCR_HIGHCAP);
	if (block_addressed)
		kprintf("(high capacity) ");

	/* CARD IS IN READY MODE */

	/* Fetch the CID. (We don't care, but the card wants this or else it won't
	 * initialise correctly. */

	if (mmc_rpc(2 | ALTMMC_LONG_RSP, 0)) /* ALL_SEND_CID */
		goto failed;

	/* CARD IS IN IDENT MODE */

	/* Fetch the card address and select the card. */

	if (mmc_rpc(3, 0)) /* SEND_RELATIVE_ADDR */
		goto failed;
	card_address = ALTMMC.RSP0;

	/* CARD IS IN STANDBY MODE */

	/* Fetch the CSD. This has the card size in it. */

	uint32_t number_of_sectors = 0;
	{
		if (mmc_rpc(9 | ALTMMC_LONG_RSP, card_address)) /* SEND_CSD */
			goto failed;

		uint32_t csd0 = ALTMMC.RSP0;
		uint32_t csd1 = ALTMMC.RSP1;
		uint32_t csd2 = ALTMMC.RSP2;
		uint32_t csd3 = ALTMMC.RSP3;

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
				kprintf("\naltmmc: unsupported card type! Please quote:\n%x %x %x %x\n",
					csd0, csd1, csd2, csd3);
				return;
		}

		kprintf("%d MB: ", number_of_sectors / (2*1024));
	}

	if (mmc_rpc(7, card_address)) /* SELECT_CARD */
		goto failed;

	/* CARD IS IN TRANSFER MODE */

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

