#include <kernel.h>
#include <printf.h>
#include <kdata.h>
#include "raspberrypi.h"
#include "externs.h"

struct u_data* udata_ptr;

typedef uint32_t kernelstack_t[1024];

static struct u_data udatas[PTABSIZE];
static kernelstack_t kstacks[PTABSIZE];

void* get_udata_for_page(int page)
{
	return &udatas[page];
}

void* get_svc_stack_for_page(int page)
{
	uint8_t* sp = (uint8_t*) &kstacks[page];
	sp += sizeof(kernelstack_t);
	return sp;
}

