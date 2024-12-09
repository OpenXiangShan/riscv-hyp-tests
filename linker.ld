#include <platform.h>

OUTPUT_FORMAT(elf64-littleriscv)

ENTRY(_boot)

STACK_SIZE = 1M;


SECTIONS {

	. = MEM_BASE;

	.boot : {
		*(.boot)
	} 

	.text :  {
		*(.text)
	} 

	.rodata :  {
		*(.rodata)
	}

	.hyp_test_table :  {
		_hyp_test_table = .;
		*(.hyp_test_table)
		_hyp_test_table_end = .;
	}
	_hyp_test_table_size = (_hyp_test_table_end - _hyp_test_table) / 8;
	
	.non_hyp_test_table : {
		_non_hyp_test_table = .;
		*(.non_hyp_test_table)
		_non_hyp_test_table_end = .;
	}
	_non_hyp_test_table_size = (_non_hyp_test_table_end - _non_hyp_test_table) / 8;

	.data : {
		*(.data)
	}

	// HTIF memory needs to be located on its own page
	.tohost ALIGN(0x1000) :  {
		*(.tohost)
	}

	.bss ALIGN(0x1000) (NOLOAD) :  {
		__bss_start = .;
		*(.bss) 
		*(.sbss) 
		*(COMMON)	
		__bss_end = .;
	} 

	. = MEM_BASE + 0x1ff000;
    
    . = ALIGN(16) + STACK_SIZE;
    __stack_top = .;
    _end = .;

	. = 0x8806c000;
	.nop_test : {
		*(.nop_test)
	}

}
