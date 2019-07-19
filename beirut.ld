_link_addr = 0x00210000;

SECTIONS
{
  ENTRY(_start)
  . = _link_addr;

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
