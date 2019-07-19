/*
 * \brief   Utility functions for a bootloader
 * \date    2006-03-28
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2006  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the OSLO package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#include <string.h>
#include <stdarg.h>
#include "util.h"

/**
 * Wait roughly a given number of milliseconds.
 *
 * We use the PIT for this.
 */
void
wait(int ms)
{
  /* the PIT counts with 1.193 Mhz */
  ms*=1193;

  /* initalize the PIT, let counter0 count from 256 backwards */
  asm volatile ("outb %%al,$0x43" :: "a"(0x10));
  asm volatile ("outb %%al,$0x40" :: "a"(0)); 

  
  unsigned state;
  unsigned old = 0;
  while (ms>0)
    {
      /* read the current value of counter0 */
      asm volatile ("outb %%al,$0x43;" 
		    "inb  $0x40,%%al;"
		    : "=a"(state) : "a"(0));
      ms -= (unsigned char)(old - state);
      old = state;
    }
}

/**
 * Print the exit status and reboot the machine.
 */
void
_exit(unsigned status)
{
  out_char('\n');
  out_description("exit()", status);
  for (unsigned i=0; i<16;i++)
    {      
      wait(1000);
      out_char('.');
    }
  out_string("-> OK, reboot now!\n");
  reboot();
}




/**
 * Checks whether we have SVM support and a local APIC.
 *
 * @return: the SVM revision of the processor or a negative value, if
 * not supported.
 */
int
check_cpuid()
{
  int res;
  CHECK3(-31,0x8000000A > cpuid_eax(0x80000000), "no ext cpuid");
  CHECK3(-32,!(0x4   & cpuid_ecx(0x80000001)), "no SVM support");
  CHECK3(-33,!(0x200 & cpuid_edx(0x80000001)), "no APIC support");
  res = cpuid_eax(0x8000000A) & 0xff;
  return res;
}


/**
 * Output a single char.
 * Note: We allow only to put a char on the last line.
 */
int
out_char(unsigned value)
{

#define BASE(ROW) ((unsigned short *) (0xb8000+ROW*160))
  static unsigned int col;
  switch (value)
    {
    case '\n':
      col=80;
      break;
    default:
      {	
	unsigned short *p = BASE(24)+col;
	*p = 0x0f00 | value;
	col++;
      }
    }
  if (col>=80)
    {
      col=0;
      unsigned short *p=BASE(0);
      memcpy(p, p+80, 24*160);
      memset(BASE(24), 0, 160);
    }
  return value;
}


/**
 * Output a string.
 */
void
out_string(char *value)
{
  for(; *value; value++)
    out_char(*value);
}


/**
 * Output a single hex value.
 */
void
out_hex(unsigned value, unsigned len)
{
  unsigned i;
  for (i=32; i>0; i-=4)
    {
      unsigned a = value>>(i-4);
      if (a || i==4 || (len*4>=i))
	{
	  out_char("0123456789abcdef"[a & 0xf]);
	}
    }
}

/**
 * Output a string followed by a single hex value, prefixed with a
 * message label.
 */
void
out_description(char *prefix, unsigned int value)
{
  out_string(message_label);
  out_string(prefix);
  out_char(' ');
  out_hex(value, 0);
  out_char('\n');
}

/**
 * Output a string, prefixed with a message label.
 */
void
out_info(char *msg)
{
  out_string(message_label);
  out_string(msg);
  out_char('\n');
}
