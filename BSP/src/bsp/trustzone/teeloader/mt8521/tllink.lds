OUTPUT_ARCH(arm)

ENTRY(_start)

SECTIONS {

	. = %BASE_ADDR%;
	.start ALIGN(4) : {	
		*(.text.start)
	} 

	. = . + 0x01FC;
	.rom_info ALIGN(4) : {	    	    	    	     
		*(.data.rom_info)
	} 
	.text ALIGN(4) : {
		*(.text)
		*(.text.*)        
	} 
	.rodata ALIGN(4) : {
		*(.rodata)
		*(.rodata.*)        
	} 
	.data ALIGN(4) : {
		*(.data)
		*(.data.*)        
	} 
	.got ALIGN(4) : {
		*(.got)
		*(.got.*)        
	} 

	. = %BASE_ADDR%-0x100000 ;
	.bss ALIGN(16) : {
		_bss_start = .;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		/* make _bss_end as 4 bytes alignment */
		. = ALIGN(4);
		_bss_end = .;
	}

	.secbuf ALIGN(4) : {
		_secbuf_start = .;
		*(.secbuf)
		_secbuf_end = .;
	} 

}

