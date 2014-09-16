OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
	. = 0x0020c000;
	.text.entry : {
		*(.entry)
	}

	.text : {
	  *(.text)
	}

	.rodata : { *(.rodata*) }
	.data :   { *(.data) }
	.got :    { *(.got) }
	.glue :   { *(.glue*) }
	.bss :    { *(.bss*) }
}
