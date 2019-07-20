SECTIONS
{
  ENTRY(__start)

  .text :
  {
    KEEP(*(.text.__mbheader));
    KEEP(*(.text.__start));
    *(.text .text.*);
    *(.rodata .rodata.*);
  }

  .bss :
  {
     *(.bss);
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
