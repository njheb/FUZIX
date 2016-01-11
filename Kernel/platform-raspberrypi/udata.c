#include <kernel.h>
#include <printf.h>
#include <kdata.h>
#include "raspberrypi.h"
#include "externs.h"

struct u_data* udata_ptr;

static struct u_data udatas[PTABSIZE];

void set_udata_for_page(int page)
{
	udata_ptr = &udatas[page];
}

