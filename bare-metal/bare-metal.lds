/* FILE: bare_metal.ld - linker script for bare metal */
ENTRY(_start)

SECTIONS
{
    . = 0x8000; /* Raspbery Pi will load the image here by default */

    __text_start__ = .;
    .text :
	{
        KEEP(*(.text.boot)) /* put the start function first ! */
		*(.text)
	}
	. = ALIGN(4096); /* align to page size */
	__text_end__ = .;

	__rodata_start__ = .;
	.rodata :
	{
		*(.rodata)
	}
	. = ALIGN(4096); /* align to page size */
	__rodata_end__ = .;

	. = 0x40000000 + 0x100000;	/* 1MB for FDT */
	__data_start__ = .;

	.data :
	{
		*(.data)
	}
	. = ALIGN(4096); /* align to page size */
	__data_end__ = .;

	__bss_start__ = .;
	.bss :
	{
		bss = .;
		*(.bss)
	}
	. = ALIGN(4096); /* align to page size */
	__bss_end__ = .;
	_end = .;
}
