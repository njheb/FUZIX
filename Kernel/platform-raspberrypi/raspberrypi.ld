#include "raspberrypi.ld.h"

ENTRY(reset)

PROVIDE (SYSTIMER        = 0x3f003000);
PROVIDE (ARMIC           = 0x3f00b200);
PROVIDE (SBM             = 0x3f00b880);
PROVIDE (GPIO            = 0x3f200000);
PROVIDE (UART0           = 0x3f201000);
PROVIDE (SDHOST          = 0x3f202000);
PROVIDE (EMMC            = 0x3f300000);

PROVIDE (__kerneltop     = 0x00100000);
PROVIDE (__progbase      = USER_BASE);
PROVIDE (__progtop       = USER_BASE + USER_SIZE);
PROVIDE (udata           = UDATA_BASE);
PROVIDE (udatatop        = UDATA_BASE + UDATA_SIZE);

MEMORY {
	LOWRAM : ORIGIN = 0x0000, LENGTH = 0x8000
	KERNELRAM : ORIGIN = 0x8000, LENGTH = (1024*1024) - 0x8000
}

SECTIONS {
	.text : {
		*(.text.reset)
	} >KERNELRAM

	.vectors 0 : {
		*(.vectors)
	} >LOWRAM AT>KERNELRAM
	PROVIDE(__vectorsstart = LOADADDR(.vectors));
	PROVIDE(__vectorsend = __vectorsstart + SIZEOF(.vectors));

	.text : {
		*(.text*)
	} >KERNELRAM

	.data : {
		*(.rodata*)
		*(.data*)
	} >KERNELRAM

	.stacks (NOLOAD) : {
		*(.stack*)
	} >LOWRAM

    .bss (NOLOAD) : ALIGN(64*1024) {
		PROVIDE (__bssstart = .);
		PROVIDE (tlbtable = .);
		. = . + (4096 * 4);
		*(.bss*)
		*(COMMON)
		PROVIDE (__bssend = .);
	} >KERNELRAM
}

