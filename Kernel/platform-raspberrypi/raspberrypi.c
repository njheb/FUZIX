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

void led_on(void)
{
	led_set(true);
}

void led_off(void)
{
	led_set(false);
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
	while (SBM.STATUS & MAIL_FULL)
		;

	SBM.WRITE = channel | value;
}

uint32_t mbox_read(int channel)
{
	for (;;)
	{
		while (SBM.STATUS & MAIL_EMPTY)
			;

		uint32_t data = SBM.READ;
		if ((data & 0xf) == channel)
			return data & ~0xf;
	}
}

static void rpc(void* buffer)
{
	clean_data_cache();
	data_sync_barrier();
	data_mem_barrier();
	mbox_write(8, (uint32_t)buffer | 0xc0000000);
	mbox_read(8);
	invalidate_data_cache();
	data_sync_barrier();
	data_mem_barrier();
}

#define ALIGNED(n) __attribute__ ((aligned (n)))

uint32_t mbox_get_arm_memory(void)
{
	struct payload
	{
		uint32_t baseaddress;
		uint32_t size;
	};

	struct ALIGNED(16)
	{
		uint32_t size;
		uint32_t request;
		uint32_t command;
		uint32_t reqlen;
		uint32_t resplen;
		struct payload payload;
		uint32_t terminator;
	} packet = {
		.size = sizeof(packet),
		.request = MBOX_PROCESS_REQUEST,
		.command = MBOX_GET_ARM_MEMORY,
		.reqlen = sizeof(struct payload),
	};

	rpc(&packet);
	if ((packet.request != MBOX_REQUEST_OK) ||
        (!(packet.resplen & MBOX_REQUEST_OK)))
		panic("failed VC4 RPC");

	return packet.payload.size;
}

int mbox_get_clock_rate(int clockid)
{
	struct payload
	{
		uint32_t clockid;
		uint32_t frequency;
	};

	struct ALIGNED(16)
	{
		uint32_t size;
		uint32_t request;
		uint32_t command;
		uint32_t reqlen;
		uint32_t resplen;
		struct payload payload;
		uint32_t terminator;
	} packet = {
		.size = sizeof(packet),
		.request = MBOX_PROCESS_REQUEST,
		.command = MBOX_GET_CLOCK_RATE,
		.reqlen = sizeof(struct payload),
		.payload.clockid = clockid,
	};

	rpc(&packet);

	if ((packet.request != MBOX_REQUEST_OK) ||
        (!(packet.resplen & MBOX_REQUEST_OK)))
		panic("failed VC4 RPC");

	return packet.payload.frequency;
}

int mbox_get_power_state(int deviceid)
{
	struct payload
	{
		uint32_t deviceid;
		uint32_t state;
	};

	struct ALIGNED(16)
	{
		uint32_t size;
		uint32_t request;
		uint32_t command;
		uint32_t reqlen;
		uint32_t resplen;
		struct payload payload;
		uint32_t terminator;
	} packet = {
		.size = sizeof(packet),
		.request = MBOX_PROCESS_REQUEST,
		.command = MBOX_GET_POWER_STATE,
		.reqlen = sizeof(struct payload),
		.payload.deviceid = deviceid,
	};

	rpc(&packet);

	if ((packet.request != MBOX_REQUEST_OK) ||
        (!(packet.resplen & MBOX_REQUEST_OK)))
		panic("failed VC4 RPC");

	return packet.payload.state;
}

void mbox_set_power_state(int deviceid, bool state)
{
	struct payload
	{
		uint32_t deviceid;
		uint32_t state;
	};

	struct ALIGNED(16)
	{
		uint32_t size;
		uint32_t request;
		uint32_t command;
		uint32_t reqlen;
		uint32_t resplen;
		struct payload payload;
		uint32_t terminator;
	} packet = {
		.size = sizeof(packet),
		.request = MBOX_PROCESS_REQUEST,
		.command = MBOX_SET_POWER_STATE,
		.reqlen = sizeof(struct payload),
		.payload.deviceid = deviceid,
		.payload.state = state | 2,
	};

	rpc(&packet);

	if ((packet.request != MBOX_REQUEST_OK) ||
        (!(packet.resplen & MBOX_REQUEST_OK)))
		panic("failed VC4 RPC");
}


