#include <kernel.h>
#include <printf.h>
#include "raspberrypi.h"

static void delay(volatile int count)
{
	while (count--)
		;
}

void gpio_set_pin_func(int pin, int func, int pull)
{
	volatile uint32_t* fsel = &GPIO.FSEL0;
	int pind10 = pin / 10;
	int pinm10 = pin % 10;
	int fselshift = pinm10 * 3;

	fsel[pind10] = fsel[pind10]
		& ~(7<<fselshift)
		| (func<<fselshift);

	volatile uint32_t* pudclk = &GPIO.PUDCLK0;
	int pind32 = pin / 32;
	int pinm32 = pin % 32;

	GPIO.PUD = pull;
	delay(150);
	pudclk[pind32] |= (1<<pinm32);
	delay(150);
	GPIO.PUD = GPIO_PULL_OFF;
	pudclk[pind32] = 0;
}

void gpio_set_output_pin(int pin, bool value)
{
	volatile uint32_t* reg = value ? &GPIO.SET0 : &GPIO.CLR0;
	int pind32 = pin / 32;
	int pinm32 = pin % 32;

	reg[pind32] |= (1<<pinm32);
}

void led_init(void)
{
	gpio_set_pin_func(16, GPIO_FUNC_OUTPUT, GPIO_PULL_OFF);
}

void led_set(bool value)
{
	gpio_set_output_pin(16, !value);
}

void led_halt_and_blink(int count)
{
	const int d = 0x1000000;

	for (;;)
	{
		kprintf("halt %d\n", count);
		for (int i=0; i<count; i++)
		{
			led_set(true);
			delay(d);
			led_set(false);
			delay(d);
		}

		delay(d*3);
	}
}

void mbox_write(int channel, uint32_t value)
{
	while (!(SBM.STATUS & MAIL_FULL))
		;

	SBM.WRITE = channel | ((uint32_t)value << 4);
}

uint32_t mbox_read(int channel)
{
	for (;;)
	{
		while (!(SBM.STATUS & MAIL_EMPTY))
			;

		uint32_t data = SBM.READ;
		if ((data & 0xf) == channel)
			return (data >> 4);
	}
}

