#include <kernel.h>
#include <kdata.h>
#include <printf.h>
#include <stdbool.h>
#include <devtty.h>
#include <vt.h>
#include <tty.h>
#include "raspberrypi.h"
#include "externs.h"

void sd_spi_clock(bool go_fast)
{
}

void sd_spi_raise_cs(void)
{
}

void sd_spi_lower_cs(void)
{
}

static uint8_t xmit_recv(uint8_t b)
{
	return 0;
}

void sd_spi_transmit_byte(uint8_t b)
{
}

uint8_t sd_spi_receive_byte(void)
{
}


bool sd_spi_receive_sector(void)
{
	return 0;
}

bool sd_spi_transmit_sector(void)
{
	return 0;
}

void sd_rawinit(void)
{
	ALTMMC.CLKDIV = 0x96;
	ALTMMC.HOST_CFG = 0xA;
	ALTMMC.VDD = 0x1;
}

