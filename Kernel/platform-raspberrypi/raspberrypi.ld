ENTRY(_start)

PROVIDE (tlbtable        = 0);
PROVIDE (GPIO            = 0x3f200000);
PROVIDE (UART0           = 0x3f201000);

PHDRS {
	text PT_LOAD AT (0x8000);
	bss PT_LOAD;
}

SECTIONS {
	.text 0x8000 : {
		*(.text*)
	} :text

	.data : {
		*(.rodata*)
		*(.data*)
	} :text

    .bss (NOLOAD) : {
		PROVIDE (__bssstart = .);
		*(.bss*)
		*(COMMON)
		PROVIDE (__bssend = .);
	} :bss
}

