SECTIONS
{
  ENTRY(_start)

  .text :
  {
    KEEP(*(.text._mbheader));
    KEEP(*(.text._start));
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
