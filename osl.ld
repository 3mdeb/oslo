SECTIONS
{
  ENTRY(__start)

  .slheader :
  {
    . = ALIGN(0x10000);
    __LOADER_START__ = .;
    SHORT (_skinit - __LOADER_START__);
    SHORT (__LOADER_END__ - __LOADER_START__);
  }

  .loader :
  {
    KEEP(*(.text._mbheader));
    KEEP(*(.text._start));
    KEEP(*(.text._skinit));
    *(.text .text.*);
    *(.rodata .rodata.*);
  }

  /* some processors assume that the hash size is a multiple of 4... */
  . = ALIGN(0x4);
  __LOADER_END__ = .;

  .bss :
  {
     *(.bss);
  }

  .data :
  {
     *(.data);
  }

  .debug :
  {
     *(.debug*);
  }


  /DISCARD/ :
  {
    *(.comment)
  }
}
